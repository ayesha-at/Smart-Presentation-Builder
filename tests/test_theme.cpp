// Tests for Theme.h/.cpp: getters, the 7 built-in theme instances, and
// that buildCSS() actually incorporates each theme's colors rather than
// silently falling back to hardcoded defaults.
#include "Theme.h"
#include <cassert>
#include <iostream>

using namespace std;

int main()
{
    // Constructor + all getters return exactly what was passed in
    {
        Theme t("TestTheme", "#111111", "#222222", "#333333", "Arial, sans-serif", "#444444");
        assert(t.getName() == "TestTheme");
        assert(t.getBgColor() == "#111111");
        assert(t.getHeadingColor() == "#222222");
        assert(t.getTextColor() == "#333333");
        assert(t.getFontFamily() == "Arial, sans-serif");
        assert(t.getAccentColor() == "#444444");
    }

    // buildCSS() must actually embed this theme's own colors, not some
    // other theme's or a hardcoded default - this is what makes each of
    // the 7 built-in themes visually distinct in the exported HTML.
    {
        Theme t("Distinctive", "#ABCDEF", "#123456", "#654321", "Georgia", "#FEDCBA");
        string css = t.buildCSS();
        assert(css.find("ABCDEF") != string::npos);
        assert(css.find("123456") != string::npos);
        assert(css.find("654321") != string::npos);
        assert(css.find("FEDCBA") != string::npos);
        // Sanity check it's actually a <style> block, not empty/garbage.
        assert(css.find("<style>") != string::npos);
        assert(css.find("</style>") != string::npos);
        assert(css.find(".slide") != string::npos);
    }

    // The Coffee theme gets a distinct gradient body background (special
    // case in buildCSS) - confirm it takes that branch and not the
    // generic dark-gradient one every other theme uses.
    {
        Theme coffee("Coffee", "#2c1810", "#e8b06e", "#d4a373", "Georgia", "#c77d4e");
        string css = coffee.buildCSS();
        assert(css.find("C67B3D") != string::npos); // Coffee-specific gradient stop

        Theme other("NotCoffee", "#2c1810", "#e8b06e", "#d4a373", "Georgia", "#c77d4e");
        string otherCss = other.buildCSS();
        assert(otherCss.find("C67B3D") == string::npos); // must NOT get the Coffee-only gradient
    }

    // All 7 built-in global theme instances exist, are constructed, and
    // have distinct names (a copy-paste error while defining them could
    // silently duplicate a name or color).
    {
        Theme* themes[] = { &oceanTheme, &sunsetTheme, &coffeeTheme, &forestTheme,
                            &darkTheme, &minimalTheme, &retroTheme };
        for (auto* t : themes)
            assert(!t->getName().empty());

        for (int i = 0; i < 7; i++)
            for (int j = i + 1; j < 7; j++)
                assert(themes[i]->getName() != themes[j]->getName());
    }

    cout << "All Theme tests passed.\n";
    return 0;
}
