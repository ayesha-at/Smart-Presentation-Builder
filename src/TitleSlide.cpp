#include "TitleSlide.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include "Utils.h"
#include "Json.h"
#include <iostream>

using namespace std;

TitleSlide::TitleSlide(string t, string s)
{
    title = t;
    subtitle = s;
}

void TitleSlide::display()
{
    cout << "\n  [Title Slide]\n";
    cout << "  Title    : " << title << "\n";
    cout << "  Subtitle : " << subtitle << "\n";
}

string TitleSlide::generateHTML(Theme& t, int n)
{
    return "<div class='slide'>"
        "<p class='number'>Slide " + to_string(n) + "</p>"
        "<h1>" + escapeHTML(title) + "</h1>"
        "<h3>" + escapeHTML(subtitle) + "</h3>"
        "</div>\n";
}

void TitleSlide::addToPPTX(PptxBuilder& b, Theme& t)
{
    b.addTitleSlide(t.getBgColor(), title, t.getHeadingColor(), subtitle, t.getTextColor());
}

string TitleSlide::toTemplateJSON()
{
    return "{\"type\":\"title\",\"title\":\"" + jsonEscape(title) +
           "\",\"subtitle\":\"" + jsonEscape(subtitle) + "\"}";
}

string TitleSlide::summary() { return "Title Slide    : " + title; }
