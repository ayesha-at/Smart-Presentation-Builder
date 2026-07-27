#pragma once
#include <string>
#include <vector>

// One slide's worth of AI-generated content. Deliberately decoupled from
// the Slide class hierarchy - AIAssistant only describes what to build;
// the caller (main.cpp) turns each spec into a real TitleSlide/
// ContentSlide/BulletSlide using the existing constructors.
struct AISlideSpec
{
    std::string type; // "title" | "content" | "bullets"
    std::string heading;               // title (type=title) or heading (content/bullets)
    std::string subtitle;              // type=title only
    std::string content;               // type=content only
    std::vector<std::string> bullets;  // type=bullets only
};

// Talks to a local LLM server (default: Ollama on localhost:11434) to
// generate slide content. No internet connection required - everything
// runs against a model already running on the user's own machine.
class AIAssistant
{
private:
    std::string host = "localhost";
    int port = 11434;
    std::string model = "llama3.2";
    int timeoutSeconds = 180; // local CPU inference can be slow, especially for longer prompts

    // Sends `prompt` to the LLM asking for JSON-only output, and returns
    // the raw text (expected to itself be parseable JSON).
    bool queryJSON(const std::string& prompt, std::string& rawJsonOut, std::string& error);

public:
    void configure(const std::string& h, int p, const std::string& m);
    std::string getHost()  const;
    int         getPort()  const;
    std::string getModel() const;

    void setTimeoutSeconds(int seconds);
    int  getTimeoutSeconds() const;

    // Verifies the server is reachable (and, if possible, that the
    // configured model is available). Fills `error` with next steps if not.
    bool checkConnection(std::string& error);

    bool suggestBullets(const std::string& heading, int count,
                         std::vector<std::string>& outBullets, std::string& error);

    // Real paragraph content (not bullets) for a single slide.
    bool suggestContent(const std::string& heading, std::string& outContent, std::string& error);

    // A title + subtitle for a given topic - used both by generateOutline
    // internally and by the "AI-fill a template" feature to replace a
    // template's placeholder title slide.
    bool suggestTitleSubtitle(const std::string& topic, std::string& outTitle,
                               std::string& outSubtitle, std::string& error);

    // `warnings` is filled with a human-readable note per section whose
    // bullets couldn't be generated (heading skipped, not the whole
    // outline) - always check it, even when this returns true, so partial
    // failures are visible instead of silently dropped.
    bool generateOutline(const std::string& topic, int numSlides,
                          std::vector<AISlideSpec>& outSlides, std::string& error,
                          std::vector<std::string>& warnings);

private:
    // Asks which section headings (by 1-based number) would work better as
    // a short paragraph instead of a bullet list. A flat array of numbers
    // is a far more reliable request shape for small local models than
    // asking them to classify each slide inside a nested object - so this
    // stays a separate, simple call rather than folding into the main
    // outline request. Returns an empty list (never fails outright) if the
    // model doesn't answer usefully; callers should just default to
    // bullets in that case.
    std::vector<int> pickParagraphIndices(const std::vector<std::string>& headings);
};
