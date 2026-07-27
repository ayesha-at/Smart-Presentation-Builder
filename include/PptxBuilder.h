#pragma once
#include <string>
#include <vector>
#include <utility>

// Builds a valid PowerPoint (.pptx) file directly - no Node.js, no Python,
// no internet connection needed. A .pptx is just a zip of XML files; this
// class assembles that XML and zips it via miniz (vendored in third_party/).
class PptxBuilder
{
private:
    static const long long SLIDE_W = 9144000; // 10 in, EMU (16:9 canvas)
    static const long long SLIDE_H = 5143500; // 5.625 in, EMU

    std::vector<std::string> slideXml;
    std::vector<std::string> slideRels;
    std::vector<std::pair<std::string, std::vector<unsigned char>>> media;
    int nextMediaId = 1;

    static std::string stripHash(const std::string& c);
    static std::string xmlEscape(const std::string& s);
    static std::string textBox(long long x, long long y, long long cx, long long cy,
                                const std::string& text, int sizePt, const std::string& color,
                                bool bold, const std::string& align, int shapeId);
    static std::string bulletBox(long long x, long long y, long long cx, long long cy,
                                  const std::vector<std::string>& items, int sizePt,
                                  const std::string& color, int shapeId);
    static std::string slideShell(const std::string& bgColor, const std::string& shapesXml);

public:
    void addTitleSlide(const std::string& bgColor, const std::string& title,
                        const std::string& headingColor, const std::string& subtitle,
                        const std::string& textColor);

    void addContentSlide(const std::string& bgColor, const std::string& heading,
                          const std::string& headingColor, const std::string& content,
                          const std::string& textColor);

    void addBulletSlide(const std::string& bgColor, const std::string& heading,
                         const std::string& headingColor, const std::vector<std::string>& bullets,
                         const std::string& textColor);

    // Embeds the actual image file if it can be read from disk; otherwise
    // falls back to a text-only slide so export never silently loses content.
    void addImageSlide(const std::string& bgColor, const std::string& imagePath,
                        const std::string& caption, const std::string& textColor);

    // Custom raw-HTML slides can't be faithfully converted to OOXML, so this
    // surfaces the slide's description as plain text rather than dropping it.
    void addNoteSlide(const std::string& bgColor, const std::string& description,
                       const std::string& textColor);

    bool save(const std::string& filename);
};
