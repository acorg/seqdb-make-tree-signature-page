#include "continent-map.hh"
#include "settings.hh"
#include "continent-path.inc"

// ----------------------------------------------------------------------

void ColoringByContinentMapLegend::draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const
{
    for (const auto& continent: ColoringByContinentLegendLabels) {
        cairo_path_t* map_outline = outline(aSurface, continent_map_path, continent);
        const double scale = aSurface.draw_path_scale(map_outline, aViewport);
        aSurface.draw_path(map_outline, aViewport, scale, mColoring.color(continent), aSettings.geographic_map_outline_width);
        aSurface.destroy_path(map_outline);
    }

} // ColoringByContinentMapLegend::draw

// ----------------------------------------------------------------------

Size ColoringByContinentMapLegend::size(Surface& aSurface, const SettingsLegend& aSettings) const
{
    const double map_height = aSurface.canvas_size().height * aSettings.geographic_map_fraction;
    return Size(continent_map_size[0] / continent_map_size[1] * map_height, map_height);

} // ColoringByContinentMapLegend::size

// ----------------------------------------------------------------------

cairo_path_t* ColoringByContinentMapLegend::outline(Surface& aSurface, const std::map<std::string, std::vector<ContinentMapPathElement>>& aPath, std::string aContinent) const
{
    aSurface.new_path();
    auto continent_path = aPath.find(aContinent);
    if (continent_path == aPath.end())
        throw std::runtime_error("No path for " + aContinent);
    for (const auto& element: continent_path->second) {
        if (element.x < 0) {
              //aSurface.close_path();
            aSurface.move_to(- element.x, element.y);
        }
        else {
            aSurface.line_to(element.x, element.y);
        }
    }
    auto path = aSurface.copy_path();
    aSurface.new_path();
    return path;

} // ColoringByContinentMapLegend::outline

// ----------------------------------------------------------------------
