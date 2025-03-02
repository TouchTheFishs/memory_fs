#include <cstring>
#include <ctime>
#include <dlfcn.h>
#include <cxxabi.h>
#include <libgen.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>
#include <getopt.h>
#include <execinfo.h>

#include "log_utils.h"

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

#define MAX_STACK_FRAMES 64

void dump_backtrace()
{
	void* buffer[MAX_STACK_FRAMES];
	const int frames = backtrace(buffer, MAX_STACK_FRAMES);
	const pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
	fprintf(stderr, "\n[线程 %d] 调用栈追踪 (共 %d 帧):\n", tid, frames);

	for (int i = 0; i < frames; ++i) {
		Dl_info info = {};
		const bool dladdr_ok = dladdr(buffer[i], &info);

		fprintf(stderr, "#%-2d ", i);  // 序号与内容间留1空格

		if (dladdr_ok && info.dli_fname) {
			// 精简路径处理
			char* fname_copy = strdup(info.dli_fname);
			const char* base_name = basename(fname_copy);

			// 计算相对偏移
			const size_t offset = reinterpret_cast<size_t>(buffer[i]) - reinterpret_cast<size_t>(info.dli_fbase);

			fprintf(stderr, "%-16s 0x%012zX", base_name, reinterpret_cast<size_t>(info.dli_fbase));

			// 显示紧凑的偏移量
			fprintf(stderr, "+0x%-7zx ", offset);

			free(fname_copy);

			// 符号解析
			const char* symbol = "<未知符号>";
			char* demangled = nullptr;
			if (info.dli_sname) {
				int status = 0;
				demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
				symbol = (status == 0) ? demangled : info.dli_sname;
			}
			fprintf(stderr, "@ %-20s", symbol);

			// 使用 addr2line 解析行号信息
			char cmd[1024];
			snprintf(cmd, sizeof(cmd), "addr2line -e '%s' -pfC %zx 2>/dev/null", info.dli_fname, offset);

			if (FILE* fp = popen(cmd, "r")) {
				char line[512] = "";
				if (fgets(line, sizeof(line), fp) && !strstr(line, "??")) {
					line[strcspn(line, "\n")] = '\0';				// 去除换行符
					fprintf(stderr, " \033[33m(%s)\033[0m", line);	// 黄色高亮
				}
				pclose(fp);
			}

			if (demangled)
				free(demangled);

			// 显示符号地址
			if (info.dli_saddr) {
				fprintf(stderr, " [sym:0x%012zX]", reinterpret_cast<size_t>(info.dli_saddr));
			}
		} else {
			// 无法解析时显示原始地址
			fprintf(stderr, "<无法解析> [0x%14p]", buffer[i]);
		}

		fprintf(stderr, "\n");
	}
	fflush(stderr);
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