#include <sstream>
#include "otl_utils.h"

otl_datetime OTL_Utils::Time_t_to_OTL_datetime(time_t timeT)
{
    otl_datetime otlDt;

    tm tmT ;
    localtime_r(&timeT, &tmT);
    otlDt.year = tmT.tm_year + 1900;
    otlDt.month = tmT.tm_mon + 1;
    otlDt.day = tmT.tm_mday;
    otlDt.hour = tmT.tm_hour;
    otlDt.minute = tmT.tm_min;
    otlDt.second = tmT.tm_sec;
    return otlDt;
}


std::string OTL_Utils::OtlExceptionToText(const otl_exception& otlEx)
{
    std::stringstream ss;
    ss << reinterpret_cast<const char*>(otlEx.msg) << std::endl
        << reinterpret_cast<const char*>(otlEx.stm_text) << std::endl
        << reinterpret_cast<const char*>(otlEx.var_info);
    return ss.str();
}

time_t OTL_Utils::OTL_datetime_to_Time_t(const otl_datetime& t)
{
    tm result;
    result.tm_year = t.year - 1900;
    result.tm_mon = t.month - 1;
    result.tm_mday = t.day;
    result.tm_hour = t.hour;
    result.tm_min = t.minute;
    result.tm_sec = t.second;
    result.tm_wday = 0; // not used
    result.tm_yday = 0; // not used
    result.tm_isdst = 0; //Daylight saving Time flag
    return mktime(&result);
}

