#pragma once

#include "json-read.hh"
#include "draw.hh"

// ----------------------------------------------------------------------

class Point
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(Point& aPoint) : mPoint(aPoint) {}
            axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const;
          private:
            Point& mPoint;
        };

 public:
    inline Point () : antigen(true), egg(false), reassortant(false), reference(false), vaccine(false), vaccine_fill_color(0xFFC0CB), vaccine_outline_color(0), vaccine_aspect(0.5) {}

    std::string name;
    std::vector<double> coordinates;
    std::string lab_id;
    bool antigen;
    bool egg;
    bool reassortant;
    bool reference;
    bool vaccine;
    Color vaccine_fill_color;
    Color vaccine_outline_color;
    double vaccine_aspect;

    inline auto json_parser() { return json_parser_t(*this); }

}; // class Point

// ----------------------------------------------------------------------

class ChartInfo
{
 public:
    inline ChartInfo() {}

    std::string date;
    std::string lab;
    std::string virus_type;
    std::string lineage;
    std::string name;
    std::string rbc_species;

}; // class ChartInfo

// ----------------------------------------------------------------------

class Chart
{
 public:
    inline Chart() : mStress(-1) {}

    static Chart from_json(std::string data);

 private:
    double mStress;
    ChartInfo mInfo;
    std::vector<Point> mPoints;
    std::string mMinimumColumnBasis;
    std::vector<double> mColumnBases;

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
