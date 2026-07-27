#include "CustomHTMLSlide.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include "Json.h"
#include <iostream>
#include <cctype>

using namespace std;

namespace
{
    // If the user pasted a full HTML document into a custom slide,
    // strip the outer <!DOCTYPE>...<html>...<head>...<body>...</body></html>
    // wrapper so the combined export doesn't end up with nested full
    // documents mid-file (which breaks the rest of the deck). Returns
    // the input unchanged if it doesn't look like a full document.
    string bodyOnly(const string& s)
    {
        // Find the first non-whitespace character.
        size_t pos = 0;
        while (pos < s.size() && isspace((unsigned char)s[pos])) ++pos;

        auto ciStartsWith = [&](const char* lit) -> bool {
            size_t n = 0;
            while (lit[n]) ++n;
            if (pos + n > s.size()) return false;
            for (size_t i = 0; i < n; ++i)
            {
                char a = s[pos + i];
                char b = lit[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) return false;
            }
            return true;
            };

        // Quick gate: only unwrap if the input really does look like a
        // full HTML document. Anything else (a fragment, plain text, a
        // partial paste) is returned unchanged.
        if (!ciStartsWith("<!doctype") && !ciStartsWith("<html"))
            return s;

        // Find the body element. The opener may have attributes
        // (e.g. <body class="foo">), so scan for "<body" then advance
        // past the closing '>'.
        size_t bodyOpen = s.find("<body");
        if (bodyOpen == string::npos) return s;
        size_t bodyOpenEnd = s.find('>', bodyOpen);
        if (bodyOpenEnd == string::npos) return s;

        // Use the LAST closing </body> tag as the document close.
        // Inner HTML examples shown inside <code> or <script> blocks
        // may legitimately contain the literal string "</body>" before
        // the actual document end. Using rfind() instead of find()
        // unwraps the outer document rather than stopping early on
        // some example code.
        size_t bodyClose = s.rfind("</body>");
        if (bodyClose == string::npos) return s;
        if (bodyClose < bodyOpenEnd) return s;

        // Same logic for </html> - it's the last real document close,
        // and we just verify it's present and not before the </body>.
        size_t htmlClose = s.rfind("</html>");
        if (htmlClose == string::npos) return s;
        if (htmlClose < bodyClose) return s;

        // Slice from just after <body...> up to (but not including)
        // </body>. The </html> doesn't need to be sliced out because
        // it's already past bodyClose and therefore outside the
        // returned range.
        return s.substr(bodyOpenEnd + 1, bodyClose - bodyOpenEnd - 1);
    }
}

CustomHTMLSlide::CustomHTMLSlide(string html, string desc)
{
    customHTML = html;
    description = desc;
}

void CustomHTMLSlide::display()
{
    cout << "\n  [Custom HTML Slide]\n";
    cout << "  Description : " << description << "\n";
    cout << "  HTML Preview: " << customHTML.substr(0, 50) << "...\n";
}

string CustomHTMLSlide::generateHTML(Theme& t, int n)
{
    // The user is allowed to inject arbitrary HTML here, but if they
    // paste a full document (<!DOCTYPE>...</html>) the combined export
    // ends up with nested <html>/<head>/<body> tags mid-file. Strip the
    // wrapper so the slide div stays valid.
    string body = bodyOnly(customHTML);
    // Strip any <style> blocks from the unwrapped content. Inner <style>
// blocks would be hoisted into the document <head> by the browser and
// would apply to ALL slides in the combined export, breaking the rest
// of the deck. The user can still use inline `style="..."` attributes
// on individual elements.
    size_t searchFrom = 0;
    while (true) {
        size_t styleStart = body.find("<style", searchFrom);
        if (styleStart == string::npos) break;
        size_t styleOpenEnd = body.find('>', styleStart);
        if (styleOpenEnd == string::npos) break;
        size_t styleClose = body.rfind("</style>");
        if (styleClose == string::npos || styleClose < styleOpenEnd) break;
        body.erase(styleStart, styleClose + 7 - styleStart);
        searchFrom = styleStart;
    }

    return "<div class='slide'>"
        "<p class='number'>Slide " + to_string(n) + " (Custom)</p>"
        + body +
        "</div>\n";
}

void CustomHTMLSlide::addToPPTX(PptxBuilder& b, Theme& t)
{
    b.addNoteSlide(t.getBgColor(), description, t.getTextColor());
}

string CustomHTMLSlide::toTemplateJSON()
{
    return "{\"type\":\"custom_html\",\"html\":\"" + jsonEscape(customHTML) +
        "\",\"description\":\"" + jsonEscape(description) + "\"}";
}

string CustomHTMLSlide::summary() { return "Custom HTML    : " + description; }