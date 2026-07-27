#pragma once
#include "Slide.h"
#include "Constants.h"
#include <vector>

class BulletSlide : public Slide
{
private:
    std::string heading;
    std::vector<std::string> bullets;

public:
    explicit BulletSlide(std::string h);
    // Still returns bool and still enforces MAX_BULLETS: that's a
    // deliberate UI/content limit (a slide shouldn't have 50 bullets),
    // not a storage constraint - vector has no fixed capacity, so the
    // cap is now purely a business rule rather than a memory limit.
    bool        addBullet(std::string point);
    void        display() override;
    std::string generateHTML(Theme& t, int n) override;
    void        addToPPTX(PptxBuilder& b, Theme& t) override;
    std::string toTemplateJSON() override;
    std::string summary() override;
};
