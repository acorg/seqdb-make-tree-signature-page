#pragma once

#include <iostream>
#include <stdexcept>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wunused-exception-parameter"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
#include "json.hpp"
#pragma GCC diagnostic pop
using json = nlohmann::json;

// ----------------------------------------------------------------------

template <typename T, typename K> inline void from_json(const json& j, K key, T& target)
{
    try {
        target = j.at(key);
    }
    catch (std::out_of_range&) {
    }
}

// ----------------------------------------------------------------------

template <typename T, typename K> inline void from_json(const json& j, K key, T& target, const T& aDefault)
{
    try {
        target = j.at(key);
    }
    catch (std::out_of_range&) {
        target = aDefault;
    }
}

// ----------------------------------------------------------------------

template <typename T, typename K> inline void from_json_if_non_negative(const json& j, K key, T& target)
{
    try {
        const T t = j.at(key);
        if (t >= 0)
            target = t;
    }
    catch (std::out_of_range&) {
    }
}

// ----------------------------------------------------------------------

template <typename T, typename K> inline void from_json_if_not_empty(const json& j, K key, T& target)
{
    try {
        std::string t = j.at(key);
        if (!t.empty()) {
            target = t;
        }
    }
    catch (std::out_of_range&) {
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: cannot convert " << j.at(key) << " to " << typeid(T).name() << ": " << err.what() << std::endl;
    }
}

// ----------------------------------------------------------------------
