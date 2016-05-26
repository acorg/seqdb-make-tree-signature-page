#pragma once

#include <iostream>
#include <map>
#include <set>

// ----------------------------------------------------------------------

template <typename Key, typename Value> inline std::ostream& operator << (std::ostream& out, const std::map<Key, Value>& aCollection)
{
    out << '{';
    bool sep = false;
    for (const auto& e: aCollection) {
        if (sep)
            out << ", ";
        else
            sep = true;
        out << '<' << e.first << ">: <" << e.second << '>';
    }
    return out << '}';
}

// ----------------------------------------------------------------------

namespace stream_internal
{
    template <typename Collection> inline std::ostream& write_to_stream(std::ostream& out, const Collection& aCollection, std::string prefix, std::string suffix, std::string separator)
    {
        out << prefix;
        bool sep = false;
        for (const auto& item: aCollection) {
            if (sep)
                out << separator;
            else
                sep = true;
            out << item;
        }
        return out << suffix;
    }
}

// ----------------------------------------------------------------------

template <typename Value> inline std::ostream& operator << (std::ostream& out, const std::vector<Value>& aCollection)
{
    return stream_internal::write_to_stream(out, aCollection, "[", "]", ", ");
}

// ----------------------------------------------------------------------

template <typename Value> inline std::ostream& operator << (std::ostream& out, const std::set<Value>& aCollection)
{
    return stream_internal::write_to_stream(out, aCollection, "{", "}", ", ");
}

// ----------------------------------------------------------------------

template <typename Collection> class to_stream_t
{
 public:
    typedef typename Collection::value_type value_type;
    typedef typename Collection::const_iterator const_iterator;
    typedef std::function<std::string(value_type)> transformer_t;
    inline to_stream_t(const_iterator first, const_iterator last, transformer_t transformer) : mFirst(first), mLast(last), mTransformer(transformer) {}
    inline friend std::ostream& operator << (std::ostream& out, const to_stream_t<Collection>& converter)
        {
            if ((converter.mLast - converter.mFirst) > 1)
                std::transform(converter.mFirst, converter.mLast, std::ostream_iterator<std::string>(out, " "), converter.mTransformer);
            else
                out << converter.mTransformer(*converter.mFirst);
            return out;
        }

 private:
    const_iterator mFirst, mLast;
    transformer_t mTransformer;

}; // class to_stream

template <typename Collection> inline auto to_stream(const Collection& c, typename to_stream_t<Collection>::transformer_t transformer)
{
    return to_stream_t<Collection>(c.begin(), c.end(), transformer);
}

// ----------------------------------------------------------------------
