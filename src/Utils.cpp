#include "Utils.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <cctype>

using namespace std;

string escapeHTML(const string& s)
{
    string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

string unescapeHTML(const string& s)
{
    string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size())
    {
        if (s[i] == '&')
        {
            if (s.compare(i, 5, "&amp;") == 0)  { out += '&';  i += 5; continue; }
            if (s.compare(i, 4, "&lt;") == 0)   { out += '<';  i += 4; continue; }
            if (s.compare(i, 4, "&gt;") == 0)   { out += '>';  i += 4; continue; }
            if (s.compare(i, 6, "&quot;") == 0) { out += '"';  i += 6; continue; }
            if (s.compare(i, 5, "&#39;") == 0)  { out += '\''; i += 5; continue; }
        }
        out += s[i];
        i++;
    }
    return out;
}

int readInt(const string& prompt)
{
    int value;
    cout << prompt;
    while (!(cin >> value))
    {
        if (cin.eof())
        {
            // stdin closed (piped input ran out, terminal closed, etc.) -
            // without this check, clear()+ignore() on an already-closed
            // stream succeeds instantly and this loop spins forever,
            // printing the prompt as fast as the CPU allows.
            cout << "\n  Input stream closed unexpectedly. Exiting.\n";
            exit(0);
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  Invalid input, please enter a number: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return value;
}

string trim(const string& s)
{
    size_t start = 0;
    size_t end = s.size();
    auto isTrimmable = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (start < end && isTrimmable(s[start])) start++;
    while (end > start && isTrimmable(s[end - 1])) end--;
    return s.substr(start, end - start);
}

string stripQuotes(const string& s)
{
    if (s.size() >= 2)
    {
        char first = s.front();
        char last = s.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
            return s.substr(1, s.size() - 2);
    }
    return s;
}

bool containsFullDocumentTags(const string& html)
{
    string lower = html;
    for (auto& c : lower)
        c = (char)tolower((unsigned char)c);

    if (lower.find("<!doctype") != string::npos)
        return true;

    // Checks for an actual tag (followed by '>', whitespace, or '/'), not
    // just the substring appearing anywhere - so legitimate prose like
    // "Learn HTML today" inside a slide never gets mistaken for a tag.
    auto hasTag = [&](const string& needle) {
        size_t pos = 0;
        while ((pos = lower.find(needle, pos)) != string::npos)
        {
            size_t afterPos = pos + needle.size();
            char after = (afterPos < lower.size()) ? lower[afterPos] : '>';
            if (after == '>' || after == ' ' || after == '\t' ||
                after == '\n' || after == '\r' || after == '/')
                return true;
            pos = afterPos;
        }
        return false;
    };

    return hasTag("<html") || hasTag("</html") ||
           hasTag("<head") || hasTag("</head") ||
           hasTag("<body") || hasTag("</body");
}
