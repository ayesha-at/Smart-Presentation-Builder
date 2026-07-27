// Tests for Json.h/.cpp - the hand-written JSON parser used to talk to
// Ollama. These specifically cover the shapes that real local-model output
// has been observed to produce (see AIAssistant.cpp's comments for the
// concrete failure history this project has already been through).
#include "Json.h"
#include <cassert>
#include <iostream>

using namespace std;

int main()
{
    // Simple array of strings
    {
        bool ok;
        JsonValue v = parseJson(R"(["a", "b", "c"])", ok);
        assert(ok && v.isArray() && v.arrayValue.size() == 3);
        assert(v.arrayValue[1].asString() == "b");
    }

    // Nested/escaped strings - the exact double-encoding shape Ollama's
    // {"response": "<json-as-a-string>"} envelope produces.
    {
        bool ok;
        JsonValue v = parseJson(R"({"response": "[\"point one\", \"point \\\"two\\\"\"]"})", ok);
        assert(ok && v.isObject());
        string inner = v.get("response").asString();
        JsonValue v2 = parseJson(inner, ok);
        assert(ok && v2.isArray() && v2.arrayValue.size() == 2);
        assert(v2.arrayValue[0].asString() == "point one");
        assert(v2.arrayValue[1].asString() == "point \"two\"");
    }

    // Object wrapping an array (a common small-model deviation)
    {
        bool ok;
        JsonValue v = parseJson(R"({"bullets":["x","y"]})", ok);
        assert(ok && v.isObject() && v.get("bullets").isArray());
    }

    // Malformed input should fail cleanly, not crash
    {
        bool ok;
        parseJson("{not json", ok);
        assert(!ok);
    }

    // Numbers, booleans, null
    {
        bool ok;
        JsonValue v = parseJson(R"({"n": 42, "f": 3.14, "b": true, "z": null})", ok);
        assert(ok);
        assert(v.get("n").asInt() == 42);
        assert(v.get("b").boolValue == true);
        assert(v.get("z").isNull());
    }

    // Unicode escape sequences
    {
        bool ok;
        JsonValue v = parseJson(R"("caf\u00e9")", ok);
        assert(ok);
        assert(v.stringValue == "caf\xC3\xA9"); // UTF-8 encoded "café"
    }

    // Object with an array of numbers (used by the paragraph/bullets
    // section-selection feature)
    {
        bool ok;
        JsonValue v = parseJson("[1, 4, 7]", ok);
        assert(ok && v.isArray() && v.arrayValue.size() == 3);
        assert(v.arrayValue[0].asInt() == 1);
        assert(v.arrayValue[2].asInt() == 7);
    }

    // jsonEscape should make arbitrary text safe to embed in a JSON string
    {
        string input = "Line1\nTab\tQuote\"Backslash\\";
        string escaped = jsonEscape(input);
        bool ok;
        JsonValue v = parseJson("\"" + escaped + "\"", ok);
        assert(ok && v.stringValue == input);
    }

    // stripMarkdownFence: removes ```json ... ``` wrapper
    {
        string wrapped = "```json\n[\"a\", \"b\"]\n```";
        string stripped = stripMarkdownFence(wrapped);
        bool ok;
        JsonValue v = parseJson(stripped, ok);
        assert(ok && v.isArray() && v.arrayValue.size() == 2);
    }

    // stripMarkdownFence: removes bare ``` ... ``` wrapper (no language tag)
    {
        string wrapped = "```\n{\"key\":\"value\"}\n```";
        assert(stripMarkdownFence(wrapped) == "{\"key\":\"value\"}");
    }

    // stripMarkdownFence: leaves unwrapped JSON completely unchanged
    {
        string plain = "[\"a\",\"b\"]";
        assert(stripMarkdownFence(plain) == plain);
    }

    // stripMarkdownFence: tolerates surrounding whitespace
    {
        string wrapped = "  \n```json\n[1,2,3]\n```  \n";
        assert(stripMarkdownFence(wrapped) == "[1,2,3]");
    }

    cout << "All Json tests passed.\n";
    return 0;
}
