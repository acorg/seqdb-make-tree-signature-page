#include <iostream>

#include "chart.hh"
#include "tree.hh"
#include "read-file.hh"
#include "xz.hh"
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
    apply_transformation(aSettings);
    mViewport = bounding_rectangle();
    mViewport.square();
    const Location offset(aSettings.map_x_offset, aSettings.map_y_offset);
    mViewport.center(mViewport.center() + offset);
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

void Chart::apply_transformation(const SettingsAntigenicMaps& aSettings)
{
    Transformation t = mTransformation;
    t.multiplyBy(aSettings.map_transformation);
      // std::cerr << "transformation: " << *t << std::endl;
    for (auto& p: mPoints) {
        if (!p.coordinates.isnan()) {
            p.coordinates.x = p.coordinates.x * t[0][0] + p.coordinates.y * t[1][0];
            p.coordinates.y = p.coordinates.x * t[0][1] + p.coordinates.y * t[1][1];
        }
    }

} // Chart::apply_transformation

// ----------------------------------------------------------------------

Viewport Chart::bounding_rectangle() const
{
    Location tl(1e10, 1e10), br(-1e10, -1e10);
    for (const auto& p: mPoints) {
        if (!p.coordinates.isnan()) {
            tl.min(p.coordinates);
            br.max(p.coordinates);
        }
    }
    return Viewport(tl, br);

} // Chart::bounding_area

// ----------------------------------------------------------------------

void Chart::draw_points_reset(const SettingsAntigenicMaps& /*aSettings*/) const
{
    mDrawPoints.resize(mPoints.size(), nullptr);
    for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
        const auto& p = mPoints[point_no];
        if (p.attributes.antigen) {
            if (p.attributes.vaccine) {
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
    size_t number_of_leaves = 0;
    for (const auto& leaf: aLeaves) {
        ++number_of_leaves;
        auto p = mPointByName.find(leaf->name);
        if (p == mPointByName.end()) {
            for (const std::string& hi_name: leaf->hi_names) {
                p = mPointByName.find(hi_name);
                if (p != mPointByName.end())
                    break;
            }
        }
        if (p != mPointByName.end()) {
            mSequencedAntigens.insert(p->second);
            lines.push_back(leaf->line_no);
        }
    }
    std::cout << lines.size() << " sequenced antigens found in the chart" << std::endl;
    std::cout << number_of_leaves << " leaves in the tree" << std::endl;
    return lines;

} // Chart::sequenced_antigens

// ----------------------------------------------------------------------

size_t Chart::tracked_antigens(const std::vector<std::string>& aNames, Color aFillColor, const SettingsAntigenicMaps& aSettings) const
{
    size_t tracked = 0;
    mDrawTrackedAntigen.color(aFillColor);

    for (const auto& name: aNames) {
        const auto p = mPointByName.find(name);
        if (p != mPointByName.end()) {
            mDrawPoints[p->second] = &mDrawTrackedAntigen;
            ++tracked;

            if (aSettings.show_tracked_homologous_sera) {
                  // find homologous serum
                for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
                    if (!mPoints[point_no].attributes.antigen && mPoints[point_no].attributes.homologous_antigen >= 0 && static_cast<size_t>(mPoints[point_no].attributes.homologous_antigen) == p->second)
                        mDrawPoints[point_no] = &mDrawTrackedSerum;
                }
            }
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

size_t Chart::marked_antigens(const SettingsMarkAntigens& aData, const std::vector<std::string>& aTrackedNames, size_t aSectionNo, const SettingsAntigenicMaps& aSettings) const
{
    mDrawMarkedAntigens.clear();
    mDrawMarkedAntigens.reserve(aData.size()); // to avoid copying entries during emplace_back and loosing pointer for mDrawPoints

    for (const auto& entry: aData) {
        const bool tracked = std::find(aTrackedNames.begin(), aTrackedNames.end(), entry.tree_id.empty() ? entry.id : entry.tree_id) != aTrackedNames.end();
        if (aSettings.marked_antigens_on_all_maps || tracked || (entry.show_on_map >= 0  && static_cast<size_t>(entry.show_on_map) == aSectionNo)) {
            const auto p = mPointByName.find(entry.id);
            if (p != mPointByName.end()) {
                mDrawMarkedAntigens.emplace_back(entry);
                mDrawPoints[p->second] = &mDrawMarkedAntigens[mDrawMarkedAntigens.size() - 1];
            }
            else {
                const std::string prefix(entry.id, 0, entry.id.find(1, ' '));
                if (mPrefixName.find(prefix) != mPrefixName.end())
                    std::cerr << "Error: cannot find chart antigen by name: " << entry.id << std::endl;
            }
        }
        else {
              //std::cout << "section " << aSectionNo << " not tracked [" << entry.id << "] [" << entry.tree_id << "]" << std::endl;
        }
    }
    return mDrawMarkedAntigens.size();

} // Chart::marked_antigens

// ----------------------------------------------------------------------

void Chart::draw(Surface& aSurface, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    size_t drawn = 0;
    for (size_t level = 0; level < 10 && drawn < mDrawPoints.size(); ++level) {
        for (size_t point_no = 0; point_no < mDrawPoints.size(); ++point_no) {
           if (mDrawPoints[point_no]->level() == level) {
                 mDrawPoints[point_no]->draw(aSurface, mPoints[point_no], mPlot.style(point_no), aObjectScale, aSettings);
                ++drawn;
            }
        }
    }
    if (drawn != mDrawPoints.size())
        std::cerr << "Warning: " << drawn << " points of " << mDrawPoints.size() << " were drawn" << std::endl;

} // Chart::draw

// ----------------------------------------------------------------------

void DrawSerum::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    const double size = aSettings.serum_scale * aObjectScale;
    if (!aPoint.coordinates.isnan()) {
        aSurface.rectangle_filled(aPoint.coordinates, {size * aspect(aPoint, aStyle, aSettings), size}, outline_color(aPoint, aStyle, aSettings),
                                  aSettings.serum_outline_width * aObjectScale, TRANSPARENT);
    }

} // DrawSerum::draw

// ----------------------------------------------------------------------

Color DrawSerum::outline_color(const Point& /*aPoint*/, const PointStyle& /*aStyle*/, const SettingsAntigenicMaps& aSettings) const
{
    return aSettings.serum_outline_color;

} // DrawSerum::outline_color

// ----------------------------------------------------------------------

void DrawReferenceAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.reference_antigen_scale * aObjectScale, aspect(aPoint, aStyle, aSettings), rotation(aPoint, aStyle, aSettings),
                               aSettings.reference_antigen_outline_color,
                               aSettings.reference_antigen_outline_width * aObjectScale, TRANSPARENT);
    }

} // DrawReferenceAntigen::draw

// ----------------------------------------------------------------------

void DrawTestAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.test_antigen_scale * aObjectScale, aspect(aPoint, aStyle, aSettings), rotation(aPoint, aStyle, aSettings),
                               aSettings.test_antigen_outline_color,
                               aSettings.test_antigen_outline_width * aObjectScale, aSettings.test_antigen_fill_color);
    }

} // DrawTestAntigen::draw

// ----------------------------------------------------------------------

void DrawSequencedAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.test_antigen_scale * aObjectScale, aspect(aPoint, aStyle, aSettings), rotation(aPoint, aStyle, aSettings),
                               aSettings.sequenced_antigen_outline_color,
                               aSettings.sequenced_antigen_outline_width * aObjectScale, aSettings.sequenced_antigen_fill_color);
    }

} // DrawSequencedAntigen::draw

// ----------------------------------------------------------------------

void DrawTrackedAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.tracked_antigen_scale * aObjectScale, aspect(aPoint, aStyle, aSettings), rotation(aPoint, aStyle, aSettings),
                               aSettings.tracked_antigen_outline_color,
                               aSettings.tracked_antigen_outline_width * aObjectScale, mColor);
    }

} // DrawTrackedAntigen::draw

// ----------------------------------------------------------------------

void DrawVaccineAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, aSettings.vaccine_antigen_scale * aObjectScale, aspect(aPoint, aStyle, aSettings), rotation(aPoint, aStyle, aSettings),
                               aSettings.vaccine_antigen_outline_color, // aPoint.attributes.vaccine.outline_color,
                               aSettings.vaccine_antigen_outline_width * aObjectScale,
                               fill_color(aPoint, aStyle, aSettings) /*aPoint.attributes.vaccine.fill_color*/);
    }

} // DrawVaccineAntigen::draw

// ----------------------------------------------------------------------

void DrawMarkedAntigen::draw(Surface& aSurface, const Point& aPoint, const PointStyle& /*aStyle*/, double aObjectScale, const SettingsAntigenicMaps& /*aSettings*/) const
{
    if (!aPoint.coordinates.isnan()) {
        aSurface.circle_filled(aPoint.coordinates, mData.scale * aObjectScale, mData.aspect, mData.rotation, mData.outline_color, mData.outline_width * aObjectScale, mData.fill_color);
        if (!mData.label.empty()) {
            const TextStyle style(mData.label_font);
            const auto text_size = aSurface.text_size(mData.label, mData.label_scale, style);
            const Location text_origin = aPoint.coordinates + Size(mData.label_offset_x, mData.label_offset_y);
            aSurface.text(text_origin, mData.label, mData.label_color, mData.label_scale, style);
            const Location text_middle = text_origin + Size(text_size.width / 2, text_origin.y > aPoint.coordinates.y ? - text_size.height * 1.2 : text_size.height * 0.2);
            aSurface.line(aPoint.coordinates, text_middle, mData.label_line_color, mData.label_line_width);
        }
    }

} // DrawMarkedAntigen::draw

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

// inline auto json_fields(VaccineData& a)
// {
//     a.enabled = true;
//     return std::make_tuple(
//         "aspect", &a.aspect,
//         "fill_color", json::field(&a.fill_color, &Color::to_string, &Color::from_string),
//         "outline_color", json::field(&a.outline_color, &Color::to_string, &Color::from_string)
//                            );
// }

inline auto json_fields(PointAttributes& a)
{
    return std::make_tuple(
        "ht", &a.homologous_titer,
        "h", &a.homologous_antigen,
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

inline auto json_fields(PointStyle& a)
{
    return std::make_tuple(
        "fill_color", json::field(&a.fill_color, &Color::to_string, &Color::from_string),
        "outline_color", json::field(&a.outline_color, &Color::to_string, &Color::from_string),
        "aspect", &a.aspect,
        "size", &a.size,
        "rotation", &a.rotation,
        "shape", &a.shape
                           );
}

inline auto json_fields(PlotStyle& a)
{
    return std::make_tuple(
        "points", &a.points,
        "styles", &a.styles
                           );
}

inline auto json_fields(ChartInfo& a)
{
    return std::make_tuple(
        "date", &a.date,
        "lab", &a.lab,
        "virus_type", &a.virus_type,
        "lineage", &a.lineage,
        "name", &a.name,
        "rbc_species", &a.rbc_species
                           );
}

inline auto json_fields(Chart& a)
{
    return std::make_tuple(
        "  version", &a.json_version,
        " created", json::comment(""),
        "intermediate_layouts", json::comment(std::vector<std::vector<std::vector<double>>>()), // ignored (used for optimization movie)
        "?points", json::comment(""),
        "info", &a.mInfo,
        "minimum_column_basis", &a.mMinimumColumnBasis,
        "points", &a.mPoints,
        "stress", &a.mStress,
        "column_bases", &a.mColumnBases,
          // v2
        "transformation", &a.mTransformation,
        "point?", json::comment(""),
        "plot", &a.mPlot,
        "drawing_order", &a.mDrawingOrder,
        "drawing_orde?", json::comment("")
          //"layout_sequence", &a?,      // for optimization movie
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
