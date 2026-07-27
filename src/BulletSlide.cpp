#include "BulletSlide.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include "Utils.h"
#include "Json.h"
#include <iostream>

using namespace std;

BulletSlide::BulletSlide(string h)
{
    heading = h;
}

bool BulletSlide::addBullet(string point)
{
    if ((int)bullets.size() >= MAX_BULLETS)
        return false;
    bullets.push_back(point);
    return true;
}

void BulletSlide::display()
{
    cout << "\n  [Bullet Slide]\n";
    cout << "  Heading : " << heading << "\n";
    for (const auto& b : bullets)
        cout << "    \xE2\x80\xA2 " << b << "\n";
}

string BulletSlide::generateHTML(Theme& t, int n)
{
    string items;
    for (const auto& b : bullets)
        items += "<li>" + escapeHTML(b) + "</li>";

    return "<div class='slide'>"
        "<p class='number'>Slide " + to_string(n) + "</p>"
        "<h2>" + escapeHTML(heading) + "</h2>"
        "<ul>" + items + "</ul>"
        "</div>\n";
}

void BulletSlide::addToPPTX(PptxBuilder& b, Theme& t)
{
    // No more building a temporary vector from a raw array first -
    // bullets already IS a vector<string>.
    b.addBulletSlide(t.getBgColor(), heading, t.getHeadingColor(), bullets, t.getTextColor());
}

string BulletSlide::toTemplateJSON()
{
    string items;
    for (size_t i = 0; i < bullets.size(); i++)
    {
        if (i > 0) items += ",";
        items += "\"" + jsonEscape(bullets[i]) + "\"";
    }
    return "{\"type\":\"bullets\",\"heading\":\"" + jsonEscape(heading) +
           "\",\"bullets\":[" + items + "]}";
}

string BulletSlide::summary() { return "Bullet Slide   : " + heading; }
