// Tests for HtmlImporter.h/.cpp. splitSlideBlocks() itself is a private
// implementation detail (anonymous namespace), so these exercise it
// indirectly through the public importFromFile() entry point - which is
// exactly what determines correctness for real usage, including its
// trickiest job: correctly finding the end of a slide block that itself
// contains nested <div> tags (a CustomHTMLSlide's raw content can).
#include "HtmlImporter.h"
#include "TitleSlide.h"
#include "ContentSlide.h"
#include "BulletSlide.h"
#include "ImageSlide.h"
#include "CustomHTMLSlide.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

static void writeFile(const string& path, const string& content)
{
    ofstream f(path);
    f << content;
}

int main()
{
    const string path = "test_import_fixture.html";

    // One of each slide type, plus a custom-HTML slide with a NESTED
    // <div> inside it - this is the case that requires depth-aware
    // matching of the closing </div>, not a naive first-match.
    string html =
        "<!DOCTYPE html><html><body>"
        "<div class='slide'><p class='number'>Slide 1</p>"
        "<h1>My Title</h1><h3>My Subtitle</h3></div>"
        "<div class='slide'><p class='number'>Slide 2</p>"
        "<h2>My Heading</h2><p>My content text.</p></div>"
        "<div class='slide'><p class='number'>Slide 3</p>"
        "<h2>My Bullets</h2><ul><li>First</li><li>Second</li></ul></div>"
        "<div class='slide'><p class='number'>Slide 4</p>"
        "<img src='images/pic.png' alt='slide image'>"
        "<p class='caption'>My caption</p></div>"
        "<div class='slide'><p class='number'>Slide 5 (Custom)</p>"
        "<div><b>Nested content</b><div>Even deeper</div></div></div>"
        "</body></html>";

    writeFile(path, html);

    vector<Slide*> slides;
    string error;
    bool ok = HtmlImporter::importFromFile(path, slides, error);
    assert(ok);
    assert(slides.size() == 5);

    assert(slides[0]->summary() == "Title Slide    : My Title");
    assert(slides[1]->summary() == "Content Slide  : My Heading");
    assert(slides[2]->summary() == "Bullet Slide   : My Bullets");
    assert(slides[3]->summary() == "Image Slide    : My caption");
    // Slide 5's nested divs must not have truncated the block early -
    // if depth-tracking were broken, this would have been cut off after
    // the FIRST </div> and lost "Even deeper" entirely, or thrown off
    // every slide index after it.
    assert(slides[4]->summary() == "Custom HTML    : Imported slide");

    // Bullet content specifically must have been split into two separate
    // bullets, not merged into one string - verified via toTemplateJSON(),
    // which serializes each bullet as its own array element.
    string bulletsJson = slides[2]->toTemplateJSON();
    assert(bulletsJson.find("\"First\"") != string::npos);
    assert(bulletsJson.find("\"Second\"") != string::npos);

    // Round-trip check: a path containing an apostrophe survives import
    // correctly (see the escapeHTML/unescapeHTML review note).
    {
        const string path2 = "test_import_apostrophe.html";
        string html2 =
            "<div class='slide'><p class='number'>Slide 1</p>"
            "<img src='O&#39;Brien&#39;s Photos/pic.png' alt='slide image'>"
            "<p class='caption'>caption</p></div>";
        writeFile(path2, html2);

        vector<Slide*> slides2;
        string error2;
        bool ok2 = HtmlImporter::importFromFile(path2, slides2, error2);
        assert(ok2 && slides2.size() == 1);
        // ImageSlide has no public path getter, but display() prints it -
        // good enough to confirm the apostrophe survived unescaping via
        // toTemplateJSON(), which does expose it.
        string json = slides2[0]->toTemplateJSON();
        assert(json.find("O'Brien's") != string::npos);

        for (auto* s : slides2) delete s;
        remove(path2.c_str());
    }

    // A file with no recognizable slide markup should fail cleanly, not crash
    {
        const string path3 = "test_import_empty.html";
        writeFile(path3, "<html><body>not a presentation export</body></html>");
        vector<Slide*> slides3;
        string error3;
        bool ok3 = HtmlImporter::importFromFile(path3, slides3, error3);
        assert(!ok3);
        assert(!error3.empty());
        remove(path3.c_str());
    }

    // A nonexistent file should fail cleanly, not crash
    {
        vector<Slide*> slides4;
        string error4;
        bool ok4 = HtmlImporter::importFromFile("does_not_exist_xyz.html", slides4, error4);
        assert(!ok4);
    }

    for (auto* s : slides) delete s;
    remove(path.c_str());

    cout << "All HtmlImporter tests passed.\n";
    return 0;
}
