#include "PptxBuilder.h"
#include "miniz.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>

using namespace std;

string PptxBuilder::stripHash(const string& c)
{
    return (!c.empty() && c[0] == '#') ? c.substr(1) : c;
}

namespace
{
    // Every slide part MUST declare a relationship to its slide layout -
    // this isn't optional decoration, PowerPoint validates it strictly
    // (even though more lenient readers like LibreOffice tolerate its
    // absence). rId1 is reserved for this on every slide; if a slide also
    // has an image, that gets rId2.
    string layoutOnlyRels()
    {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" "
               "Target=\"../slideLayouts/slideLayout1.xml\"/></Relationships>";
    }

    string layoutPlusImageRels(const string& mediaName)
    {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" "
               "Target=\"../slideLayouts/slideLayout1.xml\"/>"
               "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
               "Target=\"../media/" + mediaName + "\"/></Relationships>";
    }
}

string PptxBuilder::xmlEscape(const string& s)
{
    string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;
        }
    }
    return out;
}

string PptxBuilder::textBox(long long x, long long y, long long cx, long long cy,
                             const string& text, int sizePt, const string& color,
                             bool bold, const string& align, int shapeId)
{
    ostringstream o;
    o << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << shapeId << "\" name=\"TextBox " << shapeId << "\"/>"
      << "<p:cNvSpPr txBox=\"1\"/><p:nvPr/></p:nvSpPr>"
      << "<p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y << "\"/>"
      << "<a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm>"
      << "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:noFill/></p:spPr>"
      << "<p:txBody><a:bodyPr wrap=\"square\"><a:normAutofit/></a:bodyPr><a:lstStyle/>"
      << "<a:p><a:pPr algn=\"" << align << "\"/>"
      << "<a:r><a:rPr lang=\"en-US\" sz=\"" << (sizePt * 100) << "\" b=\"" << (bold ? 1 : 0) << "\">"
      << "<a:solidFill><a:srgbClr val=\"" << stripHash(color) << "\"/></a:solidFill>"
      << "<a:latin typeface=\"Arial\"/></a:rPr><a:t>" << xmlEscape(text) << "</a:t></a:r></a:p>"
      << "</p:txBody></p:sp>";
    return o.str();
}

string PptxBuilder::bulletBox(long long x, long long y, long long cx, long long cy,
                               const vector<string>& items, int sizePt,
                               const string& color, int shapeId)
{
    ostringstream o;
    o << "<p:sp><p:nvSpPr><p:cNvPr id=\"" << shapeId << "\" name=\"TextBox " << shapeId << "\"/>"
      << "<p:cNvSpPr txBox=\"1\"/><p:nvPr/></p:nvSpPr>"
      << "<p:spPr><a:xfrm><a:off x=\"" << x << "\" y=\"" << y << "\"/>"
      << "<a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/></a:xfrm>"
      << "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:noFill/></p:spPr>"
      << "<p:txBody><a:bodyPr wrap=\"square\"><a:normAutofit/></a:bodyPr><a:lstStyle/>";
    for (const auto& item : items)
    {
        o << "<a:p><a:pPr marL=\"285750\" indent=\"-285750\">"
          << "<a:buFont typeface=\"Arial\"/><a:buChar char=\"\xE2\x80\xA2\"/></a:pPr>"
          << "<a:r><a:rPr lang=\"en-US\" sz=\"" << (sizePt * 100) << "\">"
          << "<a:solidFill><a:srgbClr val=\"" << stripHash(color) << "\"/></a:solidFill>"
          << "<a:latin typeface=\"Arial\"/></a:rPr><a:t>" << xmlEscape(item) << "</a:t></a:r></a:p>";
    }
    o << "</p:txBody></p:sp>";
    return o.str();
}

string PptxBuilder::slideShell(const string& bgColor, const string& shapesXml)
{
    ostringstream o;
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      << "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
      << "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
      << "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
      << "<p:cSld><p:bg><p:bgPr><a:solidFill><a:srgbClr val=\"" << stripHash(bgColor) << "\"/></a:solidFill>"
      << "<a:effectLst/></p:bgPr></p:bg>"
      << "<p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
      << "<p:grpSpPr/>" << shapesXml << "</p:spTree></p:cSld>"
      << "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>";
    return o.str();
}

void PptxBuilder::addTitleSlide(const string& bgColor, const string& title,
                                 const string& headingColor, const string& subtitle,
                                 const string& textColor)
{
    string shapes = textBox(914400, 2011680, 7772400, 1600200, title, 44, headingColor, true, "ctr", 2);
    if (!subtitle.empty())
        shapes += textBox(914400, 3703320, 7772400, 900000, subtitle, 20, textColor, false, "ctr", 3);
    slideXml.push_back(slideShell(bgColor, shapes));
    slideRels.push_back(layoutOnlyRels());
}

void PptxBuilder::addContentSlide(const string& bgColor, const string& heading,
                                   const string& headingColor, const string& content,
                                   const string& textColor)
{
    string shapes = textBox(914400, 731520, 7772400, 1097280, heading, 32, headingColor, true, "l", 2);
    shapes += textBox(914400, 2286000, 7772400, 2743200, content, 18, textColor, false, "l", 3);
    slideXml.push_back(slideShell(bgColor, shapes));
    slideRels.push_back(layoutOnlyRels());
}

void PptxBuilder::addBulletSlide(const string& bgColor, const string& heading,
                                  const string& headingColor, const vector<string>& bullets,
                                  const string& textColor)
{
    string shapes = textBox(914400, 731520, 7772400, 1097280, heading, 32, headingColor, true, "l", 2);
    shapes += bulletBox(914400, 2286000, 7772400, 2743200, bullets, 18, textColor, 3);
    slideXml.push_back(slideShell(bgColor, shapes));
    slideRels.push_back(layoutOnlyRels());
}

void PptxBuilder::addImageSlide(const string& bgColor, const string& imagePath,
                                 const string& caption, const string& textColor)
{
    ifstream file(imagePath, ios::binary);
    if (!file)
    {
        string shapes = textBox(914400, 2011680, 7772400, 1600200,
                                 "[Image not found: " + imagePath + "]", 20, textColor, false, "ctr", 2);
        if (!caption.empty())
            shapes += textBox(914400, 3703320, 7772400, 700000, caption, 16, textColor, false, "ctr", 3);
        slideXml.push_back(slideShell(bgColor, shapes));
        slideRels.push_back(layoutOnlyRels());
        return;
    }

    vector<unsigned char> bytes((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    string ext = "png";
    size_t dot = imagePath.find_last_of('.');
    if (dot != string::npos)
    {
        ext = imagePath.substr(dot + 1);
        for (auto& c : ext) c = (char)tolower(c);
        if (ext == "jpeg") ext = "jpg";
    }

    int mediaId = nextMediaId++;
    string mediaName = "image" + to_string(mediaId) + "." + ext;
    media.push_back({ mediaName, bytes });

    long long imgY = caption.empty() ? 643000 : 300000;
    ostringstream shapes;
    shapes << "<p:pic><p:nvPicPr><p:cNvPr id=\"2\" name=\"Picture\"/>"
           << "<p:cNvPicPr/><p:nvPr/></p:nvPicPr>"
           << "<p:blipFill><a:blip r:embed=\"rId2\"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>"
           << "<p:spPr><a:xfrm><a:off x=\"1600200\" y=\"" << imgY << "\"/>"
           << "<a:ext cx=\"5943600\" cy=\"3886200\"/></a:xfrm>"
           << "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr></p:pic>";
    if (!caption.empty())
        shapes << textBox(914400, 4700000, 7772400, 400000, caption, 14, textColor, false, "ctr", 3);

    slideXml.push_back(slideShell(bgColor, shapes.str()));
    slideRels.push_back(layoutPlusImageRels(mediaName));
}

void PptxBuilder::addNoteSlide(const string& bgColor, const string& description,
                                const string& textColor)
{
    string shapes = textBox(914400, 731520, 7772400, 1097280, "Custom HTML Slide", 32, textColor, true, "l", 2);
    shapes += textBox(914400, 2286000, 7772400, 900000,
                       description.empty() ? "(no description provided)" : description, 18, textColor, false, "l", 3);
    shapes += textBox(914400, 3400000, 7772400, 900000,
                       "Note: custom HTML content can't be converted to PowerPoint shapes automatically.",
                       14, textColor, false, "l", 4);
    slideXml.push_back(slideShell(bgColor, shapes));
    slideRels.push_back(layoutOnlyRels());
}

bool PptxBuilder::save(const string& filename)
{
    int n = (int)slideXml.size();
    if (n == 0) return false;

    ostringstream contentTypes;
    contentTypes << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        << "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        << "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        << "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        << "<Default Extension=\"png\" ContentType=\"image/png\"/>"
        << "<Default Extension=\"jpg\" ContentType=\"image/jpeg\"/>"
        << "<Default Extension=\"gif\" ContentType=\"image/gif\"/>"
        << "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
        << "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
        << "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
        << "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
        << "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
        << "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>";
    for (int i = 1; i <= n; i++)
        contentTypes << "<Override PartName=\"/ppt/slides/slide" << i << ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
    contentTypes << "</Types>";

    string rootRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
        "</Relationships>";

    string coreProps =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\">"
        "<dc:title>Presentation</dc:title><dc:creator>Smart Presentation Builder</dc:creator>"
        "</cp:coreProperties>";

    string appProps =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>Smart Presentation Builder</Application><Slides>" + to_string(n) + "</Slides>"
        "</Properties>";

    ostringstream presXml;
    presXml << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        << "<p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        << "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        << "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
        << "<p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
        << "<p:sldIdLst>";
    for (int i = 0; i < n; i++)
        presXml << "<p:sldId id=\"" << (256 + i) << "\" r:id=\"rId" << (i + 2) << "\"/>";
    presXml << "</p:sldIdLst>"
        << "<p:sldSz cx=\"" << SLIDE_W << "\" cy=\"" << SLIDE_H << "\"/>"
        << "<p:notesSz cx=\"6858000\" cy=\"9144000\"/></p:presentation>";

    ostringstream presRels;
    presRels << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        << "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/>";
    for (int i = 0; i < n; i++)
        presRels << "<Relationship Id=\"rId" << (i + 2) << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide" << (i + 1) << ".xml\"/>";
    presRels << "</Relationships>";

    string slideMaster =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:sldMaster xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
        "<p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr/></p:spTree></p:cSld>"
        "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" "
        "accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>"
        "<p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst>"
        "</p:sldMaster>";

    string slideMasterRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"../theme/theme1.xml\"/>"
        "</Relationships>";

    string slideLayout =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:sldLayout xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" type=\"blank\" preserve=\"1\">"
        "<p:cSld name=\"Blank\"><p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr/></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>";

    string slideLayoutRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/>"
        "</Relationships>";

    string theme =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Custom\">"
        "<a:themeElements><a:clrScheme name=\"Custom\">"
        "<a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1>"
        "<a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1>"
        "<a:dk2><a:srgbClr val=\"1F497D\"/></a:dk2><a:lt2><a:srgbClr val=\"EEECE1\"/></a:lt2>"
        "<a:accent1><a:srgbClr val=\"4F81BD\"/></a:accent1><a:accent2><a:srgbClr val=\"C0504D\"/></a:accent2>"
        "<a:accent3><a:srgbClr val=\"9BBB59\"/></a:accent3><a:accent4><a:srgbClr val=\"8064A2\"/></a:accent4>"
        "<a:accent5><a:srgbClr val=\"4BACC6\"/></a:accent5><a:accent6><a:srgbClr val=\"F79646\"/></a:accent6>"
        "<a:hlink><a:srgbClr val=\"0000FF\"/></a:hlink><a:folHlink><a:srgbClr val=\"800080\"/></a:folHlink>"
        "</a:clrScheme>"
        "<a:fontScheme name=\"Custom\"><a:majorFont><a:latin typeface=\"Arial\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont>"
        "<a:minorFont><a:latin typeface=\"Arial\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont></a:fontScheme>"
        "<a:fmtScheme name=\"Custom\">"
        "<a:fillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:fillStyleLst>"
        "<a:lnStyleLst><a:ln><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln></a:lnStyleLst>"
        "<a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst>"
        "<a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:bgFillStyleLst>"
        "</a:fmtScheme></a:themeElements></a:theme>";

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_file(&zip, filename.c_str(), 0))
        return false;

    auto add = [&](const string& path, const string& data) -> bool {
        return mz_zip_writer_add_mem(&zip, path.c_str(), data.data(), data.size(), MZ_DEFAULT_COMPRESSION);
    };

    bool ok = true;
    ok = ok && add("[Content_Types].xml", contentTypes.str());
    ok = ok && add("_rels/.rels", rootRels);
    ok = ok && add("docProps/core.xml", coreProps);
    ok = ok && add("docProps/app.xml", appProps);
    ok = ok && add("ppt/presentation.xml", presXml.str());
    ok = ok && add("ppt/_rels/presentation.xml.rels", presRels.str());
    ok = ok && add("ppt/slideMasters/slideMaster1.xml", slideMaster);
    ok = ok && add("ppt/slideMasters/_rels/slideMaster1.xml.rels", slideMasterRels);
    ok = ok && add("ppt/slideLayouts/slideLayout1.xml", slideLayout);
    ok = ok && add("ppt/slideLayouts/_rels/slideLayout1.xml.rels", slideLayoutRels);
    ok = ok && add("ppt/theme/theme1.xml", theme);

    for (int i = 0; i < n && ok; i++)
    {
        ok = add("ppt/slides/slide" + to_string(i + 1) + ".xml", slideXml[i]);
        ok = ok && add("ppt/slides/_rels/slide" + to_string(i + 1) + ".xml.rels", slideRels[i]);
    }

    for (auto& m : media)
    {
        if (!ok) break;
        ok = mz_zip_writer_add_mem(&zip, ("ppt/media/" + m.first).c_str(),
                                    m.second.data(), m.second.size(), MZ_DEFAULT_COMPRESSION);
    }

    ok = ok && mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    return ok;
}
