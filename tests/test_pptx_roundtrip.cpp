// Round-trip test for PptxBuilder: build a real .pptx file, then unzip it
// back open (using miniz's reader API - the same library used to write
// it) and assert that specific, distinctive content actually made it into
// the slide XML. This is the kind of test that would have caught the
// "every slide is missing its required layout relationship" bug from the
// PowerPoint "needs repair" investigation - it inspects the actual
// package contents, not just "did save() return true".
#include "PptxBuilder.h"
#include "miniz.h"
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>

using namespace std;

static string extractPart(const string& pptxPath, const string& partName)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, pptxPath.c_str(), 0))
        return "";

    size_t size = 0;
    void* data = mz_zip_reader_extract_file_to_heap(&zip, partName.c_str(), &size, 0);
    string result;
    if (data)
    {
        result.assign((const char*)data, size);
        mz_free(data);
    }
    mz_zip_reader_end(&zip);
    return result;
}

int main()
{
    const string path = "test_roundtrip.pptx";
    const string distinctiveBullet = "XYZZY_DISTINCTIVE_BULLET_MARKER";

    PptxBuilder builder;
    builder.addTitleSlide("2C1810", "Round Trip Title", "E8B06E", "Round Trip Subtitle", "D4A373");
    builder.addBulletSlide("2C1810", "Agenda", "E8B06E", { "First point", distinctiveBullet }, "D4A373");

    bool saved = builder.save(path);
    assert(saved);

    // 1. The file must actually be a valid zip that miniz itself can open.
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    assert(mz_zip_reader_init_file(&zip, path.c_str(), 0));
    int fileCount = (int)mz_zip_reader_get_num_files(&zip);
    assert(fileCount > 0);
    mz_zip_reader_end(&zip);

    // 2. [Content_Types].xml must exist and declare the slide content type
    // (a concrete, checkable version of the "needs repair" investigation).
    string contentTypes = extractPart(path, "[Content_Types].xml");
    assert(!contentTypes.empty());
    assert(contentTypes.find("presentationml.slide+xml") != string::npos);

    // 3. Every slide must have its required layout relationship - this is
    // the exact bug found and fixed during the PowerPoint repair
    // investigation. Regressing this should fail this test immediately.
    string slide1Rels = extractPart(path, "ppt/slides/_rels/slide1.xml.rels");
    assert(!slide1Rels.empty());
    assert(slide1Rels.find("relationships/slideLayout") != string::npos);

    string slide2Rels = extractPart(path, "ppt/slides/_rels/slide2.xml.rels");
    assert(!slide2Rels.empty());
    assert(slide2Rels.find("relationships/slideLayout") != string::npos);

    // 4. The actual slide XML must contain the text we put in.
    string slide1Xml = extractPart(path, "ppt/slides/slide1.xml");
    assert(slide1Xml.find("Round Trip Title") != string::npos);
    assert(slide1Xml.find("Round Trip Subtitle") != string::npos);

    string slide2Xml = extractPart(path, "ppt/slides/slide2.xml");
    assert(slide2Xml.find("Agenda") != string::npos);
    assert(slide2Xml.find("First point") != string::npos);
    assert(slide2Xml.find(distinctiveBullet) != string::npos);

    remove(path.c_str());

    // --- Content, image (found + missing), and note slides ---
    {
        const string path2 = "test_roundtrip_other_types.pptx";
        const string imgPath = "test_roundtrip_fixture.png";

        // Content doesn't need to be a real, decodable PNG - PptxBuilder
        // just reads whatever bytes exist and embeds them; only the file
        // extension (from the name) determines the media type it writes.
        { ofstream img(imgPath, ios::binary); img << "not a real png but that's fine for this test"; }

        PptxBuilder builder2;
        builder2.addContentSlide("111111", "Content Heading", "222222", "Distinctive paragraph text.", "333333");
        builder2.addImageSlide("111111", imgPath, "A real caption", "333333");
        builder2.addImageSlide("111111", "does_not_exist_xyz.png", "Missing image caption", "333333");
        builder2.addNoteSlide("111111", "Distinctive custom HTML description", "333333");

        assert(builder2.save(path2));

        // Slide 1: content
        string s1 = extractPart(path2, "ppt/slides/slide1.xml.rels");
        assert(s1.find("relationships/slideLayout") != string::npos);
        string s1xml = extractPart(path2, "ppt/slides/slide1.xml");
        assert(s1xml.find("Content Heading") != string::npos);
        assert(s1xml.find("Distinctive paragraph text.") != string::npos);

        // Slide 2: image found - must have BOTH layout and image
        // relationships, and an actual media file must exist in the package.
        string s2rels = extractPart(path2, "ppt/slides/_rels/slide2.xml.rels");
        assert(s2rels.find("relationships/slideLayout") != string::npos);
        assert(s2rels.find("relationships/image") != string::npos);
        string s2xml = extractPart(path2, "ppt/slides/slide2.xml");
        assert(s2xml.find("<p:pic>") != string::npos);
        assert(s2xml.find("A real caption") != string::npos);
        string mediaFile = extractPart(path2, "ppt/media/image1.png");
        assert(!mediaFile.empty());

        // Slide 3: image NOT found - must gracefully fall back to a text
        // placeholder instead of a broken/missing picture reference, and
        // still have its required layout relationship.
        string s3rels = extractPart(path2, "ppt/slides/_rels/slide3.xml.rels");
        assert(s3rels.find("relationships/slideLayout") != string::npos);
        string s3xml = extractPart(path2, "ppt/slides/slide3.xml");
        assert(s3xml.find("<p:pic>") == string::npos); // no picture shape - it wasn't found
        assert(s3xml.find("does_not_exist_xyz.png") != string::npos); // path shown in the placeholder
        assert(s3xml.find("Missing image caption") != string::npos);

        // Slide 4: custom HTML -> note slide
        string s4xml = extractPart(path2, "ppt/slides/slide4.xml");
        assert(s4xml.find("Distinctive custom HTML description") != string::npos);
        assert(s4xml.find("can't be converted") != string::npos);

        remove(path2.c_str());
        remove(imgPath.c_str());
    }

    cout << "All PPTX round-trip tests passed.\n";
    return 0;
}
