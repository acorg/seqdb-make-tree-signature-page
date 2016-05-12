#pragma once

#include "json-read.hh"

// ----------------------------------------------------------------------

class Chart
{
 public:
    inline Chart() : mStress(-1) {}

    static Chart from_json(std::string data);

 private:
    double mStress;
      // mInfo;
      // mPoints;
    std::string mMinimumColumnBasis;
      // mColumnBases;

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
