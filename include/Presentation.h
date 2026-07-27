#pragma once
#include <vector>
#include <string>
#include "Slide.h"

class Theme;

class Presentation
{
private:
    std::vector<Slide*> slides;
    Theme* currentTheme;

public:
    Presentation();

    // Rule of three: Presentation owns raw pointers, so copying must be
    // disabled to avoid a double-delete when two copies both destruct.
    Presentation(const Presentation&) = delete;
    Presentation& operator=(const Presentation&) = delete;

    void        setTheme(int choice);
    std::string getThemeName();
    bool        addSlide(Slide* s);
    void        deleteSlide(int index);
    void        moveSlide(int from, int to);
    void        summary();
    void        showSlides();
    void        exportIndividualSlides();
    void        exportHTML();      // now builds an interactive, navigable HTML deck
    void        exportPPTX();      // builds presentation.pptx directly, no external tools needed
    void        exportTemplate(const std::string& path); // saves slides as a reusable JSON template

    // Returns each slide's summary() text, in current order, without
    // printing anything. Exists mainly so tests can verify add/delete/
    // move actually reorder slides correctly, without needing to capture
    // stdout.
    std::vector<std::string> slideSummaries() const;
    int slideCount() const;

    ~Presentation();
};
