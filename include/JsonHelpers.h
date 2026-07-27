#pragma once
#include "Json.h"
#include <string>
#include <vector>

// Small JSON tree-traversal helpers used when parsing LLM output, which is
// frequently loosely/inconsistently structured (see AIAssistant.cpp for
// the concrete failure patterns these exist to work around). Pulled out
// of AIAssistant.cpp specifically so this logic has real unit test
// coverage instead of living as untestable anonymous-namespace internals.
namespace JsonHelpers
{
    // Truncates a raw AI reply for inclusion in an error message so the
    // user can see exactly what the model actually said instead of a bare
    // "parsing failed".
    std::string previewOf(const std::string& raw, size_t maxLen = 300);

    std::string toLowerCopy(std::string s);

    // Returns the value of the first key (case-insensitive) present in the
    // object that isn't empty. Small local models are inconsistent about
    // field naming, so callers typically pass several reasonable synonyms.
    std::string firstField(const JsonValue& obj, const std::vector<std::string>& keys);

    const JsonValue* firstArrayField(const JsonValue& obj, const std::vector<std::string>& keys);

    // Recursively searches for a non-empty array anywhere in the JSON
    // tree. Some local models nest the array a level or two deeper than
    // asked (e.g. {"presentation": {"slides": [...]}}) despite
    // instructions.
    const JsonValue* findArrayRecursive(const JsonValue& v, int depth = 0, int maxDepth = 4);

    // Last-resort recovery for a known llama3.2 failure mode: asked for a
    // JSON array of bullet strings, it instead returns an object whose
    // KEYS are the bullet text (values typically empty), e.g.
    // {"first bullet":"", "second bullet":""}. Returns nullptr if `v`
    // doesn't look like this specific pattern (empty object, or one that
    // has a recognizable field name and is therefore meaningfully
    // structured, not sentences-as-keys). Caller owns the returned
    // pointer.
    JsonValue* recoverBulletsFromObjectKeys(const JsonValue& v);
}
