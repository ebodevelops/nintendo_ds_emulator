#include "common/log.h"

namespace nds::log {

namespace {
Level g_level = Level::Info;

const char* level_tag(Level l) {
    switch (l) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
    }
    return "?????";
}
}  // namespace

void set_level(Level level) { g_level = level; }
Level get_level() { return g_level; }

void write(Level level, const char* category, const char* fmt, ...) {
    if (static_cast<int>(level) < static_cast<int>(g_level)) return;

    std::FILE* out = (static_cast<int>(level) >= static_cast<int>(Level::Warn)) ? stderr : stdout;
    std::fprintf(out, "[%s][%s] ", level_tag(level), category ? category : "");

    std::va_list ap;
    va_start(ap, fmt);
    std::vfprintf(out, fmt, ap);
    va_end(ap);

    std::fputc('\n', out);
    std::fflush(out);
}

}  // namespace nds::log
