#pragma once

#include <vector>
#include <map>
#include <string>
#include <limits>
#include <cstdio>

// ----------------------------------------------------------------------

namespace jsonw
{
    enum IfPrependComma { PrependComma, NoComma, NoCommaNoIndent };

    inline void _comma(std::string& target, IfPrependComma comma, size_t indent, size_t prefix)
    {
        switch (comma) {
          case PrependComma:
              target.append(",");
              break;
          case NoComma:
              break;
          case NoCommaNoIndent:
              indent = 0;
              break;
        }
        if (indent) {
            target.append(1, '\n');
            target.append(prefix, ' ');
        }
    }

      // ----------------------------------------------------------------------

      // Functions return IfPrependComma value for the next json and json_if call

    template <typename V> inline auto json(std::string& target, IfPrependComma comma, V value, size_t indent, size_t prefix) -> decltype(std::declval<V>().json(target, comma, indent, prefix), PrependComma) // decltype(&V::json, PrependComma)
    {
        return value.json(target, comma, indent, prefix);
    }

    inline IfPrependComma json(std::string& target, IfPrependComma comma, std::string value, size_t indent, size_t prefix)
    {
        _comma(target, comma, indent, prefix);
        if (comma == PrependComma && indent == 0)
            target.append(1, ' ');
        target.append(1, '"');
        target.append(value);
        target.append(1, '"');
        return PrependComma;
    }

    inline IfPrependComma json(std::string& target, IfPrependComma comma, char value, size_t indent, size_t prefix)
    {
        return json(target, comma, std::string(1, value), indent, prefix);
    }

    template <typename V> inline auto json(std::string& target, IfPrependComma comma, V value, size_t indent, size_t prefix) -> decltype(std::to_string(value), PrependComma)
    {
        _comma(target, comma, indent, prefix);
        target.append(std::to_string(value));
        return PrependComma;
    }

    template <typename V> inline auto json(std::string& target, IfPrependComma comma, V value, size_t indent, size_t prefix) -> decltype(std::declval<V>().to_string(), PrependComma)
    {
        return json(target, comma, value.to_string(), indent, prefix);
    }

    template <> inline IfPrependComma json(std::string& target, IfPrependComma comma, bool value, size_t indent, size_t prefix)
    {
        _comma(target, comma, indent, prefix);
        target.append(value ? "true" : "false");
        return PrependComma;
    }

    // template <typename V> inline std::string json(V value)
    // {
    //     std::string target;
    //     json(target, NoCommaNoIndent, value, 0, 0);
    //     return target;
    // }

    template <typename V> inline size_t _internal_indent(size_t indent)
    {
        return indent;
    }

    template <> inline size_t _internal_indent<std::string>(size_t /*indent*/)
    {
        return 0;
    }

    template <typename K, typename V> inline IfPrependComma json(std::string& target, IfPrependComma comma, K key, V value, size_t indent, size_t prefix)
    {
        json(target, comma, key, indent, prefix);
        target.append(": ");
        json(target, NoCommaNoIndent, value, indent, prefix);
        return PrependComma;
    }

    inline IfPrependComma json_begin(std::string& target, IfPrependComma comma, char symbol, size_t indent, size_t& prefix)
    {
        _comma(target, comma, indent, prefix);
        target.append(1, symbol);
        prefix += indent;
        return NoComma;
    }

    inline IfPrependComma json_end(std::string& target, char symbol, size_t indent, size_t& prefix)
    {
        if (indent) {
            prefix -= indent;
            target.append(1, '\n');
            target.append(prefix, ' ');
        }
        target.append(1, symbol);
        return PrependComma;
    }

    template <typename V> inline IfPrependComma json(std::string& target, IfPrependComma comma, const std::vector<V>& value, size_t indent, size_t prefix)
    {
        comma = json_begin(target, comma, '[', indent, prefix);
        indent = _internal_indent<V>(indent);
        for (auto it = std::begin(value); it != std::end(value); ++it) {
            comma = json(target, comma, *it, indent, prefix);
        }
        return json_end(target, ']', indent, prefix);
    }

    template <typename KK, typename VV> inline IfPrependComma json(std::string& target, IfPrependComma comma, const std::map<KK, VV>& value, size_t indent, size_t prefix)
    {
        comma = json_begin(target, comma, '{', indent, prefix);
        for (auto it = std::begin(value); it != std::end(value); ++it) {
            comma = json(target, comma, it->first, it->second, indent, prefix);
        }
        return json_end(target, '}', indent, prefix);
    }

      // ----------------------------------------------------------------------

    template <typename K, typename V> inline IfPrependComma json_if(std::string& target, IfPrependComma comma, K key, V value, bool predicate, size_t indent, size_t prefix)
    {
        return predicate ? json(target, comma, key, value, indent, prefix) : comma;
    }

    template <typename V> inline auto default_predicate(V value) -> decltype(std::declval<V>().empty(), true)
    {
        return !value.empty();
    }

    template <typename V> inline auto default_predicate(V value) -> decltype(value * value, true) // numbers have * defined, string and vector have not
    {
        return value < std::numeric_limits<V>::max();
    }

    template <typename K, typename V> inline IfPrependComma json_if(std::string& target, IfPrependComma comma, K key, V value, size_t indent, size_t prefix)
    {
        return json_if(target, comma, key, value, default_predicate(value), indent, prefix);
    }

} // namespace jsonw

// ----------------------------------------------------------------------
