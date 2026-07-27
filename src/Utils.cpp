#include "Utils.h"
#include <iostream>
#include <limits>
#include <cstdlib>

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
