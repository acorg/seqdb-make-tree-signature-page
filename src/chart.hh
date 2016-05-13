#pragma once

#include "json-read.hh"

// ----------------------------------------------------------------------

class Point
{
 public:
    inline Point () : antigen(true), egg(false), reassortant(false), reference(false) {}

    std::string name;
    std::vector<double> coordinates;
    std::string lab_id;
    bool antigen;
    bool egg;
    bool reassortant;
    bool reference;
      // vaccine

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
      // mPoints;
    std::string mMinimumColumnBasis;
    std::vector<double> mColumnBases;

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
