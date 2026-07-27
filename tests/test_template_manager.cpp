// Tests for TemplateManager.h/.cpp: the built-in template library, and
// loading previously-saved template files back into real Slide objects.
#include "TemplateManager.h"
#include "Slide.h"
#include "Presentation.h"
#include "TitleSlide.h"
#include "BulletSlide.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

static void deleteAll(vector<Slide*>& slides)
{
    for (auto* s : slides) delete s;
    slides.clear();
}

int main()
{
    // listBuiltins: exactly 4 templates, each with a non-empty name/description
    {
        auto templates = TemplateManager::listBuiltins();
        assert(templates.size() == 4);
        for (auto& t : templates)
        {
            assert(!t.name.empty());
            assert(!t.description.empty());
        }
        for (size_t i = 0; i < templates.size(); i++)
            for (size_t j = i + 1; j < templates.size(); j++)
                assert(templates[i].name != templates[j].name);
    }

    // buildBuiltin: every valid index produces a non-empty deck whose
    // first slide is a title slide (every built-in is designed this way)
    {
        auto templates = TemplateManager::listBuiltins();
        for (int i = 0; i < (int)templates.size(); i++)
        {
            vector<Slide*> slides = TemplateManager::buildBuiltin(i);
            assert(!slides.empty());
            assert(slides[0]->summary().find("Title Slide") != string::npos);
            deleteAll(slides);
        }
    }

    // buildBuiltin: an out-of-range index returns an empty deck rather
    // than crashing or returning garbage.
    {
        vector<Slide*> slides = TemplateManager::buildBuiltin(999);
        assert(slides.empty());
    }

    // loadFromFile: round-trips a real saved template correctly. Build a
    // small deck via Presentation, save it, then load it back and confirm
    // the slide count and content survived.
    {
        const string path = "test_template_roundtrip.template";
        remove(path.c_str());

        {
            Presentation p;
            p.addSlide(new TitleSlide("Roundtrip Title", "Roundtrip Subtitle"));
            BulletSlide* bs = new BulletSlide("Agenda");
            bs->addBullet("Point one");
            bs->addBullet("Point two");
            p.addSlide(bs);
            p.exportTemplate(path);
        }

        vector<Slide*> loaded;
        string error;
        bool ok = TemplateManager::loadFromFile(path, loaded, error);
        assert(ok);
        assert(loaded.size() == 2);
        assert(loaded[0]->summary() == "Title Slide    : Roundtrip Title");
        assert(loaded[1]->summary() == "Bullet Slide   : Agenda");

        deleteAll(loaded);
        remove(path.c_str());
    }

    // loadFromFile: a nonexistent file fails cleanly, not a crash
    {
        vector<Slide*> loaded;
        string error;
        bool ok = TemplateManager::loadFromFile("does_not_exist_xyz.template", loaded, error);
        assert(!ok);
        assert(!error.empty());
    }

    // loadFromFile: a file that isn't a JSON array fails cleanly
    {
        const string path = "test_template_malformed.template";
        ofstream f(path);
        f << "{\"not\":\"an array\"}";
        f.close();

        vector<Slide*> loaded;
        string error;
        bool ok = TemplateManager::loadFromFile(path, loaded, error);
        assert(!ok);
        remove(path.c_str());
    }

    // loadFromFile: unrecognized slide "type" values are skipped, not
    // guessed at - a file with only unrecognized types yields no slides
    // and reports failure rather than fabricating content.
    {
        const string path = "test_template_unknown_type.template";
        ofstream f(path);
        f << R"([{"type":"chart","heading":"Unsupported"}])";
        f.close();

        vector<Slide*> loaded;
        string error;
        bool ok = TemplateManager::loadFromFile(path, loaded, error);
        assert(!ok);
        assert(loaded.empty());
        remove(path.c_str());
    }

    cout << "All TemplateManager tests passed.\n";
    return 0;
}
