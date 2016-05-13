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

class json_parser_PointVaccine AXE_RULE
{
  public:
    inline json_parser_PointVaccine(Point& aPoint) : mPoint(aPoint) {}

    inline axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const
    {
        auto set_vaccine = axe::e_ref([&](auto, auto) { mPoint.vaccine = true; });
        return (jsonr::skey("v") > (jsonr::object(
              jsonr::object_value("aspect", mPoint.vaccine_aspect)
            | jsonr::object_string_value("fill_color", mPoint.vaccine_fill_color)
            | jsonr::object_string_value("outline_color", mPoint.vaccine_outline_color)
            | jsonr::object_string_ignore_value("?")
              ) >> set_vaccine))(i1, i2);
    }

  private:
    Point& mPoint;
};

class json_parser_PointAttributes AXE_RULE
{
  public:
    inline json_parser_PointAttributes(Point& aPoint) : mPoint(aPoint) {}

    inline axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const
    {
        auto get_point_type = axe::e_ref([&](auto b, auto e) { const std::string t(b, e); if (t == "a") mPoint.antigen = true; else if (t == "s") mPoint.antigen = false; else throw jsonr::JsonParsingError("invalid value for t point attribute: " + t); });
        auto point_type = jsonr::doublequotes > (jsonr::string_content >> get_point_type) > jsonr::doublequotes;
        return (jsonr::skey("a") > jsonr::object(
              jsonr::object_value("r", mPoint.reassortant)
            | jsonr::object_value("R", mPoint.reference)
            | jsonr::skey("t") > point_type
            | jsonr::object_value("e", mPoint.egg)
            | json_parser_PointVaccine(mPoint)
            | jsonr::object_string_ignore_value("?")
              ))(i1, i2);
    }

  private:
    Point& mPoint;
};

axe::result<std::string::iterator> Point::json_parser_t::operator()(std::string::iterator i1, std::string::iterator i2) const
{
    return jsonr::object(axe::r_named(
          jsonr::object_value("N", mPoint.name)
        | jsonr::object_value("c", mPoint.coordinates)
        | jsonr::object_value("l", mPoint.lab_id)
        | json_parser_PointAttributes(mPoint)
        | jsonr::object_string_ignore_value("?")
          , "point"))(i1, i2);
}

constexpr const char* SDB_VERSION = "acmacs-sdb-v1";

Chart Chart::from_json(std::string data)
{
    Chart chart;
    auto parse_chart = jsonr::object(
        jsonr::version(SDB_VERSION)
      | jsonr::object_string_ignore_value(" created")
      | jsonr::object_string_ignore_value("?points")
      | json_parser_ChartInfo(chart.mInfo)
      | jsonr::object_value("minimum_column_basis", chart.mMinimumColumnBasis)
      | jsonr::object_array_value("points", chart.mPoints)
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
