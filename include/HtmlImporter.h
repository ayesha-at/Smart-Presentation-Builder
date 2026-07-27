#pragma once
#include <string>
#include <vector>

class Slide;

// Parses an HTML file previously exported by this program (Export to HTML
// or Export Each Slide Separately) and reconstructs real, editable Slide
// objects from it. Designed to round-trip THIS program's own markup, not
// arbitrary third-party HTML - anything that doesn't match a recognized
// slide shape is preserved as a CustomHTMLSlide rather than dropped.
namespace HtmlImporter
{
    // Caller takes ownership of the returned Slide pointers (typically via
    // Presentation::addSlide).
    bool importFromFile(const std::string& path, std::vector<Slide*>& outSlides, std::string& error);
}
