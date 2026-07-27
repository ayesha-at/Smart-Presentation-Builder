#include "Json.h"
#include <cctype>
#include <cstdlib>
#include <sstream>

using namespace std;

namespace
{
    struct Parser
    {
        const string& text;
        size_t pos = 0;
        bool failed = false;

        explicit Parser(const string& t) : text(t) {}

        void skipWs()
        {
            while (pos < text.size() && isspace((unsigned char)text[pos]))
                pos++;
        }

        char peek()
        {
            return pos < text.size() ? text[pos] : '\0';
        }

        bool consume(char c)
        {
            if (peek() == c) { pos++; return true; }
            return false;
        }

        bool consumeLiteral(const string& lit)
        {
            if (text.compare(pos, lit.size(), lit) == 0)
            {
                pos += lit.size();
                return true;
            }
            return false;
        }

        JsonValue parseValue()
        {
            skipWs();
            char c = peek();
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') return parseNull();
            if (c == '-' || isdigit((unsigned char)c)) return parseNumber();
            failed = true;
            return JsonValue{};
        }

        JsonValue parseObject()
        {
            JsonValue v;
            v.type = JsonType::Object;
            pos++; // '{'
            skipWs();
            if (consume('}')) return v;
            while (true)
            {
                skipWs();
                if (peek() != '"') { failed = true; return v; }
                JsonValue key = parseString();
                skipWs();
                if (!consume(':')) { failed = true; return v; }
                JsonValue val = parseValue();
                if (failed) return v;
                v.objectValue.push_back({ key.stringValue, val });
                skipWs();
                if (consume(',')) continue;
                if (consume('}')) break;
                failed = true;
                return v;
            }
            return v;
        }

        JsonValue parseArray()
        {
            JsonValue v;
            v.type = JsonType::Array;
            pos++; // '['
            skipWs();
            if (consume(']')) return v;
            while (true)
            {
                JsonValue val = parseValue();
                if (failed) return v;
                v.arrayValue.push_back(val);
                skipWs();
                if (consume(',')) continue;
                if (consume(']')) break;
                failed = true;
                return v;
            }
            return v;
        }

        JsonValue parseString()
        {
            JsonValue v;
            v.type = JsonType::String;
            pos++; // opening quote
            string out;
            while (pos < text.size() && text[pos] != '"')
            {
                char c = text[pos];
                if (c == '\\' && pos + 1 < text.size())
                {
                    char e = text[pos + 1];
                    switch (e)
                    {
                        case '"':  out += '"';  pos += 2; break;
                        case '\\': out += '\\'; pos += 2; break;
                        case '/':  out += '/';  pos += 2; break;
                        case 'n':  out += '\n'; pos += 2; break;
                        case 't':  out += '\t'; pos += 2; break;
                        case 'r':  out += '\r'; pos += 2; break;
                        case 'b':  out += '\b'; pos += 2; break;
                        case 'f':  out += '\f'; pos += 2; break;
                        case 'u':
                        {
                            if (pos + 5 < text.size())
                            {
                                string hex = text.substr(pos + 2, 4);
                                unsigned int code = (unsigned int)strtoul(hex.c_str(), nullptr, 16);
                                // Minimal UTF-8 encoding (BMP only, no surrogate pairs -
                                // sufficient for the LLM text we expect to receive).
                                if (code < 0x80)
                                {
                                    out += (char)code;
                                }
                                else if (code < 0x800)
                                {
                                    out += (char)(0xC0 | (code >> 6));
                                    out += (char)(0x80 | (code & 0x3F));
                                }
                                else
                                {
                                    out += (char)(0xE0 | (code >> 12));
                                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                                    out += (char)(0x80 | (code & 0x3F));
                                }
                                pos += 6;
                            }
                            else
                            {
                                pos += 2;
                            }
                            break;
                        }
                        default:
                            out += e;
                            pos += 2;
                    }
                }
                else
                {
                    out += c;
                    pos++;
                }
            }
            if (pos >= text.size()) { failed = true; return v; }
            pos++; // closing quote
            v.stringValue = out;
            return v;
        }

        JsonValue parseBool()
        {
            JsonValue v;
            v.type = JsonType::Bool;
            if (consumeLiteral("true")) v.boolValue = true;
            else if (consumeLiteral("false")) v.boolValue = false;
            else failed = true;
            return v;
        }

        JsonValue parseNull()
        {
            JsonValue v;
            v.type = JsonType::Null;
            if (!consumeLiteral("null")) failed = true;
            return v;
        }

        JsonValue parseNumber()
        {
            JsonValue v;
            v.type = JsonType::Number;
            size_t start = pos;
            if (peek() == '-') pos++;
            while (pos < text.size() && isdigit((unsigned char)text[pos])) pos++;
            if (peek() == '.')
            {
                pos++;
                while (pos < text.size() && isdigit((unsigned char)text[pos])) pos++;
            }
            if (peek() == 'e' || peek() == 'E')
            {
                pos++;
                if (peek() == '+' || peek() == '-') pos++;
                while (pos < text.size() && isdigit((unsigned char)text[pos])) pos++;
            }
            string numStr = text.substr(start, pos - start);
            if (numStr.empty() || numStr == "-") { failed = true; return v; }
            v.numberValue = strtod(numStr.c_str(), nullptr);
            return v;
        }
    };
}

const JsonValue& JsonValue::get(const std::string& key) const
{
    static const JsonValue nullValue{};
    if (type != JsonType::Object) return nullValue;
    for (const auto& kv : objectValue)
        if (kv.first == key) return kv.second;
    return nullValue;
}

std::string JsonValue::asString(const std::string& def) const
{
    return type == JsonType::String ? stringValue : def;
}

int JsonValue::asInt(int def) const
{
    return type == JsonType::Number ? (int)numberValue : def;
}

JsonValue parseJson(const std::string& text, bool& ok)
{
    Parser p(text);
    JsonValue v = p.parseValue();
    p.skipWs();
    ok = !p.failed;
    return v;
}

namespace
{
    std::string trimWs(const std::string& s)
    {
        size_t start = 0;
        while (start < s.size() && isspace((unsigned char)s[start])) start++;
        size_t end = s.size();
        while (end > start && isspace((unsigned char)s[end - 1])) end--;
        return s.substr(start, end - start);
    }
}

std::string stripMarkdownFence(const std::string& s)
{
    std::string t = trimWs(s);

    if (t.rfind("```", 0) == 0)
    {
        size_t nl = t.find('\n');
        t = (nl != std::string::npos) ? t.substr(nl + 1) : t.substr(3);
    }
    if (t.size() >= 3 && t.compare(t.size() - 3, 3, "```") == 0)
        t = t.substr(0, t.size() - 3);

    return trimWs(t);
}

std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else
                {
                    out += (char)c;
                }
        }
    }
    return out;
}
