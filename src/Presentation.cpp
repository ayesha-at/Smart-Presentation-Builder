#include "Presentation.h"
#include "Theme.h"
#include "PptxBuilder.h"
#include <iostream>
#include <fstream>

using namespace std;

Presentation::Presentation()
{
    currentTheme = &coffeeTheme;
}

void Presentation::setTheme(int choice)
{
    if (choice == 1) currentTheme = &darkTheme;
    else if (choice == 2) currentTheme = &minimalTheme;
    else if (choice == 3) currentTheme = &retroTheme;
    else if (choice == 4) currentTheme = &oceanTheme;
    else if (choice == 5) currentTheme = &sunsetTheme;
    else if (choice == 6) currentTheme = &forestTheme;
    else if (choice == 7) currentTheme = &coffeeTheme;
    else cout << "  Invalid theme choice.\n";
}

string Presentation::getThemeName()
{
    return currentTheme->getName();
}

bool Presentation::addSlide(Slide* s)
{
    slides.push_back(s);
    return true;
}

void Presentation::deleteSlide(int index)
{
    int i = index - 1;

    if (i < 0 || i >= (int)slides.size())
    {
        cout << "  Invalid slide number.\n";
        return;
    }

    delete slides[i];
    slides.erase(slides.begin() + i);

    cout << "  Slide " << index << " deleted.\n";
}

void Presentation::moveSlide(int from, int to)
{
    int f = from - 1;
    int t = to - 1;

    if (f < 0 || f >= (int)slides.size() || t < 0 || t >= (int)slides.size())
    {
        cout << "  Invalid slide number(s).\n";
        return;
    }

    Slide* temp = slides[f];

    if (f < t)
    {
        for (int i = f; i < t; i++)
            slides[i] = slides[i + 1];
    }
    else
    {
        for (int i = f; i > t; i--)
            slides[i] = slides[i - 1];
    }

    slides[t] = temp;
    cout << "  Moved slide " << from << " to position " << to << ".\n";
}

void Presentation::summary()
{
    if (slides.empty())
    {
        cout << "  No slides yet.\n";
        return;
    }
    cout << "\n  Presentation summary  [Theme: "
        << currentTheme->getName() << "]\n";
    cout << "  ----------------------------------------\n";
    for (int i = 0; i < (int)slides.size(); i++)
    {
        cout << "  " << (i + 1) << ".  "
            << slides[i]->summary() << "\n";
    }
    cout << "  Total slides: " << slides.size() << "\n";
}

void Presentation::showSlides()
{
    if (slides.empty())
    {
        cout << "  No slides to show.\n";
        return;
    }
    cout << "\n===== PRESENTATION  [Theme: "
        << currentTheme->getName() << "] =====\n";
    for (int i = 0; i < (int)slides.size(); i++)
    {
        cout << "\nSlide " << (i + 1) << " of " << slides.size();
        slides[i]->display();
    }
}

void Presentation::exportIndividualSlides()
{
    if (slides.empty())
    {
        cout << "  No slides to export.\n";
        return;
    }

    cout << "\n\xF0\x9F\x93\x84 Exporting each slide as separate HTML file...\n";

    for (int i = 0; i < (int)slides.size(); i++)
    {
        string filename = "slide_" + to_string(i + 1) + ".html";
        ofstream file(filename);

        if (!file.is_open())
        {
            cout << "  Could not create " << filename << "\n";
            continue;
        }

        file << "<!DOCTYPE html>\n<html>\n<head>\n";
        file << "<meta charset='UTF-8'>\n";
        file << "<title>Slide " << (i + 1) << " - " << currentTheme->getName() << "</title>\n";
        file << currentTheme->buildCSS();
        file << "</head>\n<body>\n";
        file << slides[i]->generateHTML(*currentTheme, i + 1);
        file << "</body>\n</html>\n";
        file.close();

        cout << "  " << filename << "\n";
    }

    cout << "\nExported " << slides.size() << " individual slide files!\n";
    cout << "   Open any slide_*.html file directly in browser.\n";
}

void Presentation::exportHTML()
{
    if (slides.empty())
    {
        cout << "  Nothing to export.\n";
        return;
    }

    ofstream file("presentation.html");

    if (!file.is_open())
    {
        cout << "  Error: could not create presentation.html\n";
        return;
    }

    // Shows one slide at a time with prev/next buttons, a slide counter,
    // and left/right arrow key navigation. Plain vanilla JS, no external
    // scripts, so it works fully offline straight from a file:// URL.
    const char* interactiveCSS =
        "<style>"
        ".slide { display: none; }"
        ".slide.active { display: block; }"
        ".nav-controls {"
        "  position: fixed; bottom: 24px; left: 50%; transform: translateX(-50%);"
        "  display: flex; align-items: center; gap: 16px;"
        "  background: rgba(0,0,0,0.55); padding: 10px 20px; border-radius: 999px;"
        "  font-family: 'Segoe UI', sans-serif; color: #fff; z-index: 1000;"
        "}"
        ".nav-controls button {"
        "  background: rgba(255,255,255,0.15); border: none; color: #fff;"
        "  width: 36px; height: 36px; border-radius: 50%; font-size: 18px; cursor: pointer;"
        "}"
        ".nav-controls button:hover:not(:disabled) { background: rgba(255,255,255,0.3); }"
        ".nav-controls button:disabled { opacity: 0.3; cursor: default; }"
        "#slideCounter { font-size: 14px; min-width: 50px; text-align: center; }"
        "</style>";

    const char* navHTML =
        "<div class='nav-controls'>"
        "<button id='prevBtn' aria-label='Previous slide'>&#8592;</button>"
        "<span id='slideCounter'></span>"
        "<button id='nextBtn' aria-label='Next slide'>&#8594;</button>"
        "</div>\n";

    const char* interactiveJS =
        "<script>"
        "(function(){"
        "  var slides = document.querySelectorAll('.slide');"
        "  var current = 0;"
        "  function show(i) {"
        "    if (i < 0 || i >= slides.length) return;"
        "    current = i;"
        "    slides.forEach(function(s, idx) { s.classList.toggle('active', idx === current); });"
        "    document.getElementById('slideCounter').textContent = (current + 1) + ' / ' + slides.length;"
        "    document.getElementById('prevBtn').disabled = current === 0;"
        "    document.getElementById('nextBtn').disabled = current === slides.length - 1;"
        "  }"
        "  document.getElementById('prevBtn').addEventListener('click', function(){ show(current - 1); });"
        "  document.getElementById('nextBtn').addEventListener('click', function(){ show(current + 1); });"
        "  document.addEventListener('keydown', function(e){"
        "    if (e.key === 'ArrowRight' || e.key === ' ') show(current + 1);"
        "    if (e.key === 'ArrowLeft') show(current - 1);"
        "  });"
        "  show(0);"
        "})();"
        "</script>\n";

    file << "<!DOCTYPE html>\n<html lang='en'>\n<head>\n";
    file << "<meta charset='UTF-8'>\n";
    file << "<title>Presentation - " << currentTheme->getName() << "</title>\n";

    file << currentTheme->buildCSS();
    file << interactiveCSS;
    file << "</head>\n<body>\n";

    for (int i = 0; i < (int)slides.size(); i++)
        file << slides[i]->generateHTML(*currentTheme, i + 1);

    file << navHTML;
    file << interactiveJS;
    file << "</body>\n</html>\n";
    file.close();

    cout << "\n  Exported!  Open  presentation.html  in any browser.\n";
    cout << "  Navigate with the on-screen arrows, click, or left/right arrow keys.\n";
    cout << "  Theme applied: " << currentTheme->getName() << "\n";
    cout << "  Total slides : " << slides.size() << "\n";
}

void Presentation::exportTemplate(const string& path)
{
    if (slides.empty())
    {
        cout << "  Nothing to save.\n";
        return;
    }

    ofstream file(path);
    if (!file.is_open())
    {
        cout << "  Error: could not create " << path << "\n";
        return;
    }

    file << "[";
    for (size_t i = 0; i < slides.size(); i++)
    {
        if (i > 0) file << ",";
        file << slides[i]->toTemplateJSON();
    }
    file << "]";
    file.close();

    cout << "  Saved template to " << path << " (" << slides.size() << " slide(s)).\n";
    cout << "  Load it later with option 20 (Load a Saved Template).\n";
}

void Presentation::exportPPTX()
{
    if (slides.empty())
    {
        cout << "  Nothing to export.\n";
        return;
    }

    PptxBuilder builder;
    for (int i = 0; i < (int)slides.size(); i++)
        slides[i]->addToPPTX(builder, *currentTheme);

    if (builder.save("presentation.pptx"))
    {
        cout << "\n  Exported!  Open  presentation.pptx  in PowerPoint or LibreOffice Impress.\n";
        cout << "  Theme applied: " << currentTheme->getName() << "\n";
        cout << "  Total slides : " << slides.size() << "\n";
    }
    else
    {
        cout << "  Error: could not create presentation.pptx\n";
    }
}

vector<string> Presentation::slideSummaries() const
{
    vector<string> result;
    for (auto* s : slides)
        result.push_back(s->summary());
    return result;
}

int Presentation::slideCount() const
{
    return (int)slides.size();
}

Presentation::~Presentation()
{
    for (int i = 0; i < (int)slides.size(); i++)
        delete slides[i];
}
