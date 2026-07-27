#pragma once
#include <string>

class Theme
{
private:
    std::string name;
    std::string bgColor;
    std::string headingColor;
    std::string textColor;
    std::string fontFamily;
    std::string accentColor;

public:
    Theme(std::string n, std::string bg, std::string hc, std::string tc, std::string ff, std::string ac);

    std::string getName();
    std::string getBgColor();
    std::string getHeadingColor();
    std::string getTextColor();
    std::string getFontFamily();
    std::string getAccentColor();

    // Builds the CSS used by the HTML export.
    std::string buildCSS();
};

// The 7 built-in themes, defined in Theme.cpp
extern Theme oceanTheme;
extern Theme sunsetTheme;
extern Theme coffeeTheme;
extern Theme forestTheme;
extern Theme darkTheme;
extern Theme minimalTheme;
extern Theme retroTheme;
