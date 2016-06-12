#pragma once

#include <ctime>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>

// ----------------------------------------------------------------------

class Date
{
 public:
    inline Date() { reset(); }
    inline Date(std::string aText) : Date() { parse(aText); }
      // inline Date& operator =(const Date&) = default;
    inline Date& operator =(std::string aText) { if (!aText.empty()) parse(aText); return *this; }

    inline bool empty() const { return mTime.tm_year == 0; }
    inline int year() const { return mTime.tm_year; }
    inline int month() const { return mTime.tm_mon; }
    inline int day() const { return mTime.tm_mday; }

    inline void reset()
        {
            mTime.tm_sec = mTime.tm_min = mTime.tm_hour = 0;
            mTime.tm_year = mTime.tm_mon = 0;
            mTime.tm_mday = 1;
        }

    inline void parse(std::string aText)
        {
            if (aText.size() == 10) {
                strptime(aText.c_str(), "%Y-%m-%d", &mTime);
            }
            else if (aText.size() == 7) {
                strptime(aText.c_str(), "%Y-%m", &mTime);
            }
            else if (aText.size() == 0) {
                reset();
            }
            else {
                throw std::runtime_error(std::string("cannot parse date from ") + aText);
            }
        }

    inline std::string display() const
        {
            char buf[16];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &mTime);
            return buf;
        }

    inline operator std::string() const { return display(); }

    // inline std::string month_year() const
    //     {
    //         char buf[8];
    //         std::strftime(buf, sizeof(buf), "%b %y", &mTime);
    //         return buf;
    //     }

    // if this different from a date that was reset
    // inline operator bool() const { return *this != Date(); }

    inline std::string month_3() const
        {
            char buf[4];
            std::strftime(buf, sizeof(buf), "%b", &mTime);
            return buf;
        }

    inline std::string year_2() const
        {
            char buf[3];
            std::strftime(buf, sizeof(buf), "%y", &mTime);
            return buf;
        }

    inline void assign_and_remove_day(const Date& d)
        {
            *this = d;
            mTime.tm_mday = 1;
        }

    inline Date remove_day() const
        {
            Date r;
            r.assign_and_remove_day(*this);
            return r;
        }

    inline void assign_and_subtract_months(const Date& d, size_t months)
        {
            *this = d;
            int full_years = static_cast<int>(months / 12);
            int m = static_cast<int>(months % 12);
            if (m > mTime.tm_mon) {
                ++full_years;
                m -= 12;
            }
            mTime.tm_mon -= m;
            mTime.tm_year -= full_years;
        }

    inline void increment_month()
        {
            if (mTime.tm_mon < 11) {
                ++mTime.tm_mon;
            }
            else {
                mTime.tm_mon = 0;
                ++mTime.tm_year;
            }
        }

    inline void decrement_month()
        {
            if (mTime.tm_mon > 0) {
                --mTime.tm_mon;
            }
            else {
                mTime.tm_mon = 11;
                --mTime.tm_year;
            }
        }

    inline bool operator < (const Date& d) const
        {
              // return std::mktime(const_cast<std::tm*>(&mTime)) < std::mktime(const_cast<std::tm*>(&d.mTime));
            return year() == d.year() ? (month() == d.month() ? day() < d.day() : month() < d.month()) : year() < d.year();
        }

    inline bool operator == (const Date& d) const
        {
            return year() == d.year() && month() == d.month() && day() == d.day();
        }

 private:
    std::tm mTime;

}; // class Date

// ----------------------------------------------------------------------

inline std::ostream& operator << (std::ostream& out, const Date& aDate)
{
    return out << aDate.display();
}

// ----------------------------------------------------------------------

// returns negative if b is earlier than a
inline int months_between_dates(const Date& a, const Date& b)
{
    int months = 0;
    if (b < a) {
        months = - months_between_dates(b, a);
    }
    else {
        if (a.year() == b.year()) {
            months = b.month() - a.month();
        }
        else {
            months = 12 - a.month() + b.month() + (b.year() - a.year() - 1) * 12;
        }
    }
    return months;
}

inline int months_between_dates(const std::pair<Date, Date>& aDates)
{
    return months_between_dates(aDates.first, aDates.second);
}

// ----------------------------------------------------------------------
