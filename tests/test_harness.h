#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace nds::testing {

struct TestCase {
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Register {
    Register(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline int g_failures = 0;
inline const char* g_current_test = "?";

inline void report_failure(const char* file, int line, const std::string& expr,
                           const std::string& detail) {
    g_failures++;
    std::fprintf(stderr, "[FAIL] %s\n  %s:%d  %s\n  %s\n",
                 g_current_test, file, line, expr.c_str(), detail.c_str());
}

inline int run_all() {
    int total = 0, failed = 0;
    for (const auto& tc : registry()) {
        g_current_test = tc.name;
        const int before = g_failures;
        tc.fn();
        const bool ok = g_failures == before;
        std::fprintf(stdout, "[%s] %s\n", ok ? " OK " : "FAIL", tc.name);
        ++total;
        if (!ok) ++failed;
    }
    std::fprintf(stdout, "\n%d/%d tests passed\n", total - failed, total);
    return failed == 0 ? 0 : 1;
}

}  // namespace nds::testing

#define NDS_TEST(name) \
    static void name(); \
    static ::nds::testing::Register _reg_##name(#name, name); \
    static void name()

#define EXPECT_EQ(a, b) do {                                                  \
    auto _a = (a); auto _b = (b);                                             \
    if (!(_a == _b)) {                                                        \
        char buf[256];                                                        \
        std::snprintf(buf, sizeof(buf),                                       \
            "got 0x%llX, expected 0x%llX",                                    \
            static_cast<unsigned long long>(_a),                              \
            static_cast<unsigned long long>(_b));                             \
        ::nds::testing::report_failure(__FILE__, __LINE__,                    \
            "EXPECT_EQ(" #a ", " #b ")", buf);                                \
    }                                                                         \
} while (0)

#define EXPECT_TRUE(a) do {                                                   \
    if (!(a)) {                                                               \
        ::nds::testing::report_failure(__FILE__, __LINE__,                    \
            "EXPECT_TRUE(" #a ")", "value was false");                        \
    }                                                                         \
} while (0)

#define EXPECT_FALSE(a) do {                                                  \
    if ((a)) {                                                                \
        ::nds::testing::report_failure(__FILE__, __LINE__,                    \
            "EXPECT_FALSE(" #a ")", "value was true");                        \
    }                                                                         \
} while (0)
