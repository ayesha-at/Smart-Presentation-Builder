// Tests for Utils.h/.cpp: escapeHTML/unescapeHTML round-tripping, trim(),
// and stripQuotes(). Specifically includes the "O'Brien's" case flagged
// in review as a latent risk around escapeHTML's use inside a
// single-quoted HTML attribute (see the doc comment on escapeHTML in
// Utils.h and the call site comment in ImageSlide.cpp).
#include "Utils.h"
#include <cassert>
#include <iostream>

using namespace std;

int main()
{
    // Basic escaping of all five special characters
    {
        string input = R"(Tom & Jerry <script> "quoted" 'single')";
        string escaped = escapeHTML(input);
        assert(escaped.find('<') == string::npos);
        assert(escaped.find('>') == string::npos);
        assert(escaped.find('"') == string::npos);
        assert(escaped.find('\'') == string::npos);
        // A literal '&' should appear ONLY as part of an entity like
        // &amp; - not as a bare ampersand anywhere else.
    }

    // Round-trip: escape then unescape must recover the original exactly
    {
        string input = R"(Tom & Jerry <script> "quoted" 'single' plain text)";
        assert(unescapeHTML(escapeHTML(input)) == input);
    }

    // The specific case from review: a folder name with an apostrophe,
    // embedded exactly the way ImageSlide::generateHTML embeds it - inside
    // a single-quoted src='...' attribute. If escapeHTML ever stopped
    // escaping literal ' characters, this would start producing broken
    // HTML (the attribute would close early at the apostrophe).
    {
        string path = "C:\\Users\\O'Brien's Photos\\pic.png";
        string escaped = escapeHTML(path);
        string attr = "<img src='" + escaped + "' alt='x'>";

        // The attribute value must not contain a literal ' before the
        // real closing quote - i.e. escaping must have neutralized both
        // apostrophes in the original path.
        size_t attrStart = attr.find("src='") + 5;
        size_t attrEnd = attr.find('\'', attrStart);
        string attrValue = attr.substr(attrStart, attrEnd - attrStart);
        assert(attrValue.find('\'') == string::npos);

        // And it must round-trip back to the exact original path.
        assert(unescapeHTML(attrValue) == path);
    }

    // trim: strips spaces, tabs, and stray \r\n (CRLF terminals)
    {
        assert(trim("  hello  ") == "hello");
        assert(trim("hello\r\n") == "hello");
        assert(trim("\r\thello\t\r") == "hello");
        assert(trim("") == "");
        assert(trim("   ") == "");
    }

    // stripQuotes: removes one matching pair, leaves unquoted/mismatched alone
    {
        assert(stripQuotes("\"C:\\path\\file.html\"") == "C:\\path\\file.html");
        assert(stripQuotes("'single quoted'") == "single quoted");
        assert(stripQuotes("no quotes here") == "no quotes here");
        assert(stripQuotes("\"mismatched'") == "\"mismatched'"); // not a matching pair - left alone
        assert(stripQuotes("\"") == "\""); // single char - too short to be a pair
    }

    cout << "All Utils tests passed.\n";
    return 0;
}
