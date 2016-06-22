#include "coloring.hh"
#include "legend.hh"
#include "tree.hh"
#include "settings.hh"
#include "geographic-path.inc"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

const std::map<std::string, Color> ColoringByContinent::mContinents = {
    {"EUROPE",            0x00FF00},
    {"CENTRAL-AMERICA",   0xAAF9FF},
    {"MIDDLE-EAST",       0x8000FF},
    {"NORTH-AMERICA",     0x00008B},
    {"AFRICA",            0xFF8000},
    {"ASIA",              0xFF0000},
    {"RUSSIA",            0xB03060},
    {"AUSTRALIA-OCEANIA", 0xFF69B4},
    {"SOUTH-AMERICA",     0x40E0D0},
    {"ANTARCTICA",        0x808080},
    {"CHINA-SOUTH",       0xFF0000},
    {"CHINA-NORTH",       0x6495ED},
    {"CHINA-UNKNOWN",     0x808080},
    {"UNKNOWN",           0x808080},
};

// ----------------------------------------------------------------------

// http://stackoverflow.com/questions/470690/how-to-automatically-generate-n-distinct-colors (search for kellysMaxContrastSet)
const Color sDistinctColors[] = {
    0xA6BDD7, //Very Light Blue
    0xC10020, //Vivid Red
    0xFFB300, //Vivid Yellow
    0x803E75, //Strong Purple
    0xFF6800, //Vivid Orange
    0xCEA262, //Grayish Yellow
      //0x817066, //Medium Gray

    //The following will not be good for people with defective color vision
    0x007D34, //Vivid Green
    0xF6768E, //Strong Purplish Pink
    0x00538A, //Strong Blue
    0xFF7A5C, //Strong Yellowish Pink
    0x53377A, //Strong Violet
    0xFF8E00, //Vivid Orange Yellow
    0xB32851, //Strong Purplish Red
    0xF4C800, //Vivid Greenish Yellow
    0x7F180D, //Strong Reddish Brown
    0x93AA00, //Vivid Yellowish Green
    0x593315, //Deep Yellowish Brown
    0xF13A13, //Vivid Reddish Orange
    0x232C16, //Dark Olive Green
};

// ----------------------------------------------------------------------

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

Color ColoringByContinent::color(const Node& aNode) const
{
    return color(aNode.continent);

} // ColoringByContinent::color

// ----------------------------------------------------------------------

static constexpr const char* const ColoringByContinentLegendLabels[] = {"EUROPE", "CENTRAL-AMERICA", "MIDDLE-EAST", "NORTH-AMERICA", "AFRICA", "ASIA", "RUSSIA", "AUSTRALIA-OCEANIA", "SOUTH-AMERICA"};

class ColoringByContinentLegend : public Legend
{

 public:
    inline ColoringByContinentLegend(const ColoringByContinent& aColoring) : Legend(), mColoring(aColoring) {}

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const
        {
            auto const label_size = aSurface.text_size("W", aSettings.font_size, aSettings.style);
            auto y = aViewport.origin.y + label_size.height;
            for (auto& label: ColoringByContinentLegendLabels) {
                aSurface.text({aViewport.origin.x, y}, label, mColoring.color(label), aSettings.font_size, aSettings.style);
                y += label_size.height * aSettings.interline;
            }
        }

    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const
        {
            Size size(0, 0);
            for (auto& label: ColoringByContinentLegendLabels) {
                const auto label_size = aSurface.text_size(label, aSettings.font_size, aSettings.style);
                size.height += label_size.height * aSettings.interline;
                if (label_size.width > size.width)
                    size.width = label_size.width;
            }
            return size;
        }

 private:
    const ColoringByContinent& mColoring;

}; // class ColoringByContinentLegend

// ----------------------------------------------------------------------

class ColoringByContinentMapLegend : public Legend
{

 public:
    inline ColoringByContinentMapLegend(const ColoringByContinent& aColoring) : Legend(), mColoring(aColoring) {}

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const
        {
            cairo_path_t* map_outline = outline(aSurface, geographic_map_path);
            const double scale = aSurface.draw_path_scale(map_outline, aViewport);
            aSurface.draw_path(map_outline, aViewport, scale, aSettings.geographic_map_outline_color, aSettings.geographic_map_outline_width);
            aSurface.destroy_path(map_outline);
            for (auto& label: ColoringByContinentLegendLabels) {
                cairo_path_t* continent_outline = outline(aSurface, continent_path(label));
                aSurface.draw_path(continent_outline, aViewport, scale, mColoring.color(label), aSettings.geographic_map_outline_width);
                aSurface.destroy_path(continent_outline);
            }
        }

    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const
        {
            const double map_height = aSurface.canvas_size().height * aSettings.geographic_map_fraction;
            return Size(geographic_map_size[0] / geographic_map_size[1] * map_height, map_height);
        }

 private:
    const ColoringByContinent& mColoring;

    cairo_path_t* outline(Surface& aSurface, const std::vector<GeographicMapPathElement>& aPath) const
        {
            aSurface.new_path();
            for (const auto& element: aPath) {
                if (element.x < 0) {
                    aSurface.close_path();
                    aSurface.move_to(- element.x, element.y);
                }
                else {
                    aSurface.line_to(element.x, element.y);
                }
            }
            auto path = aSurface.copy_path();
            aSurface.new_path();
            return path;
        }

}; // class ColoringByContinentMapLegend

// ----------------------------------------------------------------------

Legend* ColoringByContinent::legend() const
{
      //return new ColoringByContinentLegend(*this);
    return new ColoringByContinentMapLegend(*this);

} // ColoringByContinent::legend

// ----------------------------------------------------------------------

Color ColoringByPos::color(const Node& aNode) const
{
    Color c(0);
    if (aNode.aa.size() > mPos) {
        const char aa = aNode.aa[mPos];
        try {
            c = mUsed.at(aa);
        }
        catch (std::out_of_range&) {
            if (aa != 'X')      // X is always black
                c = sDistinctColors[mColorsUsed++];
            mUsed[aa] = c;
        }
    }
    return c;

} // ColoringByPos::color

// ----------------------------------------------------------------------

class ColoringByPosLegend : public Legend
{
 public:
    inline ColoringByPosLegend(const ColoringByPos& aColoring) : Legend(), mColoring(aColoring) {}

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const
        {
            const auto label_size = aSurface.text_size("W", aSettings.font_size, aSettings.style);
            auto x = aViewport.origin.x;
            auto y = aViewport.origin.y + label_size.height;
            const std::string title = std::to_string(mColoring.pos() + 1);
            aSurface.text({x, y}, title, BLACK, aSettings.font_size, aSettings.style);
            x += (aSurface.text_size(title, aSettings.font_size, aSettings.style).width - label_size.width) / 2;
            y += label_size.height * aSettings.interline;
            for (auto& label_color: mColoring.aa_color()) {
                aSurface.text({x, y}, std::string(1, label_color.first), label_color.second, aSettings.font_size, aSettings.style);
                y += label_size.height * aSettings.interline;
            }
        }

    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const
        {
            const auto label_size = aSurface.text_size("W", aSettings.font_size, aSettings.style);
            return {label_size.width, label_size.height * aSettings.interline * (mColoring.aa_color().size() + 1)};
        }

 private:
    const ColoringByPos& mColoring;

}; // class ColoringByContinentLegend

Legend* ColoringByPos::legend() const
{
    return new ColoringByPosLegend(*this);

} // ColoringByPos::legend

// ----------------------------------------------------------------------
