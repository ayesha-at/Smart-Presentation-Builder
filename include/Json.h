#pragma once
#include <string>
#include <vector>
#include <utility>

// A minimal, dependency-free JSON representation. Not a general-purpose
// JSON library - just enough to build request bodies and parse the
// responses we get back from a local LLM server (and, later, our own
// REST API).
enum class JsonType { Null, Bool, Number, String, Array, Object };

class JsonValue
{
public:
    JsonType type = JsonType::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::string, JsonValue>> objectValue;

    bool isNull()   const { return type == JsonType::Null; }
    bool isBool()   const { return type == JsonType::Bool; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray()  const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    // Object member lookup. Returns a Null JsonValue (not a crash) if the
    // key is missing or this isn't an object - callers can chain safely.
    const JsonValue& get(const std::string& key) const;

    std::string asString(const std::string& def = "") const;
    int         asInt(int def = 0) const;
};

// Parses `text` as JSON. Sets ok=false and returns a Null JsonValue on
// malformed input rather than throwing.
JsonValue parseJson(const std::string& text, bool& ok);

// Strips a wrapping markdown code fence (``` or ```json ... ```), if
// present, along with surrounding whitespace. Some LLMs ignore
// instructions to avoid markdown and wrap their JSON reply in a fence
// anyway; call this on raw model output before parseJson.
std::string stripMarkdownFence(const std::string& s);

// Escapes a string so it can be embedded inside a JSON string literal.
std::string jsonEscape(const std::string& s);
