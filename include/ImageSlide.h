#pragma once
#include "Slide.h"

class ImageSlide : public Slide
{
private:
    std::string imagePath;
    std::string caption;

public:
    ImageSlide(std::string path, std::string cap);
    void        display() override;
    std::string generateHTML(Theme& t, int n) override;
    void        addToPPTX(PptxBuilder& b, Theme& t) override;
    std::string toTemplateJSON() override;
    std::string summary() override;
};
