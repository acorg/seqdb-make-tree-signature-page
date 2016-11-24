#pragma once

#include <sstream>
#include "string.hh"

// ----------------------------------------------------------------------

class Messages
{
 public:
    inline Messages() = default;

    inline std::ostream& warning() { return mWarnings; }

    inline operator std::string() const { return string::strip(mWarnings.str()); }

    inline void add(const Messages& aSource)
        {
            mWarnings << static_cast<std::string>(aSource);
        }

 private:
    std::stringstream mWarnings;

}; // class Messages

// ----------------------------------------------------------------------
