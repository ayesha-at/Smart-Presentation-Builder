#include "AIAssistant.h"
#include "HttpClient.h"
#include "Json.h"
#include "JsonHelpers.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>
#include <memory>

using namespace std;
using namespace JsonHelpers;

void AIAssistant::configure(const string& h, int p, const string& m)
{
    host = h;
    port = p;
    model = m;
}

string AIAssistant::getHost()  const { return host; }
int    AIAssistant::getPort()  const { return port; }
string AIAssistant::getModel() const { return model; }

void AIAssistant::setTimeoutSeconds(int seconds) { timeoutSeconds = seconds; }
int  AIAssistant::getTimeoutSeconds() const { return timeoutSeconds; }

bool AIAssistant::checkConnection(string& error)
{
    int statusCode = 0;
    string body;
    if (!HttpClient::get(host, port, "/api/tags", statusCode, body, error, 5))
    {
        error += "\n  Make sure Ollama is installed and running (command: 'ollama serve').";
        return false;
    }
    if (statusCode != 200)
    {
        error = "Ollama responded with HTTP " + to_string(statusCode) +
                " - is the right host/port configured?";
        return false;
    }
    return true;
}

bool AIAssistant::queryJSON(const string& prompt, string& rawJsonOut, string& error)
{
    ostringstream body;
    body << "{"
         << "\"model\":\"" << jsonEscape(model) << "\","
         << "\"prompt\":\"" << jsonEscape(prompt) << "\","
         << "\"stream\":false,"
         << "\"format\":\"json\""
         << "}";

    int statusCode = 0;
    string responseBody;
    bool success = false;

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        if (HttpClient::post(
            host,
            port,
            "/api/generate",
            body.str(),
            statusCode,
            responseBody,
            error,
            timeoutSeconds))
        {
            success = true;
            break;
        }

        if (attempt == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            error.clear();
        }
    }

    if (!success)
    {
        error += "\n  Make sure Ollama is installed and running (command: 'ollama serve').";
        return false;
    }

    bool ok = false;
    JsonValue parsed = parseJson(responseBody, ok);

    if (statusCode != 200)
    {
        // Ollama's error responses look like {"error":"model \"x\" not found..."}
        string ollamaError = ok ? parsed.get("error").asString() : "";
        error = !ollamaError.empty()
            ? "Ollama error: " + ollamaError
            : "Ollama responded with HTTP " + to_string(statusCode);
        return false;
    }

    if (!ok)
    {
        error = "Received a malformed response from Ollama.";
        return false;
    }

    rawJsonOut = parsed.get("response").asString();
    if (rawJsonOut.empty())
    {
        error = "Ollama's response didn't include any generated text.";
        return false;
    }

    // Some small models occasionally ignore "format":"json" and still
    // wrap the reply in a markdown fence. Strip it here, once, so every
    // caller (bullets, content, paragraph-index selection, everything)
    // gets clean JSON regardless.
    rawJsonOut = stripMarkdownFence(rawJsonOut);

    return true;
}

bool AIAssistant::suggestBullets(const string& heading, int count,
                                  vector<string>& outBullets, string& error)
{
    ostringstream prompt;
    prompt << "You are a presentation writing assistant.\n"
           << "Suggest exactly " << count << " concise bullet points for a presentation slide about: \""
           << heading << "\".\n"
           << "Each bullet should be a short phrase, under 12 words, with no leading dashes, numbers, or quotes.\n"
           << "Return ONLY a JSON array of strings in exactly this format:\n"
           << "[\"first bullet\", \"second bullet\", \"third bullet\", \"fourth bullet\"]\n"
           << "Do NOT return an object. Do NOT use keys. Do NOT explain. Do NOT use markdown. "
           << "Generate exactly " << count << " strings. No other text.";

    string raw;
    if (!queryJSON(prompt.str(), raw, error))
        return false;

    bool ok = false;
    JsonValue parsed = parseJson(raw, ok);

    const JsonValue* arr = nullptr;
    unique_ptr<JsonValue> recoveredBullets; // owns the fallback array, if any, so its lifetime is bounded

    if (ok)
    {
        if (parsed.isArray())
            arr = &parsed;
        else
            arr = findArrayRecursive(parsed);

        // Last resort: llama3.2 sometimes returns the bullets as object
        // keys instead of a JSON array, e.g. {"first bullet":"", ...}.
        // Guarded against false-positiving on a legitimately-structured
        // reply (see recoverBulletsFromObjectKeys's own doc comment).
        if (!arr)
        {
            recoveredBullets.reset(recoverBulletsFromObjectKeys(parsed));
            arr = recoveredBullets.get();
        }
    }
    if (!arr)
    {
        error = "AI response wasn't a usable JSON array of bullets.\n"
                "  Raw AI reply: " + previewOf(raw);
        return false;
    }

    outBullets.clear();
    for (const auto& item : arr->arrayValue)
    {
        // Usually a plain string, but some models return [{"text": "..."}]
        // instead - handle both shapes.
        string s = item.isString() ? item.asString()
                                    : firstField(item, { "text", "bullet", "point", "content" });
        if (!s.empty())
            outBullets.push_back(s);
    }

    if (outBullets.empty())
    {
        error = "AI didn't return any usable bullet points.\n"
                "  Raw AI reply: " + previewOf(raw);
        return false;
    }

    return true;
}

bool AIAssistant::suggestContent(const string& heading, string& outContent, string& error)
{
    ostringstream prompt;
    prompt << "Write a short 1-2 sentence paragraph (not bullet points) for a presentation "
           << "slide about: \"" << heading << "\".\n"
           << "Respond with ONLY this JSON object: {\"content\":\"...\"}\n"
           << "No markdown, no bullet points, no line breaks, no other text.";

    string raw;
    if (!queryJSON(prompt.str(), raw, error))
        return false;

    bool ok = false;
    JsonValue parsed = parseJson(raw, ok);

    string content;
    if (ok && parsed.isObject())
        content = firstField(parsed, { "content", "text", "body", "paragraph" });
    else if (ok && parsed.isString())
        content = parsed.stringValue; // some models skip the wrapper object entirely

    if (content.empty())
    {
        error = "AI didn't return usable paragraph content.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }

    outContent = content;
    return true;
}

bool AIAssistant::suggestTitleSubtitle(const string& topic, string& outTitle,
                                       string& outSubtitle, string& error)
{
    ostringstream prompt;
    prompt << "Write a title and subtitle for a presentation about \"" << topic << "\".\n"
           << "Respond with ONLY this JSON object: {\"title\":\"...\",\"subtitle\":\"...\"}\n"
           << "No markdown, no other text.";

    string raw;
    if (!queryJSON(prompt.str(), raw, error))
        return false;

    bool ok = false;
    JsonValue parsed = parseJson(raw, ok);
    if (!ok || !parsed.isObject())
    {
        error = "AI didn't return a usable title/subtitle.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }

    outTitle = firstField(parsed, { "title", "heading", "name" });
    outSubtitle = firstField(parsed, { "subtitle", "sub_title", "tagline" });

    if (outTitle.empty())
    {
        error = "AI didn't return a usable title.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }
    return true;
}

vector<int> AIAssistant::pickParagraphIndices(const vector<string>& headings)
{
    if (headings.empty()) return {};

    ostringstream prompt;
    prompt << "Here are section headings for a presentation:\n";
    for (size_t i = 0; i < headings.size(); i++)
        prompt << (i + 1) << ". " << headings[i] << "\n";
    prompt << "Which of these (by number) would work better as a short 1-2 sentence paragraph "
           << "instead of a bullet list? Usually only summary, motivation, or context-setting "
           << "sections should be paragraphs - most sections should stay as bullets.\n"
           << "Respond with ONLY a JSON array of numbers, for example [1, 4]. "
           << "Use an empty array [] if none fit. No other text.";

    string raw, err;
    if (!queryJSON(prompt.str(), raw, err))
        return {}; // non-fatal: caller just defaults everything to bullets

    bool ok = false;
    JsonValue parsed = parseJson(raw, ok);
    const JsonValue* arr = ok ? findArrayRecursive(parsed) : nullptr;
    if (!arr) return {};

    vector<int> indices;
    for (const auto& item : arr->arrayValue)
        if (item.isNumber())
            indices.push_back(item.asInt());
    return indices;
}

bool AIAssistant::generateOutline(const string& topic, int numSlides,
                                   vector<AISlideSpec>& outSlides, string& error,
                                   vector<string>& warnings)
{
    warnings.clear();
    int sectionCount = numSlides - 1;
    if (sectionCount < 1) sectionCount = 1;

    // Step 1: ask for a title + subtitle + list of section headings only.
    // This is a small, flat JSON shape - much more reliable for a small
    // local model than asking it to produce a whole nested array of full
    // slide objects in one shot (which it may simply refuse/truncate).
    ostringstream prompt;
    prompt << "Create a title, subtitle, and " << sectionCount
           << " short section headings for a presentation about \"" << topic << "\".\n"
           << "Respond with ONLY this JSON object shape, no markdown, no extra text:\n"
           << "{\"title\":\"...\",\"subtitle\":\"...\",\"headings\":[\"...\", \"...\"]}\n"
           << "The \"headings\" array must contain exactly " << sectionCount
           << " section titles, each under 6 words, in a logical order for a beginner-friendly talk.";

    string raw;
    if (!queryJSON(prompt.str(), raw, error))
        return false;

    bool ok = false;
    JsonValue parsed = parseJson(raw, ok);
    // The whole reply might itself be nested (e.g. {"outline": {...}}), so
    // search for an object carrying a headings-like array, same tolerance
    // as elsewhere in this file.
    const JsonValue* obj = &parsed;
    if (ok && !parsed.isObject())
    {
        error = "AI response wasn't a usable JSON object.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }
    if (!ok)
    {
        error = "AI response wasn't valid JSON.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }

    string title = firstField(*obj, { "title", "heading", "name" });
    string subtitle = firstField(*obj, { "subtitle", "sub_title", "tagline" });
    const JsonValue* headingsArr = firstArrayField(*obj, { "headings", "sections", "topics" });

    if (title.empty() || !headingsArr || headingsArr->arrayValue.empty())
    {
        error = "AI didn't return a usable title/headings list.\n  Raw AI reply: " + previewOf(raw);
        return false;
    }

    vector<string> headings;
    for (const auto& h : headingsArr->arrayValue)
    {
        string s = h.isString() ? h.asString() : firstField(h, { "text", "heading" });
        if (!s.empty())
            headings.push_back(s);
    }
    if ((int)headings.size() > sectionCount)
        headings.resize(sectionCount);

    outSlides.clear();

    AISlideSpec titleSpec;
    titleSpec.type = "title";
    titleSpec.heading = title;
    titleSpec.subtitle = subtitle;
    outSlides.push_back(titleSpec);

    // Decide which sections should be short paragraphs instead of bullet
    // lists (a separate, simple, low-risk call - see pickParagraphIndices).
    vector<int> paragraphIndices = pickParagraphIndices(headings);

    // Step 2: one small, simple call per heading to get its content - the
    // same request shape as suggestBullets()/suggestContent(), both
    // already reliable.
    for (size_t idx = 0; idx < headings.size(); idx++)
    {
        const string& heading = headings[idx];
        bool wantParagraph = find(paragraphIndices.begin(), paragraphIndices.end(), (int)idx + 1)
                              != paragraphIndices.end();

        if (wantParagraph)
        {
            string content;
            string sectionError;
            if (suggestContent(heading, content, sectionError))
            {
                AISlideSpec spec;
                spec.type = "content";
                spec.heading = heading;
                spec.content = content;
                outSlides.push_back(spec);
                continue;
            }
            // Paragraph generation failed - fall through and try bullets
            // for this section instead of giving up on it entirely.
            warnings.push_back("\"" + heading + "\" (paragraph attempt): " + sectionError +
                                " - falling back to bullets.");
        }

        vector<string> bullets;
        string sectionError;
        if (suggestBullets(heading, 4, bullets, sectionError))
        {
            AISlideSpec spec;
            spec.type = "bullets";
            spec.heading = heading;
            spec.bullets = bullets;
            outSlides.push_back(spec);
        }
        else
        {
            // Skip just this section rather than failing the whole
            // outline - but always record why, so a partial result is
            // diagnosable instead of a silent, confusing shortfall.
            warnings.push_back("\"" + heading + "\": " + sectionError);

            // If the model just timed out or choked, immediately hitting
            // it with the next section's request often just repeats the
            // failure. A short pause gives it a chance to recover; this
            // only costs time on the already-slow failure path, never on
            // the happy path.
            this_thread::sleep_for(chrono::seconds(2));
        }
    }

    if (outSlides.size() <= 1)
    {
        error = "Got a title from the AI, but couldn't get usable bullets for any section.\n"
                "  Try again, or use option 15 to fill in bullets one heading at a time.";
        return false;
    }

    return true;
}
