#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/* ================= 总开关控制 ================= */
#define LOG_ENABLE 1  // 1:开启日志系统 0:关闭所有日志

#if LOG_ENABLE  // 总开关开启时才编译后续日志相关代码

/* ================= 细分级别开关 ================= */
#define LOG_ENABLE_ERROR    1
#define LOG_ENABLE_WARN     1
#define LOG_ENABLE_INFO     1
#define LOG_ENABLE_DEBUG    1

/* 打开 RTOS 支持互斥量，0 = 不使用 RTOS 互斥量 */
#define LOG_USE_RTOS 0

/* 初始化日志系统 */
void log_init(void);

/* 内部核心输出函数，不直接调用 */
void log_output(const char *level, const char *file, int line, const char *fmt, ...);

/* ================= 日志接口 ================= */
#if LOG_ENABLE_ERROR
#define log_error(fmt, ...) log_output("[ERROR]", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define log_error(fmt, ...) do {} while(0)
#endif

#if LOG_ENABLE_WARN
#define log_warn(fmt, ...)  log_output("[WARN] ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define log_warn(fmt, ...) do {} while(0)
#endif

#if LOG_ENABLE_INFO
#define log_info(fmt, ...)  log_output("[INFO] ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define log_info(fmt, ...) do {} while(0)
#endif

#if LOG_ENABLE_DEBUG
#define log_debug(fmt, ...) log_output("[DEBUG]", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...) do {} while(0)
#endif

#else  // 总开关关闭时，所有日志接口为空实现

#define log_init() do {} while(0)
#define log_error(fmt, ...) do {} while(0)
#define log_warn(fmt, ...) do {} while(0)
#define log_info(fmt, ...) do {} while(0)
#define log_debug(fmt, ...) do {} while(0)

#endif  // LOG_ENABLE

#endif  // LOG_H
