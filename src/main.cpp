#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <functional>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include "Presentation.h"
#include "TitleSlide.h"
#include "ContentSlide.h"
#include "ImageSlide.h"
#include "BulletSlide.h"
#include "CustomHTMLSlide.h"
#include "Utils.h"
#include "Constants.h"
#include "AIAssistant.h"
#include "HtmlImporter.h"
#include "TemplateManager.h"
#include "Json.h"

using namespace std;
namespace fs = std::filesystem;

// Runs `func` on the calling thread while a lightweight animated spinner
// runs concurrently on a background thread, so the console doesn't look
// frozen during a blocking network call to the AI. IMPORTANT: `func`
// itself must not print anything to stdout while running - it should
// only compute/return a result and let the caller print status *after*
// the spinner is cleared - otherwise its output would interleave with the
// spinner's \r-based redraws and look garbled. (This is why
// aiFillTemplateSlides collects failure notes into a vector instead of
// printing them inline - see its own comment.)
template <typename Func>
auto runWithSpinner(const string& message, Func&& func) -> decltype(func())
{
    atomic<bool> done{false};
    thread spinner([&]() {
        const char frames[] = { '|', '/', '-', '\\' };
        int i = 0;
        while (!done)
        {
            cout << "\r  " << message << "... " << frames[i % 4] << flush;
            i++;
            this_thread::sleep_for(chrono::milliseconds(150));
        }
    });

    auto result = func();

    done = true;
    spinner.join();
    cout << "\r" << string(message.size() + 8, ' ') << "\r" << flush;
    return result;
}

// Same as above, for calls that return void (e.g. aiFillTemplateSlides,
// which mutates its argument in place rather than returning a value).
template <typename Func>
void runWithSpinnerVoid(const string& message, Func&& func)
{
    atomic<bool> done{false};
    thread spinner([&]() {
        const char frames[] = { '|', '/', '-', '\\' };
        int i = 0;
        while (!done)
        {
            cout << "\r  " << message << "... " << frames[i % 4] << flush;
            i++;
            this_thread::sleep_for(chrono::milliseconds(150));
        }
    });

    func();

    done = true;
    spinner.join();
    cout << "\r" << string(message.size() + 8, ' ') << "\r" << flush;
}

string readMultilineHTML()
{
    string input;
    string line;
    cout << "  Enter your HTML code (type 'END' on a new line to finish):\n";
    cout << "  ---------------------------------------------------------\n";

    while (true)
    {
        getline(cin, line);
        if (line == "END" || line == "end")
            break;
        input += line + "\n";
    }

    cout << "  ---------------------------------------------------------\n";
    return input;
}

void addCustomHTMLSlide(Presentation& ppt)
{
    string description;
    cout << "  Description (what is this slide?): ";
    getline(cin, description);

    string html = readMultilineHTML();

    if (html.empty())
    {
        cout << " No HTML provided. Slide not added.\n";
        return;
    }

    ppt.addSlide(new CustomHTMLSlide(html, description));
    cout << "Custom HTML slide added!\n";
    cout << "Tip: Your HTML will appear inside a styled slide container.\n";
}

void addTitleSlide(Presentation& ppt)
{
    string title, subtitle;
    cout << "  Title    : "; getline(cin, title);
    cout << "  Subtitle : "; getline(cin, subtitle);
    ppt.addSlide(new TitleSlide(title, subtitle));
    cout << "  Title slide added.\n";
}

void addContentSlide(Presentation& ppt)
{
    string heading, content;
    cout << "  Heading : "; getline(cin, heading);
    cout << "  Content : "; getline(cin, content);
    ppt.addSlide(new ContentSlide(heading, content));
    cout << "  Content slide added.\n";
}

void addImageSlide(Presentation& ppt)
{
    string path, caption;
    cout << "  Image path : "; getline(cin, path); path = stripQuotes(trim(path));
    cout << "  Caption    : "; getline(cin, caption);
    ppt.addSlide(new ImageSlide(path, caption));
    cout << "  Image slide added.\n";
}

void addBulletSlide(Presentation& ppt)
{
    string heading;
    cout << "  Heading : "; getline(cin, heading);

    BulletSlide* bs = new BulletSlide(heading);

    cout << "  Enter bullet points (blank line to stop, max "
        << MAX_BULLETS << "):\n";
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        string point;
        cout << "  Bullet " << (i + 1) << " : ";
        getline(cin, point);
        if (point.empty()) break;
        bs->addBullet(point);
    }

    ppt.addSlide(bs);
    cout << "  Bullet slide added.\n";
}

void changeTheme(Presentation& ppt)
{
    cout << "\n CHOOSE YOUR THEME:\n";
    cout << " 1. Dark      (Navy + Pink)        \n";
    cout << " 2. Minimal   (White + Black)      \n";
    cout << " 3. Retro     (Slate + Red)        \n";
    cout << " 4. Ocean     (Deep blue + Teal)   \n";
    cout << " 5. Sunset    (Purple + Coral)     \n";
    cout << " 6. Forest    (Green + Mint)       \n";
    cout << " 7. Coffee    (Brown + Caramel)    \n";
    int c = readInt("  Choice: ");
    ppt.setTheme(c);
    cout << "Theme set to: " << ppt.getThemeName() << "\n";
}

void deleteSlideMenu(Presentation& ppt)
{
    ppt.summary();
    int n = readInt("\n  Enter slide number to delete: ");
    ppt.deleteSlide(n);
}

void moveSlideMenu(Presentation& ppt)
{
    ppt.summary();
    int from = readInt("\n  Move FROM position: ");
    int to   = readInt("  Move TO   position: ");
    ppt.moveSlide(from, to);
}

void aiSettingsMenu(AIAssistant& ai)
{
    cout << "\n  AI SETTINGS (current: " << ai.getHost() << ":" << ai.getPort()
         << ", model: " << ai.getModel() << ", timeout: " << ai.getTimeoutSeconds() << "s)\n";
    cout << "  Leave a field blank to keep its current value.\n";

    string host, portStr, model, timeoutStr;
    cout << "  Host    [" << ai.getHost() << "]: "; getline(cin, host);   host = trim(host);
    cout << "  Port    [" << ai.getPort() << "]: "; getline(cin, portStr); portStr = trim(portStr);
    cout << "  Model   [" << ai.getModel() << "]: "; getline(cin, model); model = trim(model);
    cout << "  Timeout (seconds, for slow local hardware) [" << ai.getTimeoutSeconds() << "]: ";
    getline(cin, timeoutStr); timeoutStr = trim(timeoutStr);

    int port = portStr.empty() ? ai.getPort() : atoi(portStr.c_str());
    ai.configure(host.empty() ? ai.getHost() : host, port, model.empty() ? ai.getModel() : model);
    if (!timeoutStr.empty())
        ai.setTimeoutSeconds(atoi(timeoutStr.c_str()));

    string error;
    bool connected = runWithSpinner("Checking connection to " + ai.getHost() + ":" + to_string(ai.getPort()),
                                     [&]{ return ai.checkConnection(error); });
    if (connected)
        cout << "  Connected. Ready to use AI features (model: " << ai.getModel() << ").\n";
    else
        cout << "  Could not connect:\n  " << error << "\n";
}

void aiSuggestBulletsMenu(Presentation& ppt, AIAssistant& ai)
{
    string heading;
    cout << "  Heading to suggest bullets for: ";
    getline(cin, heading);
    heading = trim(heading);
    if (heading.empty())
    {
        cout << "  No heading given, cancelled.\n";
        return;
    }

    int count = readInt("  How many bullets (1-" + to_string(MAX_BULLETS) + "): ");
    if (count < 1) count = 1;
    if (count > MAX_BULLETS) count = MAX_BULLETS;

    vector<string> bullets;
    string error;
    bool success = runWithSpinner("Asking " + ai.getModel() + " for suggestions",
                                   [&]{ return ai.suggestBullets(heading, count, bullets, error); });
    if (!success)
    {
        cout << "  AI request failed:\n  " << error << "\n";
        return;
    }

    cout << "\n  Suggested bullets:\n";
    for (size_t i = 0; i < bullets.size(); i++)
        cout << "    " << (i + 1) << ". " << bullets[i] << "\n";

    cout << "\n  Add these as a Bullet Slide? (y/n): ";
    string confirm;
    getline(cin, confirm);
    confirm = trim(confirm);
    if (confirm != "y" && confirm != "Y")
    {
        cout << "  Discarded.\n";
        return;
    }

    BulletSlide* bs = new BulletSlide(heading);
    for (const auto& b : bullets)
        bs->addBullet(b);
    ppt.addSlide(bs);
    cout << "  Bullet slide added.\n";
}

void aiGenerateOutlineMenu(Presentation& ppt, AIAssistant& ai)
{
    string topic;
    cout << "  Presentation topic: ";
    getline(cin, topic);
    topic = trim(topic);
    if (topic.empty())
    {
        cout << "  No topic given, cancelled.\n";
        return;
    }

    int numSlides = readInt("  How many slides total (including title slide): ");
    if (numSlides < 2) numSlides = 2;

    vector<AISlideSpec> specs;
    vector<string> warnings;
    string error;
    bool success = runWithSpinner("Asking " + ai.getModel() + " to draft an outline",
                                   [&]{ return ai.generateOutline(topic, numSlides, specs, error, warnings); });

    if (!warnings.empty())
    {
        cout << "\n  Note: " << warnings.size() << " section(s) couldn't be generated and were skipped:\n";
        for (const auto& w : warnings)
            cout << "    - " << w << "\n";
    }

    if (!success)
    {
        cout << "\n  AI request failed:\n  " << error << "\n";
        return;
    }

    cout << "\n  AI proposed " << specs.size() << " slide(s):\n";
    for (size_t i = 0; i < specs.size(); i++)
    {
        const AISlideSpec& s = specs[i];
        cout << "    " << (i + 1) << ". [" << s.type << "] " << s.heading << "\n";
    }

    cout << "\n  Add all of these to the presentation? (y/n): ";
    string confirm;
    getline(cin, confirm);
    confirm = trim(confirm);
    if (confirm != "y" && confirm != "Y")
    {
        cout << "  Discarded.\n";
        return;
    }

    int added = 0;
    for (const auto& s : specs)
    {
        if (s.type == "title")
        {
            ppt.addSlide(new TitleSlide(s.heading, s.subtitle));
            added++;
        }
        else if (s.type == "bullets")
        {
            BulletSlide* bs = new BulletSlide(s.heading);
            for (const auto& b : s.bullets)
                bs->addBullet(b);
            ppt.addSlide(bs);
            added++;
        }
        else if (s.type == "content")
        {
            ppt.addSlide(new ContentSlide(s.heading, s.content));
            added++;
        }
    }
    cout << "  Added " << added << " slide(s).\n";
}

void importHtmlMenu(Presentation& ppt)
{
    string path;
    cout << "  HTML file path to import: ";
    getline(cin, path);
    path = trim(path);
    path = stripQuotes(path);
    if (path.empty())
    {
        cout << "  No path given, cancelled.\n";
        return;
    }

    vector<Slide*> imported;
    string error;
    if (!HtmlImporter::importFromFile(path, imported, error))
    {
        cout << "  Import failed: " << error << "\n";
        return;
    }

    for (auto* s : imported)
        ppt.addSlide(s);
    cout << "  Imported " << imported.size() << " slide(s) from " << path << ".\n";
}

// Replaces a template's placeholder content with real, topic-specific
// content from the AI - one slide at a time, reusing each slide's own
// toTemplateJSON() to recover its type/heading without needing new
// getters on every Slide subclass. The template's headings (e.g. "Key
// Features", "The Problem") are deliberately kept as-is - they're already
// generic section names that apply to any topic in that template's genre
// - only the actual bullet/paragraph content underneath gets regenerated.
// Image and custom-HTML slides are left untouched: Ollama is a text
// model, it can't generate or meaningfully pick an image.
//
// Deliberately does NOT print anything itself - failures are collected
// into `notes` instead, printed by the caller after this returns. This
// function runs inside a spinner (see offerFillModeAndAdd), and printing
// mid-loop would interleave with the spinner's own \r-based redraws and
// look garbled - same reasoning as the `warnings` vector pattern used by
// AIAssistant::generateOutline.
void aiFillTemplateSlides(vector<Slide*>& slides, AIAssistant& ai, const string& topic, vector<string>& notes)
{
    vector<Slide*> filled;
    bool titleHandled = false;

    for (auto* s : slides)
    {
        bool ok = false;
        JsonValue spec = parseJson(s->toTemplateJSON(), ok);
        string type = ok ? spec.get("type").asString() : "";

        if (type == "title" && !titleHandled)
        {
            string title, subtitle, err;
            if (ai.suggestTitleSubtitle(topic, title, subtitle, err))
            {
                filled.push_back(new TitleSlide(title, subtitle));
                titleHandled = true;
                delete s;
                continue;
            }
            notes.push_back("Couldn't AI-generate the title (" + err + "), keeping placeholder.");
            filled.push_back(s);
            continue;
        }

        if (type == "bullets")
        {
            string heading = spec.get("heading").asString();
            vector<string> bullets;
            string err;
            string context = heading + " (for a presentation about " + topic + ")";
            if (ai.suggestBullets(context, 4, bullets, err))
            {
                BulletSlide* bs = new BulletSlide(heading);
                for (auto& b : bullets) bs->addBullet(b);
                filled.push_back(bs);
                delete s;
                continue;
            }
            notes.push_back("Couldn't AI-generate bullets for \"" + heading + "\" (" + err + "), keeping placeholder.");
            filled.push_back(s);
            continue;
        }

        if (type == "content")
        {
            string heading = spec.get("heading").asString();
            string content, err;
            string context = heading + " (for a presentation about " + topic + ")";
            if (ai.suggestContent(context, content, err))
            {
                filled.push_back(new ContentSlide(heading, content));
                delete s;
                continue;
            }
            notes.push_back("Couldn't AI-generate content for \"" + heading + "\" (" + err + "), keeping placeholder.");
            filled.push_back(s);
            continue;
        }

        // image / custom_html / unrecognized - AI can't meaningfully fill
        // these, keep the placeholder as-is.
        filled.push_back(s);
    }

    slides = filled;
}

// Shared by both "Use a Built-in Template" and "Load a Saved Template":
// once a template's slides are in hand, ask whether to fill placeholders
// by hand or let AI generate real content for a topic, then add the
// result to the presentation either way.
void offerFillModeAndAdd(vector<Slide*>& slides, Presentation& ppt, AIAssistant& ai)
{
    cout << "\n  Fill in placeholders (m)anually, or let (a)i generate content for a topic? [m/a]: ";
    string choice;
    getline(cin, choice);
    choice = trim(choice);

    if (choice == "a" || choice == "A")
    {
        cout << "  Topic for this presentation: ";
        string topic;
        getline(cin, topic);
        topic = trim(topic);

        if (topic.empty())
        {
            cout << "  No topic given - keeping placeholders as-is.\n";
        }
        else
        {
            cout << "  Note: AI can't generate images, so any image slides in this\n";
            cout << "  template will keep their placeholder path/caption.\n";

            vector<string> notes;
            runWithSpinnerVoid("Asking AI to fill in the template",
                                [&]{ aiFillTemplateSlides(slides, ai, topic, notes); });

            for (const auto& note : notes)
                cout << "  " << note << "\n";
        }
    }

    for (auto* s : slides)
        ppt.addSlide(s);
    cout << "  Added " << slides.size() << " slide(s).\n";
}

void useTemplateMenu(Presentation& ppt, AIAssistant& ai)
{
    auto templates = TemplateManager::listBuiltins();
    cout << "\n  BUILT-IN TEMPLATES:\n";
    for (size_t i = 0; i < templates.size(); i++)
        cout << "  " << (i + 1) << ". " << templates[i].name << " - " << templates[i].description << "\n";

    int choice = readInt("  Choice: ");
    if (choice < 1 || choice > (int)templates.size())
    {
        cout << "  Invalid choice.\n";
        return;
    }

    vector<Slide*> slides = TemplateManager::buildBuiltin(choice - 1);
    cout << "  Using the \"" << templates[choice - 1].name << "\" template (" << slides.size() << " slides).\n";
    offerFillModeAndAdd(slides, ppt, ai);
}

void saveTemplateMenu(Presentation& ppt)
{
    string path;
    cout << "  Save template as (filename, e.g. my_template.template): ";
    getline(cin, path);
    path = trim(path);
    path = stripQuotes(path);
    if (path.empty())
    {
        cout << "  No filename given, cancelled.\n";
        return;
    }
    // Default to a .template extension if the user didn't specify one -
    // Load Template (option 20) scans the current directory for these,
    // so a consistent extension makes saved templates easy to find later.
    if (path.find('.') == string::npos)
        path += ".template";
    ppt.exportTemplate(path);
}

void loadTemplateMenu(Presentation& ppt, AIAssistant& ai)
{
    auto builtins = TemplateManager::listBuiltins();

    // Find saved custom template files (*.template) in the current
    // directory. Scanning is best-effort - if it fails for any reason,
    // just fall back to showing built-ins and letting the user type a
    // custom path directly.
    vector<string> savedFiles;
    try
    {
        for (const auto& entry : fs::directory_iterator("."))
            if (entry.is_regular_file() && entry.path().extension() == ".template")
                savedFiles.push_back(entry.path().filename().string());
    }
    catch (const fs::filesystem_error&) { /* directory scan is best-effort */ }

    cout << "\n  BUILT-IN TEMPLATES:\n";
    for (size_t i = 0; i < builtins.size(); i++)
        cout << "  " << (i + 1) << ". " << builtins[i].name << " - " << builtins[i].description << "\n";

    int savedStart = (int)builtins.size() + 1;
    if (!savedFiles.empty())
    {
        cout << "\n  SAVED TEMPLATES (found in current directory):\n";
        for (size_t i = 0; i < savedFiles.size(); i++)
            cout << "  " << (savedStart + (int)i) << ". " << savedFiles[i] << "\n";
    }

    cout << "\n  Choose a template number, or type a custom file path: ";
    string input;
    getline(cin, input);
    input = trim(input);
    input = stripQuotes(input);

    if (input.empty())
    {
        cout << "  No choice given, cancelled.\n";
        return;
    }

    vector<Slide*> slides;
    string error;
    bool loaded = false;
    bool isNumber = input.find_first_not_of("0123456789") == string::npos;

    if (isNumber)
    {
        int choice = atoi(input.c_str());
        if (choice >= 1 && choice <= (int)builtins.size())
        {
            slides = TemplateManager::buildBuiltin(choice - 1);
            cout << "  Using the \"" << builtins[choice - 1].name << "\" template (" << slides.size() << " slides).\n";
            loaded = true;
        }
        else if (choice >= savedStart && choice < savedStart + (int)savedFiles.size())
        {
            string file = savedFiles[choice - savedStart];
            loaded = TemplateManager::loadFromFile(file, slides, error);
            if (loaded)
                cout << "  Loaded " << slides.size() << " slide(s) from " << file << ".\n";
        }
        else
        {
            cout << "  Invalid choice.\n";
            return;
        }
    }
    else
    {
        loaded = TemplateManager::loadFromFile(input, slides, error);
        if (loaded)
            cout << "  Loaded " << slides.size() << " slide(s) from " << input << ".\n";
    }

    if (!loaded)
    {
        cout << "  Load failed: " << error << "\n";
        return;
    }

    offerFillModeAndAdd(slides, ppt, ai);
}

// A single source of truth for "what does menu option N do" - replaces a
// printed menu list that had to be kept in sync by hand with a separate
// switch statement. Grouped visually (Create/Edit/View/Export/AI/
// Templates) so the menu reads as a set of related actions instead of one
// flat wall of 20 options.
struct MenuCommand
{
    int id;
    std::string label;
    std::function<void()> action;
};

struct MenuGroup
{
    std::string title;
    std::vector<MenuCommand> commands;
};

int main()
{
    Presentation ppt;
    AIAssistant ai; // defaults: localhost:11434, model "llama3.2" (Ollama)

    vector<MenuGroup> groups = {
        { "Create", {
            { 1, "Title Slide",           [&]{ addTitleSlide(ppt); } },
            { 2, "Content Slide",         [&]{ addContentSlide(ppt); } },
            { 3, "Image Slide",           [&]{ addImageSlide(ppt); } },
            { 4, "Bullet Slide",          [&]{ addBulletSlide(ppt); } },
            { 5, "Add raw HTML slide",    [&]{ addCustomHTMLSlide(ppt); } },
        }},
        { "Edit", {
            { 6, "Change Theme",  [&]{ changeTheme(ppt); } },
            { 7, "Move Slide",    [&]{ moveSlideMenu(ppt); } },
            { 8, "Delete Slide",  [&]{ deleteSlideMenu(ppt); } },
        }},
        { "View", {
            { 9,  "Summary",         [&]{ ppt.summary(); } },
            { 10, "View All Slides", [&]{ ppt.showSlides(); } },
        }},
        { "Export", {
            { 11, "Export HTML",            [&]{ ppt.exportHTML(); } },
            { 12, "Export Individual HTML",  [&]{ ppt.exportIndividualSlides(); } },
            { 13, "Export PPTX",            [&]{ ppt.exportPPTX(); } },
        }},
        { "AI", {
            { 14, "Generate Outline",  [&]{ aiGenerateOutlineMenu(ppt, ai); } },
            { 15, "Suggest Bullets",   [&]{ aiSuggestBulletsMenu(ppt, ai); } },
            { 16, "AI Settings",       [&]{ aiSettingsMenu(ai); } },
        }},
        { "Templates", {
            { 17, "Import HTML presentation", [&]{ importHtmlMenu(ppt); } },
            { 18, "Use Built-in Template",    [&]{ useTemplateMenu(ppt, ai); } },
            { 19, "Save as Template",         [&]{ saveTemplateMenu(ppt); } },
            { 20, "Load Template",            [&]{ loadTemplateMenu(ppt, ai); } },
        }},
    };

    int choice;
    do
    {
        cout << "\n============================================\n";
        cout << "    SMART PRESENTATION BUILDER\n";
        cout << "    Theme: " << ppt.getThemeName() << "\n";
        cout << "============================================\n";
        for (const auto& group : groups)
        {
            cout << "========== " << group.title << " ==========\n";
            for (const auto& cmd : group.commands)
                cout << cmd.id << ". " << cmd.label << "\n";
        }
        cout << "0. Exit\n";

        choice = readInt("\n  Enter choice: ");

        if (choice == 0)
        {
            cout << "  Goodbye!\n";
            continue;
        }

        const MenuCommand* found = nullptr;
        for (const auto& group : groups)
        {
            for (const auto& cmd : group.commands)
            {
                if (cmd.id == choice) { found = &cmd; break; }
            }
            if (found) break;
        }

        if (found)
            found->action();
        else
            cout << "  Invalid choice.\n";

    } while (choice != 0);

    return 0;
}
