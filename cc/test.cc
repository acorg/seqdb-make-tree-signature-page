#include <iostream>
#include <string>

#include "json.hh"
#include "acmacs-base/read-file.hh"
#include "seqdb.hh"

// ----------------------------------------------------------------------

int main(int /*argc*/, const char */*argv*/[])
{
    int exit_code = 0;
    try {
        std::string buffer = acmacs_base::read_file("/Users/eu/WHO/seqdb.json.xz");
        Seqdb seqdb;
        seqdb.from_json(buffer);
          // std::cout << seqdb.json(1) << std::endl;

          // auto j = json::parse(buffer);
          // std::cout << j["  version"] << std::endl;
    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
