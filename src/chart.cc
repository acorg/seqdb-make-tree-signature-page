#include <iostream>

#include "chart.hh"
#include "read-file.hh"
#include "xz.hh"
#include "settings.hh"

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
            | (jsonr::skey("t") > point_type)
            | jsonr::object_value("e", mPoint.egg)
            | json_parser_PointVaccine(mPoint)
            | jsonr::object_string_ignore_value("?")
              ))(i1, i2);
    }

  private:
    Point& mPoint;
};

class json_parser_Location AXE_RULE
{
  public:
    inline json_parser_Location(Location& aLocation) : mLocation(aLocation) {}

    inline axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const
    {
        using namespace jsonr;
        return (skey("c") > array_begin > r_double(mLocation.x) > comma > r_double(mLocation.y) > array_end)(i1, i2);
    }

  private:
    Location& mLocation;
};

axe::result<std::string::iterator> Point::json_parser_t::operator()(std::string::iterator i1, std::string::iterator i2) const
{
    return jsonr::object(axe::r_named(
          jsonr::object_value("N", mPoint.name)
            // | jsonr::object_value("c", mPoint.coordinates)
        | json_parser_Location(mPoint.coordinates)
        | jsonr::object_value("l", mPoint.lab_id)
        | json_parser_PointAttributes(mPoint)
        | jsonr::object_string_ignore_value("?")
          , "point"))(i1, i2);
}

constexpr const char* SDB_VERSION = "acmacs-sdb-v1";

Chart Chart::from_json(std::string data)
{
    using namespace jsonr;
    Chart chart;
    auto parse_chart = object(
        version(SDB_VERSION)
      | object_string_ignore_value(" created")
      | object_string_ignore_value("?points")
      | json_parser_ChartInfo(chart.mInfo)
      | object_value("minimum_column_basis", chart.mMinimumColumnBasis)
      | object_array_value("points", chart.mPoints)
      | object_value("stress", chart.mStress)
      | object_value("column_bases", chart.mColumnBases)
        );
    try {
        parse_chart(std::begin(data), std::end(data));
    }
    catch (axe::failure<char>& err) {
        throw JsonParsingError(err.message());
    }
    catch (failure& err) {
        throw JsonParsingError(err.message(std::begin(data)));
    }
    return chart;

} // Chart::from_json

// ----------------------------------------------------------------------

void Chart::preprocess(const SettingsAntigenicMaps& aSettings)
{
      // Calculate viewport for all points
    Location tl(1e10, 1e10), br(-1e10, -1e10);
    for (const auto& p: mPoints) {
        tl.min(p.coordinates);
        br.max(p.coordinates);
    }
    const Location center = Location::center_of(tl, br);
    for (auto& p: mPoints) {
        p.coordinates -= center;
    }
    tl -= center;
    br -= center;
    mViewport.set(tl, br);
    mViewport.square();
    mViewport.zoom(aSettings.map_zoom);
    mViewport.whole_width();

      // build point by name index
    mPointByName.clear();
    for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
        mPointByName[mPoints[point_no].name] = point_no;
    }

    draw_points_reset(aSettings);

    mPrefixName.clear();
    for (const auto& p: mPoints) {
        mPrefixName.insert(p.name.substr(0, p.name.find(1, ' ')));
    }

} // Chart::preprocess

// ----------------------------------------------------------------------

void Chart::draw_points_reset(const SettingsAntigenicMaps& /*aSettings*/) const
{
    mDrawPoints.resize(mPoints.size(), nullptr);
    for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
        const auto& p = mPoints[point_no];
        if (p.antigen) {
            if (p.vaccine) {
                mDrawPoints[point_no] = &mDrawVaccineAntigen;
            }
            else if (p.reference) {
                mDrawPoints[point_no] = &mDrawReferenceAntigen;
            }
            else {
                mDrawPoints[point_no] = &mDrawTestAntigen;
            }
        }
        else {
            mDrawPoints[point_no] = &mDrawSerum;
        }
    }

} // Chart::draw_points_reset

// ----------------------------------------------------------------------

size_t Chart::tracked_antigens(const std::vector<std::string>& aNames, Color aFillColor, const SettingsAntigenicMaps& /*aSettings*/) const
{
    size_t tracked = 0;
    mDrawTrackedAntigen.color(aFillColor);

    for (const auto& name: aNames) {
        const auto p = mPointByName.find(name);
        if (p != mPointByName.end()) {
            mDrawPoints[p->second] = &mDrawTrackedAntigen;
            ++tracked;
        }
        else {
            const std::string prefix(name, 0, name.find(1, ' '));
            if (mPrefixName.find(prefix) != mPrefixName.end())
                std::cerr << "Error: cannot find chart antigen by name: " << name << std::endl;
        }
    }
    return tracked;

} // Chart::tracked_antigens

// ----------------------------------------------------------------------

void Chart::draw(Surface& aSurface, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    size_t drawn = 0;
    for (size_t level = 0; level < 10 && drawn < mDrawPoints.size(); ++level) {
        for (size_t point_no = 0; point_no < mDrawPoints.size(); ++point_no) {
            if (mDrawPoints[point_no]->level() == level) {
                mDrawPoints[point_no]->draw(aSurface, mPoints[point_no], aObjectScale, aSettings);
                ++drawn;
            }
        }
    }
    if (drawn != mDrawPoints.size())
        std::cerr << "Warning: " << drawn << " points of " << mDrawPoints.size() << " were drawn" << std::endl;

} // Chart::draw

// ----------------------------------------------------------------------

inline double DrawPoint::rotation(const Point& aPoint, const SettingsAntigenicMaps& aSettings) const
{
    return aPoint.reassortant ? aSettings.reassortant_rotation : 0.0;

} // DrawPoint::rotation

// ----------------------------------------------------------------------

void DrawSerum::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    const double size = aSettings.serum_scale * aObjectScale;
    aSurface.rectangle_filled(aPoint.coordinates, {size * aspect(aPoint, aSettings), size}, aSettings.serum_outline_color,
                              aSettings.serum_outline_width * aObjectScale, TRANSPARENT);

} // DrawSerum::draw

// ----------------------------------------------------------------------

double DrawAntigen::aspect(const Point& aPoint, const SettingsAntigenicMaps& aSettings) const
{
    return aPoint.egg ? aSettings.egg_antigen_aspect : 1.0;

} // DrawAntigen::aspect

// ----------------------------------------------------------------------

void DrawReferenceAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    aSurface.circle_filled(aPoint.coordinates, aSettings.reference_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                           aSettings.reference_antigen_outline_color,
                           aSettings.reference_antigen_outline_width * aObjectScale, TRANSPARENT);

} // DrawReferenceAntigen::draw

// ----------------------------------------------------------------------

void DrawTestAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    aSurface.circle_filled(aPoint.coordinates, aSettings.test_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                           aSettings.test_antigen_outline_color,
                           aSettings.test_antigen_outline_width * aObjectScale, aSettings.test_antigen_fill_color);

} // DrawTestAntigen::draw

// ----------------------------------------------------------------------

void DrawTrackedAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    aSurface.circle_filled(aPoint.coordinates, aSettings.tracked_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                           aSettings.tracked_antigen_outline_color,
                           aSettings.tracked_antigen_outline_width * aObjectScale, mColor);

} // DrawTrackedAntigen::draw

// ----------------------------------------------------------------------

void DrawVaccineAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    aSurface.circle_filled(aPoint.coordinates, aSettings.vaccine_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                           aSettings.vaccine_antigen_outline_color, // aPoint.vaccine_outline_color,
                           aSettings.vaccine_antigen_outline_width * aObjectScale, aPoint.vaccine_fill_color);

} // DrawVaccineAntigen::draw

// ----------------------------------------------------------------------
