#ifdef TEST_BUILD

#include <cstdio>
#include <cstdarg>

namespace esphome {

// Mock Log implementation
class Logger {
public:
    static void log(const char* tag, const char* format, ...) {
        va_list args;
        va_start(args, format);
        printf("[%s] ", tag);
        vprintf(format, args);
        printf("\n");
        va_end(args);
    }
};

} // namespace esphome

// Mock log macros
#define ESP_LOGD(tag, ...) esphome::Logger::log(tag, __VA_ARGS__)
#define ESP_LOGI(tag, ...) esphome::Logger::log(tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) esphome::Logger::log(tag, __VA_ARGS__)
#define ESP_LOGE(tag, ...) esphome::Logger::log(tag, __VA_ARGS__)

#endif // TEST_BUILD
