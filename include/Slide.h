#pragma once
#include <string>

// Forward declarations only - keeps this header lightweight since it's
// included by every slide type and by Presentation.h.
class Theme;
class PptxBuilder;

class Slide
{
public:
    virtual void        display() = 0;
    virtual std::string generateHTML(Theme& t, int n) = 0;
    virtual void        addToPPTX(PptxBuilder& b, Theme& t) = 0;
    // Serializes this slide to a small JSON object (same shape the AI
    // outline feature uses: type/heading/subtitle/content/bullets), so
    // presentations can be saved and reloaded as reusable templates.
    virtual std::string toTemplateJSON() = 0;
    virtual std::string summary() { return "[Slide]"; }
    virtual ~Slide() {}
};
