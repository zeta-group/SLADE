
#include "Main.h"
#include "DateTime.h"
#include "thirdparty/fmt/fmt/time.h"
#include <ctime>
#include <chrono>

time_t DateTime::now()
{
	using clock = std::chrono::system_clock;
	return clock::to_time_t(clock::now());
}

time_t DateTime::toLocalTime(time_t time_utc)
{
//#ifdef WIN32
//	tm local{};
//	localtime_s(&local, &time_utc);
//	return mktime(&local);
//#else
	return mktime(std::localtime(&time_utc));
//#endif
}

time_t DateTime::toUniversalTime(time_t time_local)
{
//#ifdef WIN32
//	tm local{};
//	gmtime_s(&local, &time_local);
//	return mktime(&local);
//#else
	return mktime(std::gmtime(&time_local));
//#endif
}

string DateTime::toString(time_t time, DateTime::Format format, string_view custom_format)
{
//#ifdef WIN32
//    tm as_tm{time};
//#else
	auto as_tm = *std::localtime(&time);
//#endif
    switch (format)
    {
    case DateTime::Format::ISO: return fmt::format("{:%F %T}", as_tm);
    case DateTime::Format::Local: return fmt::format("{:%c}", as_tm);
    case DateTime::Format::Custom: return fmt::format(fmt::format("{{:{}}}", custom_format), as_tm);
    }

	return {};
}
