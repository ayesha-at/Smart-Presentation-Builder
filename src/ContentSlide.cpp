#include "ContentSlide.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include "Utils.h"
#include "Json.h"
#include <iostream>

using namespace std;

ContentSlide::ContentSlide(string h, string c)
{
    heading = h;
    content = c;
}

void ContentSlide::display()
{
    cout << "\n  [Content Slide]\n";
    cout << "  Heading : " << heading << "\n";
    cout << "  Content : " << content << "\n";
}

string ContentSlide::generateHTML(Theme& t, int n)
{
    return "<div class='slide'>"
        "<p class='number'>Slide " + to_string(n) + "</p>"
        "<h2>" + escapeHTML(heading) + "</h2>"
        "<p>" + escapeHTML(content) + "</p>"
        "</div>\n";
}

void ContentSlide::addToPPTX(PptxBuilder& b, Theme& t)
{
    b.addContentSlide(t.getBgColor(), heading, t.getHeadingColor(), content, t.getTextColor());
}

string ContentSlide::toTemplateJSON()
{
    return "{\"type\":\"content\",\"heading\":\"" + jsonEscape(heading) +
           "\",\"content\":\"" + jsonEscape(content) + "\"}";
}

string ContentSlide::summary() { return "Content Slide  : " + heading; }
