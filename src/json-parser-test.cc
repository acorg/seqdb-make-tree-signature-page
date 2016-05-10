#include <iostream>

#include "json-read.hh"

// ----------------------------------------------------------------------

class C2
{
 public:
    std::string s2;
    int i2;
    double f2;

    class C2_parse AXE_RULE
    {
      public:
        inline C2_parse(C2& aC2) : mC2(aC2) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator it1, Iterator it2) const
        {
            auto r_s2 = jsonr::object_value("s2", mC2.s2);
            auto r_i2 = jsonr::object_value("i2", mC2.i2);
            auto r_f2 = jsonr::object_value("f2", mC2.f2);
            return jsonr::object(r_s2 | r_i2 | r_f2)(it1, it2);
        }

      private:
        C2& mC2;
    };

    inline auto json_parser() { return C2_parse(*this); }

};

inline std::ostream& operator << (std::ostream& out, const C2& aC2)
{
    return out << "C2(s2=\"" << aC2.s2 << "\", i2=" << aC2.i2 << ", f2=" << aC2.f2 << ")";
}


class C1
{
 public:
    std::string s1;
    std::vector<C2> a1;

    class C1_parse AXE_RULE
    {
      public:
        inline C1_parse(C1& aC1) : mC1(aC1) {}

        template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
        {
            auto r_s1 = jsonr::object_string_value("s1", mC1.s1);
            auto r_a1 = jsonr::skey("a1") > jsonr::array2(mC1.a1);
            return jsonr::object(r_s1 | r_a1)(i1, i2);
        }

      private:
        C1& mC1;
    };

    inline auto json_parser() { return C1_parse(*this); }

    inline void parse(std::string source)
        {
            try {
                json_parser()(source.begin(), source.end());
            }
            catch (axe::failure<char>& err) {
                throw jsonr::JsonParsingError(err.message());
            }
        }
};

inline std::ostream& operator << (std::ostream& out, const C1& aC1)
{
    out << "C1(s1=\"" << aC1.s1 << "\", a1=[";
    std::copy(aC1.a1.begin(), aC1.a1.end(), std::ostream_iterator<C2>(out, ", "));
    out << "])";
    return out;
}

// ----------------------------------------------------------------------

int main()
{
    std::string source = "{\"s1\": \"11\", \"a1\": [{\"s2\": \"222\", \"i2\": 22, \"f2\": 22.22}, {\"s2\": \"444\", \"i2\": 44, \"f2\": 44.44}]}";
    std::cout << source << std::endl;
    C1 c1;
    try {
        c1.parse(source);
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
    }
    std::cout << c1 << std::endl;
    return 0;
}

// ----------------------------------------------------------------------
