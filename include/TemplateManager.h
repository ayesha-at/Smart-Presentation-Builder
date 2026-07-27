#pragma once
#include <string>
#include <vector>

class Slide;

// A small library of built-in multi-slide skeletons, plus loading
// previously-saved templates (written by Presentation::exportTemplate)
// back into real Slide objects. Templates use the same slide-spec JSON
// shape as the AI outline feature (type/heading/subtitle/content/bullets),
// so the two features share one mental model.
namespace TemplateManager
{
    struct BuiltinTemplate
    {
        std::string name;
        std::string description;
    };

    // Lists available built-in templates for menu display.
    std::vector<BuiltinTemplate> listBuiltins();

    // Builds the slides for a built-in template by index (from
    // listBuiltins()). Caller takes ownership (typically via
    // Presentation::addSlide).
    std::vector<Slide*> buildBuiltin(int index);

    // Loads a template JSON file (as written by Presentation::
    // exportTemplate) into real Slide objects.
    bool loadFromFile(const std::string& path, std::vector<Slide*>& outSlides, std::string& error);
}
