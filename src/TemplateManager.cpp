#include "TemplateManager.h"
#include "TitleSlide.h"
#include "ContentSlide.h"
#include "BulletSlide.h"
#include "ImageSlide.h"
#include "CustomHTMLSlide.h"
#include "Json.h"
#include <fstream>
#include <sstream>

using namespace std;

vector<TemplateManager::BuiltinTemplate> TemplateManager::listBuiltins()
{
    return {
        { "Business Pitch",        "Problem, solution, market, business model, the ask" },
        { "Project Status Update", "Summary, completed work, upcoming work, risks, actions" },
        { "Lecture Outline",       "Objectives, intro, key concepts, summary, further reading" },
        { "Product Launch",        "Motivation, features, availability, what's next" },
    };
}

vector<Slide*> TemplateManager::buildBuiltin(int index)
{
    vector<Slide*> slides;

    auto bullets = [](const string& heading, vector<string> items) {
        BulletSlide* bs = new BulletSlide(heading);
        for (auto& item : items) bs->addBullet(item);
        return bs;
    };

    switch (index)
    {
    case 0: // Business Pitch
        slides.push_back(new TitleSlide("Your Company Name", "One-line value proposition"));
        slides.push_back(new ContentSlide("The Problem", "Describe the problem your customers face."));
        slides.push_back(new ContentSlide("Your Solution", "Describe how your product solves it."));
        slides.push_back(bullets("Market Opportunity", { "Target market size", "Growth trends", "Why now" }));
        slides.push_back(bullets("Business Model", { "How you make money", "Pricing", "Unit economics" }));
        slides.push_back(bullets("The Ask", { "Funding amount", "Use of funds", "Next milestones" }));
        break;

    case 1: // Project Status Update
        slides.push_back(new TitleSlide("Project Name - Status Update", "Reporting period"));
        slides.push_back(bullets("Summary", { "Overall status: on track / at risk", "Key highlight", "Key concern" }));
        slides.push_back(bullets("Completed This Period", { "Item one", "Item two", "Item three" }));
        slides.push_back(bullets("Upcoming Next Period", { "Item one", "Item two" }));
        slides.push_back(new ContentSlide("Risks & Blockers", "Describe any risks or blockers here."));
        slides.push_back(bullets("Action Items", { "Owner - action - due date" }));
        break;

    case 2: // Lecture Outline
        slides.push_back(new TitleSlide("Lecture Title", "Course name - Week N"));
        slides.push_back(bullets("Learning Objectives", { "Objective one", "Objective two", "Objective three" }));
        slides.push_back(new ContentSlide("Introduction", "Brief context or motivation for today's topic."));
        slides.push_back(bullets("Key Concept 1", { "Point A", "Point B" }));
        slides.push_back(bullets("Key Concept 2", { "Point A", "Point B" }));
        slides.push_back(new ContentSlide("Summary", "Recap the main takeaways."));
        slides.push_back(bullets("Further Reading", { "Reference one", "Reference two" }));
        break;

    case 3: // Product Launch
        slides.push_back(new TitleSlide("Product Name", "Launch date / tagline"));
        slides.push_back(new ContentSlide("Why We Built This", "Motivation and customer insight."));
        slides.push_back(bullets("Key Features", { "Feature one", "Feature two", "Feature three" }));
        slides.push_back(bullets("Availability", { "Pricing", "Where to buy", "Launch regions" }));
        slides.push_back(new ContentSlide("What's Next", "Roadmap teaser."));
        break;

    default:
        break;
    }

    return slides;
}

bool TemplateManager::loadFromFile(const string& path, vector<Slide*>& outSlides, string& error)
{
    ifstream file(path);
    if (!file.is_open())
    {
        error = "Could not open " + path;
        return false;
    }

    stringstream ss;
    ss << file.rdbuf();
    string text = ss.str();

    bool ok = false;
    JsonValue parsed = parseJson(text, ok);

    if (!ok || !parsed.isArray())
    {
        error = path + " doesn't look like a valid template file (expected a JSON array of slides).";
        return false;
    }

    outSlides.clear();
    for (const auto& item : parsed.arrayValue)
    {
        if (!item.isObject()) continue;
        string type = item.get("type").asString();

        if (type == "title")
        {
            outSlides.push_back(new TitleSlide(item.get("title").asString(), item.get("subtitle").asString()));
        }
        else if (type == "content")
        {
            outSlides.push_back(new ContentSlide(item.get("heading").asString(), item.get("content").asString()));
        }
        else if (type == "bullets")
        {
            BulletSlide* bs = new BulletSlide(item.get("heading").asString());
            const JsonValue& bulletsVal = item.get("bullets");
            if (bulletsVal.isArray())
                for (const auto& b : bulletsVal.arrayValue)
                    bs->addBullet(b.asString());
            outSlides.push_back(bs);
        }
        else if (type == "image")
        {
            outSlides.push_back(new ImageSlide(item.get("path").asString(), item.get("caption").asString()));
        }
        else if (type == "custom_html")
        {
            outSlides.push_back(new CustomHTMLSlide(item.get("html").asString(), item.get("description").asString()));
        }
        // Unrecognized types are skipped rather than guessed at - this is
        // our own file format, so an unknown type means a version
        // mismatch or hand-edited file, not something worth patching over.
    }

    if (outSlides.empty())
    {
        error = "No usable slides found in " + path;
        return false;
    }

    return true;
}
