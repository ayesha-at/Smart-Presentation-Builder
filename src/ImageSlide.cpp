#include "ImageSlide.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include "Utils.h"
#include "Json.h"
#include <iostream>

using namespace std;

ImageSlide::ImageSlide(string path, string cap)
{
    imagePath = path;
    caption = cap;
}

void ImageSlide::display()
{
    cout << "\n  [Image Slide]\n";
    cout << "  Image   : " << imagePath << "\n";
    cout << "  Caption : " << caption << "\n";
}

string ImageSlide::generateHTML(Theme& t, int n)
{
    // src is delimited with single quotes - this relies on escapeHTML
    // also escaping literal ' (see its doc comment in Utils.h). A path
    // like O'Brien's Photos/pic.png would otherwise close the attribute
    // early and corrupt the markup.
    return "<div class='slide'>"
        "<p class='number'>Slide " + to_string(n) + "</p>"
        "<img src='" + escapeHTML(imagePath) + "' alt='slide image'>"
        "<p class='caption'>" + escapeHTML(caption) + "</p>"
        "</div>\n";
}

void ImageSlide::addToPPTX(PptxBuilder& b, Theme& t)
{
    b.addImageSlide(t.getBgColor(), imagePath, caption, t.getTextColor());
}

string ImageSlide::toTemplateJSON()
{
    return "{\"type\":\"image\",\"path\":\"" + jsonEscape(imagePath) +
           "\",\"caption\":\"" + jsonEscape(caption) + "\"}";
}

string ImageSlide::summary() { return "Image Slide    : " + caption; }
