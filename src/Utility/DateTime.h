#pragma once

namespace DateTime
{
    enum class Format
    {
        ISO,
        Local,
        Custom
    };

	time_t now();
    time_t toLocalTime(time_t time_utc);
    time_t toUniversalTime(time_t time_local);
    string toString(time_t time, Format format = Format::ISO, string_view custom_format = {});
}
