#include "log.h"

#if LOG_ENABLE  // 总开关关闭时，整个 log.c 文件不参与编译

#include <string.h>
#include <stdarg.h>
#include "bsp_console.h"

#if LOG_USE_RTOS
#include "osal.h"
#endif

static bsp_console_t *console = NULL;

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
    console = bsp_console_get();
    console->ops->init(console);
    console->ops->printf(console, 
        "\r\n\r\n================ [System initializing...] ================\r\n\r\n"
    );

#if LOG_USE_RTOS
    log_mutex = osal_mutex_create("log_mutex");
#endif
}

/* 核心输出函数 */
void log_output(const char *level, const char *file, int line, const char *fmt, ...)
{
    if (!console)
        return;

#if LOG_USE_RTOS
    osal_mutex_take(log_mutex, OSAL_WAIT_FOREVER);
#endif

    console->ops->printf(console, "%s %s:%d: ", level, filename_only(file), line);

    va_list args;
    va_start(args, fmt);
    if (console->ops && console->ops->vprintf)
        console->ops->vprintf(console, fmt, args);
    va_end(args);

    console->ops->printf(console, "\r\n");

#if LOG_USE_RTOS
    osal_mutex_release(log_mutex);
#endif
}

#endif  /* LOG_ENABLE */
