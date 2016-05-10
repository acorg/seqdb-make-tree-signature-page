#pragma once

#include <iostream>
#include <map>

// ----------------------------------------------------------------------

template <typename Key, typename Value> inline std::ostream& operator << (std::ostream& out, const std::map<Key, Value>& aCollection)
{
    out << '{';
    for (const auto& e: aCollection) {
        out << '<' << e.first << ">: <" << e.second << ">, ";
    }
    return out << '}';
}

// ----------------------------------------------------------------------

template <typename Value> inline std::ostream& operator << (std::ostream& out, const std::vector<Value>& aCollection)
{
    out << '[';
    std::copy(aCollection.begin(), aCollection.end(), std::ostream_iterator<Value>(out, ", "));
    return out << ']';
}

// ----------------------------------------------------------------------
