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

class json_parser_ChartInfo AXE_RULE
{
  public:
    inline json_parser_ChartInfo(ChartInfo& aChartInfo) : mChartInfo(aChartInfo) {}

    inline axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const
    {
        return (jsonr::skey("info") > jsonr::object(
              jsonr::object_value("date", mChartInfo.date)
            | jsonr::object_value("lab", mChartInfo.lab)
            | jsonr::object_value("virus_type", mChartInfo.virus_type)
            | jsonr::object_value("lineage", mChartInfo.lineage)
            | jsonr::object_value("name", mChartInfo.name)
            | jsonr::object_value("rbc_species", mChartInfo.rbc_species)
            | jsonr::object_string_ignore_value("?")
              ))(i1, i2);
    }

  private:
    ChartInfo& mChartInfo;
};

constexpr const char* SDB_VERSION = "acmacs-sdb-v1";

Chart Chart::from_json(std::string data)
{
    Chart chart;
    auto parse_chart = jsonr::object(
        jsonr::version(SDB_VERSION)
      | jsonr::object_string_ignore_value(" created")
      | jsonr::object_string_ignore_value("?points")
          // | chart.mInfo.json_parser()
      | json_parser_ChartInfo(chart.mInfo)
      | jsonr::object_value("minimum_column_basis", chart.mMinimumColumnBasis)
          // | chart.mPoints.json_parser()
      | jsonr::object_value("stress", chart.mStress)
      | jsonr::object_value("column_bases", chart.mColumnBases)
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
