#pragma once
#include "Slide.h"

class TitleSlide : public Slide
{
private:
    std::string title;
    std::string subtitle;

public:
    TitleSlide(std::string t, std::string s);
    void        display() override;
    std::string generateHTML(Theme& t, int n) override;
    void        addToPPTX(PptxBuilder& b, Theme& t) override;
    std::string toTemplateJSON() override;
    std::string summary() override;
};
