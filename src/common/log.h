#pragma once

#include <cstdarg>
#include <cstdio>

namespace nds::log {

enum class Level { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

void set_level(Level level);
Level get_level();

void write(Level level, const char* category, const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 3, 4)))
#endif
    ;

}  // namespace nds::log

#define NDS_LOG_INFO(cat, ...)  ::nds::log::write(::nds::log::Level::Info,  cat, __VA_ARGS__)
#define NDS_LOG_WARN(cat, ...)  ::nds::log::write(::nds::log::Level::Warn,  cat, __VA_ARGS__)
#define NDS_LOG_ERROR(cat, ...) ::nds::log::write(::nds::log::Level::Error, cat, __VA_ARGS__)
#define NDS_LOG_DEBUG(cat, ...) ::nds::log::write(::nds::log::Level::Debug, cat, __VA_ARGS__)
