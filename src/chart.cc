#include <iostream>

#include "chart.hh"
#include "tree.hh"
#include "read-file.hh"
#include "xz.hh"
#include "settings.hh"
#include "json-struct.hh"

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

void Chart::preprocess(const SettingsAntigenicMaps& aSettings)
{
      // Calculate viewport for all points
    Location tl(1e10, 1e10), br(-1e10, -1e10);
    for (const auto& p: mPoints) {
        if (!p.coordinates.isnan()) {
            tl.min(p.coordinates);
            br.max(p.coordinates);
        }
    }
    const Location center = Location::center_of(tl, br);
    for (auto& p: mPoints) {
        if (!p.coordinates.isnan()) {
            p.coordinates -= center;
        }
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
        if (p.attributes.antigen) {
            if (p.attributes.vaccine.enabled) {
                mDrawPoints[point_no] = &mDrawVaccineAntigen;
            }
            else if (mSequencedAntigens.find(point_no) != mSequencedAntigens.end()) {
                mDrawPoints[point_no] = &mDrawSequencedAntigen;
            }
            else if (p.attributes.reference) {
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

std::vector<size_t> Chart::sequenced_antigens(const std::vector<const Node*>& aLeaves)
{
    mSequencedAntigens.clear();
    std::vector<size_t> lines;
    for (const auto& leaf: aLeaves) {
        const auto p = mPointByName.find(leaf->name);
        if (p != mPointByName.end()) {
            mSequencedAntigens.insert(p->second);
            lines.push_back(leaf->line_no);
        }
    }
    return lines;

} // Chart::sequenced_antigens

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
    return aPoint.attributes.reassortant ? aSettings.reassortant_rotation : 0.0;

} // DrawPoint::rotation

// ----------------------------------------------------------------------

void DrawSerum::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    const double size = aSettings.serum_scale * aObjectScale;
    if (!aPoint.coordinates.isnan()) {
        aSurface.rectangle_filled(aPoint.coordinates, {size * aspect(aPoint, aSettings), size}, aSettings.serum_outline_color,
                                  aSettings.serum_outline_width * aObjectScale, TRANSPARENT);
    }

} // DrawSerum::draw

// ----------------------------------------------------------------------

double DrawAntigen::aspect(const Point& aPoint, const SettingsAntigenicMaps& aSettings) const
{
    return aPoint.attributes.egg ? aSettings.egg_antigen_aspect : 1.0;

} // DrawAntigen::aspect

// ----------------------------------------------------------------------

void DrawReferenceAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.reference_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                               aSettings.reference_antigen_outline_color,
                               aSettings.reference_antigen_outline_width * aObjectScale, TRANSPARENT);
    }

} // DrawReferenceAntigen::draw

// ----------------------------------------------------------------------

void DrawTestAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.test_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                               aSettings.test_antigen_outline_color,
                               aSettings.test_antigen_outline_width * aObjectScale, aSettings.test_antigen_fill_color);
    }

} // DrawTestAntigen::draw

// ----------------------------------------------------------------------

void DrawSequencedAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.test_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                               aSettings.sequenced_antigen_outline_color,
                               aSettings.sequenced_antigen_outline_width * aObjectScale, aSettings.sequenced_antigen_fill_color);
    }

} // DrawSequencedAntigen::draw

// ----------------------------------------------------------------------

void DrawTrackedAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.tracked_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                               aSettings.tracked_antigen_outline_color,
                               aSettings.tracked_antigen_outline_width * aObjectScale, mColor);
    }

} // DrawTrackedAntigen::draw

// ----------------------------------------------------------------------

void DrawVaccineAntigen::draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.vaccine_antigen_scale * aObjectScale, aspect(aPoint, aSettings), rotation(aPoint, aSettings),
                               aSettings.vaccine_antigen_outline_color, // aPoint.attributes.vaccine.outline_color,
                               aSettings.vaccine_antigen_outline_width * aObjectScale, aPoint.attributes.vaccine.fill_color);
    }

} // DrawVaccineAntigen::draw

// ----------------------------------------------------------------------
// json
// ----------------------------------------------------------------------

static inline std::string make_PointAntigenSerializer(const bool*) { throw std::runtime_error("make_PointAntigenSerializer not implemented"); }

static inline void make_PointAntigenDeserializer(bool* antigen, std::string& source)
{
    if (source == "a")
        *antigen = true;
    else if (source == "s")
        *antigen = false;
    else
        throw json::parsing_error("invalid value for t point attribute: " + source);
}

inline auto json_fields(VaccineData& a)
{
    a.enabled = true;
    return std::make_tuple(
        "aspect", &a.aspect,
        "fill_color", json::field(&a.fill_color, &Color::to_string, &Color::from_string),
        "outline_color", json::field(&a.outline_color, &Color::to_string, &Color::from_string)
                           );
}

inline auto json_fields(PointAttributes& a)
{
    return std::make_tuple(
        "t", json::field(&a.antigen, &make_PointAntigenSerializer, &make_PointAntigenDeserializer),
        "v", &a.vaccine,
        "r", &a.reassortant,
        "R", &a.reference,
        "e", &a.egg
                           );
}

inline auto json_fields(Point& a)
{
    return std::make_tuple(
        "N", &a.name,
        "c", json::field(&a.coordinates, &Location::to_vector, &Location::from_vector),
        "l", &a.lab_id,
        "a", &a.attributes
                           );
}

// ----------------------------------------------------------------------

Chart Chart::from_json(std::string data)
{
    Chart chart;
    try {
        json::parse(data, chart);
    }
    catch (json::parsing_error& err) {
        std::cerr << "chart parsing error: "<< err.what() << std::endl;
        throw;
    }
    return chart;

} // Chart::from_json

// ----------------------------------------------------------------------
