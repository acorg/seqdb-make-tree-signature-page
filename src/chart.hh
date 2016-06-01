#pragma once

#include <set>

#include "json-read.hh"
#include "draw.hh"

// ----------------------------------------------------------------------

class Point;
class SettingsAntigenicMaps;

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

class Point
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(Point& aPoint) : mPoint(aPoint) {}
            axe::result<std::string::iterator> operator()(std::string::iterator i1, std::string::iterator i2) const;
          private:
            Point& mPoint;
        };

 public:
    inline Point () : antigen(true), egg(false), reassortant(false), reference(false), vaccine(false), vaccine_fill_color(0xFFC0CB), vaccine_outline_color(0), vaccine_aspect(0.5) {}

    std::string name;
    Location coordinates;
    std::string lab_id;
    bool antigen;
    bool egg;
    bool reassortant;
    bool reference;
    bool vaccine;
    Color vaccine_fill_color;
    Color vaccine_outline_color;
    double vaccine_aspect;

    inline auto json_parser() { return json_parser_t(*this); }

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
    void sequenced_antigens(const std::vector<std::string>& aNames);

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

}; // class Chart

// ----------------------------------------------------------------------

Chart import_chart(std::string buffer);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
