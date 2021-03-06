#include <iostream>

#include "chart.hh"
#include "tree.hh"
#include "acmacs-base/read-file.hh"
#include "json-struct.hh"

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer)
{
    Chart chart;
    if (buffer == "-")
        buffer = acmacs_base::read_stdin();
    else
        buffer = acmacs_base::read_file(buffer);
    if (buffer[0] == '{')
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
        mPointByName.emplace(mPoints[point_no].name, point_no);
          // mPointByName[mPoints[point_no].name] = point_no;
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
    std::cout << "transformation: " << t << std::endl;
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
        if (!leaf->hidden) {
            ++number_of_leaves;
            auto p = mPointByName.find(leaf->name);
            if (p == mPointByName.end()) {
                for (const std::string& hi_name: leaf->hi_names) {
                    p = mPointByName.find(hi_name);
                    if (p != mPointByName.end()) {
                        // std::cout << "Sequenced2 " << hi_name << std::endl;
                        break;
                    }
                }
                // std::cout << "Sequenced? " << leaf->name << " ";
                // std::copy(leaf->hi_names.begin(), leaf->hi_names.end(), std::ostream_iterator<std::string>(std::cout, " "));
                // std::cout << std::endl;
            }
            if (p != mPointByName.end()) {
                // std::cout << "Sequenced " << leaf->name << std::endl;
                mSequencedAntigens.insert(p->second);
                lines.push_back(leaf->line_no);
            }
        }
    }
    std::cout << lines.size() << " sequenced antigens found in the chart" << std::endl;
    std::cout << number_of_leaves << " leaves in the tree" << std::endl;
    return lines;

} // Chart::sequenced_antigens

// ----------------------------------------------------------------------

size_t Chart::tracked_antigens(const std::vector<std::string>& aNames, Color aFillColor, const SettingsAntigenicMaps& /*aSettings*/) const
{
      // init_tracked_sera(aNames.size(), aSettings);
    size_t tracked = 0;
    mDrawTrackedAntigen.color(aFillColor);

    for (const auto& name: aNames) {
        const auto p = mPointByName.find(name);
        if (p != mPointByName.end()) {
            mDrawPoints[p->second] = &mDrawTrackedAntigen;
            ++tracked;
              // add_tracked_serum(p->second, aSettings);
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

size_t Chart::tracked_antigens_colored_by_clade(const std::vector<std::string>& aNames, const std::map<std::string, const Node*>& aNodeByName, const SettingsAntigenicMaps& /*aSettings*/) const
{
    // init_tracked_sera(aNames.size(), aSettings);
    size_t tracked = 0;
    mDrawTrackedAntigensColoredByClade.clear();
    mDrawTrackedAntigensColoredByClade.reserve(aNames.size()); // to avoid copying entries during emplace_back and loosing pointer for mDrawPoints

      // std::set<size_t> antigens_on_this_map;

    for (const auto& name: aNames) {
        const auto p = mPointByName.find(name);
        if (p != mPointByName.end()) {
            Color fill_color = 0xFF000000;
            const auto node = aNodeByName.find(name);
            if (node != aNodeByName.end()) {
                for (const auto& clade: node->second->clades) {
                    if (clade == "3C3b") {
                        fill_color = 0x0000FF;
                        break;
                    }
                    else if (clade == "3C3a") {
                        fill_color = 0x00FF00;
                        break;
                    }
                    else if (clade == "3C2a") {
                        fill_color = 0xFF0000;
                        break;
                    }
                    else if (clade == "3C3") {
                        fill_color = 0x6495ED;
                        break;
                    }
                }
            }
            if (fill_color == Color(0xFF000000)) {
                if (node != aNodeByName.end()) {
                    // std::cerr << "no colored clade " << name << " [";
                    // std::copy(node->second->clades.begin(), node->second->clades.end(), std::ostream_iterator<std::string>(std::cerr, " "));
                    // std::cerr << "]" << std::endl;
                }
                else {
                    std::cerr << "no clade " << name << std::endl;
                }
            }
            mDrawTrackedAntigensColoredByClade.emplace_back(fill_color);
            mDrawPoints[p->second] = &mDrawTrackedAntigensColoredByClade.back();
              // antigens_on_this_map.insert(static_cast<size_t>(p->second));
            ++tracked;
            // add_tracked_serum(aSectionNo, p->second, aSettings);
        }
        else {
            const std::string prefix(name, 0, name.find(1, ' '));
            if (mPrefixName.find(prefix) != mPrefixName.end())
                std::cerr << "Error: cannot find chart antigen by name: " << name << std::endl;
        }
    }

    // std::map<size_t, std::vector<size_t>> antigen_to_homologous_serum;
    // for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
    //     if (!mPoints[point_no].attributes.antigen && mPoints[point_no].attributes.homologous_antigen >= 0) {
    //         antigen_to_homologous_serum.emplace(mPoints[point_no].attributes.homologous_antigen, std::vector<size_t>());
    //         antigen_to_homologous_serum[static_cast<size_t>(mPoints[point_no].attributes.homologous_antigen)].push_back(point_no);
    //     }
    // }
    // std::cerr << "antigen_to_homologous_serum " << antigen_to_homologous_serum.size() << std::endl;
    // for (auto& ag_to_sr: antigen_to_homologous_serum) {
    //     auto name = mPoints[ag_to_sr.first].name;
    //     auto in_names = std::find(aNames.begin(), aNames.end(), name) != aNames.end();
    //     std::cerr << "antigen_to_homologous_serum " << (in_names ? "in" : "NO") << " names " << name << " " << ag_to_sr.first << " ";
    //     std::copy(ag_to_sr.second.begin(), ag_to_sr.second.end(), std::ostream_iterator<size_t>(std::cerr, " "));
    //     std::cerr << std::endl;
    // }

    // // for (size_t ag: antigens_on_this_map) {
    // //     auto p = antigen_to_homologous_serum.find(ag);
    // //     if (p != antigen_to_homologous_serum.end()) {
    // //         std::cerr << "homologous ag:" << ag << " [";
    // //         std::copy(p->second.begin(), p->second.end(), std::ostream_iterator<size_t>(std::cerr, " "));
    // //         std::cerr << "]" << std::endl;
    // //     }
    // // }

    return tracked;

} // Chart::tracked_antigens_colored_by_clade

// ----------------------------------------------------------------------

// void Chart::init_tracked_sera(size_t /*aSize*/, const SettingsAntigenicMaps& /*aSettings*/) const
// {
//     // if (aSettings.show_tracked_homologous_sera) {
//     //     mDrawTrackedSera.clear();
//     //     mDrawTrackedSera.reserve(aSize); // to avoid copying entries during emplace_back and loosing pointer for mDrawPoints
//     // }

// } // Chart::init_tracked_sera

// // ----------------------------------------------------------------------

// void Chart::add_tracked_serum(size_t aSectionNo, size_t aAntigenNo, const SettingsAntigenicMaps& aSettings) const
// {
//     if (aSettings.show_tracked_homologous_sera) {
//           // find homologous serum
//         for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
//             if (!mPoints[point_no].attributes.antigen
//                 && mPoints[point_no].attributes.homologous_antigen >= 0
//                 && static_cast<size_t>(mPoints[point_no].attributes.homologous_antigen) == aAntigenNo
//                 && mPoints[point_no].section_for_serum_circle == static_cast<int>(aSectionNo)) {
//                 // mDrawTrackedSera.emplace_back(0x40000000);
//                 // mDrawPoints[point_no] = &mDrawTrackedSera.back();
//                 mDrawPoints[point_no] = &mDrawTrackedSerum;
//             }
//         }
//     }

// } // Chart::add_tracked_serum

// ----------------------------------------------------------------------

void Chart::tracked_sera(size_t aSectionNo, const SettingsAntigenicMaps& aSettings) const
{
    if (aSettings.show_tracked_homologous_sera) {
        std::vector<std::pair<std::string, size_t>> infix_colors = {
            {"NEBRASKA/19/2015", 0x00A0A0},
            {"NEVADA/22/2016", 0xA0A000},
            {"MONTANA/28/2015", 0x00A0A0},
            {"ONTARIO/RV2414/2015", 0x00A0A0},
            {"ALASKA/232/2015 CDC 2016-083 SIAT4", 0xA0A000},
              // MELB
            {"HONG KONG/4801/2014 F3419-14D E?", 0xFF0000},
            {"HONG KONG/4801/2014 F3491-14D MDCK?", 0x00A000},
            {"ALASKA/232/2015 F3698-13D", 0x00A0A0},
            {"SRI LANKA/61/2015", 0xA0A000},
              // NIID
            {"SAPPORO/71/2015", 0x00A0A0},
              // NIMR
            {"HONG KONG/4801/2014 F12/15", 0xFFA000},
              // common
            {"HONG KONG/4801/2014", 0xFF0000},
            {"SAITAMA/103/2014", 0xA000A0},
        };
        mDrawTrackedSera.clear();
        mDrawTrackedSera.reserve(mPoints.size()); // to avoid copying entries during emplace_back and loosing pointer for mDrawPoints
        for (size_t point_no = 0; point_no < mPoints.size(); ++point_no) {
            if (!mPoints[point_no].attributes.antigen
                && mPoints[point_no].attributes.homologous_antigen >= 0
                && mPoints[point_no].section_for_serum_circle == static_cast<int>(aSectionNo)) {
                  // std::cout << "tracked serum " << mPoints[point_no].name << std::endl;
                size_t color = 0;
                for (const auto& entry: infix_colors) {
                    if (mPoints[point_no].name.find(entry.first) != std::string::npos) {
                        color = entry.second;
                        break;
                    }
                }
                mDrawTrackedSera.emplace_back(color);
                mDrawPoints[point_no] = &mDrawTrackedSera.back();
                //mDrawPoints[point_no] = &mDrawTrackedSerum;
            }
        }
    }

} // Chart::tracked_sera

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
                std::cout << aSectionNo << " marking antigen " << p->second << " " << p->first << std::endl;
                mDrawMarkedAntigens.emplace_back(entry);
                mDrawPoints[p->second] = &mDrawMarkedAntigens.back();
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
                                  outline_width(aPoint, aStyle, aSettings) * aObjectScale, TRANSPARENT);
    }

} // DrawSerum::draw

// ----------------------------------------------------------------------

double DrawSerum::outline_width(const Point& /*aPoint*/, const PointStyle& /*aStyle*/, const SettingsAntigenicMaps& aSettings) const
{
    return aSettings.serum_outline_width;

} // DrawSerum::outline_width

// ----------------------------------------------------------------------

Color DrawSerum::outline_color(const Point& /*aPoint*/, const PointStyle& /*aStyle*/, const SettingsAntigenicMaps& aSettings) const
{
    return aSettings.serum_outline_color;

} // DrawSerum::outline_color

// ----------------------------------------------------------------------

void DrawTrackedSerum::draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const
{
    DrawSerum::draw(aSurface, aPoint, aStyle, aObjectScale, aSettings);
    std::cout << "    Tracked serum " << aPoint.name << " radius:" << aPoint.attributes.serum_circle_radius << std::endl;
    aSurface.circle(aPoint.coordinates, aPoint.attributes.serum_circle_radius * 2, 1, 0,
                    outline_color(aPoint, aStyle, aSettings), // aSettings.serum_circle_color,
                    aSettings.serum_circle_thickness * aObjectScale);

} // DrawTrackedSerum::draw

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
        std::cout << "Vaccine " << aPoint.name << " " << fill_color(aPoint, aStyle, aSettings) << std::endl;
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
        "ha", &a.homologous_antigen,
        "ra", &a.serum_circle_radius,
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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
