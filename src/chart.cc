#include <iostream>

#include "chart.hh"
#include "read-file.hh"
#include "xz.hh"

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer)
{
    Chart chart;
    if (buffer == "-")
        buffer = read_stdin();
    else
        buffer = read_file(buffer);
    if (xz_compressed(buffer))
        buffer = xz_decompress(buffer);
    else if (buffer[0] == '{')
        chart = Chart::from_json(buffer);
    else
        throw std::runtime_error("cannot import chart: unrecognized source format");
    return chart;

} // import_chart

// ----------------------------------------------------------------------

constexpr const char* SDB_VERSION = "acmacs-sdb-v1";

Chart Chart::from_json(std::string data)
{
    Chart chart;
    auto parse_chart = jsonr::object(
        jsonr::version(SDB_VERSION)
      | jsonr::object_string_ignore_value(" created")
      | jsonr::object_string_ignore_value("?points")
          // | chart.mInfo.json_parser()
      | jsonr::object_value("minimum_column_basis", chart.mMinimumColumnBasis)
          // | chart.mPoints.json_parser()
      | jsonr::object_value("stress", chart.mStress)
          // | chart.mColumnBases.json_parser()
        );
    try {
        parse_chart(std::begin(data), std::end(data));
    }
    catch (axe::failure<char>& err) {
        throw jsonr::JsonParsingError(err.message());
    }
    return chart;

} // Chart::from_json

// ----------------------------------------------------------------------
