#include "HtmlImporter.h"
#include "TitleSlide.h"
#include "ContentSlide.h"
#include "BulletSlide.h"
#include "ImageSlide.h"
#include "CustomHTMLSlide.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

using namespace std;

namespace
{
    // Exact substring extraction between an opening and closing tag. Our
    // own export always emits these tags without extra attributes (except
    // where noted below), so exact matches are simpler and safer than a
    // general-purpose attribute-aware HTML parser.
    string extractBetween(const string& block, const string& openNeedle, const string& closeNeedle)
    {
        size_t start = block.find(openNeedle);
        if (start == string::npos) return "";
        size_t contentStart = start + openNeedle.size();
        size_t end = block.find(closeNeedle, contentStart);
        if (end == string::npos) return "";
        return block.substr(contentStart, end - contentStart);
    }

    vector<string> extractAllLi(const string& block)
    {
        vector<string> out;
        size_t pos = 0;
        while (true)
        {
            size_t start = block.find("<li>", pos);
            if (start == string::npos) break;
            size_t contentStart = start + 4;
            size_t end = block.find("</li>", contentStart);
            if (end == string::npos) break;
            out.push_back(block.substr(contentStart, end - contentStart));
            pos = end + 5;
        }
        return out;
    }

    string extractImgSrc(const string& block)
    {
        const string needle = "<img src='";
        size_t start = block.find(needle);
        if (start == string::npos) return "";
        start += needle.size();
        size_t end = block.find('\'', start);
        if (end == string::npos) return "";
        return block.substr(start, end - start);
    }

    // Removes the "Slide N" (or "Slide N (Custom)") marker paragraph our
    // own export always adds, so it doesn't get mistaken for real content.
    string stripNumberMarker(const string& block)
    {
        size_t start = block.find("<p class='number'>");
        if (start == string::npos) return block;
        size_t end = block.find("</p>", start);
        if (end == string::npos) return block;
        return block.substr(0, start) + block.substr(end + 4);
    }

    // Finds each top-level <div class='slide'>...</div> block, correctly
    // handling nested <div> tags inside (relevant for CustomHTMLSlide
    // content, which may itself contain divs).
    vector<string> splitSlideBlocks(const string& html)
    {
        vector<string> blocks;
        const string needle = "<div class='slide'>";
        size_t pos = 0;
        while (true)
        {
            size_t start = html.find(needle, pos);
            if (start == string::npos) break;
            size_t cursor = start + needle.size();
            size_t blockContentStart = cursor;
            int depth = 1;

            while (depth > 0)
            {
                size_t nextOpen = html.find("<div", cursor);
                size_t nextClose = html.find("</div>", cursor);

                if (nextClose == string::npos)
                {
                    cursor = html.size();
                    depth = 0;
                    break;
                }
                if (nextOpen != string::npos && nextOpen < nextClose)
                {
                    depth++;
                    cursor = nextOpen + 4;
                }
                else
                {
                    depth--;
                    if (depth == 0)
                    {
                        blocks.push_back(html.substr(blockContentStart, nextClose - blockContentStart));
                        cursor = nextClose + 6;
                    }
                    else
                    {
                        cursor = nextClose + 6;
                    }
                }
            }
            pos = cursor;
        }
        return blocks;
    }
}

bool HtmlImporter::importFromFile(const string& path, vector<Slide*>& outSlides, string& error)
{
    ifstream file(path);
    if (!file.is_open())
    {
        error = "Could not open " + path;
        return false;
    }

    stringstream ss;
    ss << file.rdbuf();
    string html = ss.str();

    vector<string> blocks = splitSlideBlocks(html);
    if (blocks.empty())
    {
        error = "No slides found - is this a file exported by this program (Export to HTML)?";
        return false;
    }

    outSlides.clear();
    for (auto& rawBlock : blocks)
    {
        string block = stripNumberMarker(rawBlock);

        string h1 = extractBetween(block, "<h1>", "</h1>");
        string img = extractImgSrc(block);
        string h2 = extractBetween(block, "<h2>", "</h2>");
        vector<string> lis = extractAllLi(block);

        if (!h1.empty())
        {
            string h3 = extractBetween(block, "<h3>", "</h3>");
            outSlides.push_back(new TitleSlide(unescapeHTML(h1), unescapeHTML(h3)));
        }
        else if (!img.empty())
        {
            string caption = extractBetween(block, "<p class='caption'>", "</p>");
            outSlides.push_back(new ImageSlide(unescapeHTML(img), unescapeHTML(caption)));
        }
        else if (!h2.empty() && !lis.empty())
        {
            BulletSlide* bs = new BulletSlide(unescapeHTML(h2));
            for (auto& li : lis)
                bs->addBullet(unescapeHTML(li));
            outSlides.push_back(bs);
        }
        else if (!h2.empty())
        {
            string content = extractBetween(block, "<p>", "</p>");
            outSlides.push_back(new ContentSlide(unescapeHTML(h2), unescapeHTML(content)));
        }
        else
        {
            // Doesn't match a recognized shape (e.g. a CustomHTMLSlide, or
            // hand-edited markup) - preserve it rather than dropping it.
            outSlides.push_back(new CustomHTMLSlide(block, "Imported slide"));
        }
    }

    return true;
}
