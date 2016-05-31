#pragma once

#include <vector>
#include <map>
#include <iostream>

#include "axe.h"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
// #pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

namespace jsonr
{
    class JsonParsingError : public std::runtime_error
    {
     public:
        using std::runtime_error::runtime_error;
    };

      // ----------------------------------------------------------------------

    class failure : public std::exception
    {
        std::string msg;
        std::string::iterator text_start;
        std::string text;

     public:
        virtual ~failure() noexcept {}
        template<class T> failure(T&& aMsg, std::string::iterator i1, std::string::iterator i2) : msg(std::forward<T>(aMsg)), text_start(i1), text(i1, std::min(i1 + 40, i2)) {}
        failure(const failure& aNother) = default;
        virtual const char* what() const noexcept { return msg.c_str(); }
        std::string message(std::string::iterator buffer_start) const { return msg + "\nat offset " + std::to_string(text_start - buffer_start) + " when parsing " + text; }
    };

    template<class R1, class R2> class r_atomic_t AXE_RULE
    {
        R1 r1_;
        R2 r2_;
      public:
        r_atomic_t(R1&& r1, R2&& r2) : r1_(std::forward<R1>(r1)), r2_(std::forward<R2>(r2)) {}

        inline axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const
        {
            auto match = r1_(i1, i2);
            if (match.matched) {   // if r1_ matched r2_ must match too
                match = r2_(match.position, i2);
                if (!match.matched)
                    throw failure(std::string("R1 >= R2 rule failed with\n   R1: ") + axe::get_name(r1_) + "\n   R2: " + axe::get_name(r2_), match.position, i2);
                return axe::make_result(true, match.position);
            }
            return match;
        }
    };

    template<class R1, class R2> inline r_atomic_t<typename std::enable_if<AXE_IS_RULE(R1), R1>::type, typename std::enable_if<AXE_IS_RULE(R2), R2>::type> operator >= (R1&& r1, R2&& r2)
    {
        return r_atomic_t<R1, R2>(std::forward<R1>(r1), std::forward<R2>(r2));
    }

      // ----------------------------------------------------------------------

    const auto space = axe::r_any(" \t\n\r");
    const auto object_begin = axe::r_named(*space & axe::r_lit('{') & *space, "object_begin");
    const auto object_end = *space & axe::r_lit('}') & *space;
    const auto array_begin = axe::r_named(*space & axe::r_lit('[') & *space, "array_begin");
    const auto array_end = *space & axe::r_lit(']') & *space;
    const auto comma = axe::r_named(*space & axe::r_lit(',') & *space, "comma");
    const auto doublequotes = axe::r_named(axe::r_lit('"'), "doublequotes");
    const auto colon = *space & axe::r_lit(':') & *space;
    const auto string_content = axe::r_named(*("\\\"" | (axe::r_any() - axe::r_any("\"\n\r"))), "string_content");

      // ----------------------------------------------------------------------

    inline auto ckey(char key) {
        return axe::r_named((doublequotes & key) >= doublequotes >= colon, "ckey");
    };

    inline auto skey(const char* key) {
        return axe::r_named((doublequotes & key) >= doublequotes >= colon, "skey");
    };

    inline auto r_bool(bool& target)
    {
        auto yes = axe::e_ref([&](auto, auto) { target = true; });
        auto no = axe::e_ref([&](auto, auto) { target = false; });
        return (axe::r_str("true") >> yes) | (axe::r_str("1") >> yes) | (axe::r_str("false") >> no) | (axe::r_str("0") >> no) | axe::r_fail("true or 1 or false or 0 expected");
    };

    inline auto r_null(std::string& target)
    {
        return axe::r_lit("null") >> axe::e_ref([&](auto, auto) { target.clear(); });
    };

    inline auto r_string(std::string& target)
    {
        return (doublequotes >= axe::r_named(string_content >> target, "string_content to target") >= doublequotes) | r_null(target);
    };

    template <typename T> inline auto r_assign_string_to(T& target)
    {
        return (doublequotes >= (string_content >> axe::e_ref([&](auto b, auto e) { target = std::string(b, e); })) >= doublequotes)
                | (axe::r_lit("null") >> axe::e_ref([&](auto, auto) { target = std::string(); }));
    };

    inline auto r_double(double& target)
    {
        return axe::r_double() >> axe::e_ref([&target](auto b, auto e) {target = std::stod(std::string(b, e)); });
    };

    inline auto r_double(std::vector<double>& target)
    {
        return axe::r_double() >> axe::e_ref([&target](auto b, auto e) {target.push_back(std::stod(std::string(b, e))); });
    };

      // ----------------------------------------------------------------------

    template <typename Item> class object_t AXE_RULE
    {
      public:
        inline object_t(Item&& aItem) : mItem(aItem) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            return axe::r_named(object_begin >= ~( mItem & *(comma >= mItem) ) >= object_end, "jsonr::object")(i1, i2);
        }

      private:
        Item mItem;
    };

    template <typename Item> inline auto object(Item&& aItem) { return object_t<Item>(std::forward<Item>(aItem)); }

      // ----------------------------------------------------------------------

    template <typename Item> class array_item_t AXE_RULE
    {
      public:
        inline array_item_t(Item&& aItem) : mItem(aItem) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            return axe::r_named(array_begin >= ~(mItem & *(comma >= mItem) ) >= array_end, "jsonr::array")(i1, i2);
        }

      private:
        Item mItem;
    };

    template <typename Item> inline auto array_item(Item&& aItem) { return array_item_t<Item>(std::forward<Item>(aItem)); }

      // ----------------------------------------------------------------------

    template <typename ArrayItem> class array_t AXE_RULE
    {
      public:
        inline array_t(std::vector<ArrayItem>& aTarget) : mTarget(aTarget) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto new_item = axe::e_ref([this](auto, auto) { mTarget.emplace_back(); });
            auto parse_item = axe::r_named(axe::r_ref([this](auto it1, auto it2) { return mTarget.back().json_parser()(it1, it2); }), "jsonr::array::parse_item");
            auto item = axe::r_named(axe::r_empty() >> new_item, "jsonr::array::new_item") & parse_item;
            return axe::r_named(array_begin >= ~(item & *(comma >= item) ) >= array_end, "jsonr::array")(i1, i2);
        }

      private:
        std::vector<ArrayItem>& mTarget;
    };

    template <> template<class Iterator> inline axe::result<Iterator> array_t<std::string>::operator()(Iterator i1, Iterator i2) const
    {
        auto extractor = [this](auto it1, auto it2) { mTarget.emplace_back(it1, it2); };
        auto item = doublequotes >= (string_content >> axe::e_ref(extractor)) >= doublequotes;
        return axe::r_named(array_begin >= ~(item & *(comma >= item) ) >= array_end, "jsonr::array<std::string>")(i1, i2);
    }

    template <> template<class Iterator> inline axe::result<Iterator> array_t<double>::operator()(Iterator i1, Iterator i2) const
    {
        auto item = r_double(mTarget);
        return axe::r_named(array_begin >= ~(item & *(comma >= item) ) >= array_end, "jsonr::array<double>")(i1, i2);
    }

    template <typename ArrayItem> inline auto array(std::vector<ArrayItem>& aTarget) { return array_t<ArrayItem>(aTarget); }

      // ----------------------------------------------------------------------

    template <typename Value> inline auto value(Value& aTarget) -> decltype(std::declval<Value>().json_parser(), aTarget.json_parser()) { return aTarget.json_parser(); }
    template <typename Value> inline auto value(std::vector<Value>& aTarget) { return array(aTarget); }
      //inline auto value(std::vector<std::string>& aTarget) { return array(aTarget); }
    inline auto value(int& aTarget) { return axe::r_decimal(aTarget); }
    inline auto value(size_t& aTarget) { return axe::r_udecimal(aTarget); }
    inline auto value(double& aTarget) { return r_double(aTarget); }
    inline auto value(std::string& aTarget) { return r_string(aTarget); }
    inline auto value(bool& aTarget) { return r_bool(aTarget); }

      // ----------------------------------------------------------------------

    template <typename Value> class map_t AXE_RULE
    {
      public:
        inline map_t(std::map<std::string, Value>& aTarget) : mTarget(aTarget) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto update = axe::e_ref([this](auto, auto) { mTarget[mKey] = mValue; });
            auto field = (r_string(mKey) >= colon >= value(mValue)) >> update;
            return object(field)(i1, i2);
        }

      private:
        std::map<std::string, Value>& mTarget;
        mutable std::string mKey;
        mutable Value mValue;
    };

    template <typename Value> inline auto map(std::map<std::string, Value>& aTarget) { return map_t<Value>(aTarget); }
    template <typename Value> inline auto value(std::map<std::string, Value>& aTarget) { return map(aTarget); }

      // ----------------------------------------------------------------------

    template <typename Value> inline Value validator_id(Value value) { return value; }

      // ----------------------------------------------------------------------

    template <typename Value> class object_array_value_t AXE_RULE
    {
      public:
        inline object_array_value_t(const char* aKey, std::vector<Value>& aTarget) : mKey(aKey), mTarget(aTarget) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            return (skey(mKey) >= array(mTarget))(i1, i2);
        }

      private:
        const char* mKey;
        std::vector<Value>& mTarget;
    };

    template <typename Value> inline auto object_array_value(const char* aKey, std::vector<Value>& aTarget) { return object_array_value_t<Value>(aKey, aTarget); }

      // ----------------------------------------------------------------------

    template <typename Value> class object_string_value_t AXE_RULE
    {
      public:
        inline object_string_value_t(const char* aKey, Value& aTarget) : mKey(aKey), mTarget(aTarget) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            return (skey(mKey) >= r_assign_string_to(mTarget))(i1, i2);
        }

      private:
        const char* mKey;
        Value& mTarget;
    };

    template <typename Value> inline auto object_string_value(const char* aKey, Value& aTarget) { return object_string_value_t<Value>(aKey, aTarget); }

      // ----------------------------------------------------------------------

    template <typename Value, typename Reader> class object_enum_value_t AXE_RULE
    {
      public:
        inline object_enum_value_t(const char* aKey, Value& aTarget, Reader aReader) : mKey(aKey), mTarget(aTarget), mReader(aReader) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto read_value = [this](auto b, auto e) {
                mTarget = mReader(std::string(b, e));
            };
            return axe::r_named(skey(mKey) >= doublequotes >= (string_content >> axe::e_ref(read_value)) >= doublequotes, "jsonr::object_enum_value")(i1, i2);
        }

      private:
        const char* mKey;
        Value& mTarget;
        Reader mReader;
    };

    template <typename Value, typename Reader> inline auto object_enum_value(const char* aKey, Value& aTarget, Reader aReader) { return object_enum_value_t<Value, Reader>(aKey, aTarget, aReader); }

      // ----------------------------------------------------------------------

    class object_string_ignore_value AXE_RULE
    {
      public:
        inline object_string_ignore_value(const char* aKey) : mKey(aKey) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            return axe::r_named(skey(mKey) >= doublequotes >= string_content >= doublequotes, "jsonr::object_string_ignore_value")(i1, i2);
        }

      private:
        const char* mKey;
    };

      // ----------------------------------------------------------------------

    template <typename Int> Int stoi(std::string::iterator b, std::string::iterator e, int base);
    template <> inline int stoi<int>(std::string::iterator b, std::string::iterator e, int base)
    {
        try {
            return std::stoi(std::string(b, e), nullptr, base);
        }
        catch (std::exception& err) {
            throw std::invalid_argument("cannot read int from " + std::string(b, e) + ": " + err.what());
        }
    }
    template <> inline size_t stoi<size_t>(std::string::iterator b, std::string::iterator e, int base)
    {
        try {
            return static_cast<size_t>(std::stoul(std::string(b, e), nullptr, base));
        }
        catch (std::exception& err) {
            throw std::invalid_argument("cannot read size_t from " + std::string(b, e) + ": " + err.what());
        }
    }

    template <typename Int, typename Validator> class object_int_value_t AXE_RULE
    {
      public:
        inline object_int_value_t(const char* aKey, Int& aTarget, int aBase, Validator aValidator) : mKey(aKey), mTarget(aTarget), mBase(aBase), mValidator(aValidator) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto read_value = [this](auto b, auto e) {
                try {
                    mTarget = mValidator(stoi<Int>(b, e, mBase));
                }
                catch (std::exception& err) {
                    throw std::invalid_argument("cannot read Int from " + std::string(b, e) + ": " + err.what());
                }
            };
            return axe::r_named(skey(mKey) >= (axe::r_decimal() >> axe::e_ref(read_value)), "jsonr::object_int_value")(i1, i2);
        }

      private:
        const char* mKey;
        Int& mTarget;
        int mBase;
        Validator mValidator;
    };

    inline int validator_int_non_negative(int value) { if (value < 0) throw std::domain_error("invalid value (non-negative int expected)"); return value; }
    inline int validator_int_positive(int value) { if (value <= 0) throw std::domain_error("invalid value (positive int expected)"); return value; }
    inline size_t validator_size_t_positive(size_t value) { if (value == 0) throw std::domain_error("invalid value (positive size_t expected)"); return value; }

    template <typename Int=int, typename Validator = decltype(&validator_id<Int>)> inline auto object_int_value(const char* aKey, Int& aTarget, int aBase = 10, Validator aValidator = &validator_id<Int>) { return object_int_value_t<Int, Validator>(aKey, aTarget, aBase, aValidator); }

      // ----------------------------------------------------------------------

    template <typename Validator> class object_double_value_t AXE_RULE
    {
      public:
        inline object_double_value_t(const char* aKey, double& aTarget, Validator aValidator) : mKey(aKey), mTarget(aTarget), mValidator(aValidator) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto read_value = [this](auto b, auto e) {
                mTarget = mValidator(std::stod(std::string(b, e)));
            };
            return axe::r_named(skey(mKey) >= (axe::r_double() >> axe::e_ref(read_value)), "jsonr::object_double_value")(i1, i2);
        }

      private:
        const char* mKey;
        double& mTarget;
        Validator mValidator;
    };

    inline double validator_double_non_negative(double value) { if (value < 0) throw std::domain_error("invalid value (non-negative double expected)"); return value; }

    template <typename Validator = decltype(&validator_id<double>)> inline auto object_double_value(const char* aKey, double& aTarget, Validator aValidator = &validator_id<double>) { return object_double_value_t<Validator>(aKey, aTarget, aValidator); }
    inline auto object_double_non_negative_value(const char* aKey, double& aTarget) { return object_double_value_t<decltype(&validator_double_non_negative)>(aKey, aTarget, &validator_double_non_negative); }

      // ----------------------------------------------------------------------

    template <typename Value> class object_value_t AXE_RULE
    {
      public:
        inline object_value_t(const char* aKey, Value& aTarget) : mKey(aKey), mTarget(aTarget) {}

        template<class Iterator> inline auto operator()(Iterator i1, Iterator i2) const
        {
            return (skey(mKey) >= value(mTarget))(i1, i2);
        }

      private:
        const char* mKey;
        Value& mTarget;
    }; // class object_value_t<> AXE_RULE

    template <typename Value> inline auto object_value(const char* aKey, Value& aTarget) { return object_value_t<Value>(aKey, aTarget); }

    template <> inline auto object_value(const char* aKey, std::string& aTarget) { return object_string_value(aKey, aTarget); }
    template <> inline auto object_value(const char* aKey, std::vector<std::string>& aTarget) { return object_array_value(aKey, aTarget); }
    template <> inline auto object_value(const char* aKey, int& aTarget) { return object_int_value(aKey, aTarget); }
    template <> inline auto object_value(const char* aKey, double& aTarget) { return object_double_value(aKey, aTarget); }

      // ----------------------------------------------------------------------

    inline auto message(std::string msg) {
        return axe::e_ref([msg](auto, auto) { std::cerr << "MSG: " << msg << std::endl; });
    }

    template <typename Rule> inline auto message_done(Rule rule, std::string msg) {
        return rule >> message(msg);
    }

      // ----------------------------------------------------------------------
      // ----------------------------------------------------------------------

    class version AXE_RULE
    {
     public:
        inline version(std::string expected_version) : mExpected(expected_version) {}

        template<class Iterator> inline auto operator()(Iterator i1, Iterator i2) const
        {
            auto validator = [this](auto b, auto e) {
                const std::string version_extracted(b , e);
                if (version_extracted != mExpected) {
                    throw JsonParsingError(std::string("Unsupported version: \"") + version_extracted + "\", expected: \"" + mExpected + "\"");
                }
            };
            return (skey("  version") >= doublequotes >= (string_content >> axe::e_ref(validator)) >= doublequotes)(i1, i2);
        }

     private:
        std::string mExpected;

    }; // class version AXE_RULE

} // namespace jsonr

// ----------------------------------------------------------------------

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
