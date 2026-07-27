#pragma once
#include "Slide.h"

class ContentSlide : public Slide
{
private:
    std::string heading;
    std::string content;

public:
    ContentSlide(std::string h, std::string c);
    void        display() override;
    std::string generateHTML(Theme& t, int n) override;
    void        addToPPTX(PptxBuilder& b, Theme& t) override;
    std::string toTemplateJSON() override;
    std::string summary() override;
};
