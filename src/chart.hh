#pragma once

#include <set>

#include "draw.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Point;
class PointStyle;
class Node;

// ----------------------------------------------------------------------

class DrawPoint
{
 public:
    inline DrawPoint() = default;
    inline DrawPoint(const DrawPoint&) = default;
    inline DrawPoint(DrawPoint&&) = default;
    inline virtual ~DrawPoint() = default;
    inline DrawPoint& operator=(const DrawPoint&) = default;
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const = 0;
    virtual size_t level() const = 0;
    virtual double aspect(const Point&, const PointStyle& aStyle, const SettingsAntigenicMaps&) const;
    virtual double rotation(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const;
    virtual Color fill_color(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const;
    virtual Color outline_color(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const;
};

class DrawSerum : public DrawPoint
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 1; }
    virtual Color outline_color(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const;
};

class DrawAntigen : public DrawPoint
{
 public:
    virtual double aspect(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const;
};

class DrawReferenceAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 2; }
};

class DrawTestAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 3; }
};

class DrawSequencedAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 4; }
};

class DrawTrackedAntigen : public DrawAntigen
{
 public:
    inline DrawTrackedAntigen() : mColor(0xFFC0CB) {}

    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 5; }
    inline void color(Color aColor) { mColor = aColor; }

 private:
    Color mColor;
};

class DrawTrackedSerum : public DrawSerum
{
 public:
    inline DrawTrackedSerum() : mOutlineColor(BLACK) {}

    virtual inline size_t level() const { return 6; }
    virtual inline Color outline_color(const Point& /*aPoint*/, const PointStyle& /*aStyle*/, const SettingsAntigenicMaps& /*aSettings*/) const { return mOutlineColor; }

 private:
    Color mOutlineColor;
};

class DrawVaccineAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 9; }
};

class DrawMarkedAntigen : public DrawAntigen
{
 public:
    inline DrawMarkedAntigen(const SettingsMarkAntigen& aData) : mData(aData) {}

    virtual void draw(Surface& aSurface, const Point& aPoint, const PointStyle& aStyle, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 7; }

 private:
    SettingsMarkAntigen mData;
};

// ----------------------------------------------------------------------

// class VaccineData
// {
//  public:
//     inline VaccineData() : enabled(false), fill_color(0xFFC0CB), outline_color(0), aspect(0.5) {}

//     bool enabled;
//     Color fill_color;
//     Color outline_color;
//     double aspect;
// };

class PointAttributes
{
 public:
    inline PointAttributes() : antigen(true), egg(false), reassortant(false), reference(false), vaccine(false), homologous_antigen(-1) {}

    bool antigen;
    bool egg;
    bool reassortant;
    bool reference;
      // v1 VaccineData vaccine;
    bool vaccine;
    int homologous_antigen;
    std::string homologous_titer;
};

class Point
{
 public:
    inline Point() {}

    std::string name;
    Location coordinates;
    std::string lab_id;
    PointAttributes attributes;

}; // class Point

// ----------------------------------------------------------------------

class PointStyle
{
 public:
    inline PointStyle()
        : fill_color(TRANSPARENT), outline_color(BLACK), aspect(1), size(1), rotation(0) {}

    Color fill_color;
    Color outline_color;
    double aspect;
    std::string shape;
    double size;
    double rotation;

}; // class PointStyle

// ----------------------------------------------------------------------

class PlotStyle
{
 public:
    inline PlotStyle() {}

    std::vector<size_t> points;
    std::vector<PointStyle> styles;

    inline const PointStyle& style(size_t aPointNo) const { return styles[points[aPointNo]]; }

}; // class PlotStyle

// ----------------------------------------------------------------------

class ChartInfo
{
 public:
    inline ChartInfo() {}

    std::string date;
    std::string lab;
    std::string virus_type;
    std::string lineage;
    std::string name;
    std::string rbc_species;

}; // class ChartInfo

// ----------------------------------------------------------------------

inline double DrawPoint::aspect(const Point&, const PointStyle& aStyle, const SettingsAntigenicMaps&) const
{
    return aStyle.aspect;

} // DrawPoint::aspect

inline double DrawPoint::rotation(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const
{
    return aPoint.attributes.reassortant ? aSettings.reassortant_rotation : aStyle.rotation;

} // DrawPoint::rotation

inline Color DrawPoint::fill_color(const Point&, const PointStyle& aStyle, const SettingsAntigenicMaps&) const
{
    return aStyle.fill_color;

} // DrawPoint::fill_color

inline Color DrawPoint::outline_color(const Point&, const PointStyle& aStyle, const SettingsAntigenicMaps&) const
{
    return aStyle.outline_color;

} // DrawPoint::outline_color

inline double DrawAntigen::aspect(const Point& aPoint, const PointStyle& aStyle, const SettingsAntigenicMaps& aSettings) const
{
    return aPoint.attributes.egg ? aSettings.egg_antigen_aspect : aStyle.aspect;

} // DrawAntigen::aspect

// ----------------------------------------------------------------------

class Chart
{
 public:
    inline Chart() : mStress(-1) {}

    void preprocess(const SettingsAntigenicMaps& aSettings);
    void draw_points_reset(const SettingsAntigenicMaps& aSettings) const;

      // returns number of antigens from aNames list found in the chart
    size_t tracked_antigens(const std::vector<std::string>& aNames, Color aFillColor, const SettingsAntigenicMaps& aSettings) const;
      // returns line_no for each antigen from aLeaves found in the chart
    std::vector<size_t> sequenced_antigens(const std::vector<const Node*>& aLeaves);
    size_t marked_antigens(const SettingsMarkAntigens& aData, const std::vector<std::string>& aTrackedNames, size_t aSectionNo, const SettingsAntigenicMaps& aSettings) const;

    const Viewport& viewport() const { return mViewport; }
    void draw(Surface& aSurface, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;

    static Chart from_json(std::string data);

 private:
    double mStress;
    ChartInfo mInfo;
    std::vector<Point> mPoints;
    std::map<std::string, size_t> mPointByName;
    std::string mMinimumColumnBasis;
    std::vector<double> mColumnBases;
    Transformation mTransformation;
    std::vector<size_t> mDrawingOrder;
    PlotStyle mPlot;

    std::set<size_t> mSequencedAntigens;
    mutable std::vector<const DrawPoint*> mDrawPoints;
    DrawSerum mDrawSerum;
    DrawTrackedSerum mDrawTrackedSerum;
    DrawReferenceAntigen mDrawReferenceAntigen;
    DrawTestAntigen mDrawTestAntigen;
    DrawSequencedAntigen mDrawSequencedAntigen;
    mutable DrawTrackedAntigen mDrawTrackedAntigen;
    DrawVaccineAntigen mDrawVaccineAntigen;
    mutable std::vector<DrawMarkedAntigen> mDrawMarkedAntigens;

    Viewport mViewport;

    std::set<std::string> mPrefixName;

      // constexpr const char* SDB_VERSION = "acmacs-sdb-v2";
    std::string json_version;

    void apply_transformation(const SettingsAntigenicMaps& aSettings);
    Viewport bounding_rectangle() const;

    friend auto json_fields(Chart& a);

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
