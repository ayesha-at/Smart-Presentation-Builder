#include "JsonHelpers.h"
#include <algorithm>
#include <cctype>

using namespace std;

namespace JsonHelpers
{
    string previewOf(const string& raw, size_t maxLen)
    {
        string preview = raw.substr(0, maxLen);
        if (raw.size() > maxLen) preview += "...";
        return preview;
    }

    string toLowerCopy(string s)
    {
        transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
        return s;
    }

    string firstField(const JsonValue& obj, const vector<string>& keys)
    {
        for (const auto& key : keys)
        {
            for (const auto& kv : obj.objectValue)
            {
                if (toLowerCopy(kv.first) == key)
                {
                    string s = kv.second.asString();
                    if (!s.empty()) return s;
                }
            }
        }
        return "";
    }

    const JsonValue* firstArrayField(const JsonValue& obj, const vector<string>& keys)
    {
        for (const auto& key : keys)
        {
            for (const auto& kv : obj.objectValue)
            {
                if (toLowerCopy(kv.first) == key && kv.second.isArray())
                    return &kv.second;
            }
        }
        return nullptr;
    }

    const JsonValue* findArrayRecursive(const JsonValue& v, int depth, int maxDepth)
    {
        if (v.isArray() && !v.arrayValue.empty()) return &v;
        if (depth >= maxDepth) return nullptr;
        if (v.isObject())
        {
            for (const auto& kv : v.objectValue)
            {
                const JsonValue* found = findArrayRecursive(kv.second, depth + 1, maxDepth);
                if (found) return found;
            }
        }
        return nullptr;
    }

    JsonValue* recoverBulletsFromObjectKeys(const JsonValue& v)
    {
        if (!v.isObject() || v.objectValue.empty()) return nullptr;

        // Reject objects that look like the *intended* shape (title/
        // headings/etc.) by checking for a known field name. If we find
        // one, this object is meaningfully structured (e.g. {"title":"X",
        // "subtitle":"Y"}), not a case of sentences-used-as-keys - without
        // this guard, a legitimately-structured reply could get
        // misinterpreted as a pile of unrelated bullets.
        static const vector<string> knownFields = {
            "title", "heading", "headings", "subtitle", "content", "bullets",
            "sections", "topics", "text", "body", "paragraph"
        };
        for (const auto& kv : v.objectValue)
        {
            string k = toLowerCopy(kv.first);
            for (const auto& known : knownFields)
                if (k == known) return nullptr;
        }

        // All keys look like prose, not field names - recover both the
        // key and (if present, non-empty, and distinct) the value as
        // bullet candidates. Covers both the common {"bullet text":""}
        // shape and the rarer case where a real model put a second
        // genuine sentence in the value slot instead of leaving it empty.
        JsonValue* arr = new JsonValue();
        arr->type = JsonType::Array;
        for (const auto& kv : v.objectValue)
        {
            if (!kv.first.empty())
            {
                JsonValue item;
                item.type = JsonType::String;
                item.stringValue = kv.first;
                arr->arrayValue.push_back(item);
            }
            if (kv.second.isString() && !kv.second.asString().empty() && kv.second.asString() != kv.first)
            {
                JsonValue item;
                item.type = JsonType::String;
                item.stringValue = kv.second.asString();
                arr->arrayValue.push_back(item);
            }
        }
        return arr;
    }
}
