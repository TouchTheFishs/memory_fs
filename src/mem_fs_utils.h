#ifndef MEM_FS_UTILS_H
#define MEM_FS_UTILS_H
#define RED "\033[31m"	   // ERROR
#define YELLOW "\033[33m"  // WARNING
#define BLUE "\033[32m"	   // INFO
#define WHITE "\033[37m"   // DEBUG
#define RESET "\033[0m"	   // RESET
static const char* log_level_strings[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
typedef enum { LOG_LEVEL_DEBUG = 0, LOG_LEVEL_INFO, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR, LOG_LEVEL_NONE } LogLevel;
void log_message(LogLevel level, const char* file, int line, const char* func, const char* format, ...);
#define LOGD(...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGI(...) log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGW(...) log_message(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOGE(...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
extern LogLevel current_log_level;
void set_log_level(char* level);
void setup_signal_handlers();
void handleOption(int& argc, char**& argv);

#endif	// MEM_FS_UTILS_H