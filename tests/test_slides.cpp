// Direct tests for each Slide subclass's generateHTML() and
// toTemplateJSON() output. test_html_importer.cpp already covers the
// round-trip (export -> import -> same data), but these check the raw
// output shape and escaping directly, which is what actually determines
// correctness independent of whether the importer happens to recover it.
#include "TitleSlide.h"
#include "ContentSlide.h"
#include "BulletSlide.h"
#include "ImageSlide.h"
#include "CustomHTMLSlide.h"
#include "Theme.h"
#include "Json.h"
#include <cassert>
#include <iostream>

using namespace std;

int main()
{
    Theme theme("Test", "#111111", "#222222", "#333333", "Arial", "#444444");

    // TitleSlide
    {
        TitleSlide s("My <Title>", "Sub & Title");
        string html = s.generateHTML(theme, 1);
        assert(html.find("<h1>My &lt;Title&gt;</h1>") != string::npos);
        assert(html.find("<h3>Sub &amp; Title</h3>") != string::npos);
        assert(html.find("Slide 1") != string::npos);
        assert(s.summary() == "Title Slide    : My <Title>"); // summary() uses raw text, not escaped

        bool ok;
        JsonValue json = parseJson(s.toTemplateJSON(), ok);
        assert(ok && json.get("type").asString() == "title");
        assert(json.get("title").asString() == "My <Title>"); // JSON escaping, not HTML escaping
        assert(json.get("subtitle").asString() == "Sub & Title");
    }

    // ContentSlide
    {
        ContentSlide s("Heading \"Quoted\"", "Body content.");
        string html = s.generateHTML(theme, 2);
        assert(html.find("<h2>Heading &quot;Quoted&quot;</h2>") != string::npos);
        assert(html.find("<p>Body content.</p>") != string::npos);

        bool ok;
        JsonValue json = parseJson(s.toTemplateJSON(), ok);
        assert(ok && json.get("type").asString() == "content");
        assert(json.get("heading").asString() == "Heading \"Quoted\"");
        assert(json.get("content").asString() == "Body content.");
    }

    // BulletSlide
    {
        BulletSlide s("Agenda");
        assert(s.addBullet("First point"));
        assert(s.addBullet("Second & point"));
        string html = s.generateHTML(theme, 3);
        assert(html.find("<li>First point</li>") != string::npos);
        assert(html.find("<li>Second &amp; point</li>") != string::npos);

        // MAX_BULLETS enforcement: fill to the cap, next add must fail
        BulletSlide capped("Cap Test");
        for (int i = 0; i < MAX_BULLETS; i++)
            assert(capped.addBullet("bullet " + to_string(i)));
        assert(!capped.addBullet("one too many"));

        bool ok;
        JsonValue json = parseJson(s.toTemplateJSON(), ok);
        assert(ok && json.get("type").asString() == "bullets");
        assert(json.get("bullets").isArray());
        assert(json.get("bullets").arrayValue.size() == 2);
    }

    // ImageSlide
    {
        ImageSlide s("path/to/pic.png", "A caption");
        string html = s.generateHTML(theme, 4);
        assert(html.find("<img src='path/to/pic.png' alt='slide image'>") != string::npos);
        assert(html.find("<p class='caption'>A caption</p>") != string::npos);

        // The apostrophe case flagged in review: single quotes in the
        // path must not break out of the single-quoted src attribute.
        ImageSlide apostrophe("O'Brien's Photos/pic.png", "cap");
        string html2 = apostrophe.generateHTML(theme, 5);
        size_t srcStart = html2.find("src='") + 5;
        size_t srcEnd = html2.find('\'', srcStart);
        string srcValue = html2.substr(srcStart, srcEnd - srcStart);
        assert(srcValue.find('\'') == string::npos);

        bool ok;
        JsonValue json = parseJson(s.toTemplateJSON(), ok);
        assert(ok && json.get("type").asString() == "image");
        assert(json.get("path").asString() == "path/to/pic.png");
        assert(json.get("caption").asString() == "A caption");
    }

    // CustomHTMLSlide - deliberately NOT escaped, since raw HTML is the
    // whole point of this slide type
    {
        CustomHTMLSlide s("<div><b>bold</b></div>", "My custom slide");
        string html = s.generateHTML(theme, 6);
        assert(html.find("<div><b>bold</b></div>") != string::npos); // raw, unescaped

        bool ok;
        JsonValue json = parseJson(s.toTemplateJSON(), ok);
        assert(ok && json.get("type").asString() == "custom_html");
        assert(json.get("html").asString() == "<div><b>bold</b></div>");
        assert(json.get("description").asString() == "My custom slide");
    }

    cout << "All slide-type tests passed.\n";
    return 0;
}
