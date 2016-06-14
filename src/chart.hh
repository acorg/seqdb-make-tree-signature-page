#pragma once

#include <set>

#include "draw.hh"

// ----------------------------------------------------------------------

class Point;
class SettingsAntigenicMaps;
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
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const = 0;
    virtual size_t level() const = 0;
    virtual inline double aspect(const Point&, const SettingsAntigenicMaps&) const { return 1.0; }
    virtual double rotation(const Point& aPoint, const SettingsAntigenicMaps& aSettings) const;
};

class DrawSerum : public DrawPoint
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 1; }
};

class DrawAntigen : public DrawPoint
{
 public:
    virtual double aspect(const Point& aPoint, const SettingsAntigenicMaps& aSettings) const;
};

class DrawReferenceAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 2; }
};

class DrawTestAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 3; }
};

class DrawSequencedAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 4; }
};

class DrawTrackedAntigen : public DrawAntigen
{
 public:
    inline DrawTrackedAntigen() : mColor(0xFFC0CB) {}

    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 5; }
    inline void color(Color aColor) { mColor = aColor; }

 private:
    Color mColor;
};

class DrawVaccineAntigen : public DrawAntigen
{
 public:
    virtual void draw(Surface& aSurface, const Point& aPoint, double aObjectScale, const SettingsAntigenicMaps& aSettings) const;
    virtual inline size_t level() const { return 9; }
};

// ----------------------------------------------------------------------

class VaccineData
{
 public:
    inline VaccineData() : enabled(false), fill_color(0xFFC0CB), outline_color(0), aspect(0.5) {}

    bool enabled;
    Color fill_color;
    Color outline_color;
    double aspect;
};

class PointAttributes
{
 public:
    inline PointAttributes() : antigen(true), egg(false), reassortant(false), reference(false) {}

    bool antigen;
    bool egg;
    bool reassortant;
    bool reference;
    VaccineData vaccine;
};

class Point
{
 public:
    inline Point () {}

    std::string name;
    Location coordinates;
    std::string lab_id;
    PointAttributes attributes;

}; // class Point

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

    friend inline auto json_fields(ChartInfo& a)
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

}; // class ChartInfo

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

    std::set<size_t> mSequencedAntigens;
    mutable std::vector<const DrawPoint*> mDrawPoints;
    DrawSerum mDrawSerum;
    DrawReferenceAntigen mDrawReferenceAntigen;
    DrawTestAntigen mDrawTestAntigen;
    DrawSequencedAntigen mDrawSequencedAntigen;
    mutable DrawTrackedAntigen mDrawTrackedAntigen;
    DrawVaccineAntigen mDrawVaccineAntigen;

    Viewport mViewport;

    std::set<std::string> mPrefixName;

      // constexpr const char* SDB_VERSION = "acmacs-sdb-v1";
    std::string json_version;

    friend inline auto json_fields(Chart& a)
        {
            return std::make_tuple(
                "  version", &a.json_version,
                " created", json::comment(""),
                "?points", json::comment(""),
                "info", &a.mInfo,
                "minimum_column_basis", &a.mMinimumColumnBasis,
                "points", &a.mPoints,
                "stress", &a.mStress,
                "column_bases", &a.mColumnBases
                                   );
        }

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
