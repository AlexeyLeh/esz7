//
// Created by LAV on 09.08.2026.
//

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "dbg.h"

static pthread_mutex_t m_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char m_logfname[512];
static char *m_ext = NULL;
static char m_backup_fname[512+32];
static char m_fname_filter[256];
static int m_log_fd = -1;
static bool m_log_enabled = false;

//---------------------------------------------------------
const char *log_fname()
{
    return m_logfname;
}

//---------------------------------------------------------
inline void log_lock()
{
    pthread_mutex_lock(&m_log_mutex);
}
//---------------------------------------------------------
inline void log_unlock()
{
    pthread_mutex_unlock(&m_log_mutex);
}

//---------------------------------------------------------
inline bool log_enabled()
{
    return m_log_enabled;
}

//---------------------------------------------------------
void log_set_enabled(bool enabled)
{
    m_log_enabled = enabled;
}

//---------------------------------------------------------
void close_log()
{
    if (m_log_fd != -1) {
        close(m_log_fd);
        m_log_fd = -1;
    }
}

//---------------------------------------------------------
static const char *log_gettime(void)
{
    // Безопасный статический буфер (защищен внешним мьютексом)
    static char buf[36];
    struct timespec ts;

    // Если системные часы сбоят, пишем заглушку, но не падаем
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return "[TIME ERROR]";
    }

    // Внутри мьютекса стандартный localtime быстр и безопасен
    struct tm *tm_info = localtime(&ts.tv_sec);
    if (!tm_info) {
        return "[LOCALTIME ERROR]";
    }

    // Строка формата "YYYY-MM-DD HH:MM:SS" занимает ровно 19 символа + \0
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    return buf;
}

//---------------------------------------------------------
static int log_file_filter(const struct dirent *entry)
{
    return (strncmp(entry->d_name, m_fname_filter, strlen(m_fname_filter)) == 0);
}

//---------------------------------------------------------
static int log_file_sort_alphabetical(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

//---------------------------------------------------------
static void prune_old_logs(void)
{
    struct dirent **namelist;
    int n = scandir(".", &namelist, log_file_filter, log_file_sort_alphabetical);
    if (n < 0) {
        return;
    }
    if (n > LOG_MAX_FILES) {
        int files_to_delete = n - LOG_MAX_FILES;
        for (int i = 0; i < files_to_delete; i++) {
            unlink(namelist[i]->d_name);
        }
    }
    for (int i = 0; i < n; i++) {
        free(namelist[i]);
    }
    free((void *) namelist);
}

//---------------------------------------------------------
static void check_and_rotate(void)
{
    off_t size = lseek(m_log_fd, 0, SEEK_END);
    if (size < LOG_MAX_SIZE) {
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);

    char time_str[32];
    if (tm_info) {
        strftime(time_str, sizeof(time_str), "%Y%m%d-%H%M%S", tm_info);
    } else {
        snprintf(time_str, sizeof(time_str), "%ld", (long) ts.tv_sec);
    }

    char backup_filename[1024];
    snprintf(backup_filename, sizeof(backup_filename), "%s-%s.%s", m_backup_fname, time_str, m_ext);
    fdatasync(m_log_fd);
    close(m_log_fd);
    m_log_fd = -1;
    if (rename(m_logfname, backup_filename) == 0) {
        prune_old_logs();
    }

    m_log_fd = open(m_logfname, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (m_log_fd < 0) {
        m_log_fd = STDERR_FILENO;
    }
}

//---------------------------------------------------------
void log_open_file(const char *logpath)
{
    strncpy(m_logfname, logpath, sizeof(m_logfname) - 1);
    m_ext = strrchr(m_logfname, '.');
    if (m_ext) {
        m_ext++;
    }
    DBGF(1, "log fname: %s", m_logfname);
    DBGF(1, "log ext: %s", m_ext);

    strncpy(m_fname_filter, logpath, sizeof(m_fname_filter) - 1);
    char *p = strrchr(m_fname_filter, '.');
    if (p) {
        p++;
        *p = '\0';
    }
    DBGF(1, "fname filter: %s", m_fname_filter);

    strncpy(m_backup_fname, m_logfname, sizeof(m_backup_fname) - 1);
    p = strrchr(m_backup_fname, '.');
    if (p) {
        *p = '\0';
    }
    DBGF(1, "backup filename: %s", m_backup_fname);

    if (m_log_fd != -1) {
        close(m_log_fd);
        m_log_fd = -1;
    }
    m_log_fd = open(m_logfname, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (m_log_fd < 0) {
        m_log_fd = STDERR_FILENO;
    }
    write(m_log_fd, "\n", 1);
}

//---------------------------------------------------------
void log_message(log_type_t level, const char *msg)
{
    if (m_log_fd < 0) {
        return;
    }
    if (m_log_fd != STDERR_FILENO) {
        check_and_rotate();
    }

    const char *timestamp = log_gettime(); // Стандартный localtime безопасен внутри лока
    static char log_type[32];
    snprintf(log_type, sizeof(log_type), "%s%s%s%s",
             (level == INFO ? "LOG" : ""),
             (level == ERROR ? "ERROR" : ""),
             (level == CRITICAL ? "CRITICAL" : ""),
             (level == FATAL ? "FATAL" : ""));
    static char prefix[64];
    int len = snprintf(prefix, sizeof(prefix), "%s %-10s ", timestamp, log_type);
    if (len <= 0) {
        DBG(0, "505: INTERNAL ERROR: snprintf() failed");
        return;
    }
    if (len >= (int) sizeof(prefix)) {
        len = sizeof(prefix) - 1;
    }

    for (int i = 0; i < 2; i++) {
        const char *ptr = (i == 0 ? prefix : msg);
        size_t remaining = (i == 0 ? len : (int) strlen(msg));
        while (remaining > 0) {
            ssize_t written = write(m_log_fd, ptr, remaining);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            ptr += written;
            remaining -= written;
        }
    }
    write(m_log_fd, "\n", 1);

    // Синхронизация ext2 кэша для критических ошибок
    if (m_log_fd != STDERR_FILENO) {
        if (level == ERROR || level == CRITICAL || level == FATAL) {
            fdatasync(m_log_fd);
        }
    }
}
