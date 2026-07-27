// Tests for Presentation.h/.cpp: add/delete/move slide ordering and
// bounds checking, theme switching, and that exporting an empty
// presentation is a safe no-op rather than a crash or a corrupt file.
#include "Presentation.h"
#include "TitleSlide.h"
#include "BulletSlide.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

static bool fileExists(const string& path)
{
    ifstream f(path);
    return f.good();
}

int main()
{
    // addSlide + slideCount + slideSummaries reflect insertion order
    {
        Presentation p;
        assert(p.slideCount() == 0);
        p.addSlide(new TitleSlide("First", ""));
        p.addSlide(new TitleSlide("Second", ""));
        p.addSlide(new TitleSlide("Third", ""));
        assert(p.slideCount() == 3);
        auto summaries = p.slideSummaries();
        assert(summaries[0].find("First") != string::npos);
        assert(summaries[1].find("Second") != string::npos);
        assert(summaries[2].find("Third") != string::npos);
    }

    // deleteSlide: valid index removes exactly that slide, shifts the rest
    {
        Presentation p;
        p.addSlide(new TitleSlide("A", ""));
        p.addSlide(new TitleSlide("B", ""));
        p.addSlide(new TitleSlide("C", ""));
        p.deleteSlide(2); // 1-based - removes "B"
        assert(p.slideCount() == 2);
        auto summaries = p.slideSummaries();
        assert(summaries[0].find("A") != string::npos);
        assert(summaries[1].find("C") != string::npos);
    }

    // deleteSlide: out-of-range indices must not crash or change anything
    {
        Presentation p;
        p.addSlide(new TitleSlide("Only", ""));
        p.deleteSlide(0);   // below range
        p.deleteSlide(-5);  // negative
        p.deleteSlide(99);  // above range
        assert(p.slideCount() == 1); // untouched
    }

    // moveSlide: reorders correctly in both directions
    {
        Presentation p;
        p.addSlide(new TitleSlide("A", ""));
        p.addSlide(new TitleSlide("B", ""));
        p.addSlide(new TitleSlide("C", ""));

        p.moveSlide(1, 3); // move "A" to the end -> B, C, A
        auto s1 = p.slideSummaries();
        assert(s1[0].find("B") != string::npos);
        assert(s1[1].find("C") != string::npos);
        assert(s1[2].find("A") != string::npos);

        p.moveSlide(3, 1); // move "A" back to the front -> A, B, C
        auto s2 = p.slideSummaries();
        assert(s2[0].find("A") != string::npos);
        assert(s2[1].find("B") != string::npos);
        assert(s2[2].find("C") != string::npos);
    }

    // moveSlide: out-of-range indices must not crash or change anything
    {
        Presentation p;
        p.addSlide(new TitleSlide("A", ""));
        p.addSlide(new TitleSlide("B", ""));
        p.moveSlide(0, 5);
        p.moveSlide(-1, 1);
        p.moveSlide(1, 99);
        assert(p.slideCount() == 2);
        auto s = p.slideSummaries();
        assert(s[0].find("A") != string::npos); // untouched, still original order
    }

    // Theme switching
    {
        Presentation p;
        assert(p.getThemeName() == "Coffee"); // documented default
        p.setTheme(1); // Dark
        assert(p.getThemeName() == "Dark");
        p.setTheme(4); // Ocean
        assert(p.getThemeName() == "Ocean");
        string before = p.getThemeName();
        p.setTheme(999); // invalid - must leave the theme unchanged
        assert(p.getThemeName() == before);
    }

    // summary()/showSlides() on an empty presentation must not crash
    {
        Presentation p;
        p.summary();
        p.showSlides();
    }

    // Exporting an empty presentation must be a safe no-op: no crash, and
    // no output file gets created (there's nothing meaningful to write).
    {
        remove("presentation.html");
        remove("presentation.pptx");
        remove("empty_test.template");

        Presentation p;
        p.exportHTML();
        p.exportPPTX();
        p.exportTemplate("empty_test.template");
        p.exportIndividualSlides();

        assert(!fileExists("presentation.html"));
        assert(!fileExists("presentation.pptx"));
        assert(!fileExists("empty_test.template"));
    }

    // A non-empty presentation DOES produce real export files
    {
        remove("presentation.html");
        Presentation p;
        p.addSlide(new TitleSlide("Real Content", "Subtitle"));
        p.exportHTML();
        assert(fileExists("presentation.html"));
        remove("presentation.html");
    }

    // Destructor must clean up without crashing (segfault/abort here
    // would fail this test) - scope a Presentation with slides added via
    // different code paths (including a BulletSlide, since it holds its
    // own vector<string> internally) and let it go out of scope normally.
    {
        Presentation p;
        p.addSlide(new TitleSlide("X", "Y"));
        BulletSlide* bs = new BulletSlide("Heading");
        bs->addBullet("one");
        bs->addBullet("two");
        p.addSlide(bs);
    }

    cout << "All Presentation tests passed.\n";
    return 0;
}
