#pragma once
#include "Slide.h"

class CustomHTMLSlide : public Slide
{
private:
    std::string customHTML;
    std::string description;

public:
    CustomHTMLSlide(std::string html, std::string desc);
    void        display() override;
    std::string generateHTML(Theme& t, int n) override;
    void        addToPPTX(PptxBuilder& b, Theme& t) override;
    std::string toTemplateJSON() override;
    std::string summary() override;
};
