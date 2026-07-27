#include "Theme.h"

using namespace std;

Theme::Theme(string n, string bg, string hc, string tc, string ff, string ac)
{
    name = n;
    bgColor = bg;
    headingColor = hc;
    textColor = tc;
    fontFamily = ff;
    accentColor = ac;
}

string Theme::getName() { return name; }
string Theme::getBgColor() { return bgColor; }
string Theme::getHeadingColor() { return headingColor; }
string Theme::getTextColor() { return textColor; }
string Theme::getFontFamily() { return fontFamily; }
string Theme::getAccentColor() { return accentColor; }

string Theme::buildCSS()
{
    string bodyBg;
    if (getName() == "Coffee")
    {
        bodyBg = "linear-gradient(135deg, #C67B3D 0%, #E8B17D 50%, #F3D5B5 100%)";
    }
    else
    {
        bodyBg = "linear-gradient(135deg, #0f0c29 0%, #302b63 50%, #24243e 100%)";
    }
    return "<style>"
        "* { margin:0; padding:0; box-sizing:border-box; }"

        "body {"
        "  background:" + bodyBg+ "; "
        "  font-family: 'Poppins', 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;"
        "  padding: 40px 20px;"
        "}"

        ".slide {"
        "  max-width: 1000px;"
        "  margin: 40px auto;"
        "  background: " + bgColor + ";"
        "  border-radius: 28px;"
        "  padding: 50px;"
        "  box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5);"
        "  transition: transform 0.3s ease, box-shadow 0.3s ease;"
        "  page-break-after: always;"
        "}"

        ".slide:hover {"
        "  transform: translateY(-8px);"
        "  box-shadow: 0 35px 60px -15px rgba(0,0,0,0.6);"
        "}"

        "h1 {"
        "  font-size: 68px;"
        "  font-weight: 800;"
        "  background: linear-gradient(135deg, " + headingColor + " 0%, " + accentColor + " 100%);"
        "  -webkit-background-clip: text;"
        "  background-clip: text;"
        "  color: transparent;"
        "  margin-bottom: 20px;"
        "  text-align: center;"
        "  letter-spacing: -1px;"
        "}"

        "h2 {"
        "  font-size: 44px;"
        "  font-weight: 700;"
        "  color: " + headingColor + ";"
        "  margin-bottom: 30px;"
        "  padding-bottom: 15px;"
        "  border-bottom: 3px solid " + accentColor + ";"
        "  display: inline-block;"
        "}"

        "h3 {"
        "  font-size: 28px;"
        "  color: " + textColor + ";"
        "  font-weight: 400;"
        "  text-align: center;"
        "  margin-top: 20px;"
        "  opacity: 0.9;"
        "}"

        "p {"
        "  color: " + textColor + ";"
        "  font-size: 20px;"
        "  line-height: 1.7;"
        "  margin: 20px 0;"
        "}"

        "ul {"
        "  list-style-type: disc;"
        "  padding-left: 40px;"
        "}"

        "li {"
        "  color: " + textColor + ";"
        "  font-size: 20px;"
        "  line-height: 1.6;"
        "  padding: 12px 0 12px 35px;"
        "  position: relative;"
        "  margin: 8px 0;"
        "}"

        "img {"
        "  max-width: 90%;"
        "  height: auto;"
        "  border-radius: 16px;"
        "  display: block;"
        "  margin: 30px auto;"
        "  box-shadow: 0 10px 30px rgba(0,0,0,0.2);"
        "  transition: transform 0.3s ease;"
        "}"

        "img:hover {"
        "  transform: scale(1.02);"
        "}"

        ".number {"
        "  color: " + accentColor + ";"
        "  font-size: 13px;"
        "  text-align: right;"
        "  margin-bottom: 15px;"
        "  font-weight: 600;"
        "  letter-spacing: 1px;"
        "}"

        ".caption {"
        "  text-align: center;"
        "  color: " + textColor + ";"
        "  font-size: 15px;"
        "  margin-top: 10px;"
        "  opacity: 0.7;"
        "  font-style: italic;"
        "}"

        "@media (max-width: 768px) {"
        "  .slide { padding: 30px; margin: 20px; }"
        "  h1 { font-size: 44px; }"
        "  h2 { font-size: 32px; }"
        "  p, li { font-size: 18px; }"
        "}"
        "</style>";
}

Theme oceanTheme(
    "Ocean",
    "#0a192f",
    "#64ffda",
    "#8892b0",
    "'Segoe UI', system-ui, sans-serif",
    "#64ffda"
);

Theme sunsetTheme(
    "Sunset",
    "#1a0b1c",
    "#ff6b6b",
    "#feca57",
    "'Poppins', sans-serif",
    "#ff9f43"
);

Theme coffeeTheme(
    "Coffee",
    "#2c1810",
    "#e8b06e",
    "#d4a373",
    "'Georgia', serif",
    "#c77d4e"
);

Theme forestTheme(
    "Forest",
    "#0d1f11",
    "#a8e6cf",
    "#b8e1b8",
    "'Merriweather', serif",
    "#78c091"
);

Theme darkTheme(
    "Dark",
    "#1a1a2e",
    "#e94560",
    "#a8a8b3",
    "Arial, sans-serif",
    "#e94560"
);

Theme minimalTheme(
    "Minimal",
    "#ffffff",
    "#1a1a1a",
    "#444444",
    "Georgia, serif",
    "#cccccc"
);

Theme retroTheme(
    "Retro",
    "#2b2d42",
    "#ef233c",
    "#edf2f4",
    "'Courier New', monospace",
    "#ef233c"
);
