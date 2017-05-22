#pragma once

#include <ctime>
#include <chrono>

namespace util
{
const std::tm GetCurrentDate();
const std::tm GetCurrentDateRoundedDownToHour();
const std::tm ConvertToDate(const time_t timepoint);
}
