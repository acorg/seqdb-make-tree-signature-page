#pragma once

// ----------------------------------------------------------------------
// std::vector<std::string> split(const std::string& s, const std::string& delim, bool keep_empty = Split::KeepEmpty)
// Split string by delimiter into array of substrings
// std::vector<std::string> split(const std::string& s, const std::string& delim, bool keep_empty = Split::KeepEmpty)

// std::string strip(std::string source)
// Removes leading and trailing spaces from string
//
// ----------------------------------------------------------------------

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>

// ----------------------------------------------------------------------

namespace string
{

// ----------------------------------------------------------------------

    enum class Split { RemoveEmpty, KeepEmpty };

      // http://stackoverflow.com/questions/236129/split-a-string-in-c
    inline std::vector<std::string> split(std::string s, std::string delim, Split keep_empty = Split::KeepEmpty)
    {
        std::vector<std::string> result;
        if (! delim.empty()) {
            for (std::string::iterator substart = s.begin(), subend = substart; substart <= s.end(); substart = subend + static_cast<std::string::difference_type>(delim.size())) {
                subend = std::search(substart, s.end(), delim.begin(), delim.end());
                if (substart != subend || keep_empty == Split::KeepEmpty) {
                    result.push_back(std::string(substart, subend));
                }
            }
        }
        else {
            result.push_back(s);
        }
        return result;
    }

      // ----------------------------------------------------------------------

    namespace _internal {
        template <typename InputIterator, typename Source> inline std::pair<InputIterator, InputIterator> strip_begin_end(Source& source)
        {
            auto predicate = [](auto c) { return std::isspace(c); }; // have to use lambda, other compiler cannot infer Predicate type from isspace
            auto e = std::find_if_not(source.rbegin(), source.rend(), predicate);
            auto b = std::find_if_not(source.begin(), e.base(), predicate);
            return std::make_pair(b, e.base());
        }
    } // namespace _internal

    inline std::string& strip(std::string& source)
    {
        auto be = _internal::strip_begin_end<std::string::iterator>(source);
        source.erase(be.second, source.end()); // erase at the end first
        source.erase(source.begin(), be.first); // invalidates be.second!
        return source;
    }

    inline std::string strip(std::string&& source)
    {
        auto be = _internal::strip_begin_end<std::string::iterator>(source);
        source.erase(be.second, source.end()); // erase at the end first
        source.erase(source.begin(), be.first); // invalidates be.second!
        return source;
    }

    inline std::string strip(const std::string& source)
    {
        auto be = _internal::strip_begin_end<std::string::const_iterator>(source);
        return std::string(be.first, be.second);
    }

      // ----------------------------------------------------------------------

    inline std::string& lower(std::string& source)
    {
        std::transform(source.begin(), source.end(), source.begin(), ::tolower);
        return source;
    }

    inline std::string lower(const std::string& source)
    {
        std::string result;
        std::transform(source.begin(), source.end(), std::back_inserter(result), ::tolower);
        return result;
    }

    inline std::string& upper(std::string& source)
    {
        std::transform(source.begin(), source.end(), source.begin(), ::toupper);
        return source;
    }

    inline std::string upper(const std::string& source)
    {
        std::string result;
        std::transform(source.begin(), source.end(), std::back_inserter(result), ::toupper);
        return result;
    }

    inline std::string& capitalize(std::string& source)
    {
        if (!source.empty()) {
            std::transform(source.begin(), source.begin() + 1, source.begin(), ::toupper);
            std::transform(source.begin() + 1, source.end(), source.begin() + 1, ::tolower);
        }
        return source;
    }

    inline std::string capitalize(const std::string& source)
    {
        std::string result;
        if (!source.empty()) {
            std::transform(source.begin(), source.begin() + 1, std::back_inserter(result), ::toupper);
            std::transform(source.begin() + 1, source.end(), std::back_inserter(result), ::tolower);
        }
        return result;
    }

// ----------------------------------------------------------------------

} // namespace string
