#include "log.h"

#if LOG_ENABLE  // 总开关关闭时，整个log.c文件不参与编译

#include <string.h>
#include "bsp_log.h"

#if LOG_USE_RTOS
#include "osal.h"
#endif

static char buf[128];
static bsp_log_t *log = NULL;

#if LOG_USE_RTOS
static osal_mutex_t log_mutex = NULL;
#endif

/* 提取文件名，只保留最后一部分 */
static const char *filename_only(const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *p = slash > backslash ? slash : backslash;
    return p ? p + 1 : path;
}

/* 初始化日志系统 */
void log_init(void)
{
    log = bsp_log_get();
    log->ops->init(log);
    log->ops->printf(log, 
        "\r\n\r\n================ [System initializing...] ================\r\n\r\n"
    );

#if LOG_USE_RTOS
    log_mutex = osal_mutex_create("log_mutex");
#endif
}

/* 核心输出函数 */
void log_output(const char *level, const char *file, int line, const char *fmt, ...)
{
    if (!log)
        return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    buf[sizeof(buf) - 1] = '\0';

#if LOG_USE_RTOS
    osal_mutex_take(log_mutex, OSAL_WAIT_FOREVER);
#endif

    log->ops->printf(log, "%s %s:%d: %s\r\n", level, filename_only(file), line, buf);

#if LOG_USE_RTOS
    osal_mutex_release(log_mutex);
#endif
}

#endif  // LOG_ENABLE
