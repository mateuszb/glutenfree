#include <util/Time.hpp>

using namespace std;
using namespace util;

const tm util::GetCurrentDate()
{
    tm currentDate;
    const auto now = chrono::system_clock::now();
    const auto currentTimestamp = chrono::system_clock::to_time_t(now);
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&currentDate, &currentTimestamp);
#else
    gmtime_r(&currentTimestamp, &currentDate);
#endif
    return currentDate;
}

const tm util::GetCurrentDateRoundedDownToHour()
{
    tm currentDate;
    const auto now = chrono::system_clock::now();
    const auto secondsPerHour = 60 * 60;
    const auto currentTimestamp =
	(chrono::system_clock::to_time_t(now) / secondsPerHour)
	* secondsPerHour;
    
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&currentDate, &currentTimestamp);
#else
    gmtime_r(&currentTimestamp, &currentDate);
#endif
    return currentDate;
}

const tm util::ConvertToDate(const time_t timepoint)
{
    std::tm date;
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&date, &timepoint);
#else
    gmtime_r(&timepoint, &date);
#endif
    return date;
}
