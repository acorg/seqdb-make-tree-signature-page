#pragma once

#include <sstream>
#include "string.hh"

// ----------------------------------------------------------------------

class Messages
{
 public:
    inline Messages() = default;

    inline std::ostream& warning() { return mWarnings; }

    inline operator std::string() { return string::strip(mWarnings.str()); }

 private:
    std::stringstream mWarnings;

}; // class Messages

// ----------------------------------------------------------------------
