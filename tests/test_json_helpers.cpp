// Tests for JsonHelpers.h/.cpp - JSON tree-traversal helpers extracted
// out of AIAssistant.cpp's anonymous namespace specifically so this logic
// (previously private, untested implementation detail) has real coverage.
#include "JsonHelpers.h"
#include "Json.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace JsonHelpers;

int main()
{
    // findArrayRecursive: top-level array
    {
        bool ok;
        JsonValue v = parseJson(R"(["a","b"])", ok);
        const JsonValue* arr = findArrayRecursive(v);
        assert(arr != nullptr && arr->arrayValue.size() == 2);
    }

    // findArrayRecursive: array wrapped one level deep
    {
        bool ok;
        JsonValue v = parseJson(R"({"bullets":["a","b","c"]})", ok);
        const JsonValue* arr = findArrayRecursive(v);
        assert(arr != nullptr && arr->arrayValue.size() == 3);
    }

    // findArrayRecursive: array nested two levels deep (the real llama3.2
    // failure pattern this was added to handle - see AIAssistant.cpp)
    {
        bool ok;
        JsonValue v = parseJson(R"({"presentation":{"slides":["x","y"]}})", ok);
        const JsonValue* arr = findArrayRecursive(v);
        assert(arr != nullptr && arr->arrayValue.size() == 2);
    }

    // findArrayRecursive: no array anywhere - must return nullptr, not crash
    {
        bool ok;
        JsonValue v = parseJson(R"({"title":"x","subtitle":"y"})", ok);
        const JsonValue* arr = findArrayRecursive(v);
        assert(arr == nullptr);
    }

    // findArrayRecursive: empty arrays are skipped (not "found"), since an
    // empty array is useless to the caller and a real array may be nested
    // deeper than an empty decoy one.
    {
        bool ok;
        JsonValue v = parseJson(R"({"a":[],"b":{"c":["real"]}})", ok);
        const JsonValue* arr = findArrayRecursive(v);
        assert(arr != nullptr && arr->arrayValue.size() == 1);
        assert(arr->arrayValue[0].asString() == "real");
    }

    // firstField: case-insensitive, tries synonyms in order, skips empty values
    {
        bool ok;
        JsonValue v = parseJson(R"({"Heading":"", "Title":"The Real Title"})", ok);
        string result = firstField(v, { "heading", "title", "name" });
        assert(result == "The Real Title");
    }

    // firstField: no match returns empty string, not a crash
    {
        bool ok;
        JsonValue v = parseJson(R"({"unrelated":"value"})", ok);
        assert(firstField(v, { "heading", "title" }).empty());
    }

    // firstArrayField: only matches keys whose value is actually an array
    {
        bool ok;
        JsonValue v = parseJson(R"({"bullets":"not an array","points":["real","array"]})", ok);
        const JsonValue* arr = firstArrayField(v, { "bullets", "points" });
        assert(arr != nullptr && arr->arrayValue.size() == 2);
    }

    // toLowerCopy
    {
        assert(toLowerCopy("HeLLo") == "hello");
    }

    // previewOf truncates long strings and leaves short ones alone
    {
        string longStr(500, 'x');
        string preview = previewOf(longStr, 300);
        assert(preview.size() == 303); // 300 chars + "..."
        assert(previewOf("short", 300) == "short");
    }

    // recoverBulletsFromObjectKeys: recovers bullets when the model
    // returns them as object keys (the real llama3.2 failure pattern)
    {
        bool ok;
        JsonValue v = parseJson(R"({"First point":"", "Second point":""})", ok);
        JsonValue* recovered = recoverBulletsFromObjectKeys(v);
        assert(recovered != nullptr);
        assert(recovered->arrayValue.size() == 2);
        assert(recovered->arrayValue[0].asString() == "First point");
        delete recovered;
    }

    // recoverBulletsFromObjectKeys: recovers BOTH key and value when both
    // are genuine sentences (the "Implementing Best Practices" case)
    {
        bool ok;
        JsonValue v = parseJson(R"({"Establish clear goals first":"Develop training programs next"})", ok);
        JsonValue* recovered = recoverBulletsFromObjectKeys(v);
        assert(recovered != nullptr);
        assert(recovered->arrayValue.size() == 2);
        delete recovered;
    }

    // recoverBulletsFromObjectKeys: must NOT misfire on a legitimately
    // structured object (e.g. a title/subtitle reply) - this is exactly
    // the false-positive the guard exists to prevent.
    {
        bool ok;
        JsonValue v = parseJson(R"({"title":"My Title","subtitle":"My Subtitle"})", ok);
        JsonValue* recovered = recoverBulletsFromObjectKeys(v);
        assert(recovered == nullptr);
    }

    // recoverBulletsFromObjectKeys: empty object and non-object both
    // return nullptr rather than crashing
    {
        bool ok;
        JsonValue empty = parseJson("{}", ok);
        assert(recoverBulletsFromObjectKeys(empty) == nullptr);
        JsonValue arr = parseJson("[1,2,3]", ok);
        assert(recoverBulletsFromObjectKeys(arr) == nullptr);
    }

    cout << "All JsonHelpers tests passed.\n";
    return 0;
}
