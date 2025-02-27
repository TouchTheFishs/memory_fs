#include <cinttypes>
#include <cstring>
#include <ctime>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>
#include <getopt.h>
#include <execinfo.h>
#include <vector>

#include "mem_fs_utils.h"

LogLevel current_log_level = LOG_LEVEL_NONE;

static __thread pid_t cached_tid = 0;

static pid_t get_tid()
{
	if (cached_tid == 0) {
		cached_tid = syscall(SYS_gettid);  // Linux 专用
	}
	return cached_tid;
}
void set_log_level(char* level)
{
	if (strcmp(level, "d") == 0) {
		current_log_level = LOG_LEVEL_DEBUG;
	} else if (strcmp(level, "i") == 0) {
		current_log_level = LOG_LEVEL_INFO;
	} else if (strcmp(level, "w") == 0) {
		current_log_level = LOG_LEVEL_WARNING;
	} else if (strcmp(level, "e") == 0) {
		current_log_level = LOG_LEVEL_ERROR;
	} else {
		current_log_level = LOG_LEVEL_NONE;
	}
}

const char* get_color(LogLevel level)
{
	switch (level) {
	case LOG_LEVEL_ERROR:
		return RED;
	case LOG_LEVEL_WARNING:
		return YELLOW;
	case LOG_LEVEL_INFO:
		return BLUE;
	case LOG_LEVEL_DEBUG:
		return WHITE;
	default:
		return RESET;
	}
}

static const char* extract_filename(const char* full_path)
{
	const char* last_slash = strrchr(full_path, '/');
	return (last_slash != NULL) ? last_slash + 1 : full_path;
}

void log_message(LogLevel level, const char* file, int line, const char* func, const char* format, ...)
{
	if (level < current_log_level) {
		return;
	}
	pid_t tid = get_tid();
	pid_t pid = getpid();
	const char* filename = extract_filename(file);
	const char* level_str = log_level_strings[level];  // 获取级别字符串

	time_t now = time(NULL);
	struct tm* tm_now = localtime(&now);
	char timestamp[20];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_now);
	const char* color = get_color(level);
	printf("%s[%s][%d %d][%s][%s:%d][%s] ", color, timestamp, tid, pid, level_str, filename, line, func);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf(RESET "\n");
}

// dump crash trace
#define MAX_STACK_FRAMES 64

void dump_backtrace()
{
	void* buffer[MAX_STACK_FRAMES];
	int frames = backtrace(buffer, MAX_STACK_FRAMES);
	char** symbols = backtrace_symbols(buffer, frames);

	fprintf(stderr, "\n调用栈（共 %d 帧）:\n", frames);
	for (int i = 0; i < frames; i++) {
		fprintf(stderr, "[%d] %s\n", i, symbols[i]);
	}

	free(symbols);
}

void signal_handler(int sig)
{
	fprintf(stderr, "\n程序崩溃，信号: %d\n", sig);
	dump_backtrace();
	exit(1);
}

void setup_signal_handlers()
{
	signal(SIGSEGV, signal_handler);
	signal(SIGABRT, signal_handler);
}

void crash_function()
{
	// 人为制造崩溃（解引用空指针）
	int* ptr = NULL;
	*ptr = 42;
}