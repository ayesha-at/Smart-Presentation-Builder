#pragma once
#include <string>

// Escapes special HTML characters: & < > " '
//
// This deliberately escapes BOTH quote characters, not just the three
// characters (& < >) that are strictly required inside a text node. That
// makes this function's actual contract: "safe to place inside HTML text
// content, OR inside an HTML attribute value delimited by EITHER single
// or double quotes." Every call site in this codebase relies on that
// broader guarantee (e.g. ImageSlide::generateHTML uses it inside a
// single-quoted src='...' attribute specifically because this escapes
// literal ' characters to &#39; - if this function were ever "simplified"
// to only escape & < > for text-content correctness, that attribute
// usage would silently become exploitable by a path containing a single
// quote). If you add a new attribute-emitting call site, this function
// remains safe regardless of which quote character you delimit with -
// just don't narrow what it escapes without checking existing call sites.
std::string escapeHTML(const std::string& s);

// Reverses escapeHTML(): converts HTML entities back into literal
// characters. Used when importing slides from previously-exported HTML.
std::string unescapeHTML(const std::string& s);

// Reads an int safely from std::cin, re-prompting on invalid input.
int readInt(const std::string& prompt);

// Strips leading/trailing whitespace (including stray \r from CRLF input,
// which some terminals/shells leave behind after getline).
std::string trim(const std::string& s);

// Strips one leading/trailing matching pair of "..." or '...' quotes, if
// present. Windows Explorer's "Copy as path" wraps paths in literal
// double quotes, which end up as part of the pasted text otherwise.
std::string stripQuotes(const std::string& s);
