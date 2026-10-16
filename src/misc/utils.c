/**
  \file   utils.c
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Some helper tools
*/

#include <errno.h>
#include <fts.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>

#include "cfg.h"
#include "dbg.h"
#include "utils.h"

static thread_local char innerbuf[TIMEBUFLEN] = {0};
static thread_local struct timespec m_ts;

//---------------------------------------------------------
unsigned char check_byteorder()
{
    unsigned char order;
    unsigned int x = 1; // Assign a values where the least significant byte is 1
    char *c = (char *) &x; // Cast the address of x to a char pointer
    if (*c == 1) {
        order = 'L';
        DBG(2, "System is little-endian");
    } else {
        order = 'B';
        DBG(2, "System is big-endian");
    }
    return order;
}

//---------------------------------------------------------
void ltrim(char *str)
{
    char *p = str;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (p == str) {
        return;
    }
    size_t len = strlen(p) + 1;
    memmove(str, p, len);
    p = str + len;
    while (*p != 0) {
        *p = 0;
        p++;
    }
}

//---------------------------------------------------------
void rtrim(char *str)
{
    size_t len = strlen(str);
    char *p = str + len - 1;
    while (*p == ' ' || *p == '\t') {
        *p = '\0';
        --p;
    }
}

//---------------------------------------------------------
void alltrim(char *str)
{
    ltrim(str);
    rtrim(str);
}

//---------------------------------------------------------
/**
 * Converts channel number to hex-representation for device
 * @param channel - channel number (1..4)
 * @return hex-representation
 * @return (001 010 011 100)
 */
uint8_t channel_to_hex(int channel)
{
    if (channel < 1 || channel > 4) {
        channel = 1;
    }
    return (1 << (channel - 1));
}

//---------------------------------------------------------
/**
 * Converts channel number to hex-representation for device
 * @param channel - channel number (1..4)
 * @return hex-representation
 * @return (0001 0010 0100 1000)
 */
uint8_t channel_to_onehot(int channel)
{
    if (channel < 1 || channel > 4)
        channel = 1;
    return (1 << (channel - 1));
}

//---------------------------------------------------------
int hex_to_channel(uint8_t one_hot_value)
{
    if (one_hot_value == 1) {
        return 1;
    }
    if (one_hot_value == 2) {
        return 2;
    }
    if (one_hot_value == 4) {
        return 3;
    }
    if (one_hot_value == 8) {
        return 4;
    }
    return 0;
}

//---------------------------------------------------------
int nsleep(const long int nsec)
{
    const struct timespec *reqtime = &(struct timespec){.tv_sec = 0, .tv_nsec = nsec};
    int ret = nanosleep(reqtime, NULL);
    if (ret == -1) {
        const int e = errno;
        DBGF(0, "505: INTERNAL ERROR: %s:%d failed - (%d), %s", __func__, __LINE__, e, strerror(e));
        return -1;
    }
    return 0;
}

//---------------------------------------------------------
int ssleep(const long int sec)
{
    const struct timespec *reqtime = &(struct timespec){.tv_sec = sec, .tv_nsec = 0};
    int ret = nanosleep(reqtime, NULL);
    if (ret == -1) {
        const int e = errno;
        DBGF(0, "505: INTERNAL ERROR: %s:%d failed - (%d), %s", __func__, __LINE__, e, strerror(e));
        return -1;
    }
    return 0;
}

//---------------------------------------------------------
/**
 * Return the local date/time in mode '%Y-%m-%d %H:%M:%S'
 * @return string
 */
const char *get_date_time()
{
    char bf[32];
    struct tm tm = clock_get_local_time(&m_ts);
    strftime(bf, sizeof(bf), "%d.%m.%Y %H:%M:%S", &tm);
    snprintf(innerbuf, sizeof(innerbuf), "%s.%03ld", bf, m_ts.tv_nsec / 1000000);
    return innerbuf;
#if 0
    struct tm tm = {0};
    int err = 0;
    time_t tim;
    time(&tim);
    if (!local_time(&tim, &tm)) {
        //printf("%s(): localtime() failed\n", __func__);
        return "00:00:00";
    }
    int len = (int) strftime(innerbuf, 20, "%Y-%m-%d %H:%M:%S", &tm);
    if (len == 0) {
        return "00:00:00";
    }
    return innerbuf;
#endif
}

//---------------------------------------------------------
/// @brief Выдает локальное время (POSIX / Windows)
/// @note call tzset() before using
inline struct tm clock_get_local_time(struct timespec *ts)
{
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, ts);
    local_time(&ts->tv_sec, &tm);
    ts->tv_sec += tm.tm_gmtoff;
    return tm;
}

//---------------------------------------------------------
/**
 * The time in nanoseconds to time specificaton \n
 * (relative to the UNIX epoch starting in 1970)
 * @param[in] ns - nanoseconds
 * @param[out] ts - time specificaton
 */
inline void ns_to_timespec(uint64_t ns, struct timespec *ts)
{
    ts->tv_sec = (time_t) (ns / (uint64_t) 1000000000);
    ts->tv_nsec = (long) (ns - (uint64_t) (ts->tv_sec * 1000000000));
}

//---------------------------------------------------------
/**
 * The time specification to nanosecodes \n
 * (relative to the UNIX epoch starting in 1970)
 * @param ts - time specification
 * @return time in nanoseconds
 */
inline uint64_t timespec_to_ns(const struct timespec *ts)
{
    uint64_t ns = (uint64_t) ts->tv_sec * 1000000000;
    ns += (uint64_t) ts->tv_nsec;
    return ns;
}

//---------------------------------------------------------
/**
 * The time in nanoseconds to time string
 * @param ns - nanoseconds (relative to the UNIX epoch starting in 1970)
 * @return time string
 */
const char *ns_to_str(uint64_t ns)
{
    struct timespec ts;
    ns_to_timespec(ns, &ts);
    return timespec_to_str(&ts, innerbuf);
}

//---------------------------------------------------------
/**
 * The time specification to time string
 * @param ts  time specification
 * @param out if not null then contains the output string \n
 * (length must be TIMEBUFLEN) \n
 * (example: char innerbuf[TIMEBUFLEN]) \n
 * (maybe null)
 * @return time string
 * @return either inner 'innerbuf' or pointer to 'out'
 */
const char *timespec_to_str(const struct timespec *ts, char *out)
{
    char *ptr = out;
    if (!ptr) {
        ptr = innerbuf;
    }
    long secs = ts->tv_sec;
    long days = 0;
    days = secs / 86400;
    secs = secs % 86400;
    const long hours = secs / 3600;
    secs = secs % 3600;
    const long mins = secs / 60;
    secs = secs % 60;
    if (days == 0) {
        if (hours == 0) {
            if (mins == 0) {
                snprintf(ptr, TIMEBUFLEN, "%ld.%06ld", secs, ts->tv_nsec / 1000);
            } else {
                snprintf(ptr, TIMEBUFLEN, "%02ld:%02ld.%06ld", mins, secs, ts->tv_nsec / 1000);
            }
        } else {
            snprintf(ptr, TIMEBUFLEN, "%02ld:%02ld:%02ld.%06ld", hours, mins, secs, ts->tv_nsec / 1000);
        }
    } else {
        static struct tm tm;
        if (gmt_time((time_t *) &ts->tv_sec, &tm)) {
            char bf[32];
            strftime(bf, sizeof(bf), "%H:%M:%S", &tm);
            snprintf(ptr, TIMEBUFLEN, "%s.%06ld", bf, ts->tv_nsec / 1000);
        } else {
            snprintf(ptr, TIMEBUFLEN, "%02ld(days):%02ld:%02ld:%02ld.%06ld", days, hours, mins, secs,
                     ts->tv_nsec / 1000);
        }
    }
    return ptr;
}

//---------------------------------------------------------
const char *timespec_to_gmt_yymmdd_hhmmss_nz(const struct timespec *ts, int nz)
{
    static char bf[32];
    static struct tm gmtval;
    struct tm *tm = gmt_time(&ts->tv_sec, &gmtval);
    if (!tm) {
        return "00000000_000000.000";
    }
    strftime(bf, sizeof(bf), "%y%m%d_%H%M%S", tm);
    if (nz == 6) {
        snprintf(innerbuf, sizeof(innerbuf), "%s.%.06ld", bf, ts->tv_nsec / 1000);
    } else {
        snprintf(innerbuf, sizeof(innerbuf), "%s.%.03ld", bf, ts->tv_nsec / 1000000);
    }
    return innerbuf;
}

//---------------------------------------------------------
const char *timespec_to_yymmdd_hhmmss(const struct timespec *ts)
{
    return timespec_to_gmt_yymmdd_hhmmss_nz(ts, 0);
}

//---------------------------------------------------------
const char *timespec_to_yymmdd_hhmmss_zzz(const struct timespec *ts)
{
    return timespec_to_gmt_yymmdd_hhmmss_nz(ts, 3);
}

//---------------------------------------------------------
const char *timespec_to_yymmdd_hhmmss_zzzzzz(const struct timespec *ts)
{
    return timespec_to_gmt_yymmdd_hhmmss_nz(ts, 6);
}

//---------------------------------------------------------
/**
 * Converts 'timespec' to date/time string
 * @param ts - struct timespec
 * @param nz - fractional part of seconds (3 or 6)
 * @param[out] out - if not null then contains the output string \n
 * (length must be TIMEBUFLEN) \n
 * (example: char innerbuf[TIMEBUFLEN]) \n
 * (maybe null)
 * @return either inner 'innerbuf' or pointer to 'out'
 */
const char *timespec_to_dd_mm_yyyy_hh_mm_ss_nz(const struct timespec *ts, int nz, char *out)
{
    static struct tm tm;
    if (nz != 0 && nz != 3 && nz != 6) {
        nz = 3;
    }
    char *ptr = out;
    if (!ptr) {
        ptr = innerbuf;
    }
    if (gmt_time((time_t *) &ts->tv_sec, &tm)) {
        char bf[32];
        strftime(bf, sizeof(bf), "%d.%m.%Y %H:%M:%S", &tm);
        if (nz == 6) {
            sprintf(ptr, "%s.%.06ld", bf, ts->tv_nsec / 1000);
        } else if (nz == 3) {
            sprintf(ptr, "%s.%.03ld", bf, ts->tv_nsec / 1000000);
        } else {
            strncpy(ptr, bf, TIMEBUFLEN - 1);
        }
    } else {
        sprintf(ptr, "00.00.0000 00:00:00.000");
    }
    return ptr;
}

//---------------------------------------------------------
const char *timespec_to_hh_mm_ss_zzz(const struct timespec *ts)
{
    static struct tm tm;
    if (gmt_time((time_t *) &ts->tv_sec, &tm)) {
        char bf[32];
        strftime(bf, sizeof(bf), "%H:%M:%S", &tm);
        snprintf(innerbuf, sizeof(innerbuf), "%s.%03ld", bf, ts->tv_nsec / 1000000);
    } else {
        snprintf(innerbuf, sizeof(innerbuf), "00:00:00.000");
    }
    return innerbuf;
}

//---------------------------------------------------------
/**
 * The difference beetwen times specification
 *
 * @param[in] ts_diff - the time difference
 * @param[in] tsbeg - the start time
 * @param[in] tsend - the end time
 * @param[out] minus - the presence of minus
 * @return the time difference
 */
void timespec_diff(struct timespec *ts_diff, const struct timespec *tsbeg, const struct timespec *tsend, int *minus)
{
    if (minus) {
        *minus = 0;
    }
    ts_diff->tv_sec = tsend->tv_sec - tsbeg->tv_sec;
    ts_diff->tv_nsec = tsend->tv_nsec - tsbeg->tv_nsec;
    if (ts_diff->tv_sec == 0) {
        if (ts_diff->tv_nsec < 0) {
            ts_diff->tv_nsec *= -1;
            if (minus) {
                *minus = 1;
            }
        }
        return;
    }

    if (ts_diff->tv_sec > 0) {
        if (ts_diff->tv_nsec < 0) {
            ts_diff->tv_sec--;
            ts_diff->tv_nsec += 1000000000; // nsec/sec
        }
        return;
    }

    // if ts_diff->tv_sec < 0
    ts_diff->tv_sec *= -1;
    if (minus) {
        *minus = 1;
    }

    if (ts_diff->tv_nsec > 0) {
        ts_diff->tv_sec--;
        ts_diff->tv_nsec = 1000000000 - ts_diff->tv_nsec;
    } else if (ts_diff->tv_nsec < 0) {
        ts_diff->tv_nsec *= -1;
    }
}

//---------------------------------------------------------
bool test_diff_timespec()
{
    struct num_t
    {
        struct timespec t1, t2, tex;
        int minus;
    };
    struct num_t nums[] = {
        {{0, 500000000}, {1, 200000000}, {0, 700000000}, 1},
        {{0, 500000000}, {1, 800000000}, {1, 300000000}, 1},
        {{0, 300000000}, {1, 700000000}, {1, 400000000}, 1},
        {{0, 300000000}, {0, 500000000}, {0, 200000000}, 1},
        {{2, 300000000}, {5, 500000000}, {3, 200000000}, 1},
        {{2, 500000000}, {5, 300000000}, {2, 800000000}, 1},
        {{1, 500000000}, {2, 300000000}, {0, 800000000}, 1},
        {{1, 500000000}, {1, 300000000}, {0, 200000000}, 0},
        {{1, 300000000}, {1, 500000000}, {0, 200000000}, 1},
        {{2, 500000000}, {1, 800000000}, {0, 700000000}, 0},
        {{2, 500000000}, {1, 200000000}, {1, 300000000}, 0},
    };
    int numslen = sizeof(nums) / sizeof(nums[0]);
    int minus;
    bool good = OK;
    for (int i = 0; i < numslen; i++) {
        printf("+++ %ld.%.09ld - %ld.%.09ld = (%ld.%.09ld) = ",
               nums[i].t1.tv_sec, nums[i].t1.tv_nsec,
               nums[i].t2.tv_sec, nums[i].t2.tv_nsec,
               nums[i].tex.tv_sec, nums[i].tex.tv_nsec);
        struct timespec t;
        timespec_diff(&t, &nums[i].t2, &nums[i].t1, &minus);
        bool ok = (t.tv_sec == nums[i].tex.tv_sec && t.tv_nsec == nums[i].tex.tv_nsec && minus == nums[i].minus);
        printf("%1s%ld.%.09ld sec - %s\n", (minus ? "-" : ""), t.tv_sec, t.tv_nsec, (ok ? "OK" : "FAILED"));
        if (!ok) {
            good = FAIL;
        }
    }
    return good;
}

//---------------------------------------------------------
/// @brief Выдает локальное время (POSIX / Windows)
/// @note call tzset() before using
struct tm *local_time(const time_t *secs, struct tm *tm)
{
#ifdef __linux__
    if (!localtime_r(secs, tm)) {
        return NULL;
    }
#endif
#ifdef _WIN32
    if (localtime_s(tm, secs) != 0) {
        return NULL;
    }
#endif
    return tm;
}

//---------------------------------------------------------
struct tm *gmt_time(const time_t *secs, struct tm *tm)
{
#ifdef __linux__
    if (!gmtime_r(secs, tm)) {
        return NULL;
    }
#endif
#ifdef _WIN32
    if (gmtime_s(tm, secs) != 0) {
        return NULL;
    }
#endif
    return tm;
}

//---------------------------------------------------------
const char *secs_to_gmt_yymmdd_hhmmss(time_t secs)
{
    static struct tm gmtval;
    struct tm *tm = gmt_time(&secs, &gmtval);
    if (!tm) {
        return "000000_000000";
    }
    if (tm->tm_year + 1900 < 2000 || tm->tm_year + 1900 > 3000) {
        return "000000_000000";
    }
    strftime(innerbuf, sizeof(innerbuf), "%y%m%d_%H%M%S", tm);
    return innerbuf;
}

//---------------------------------------------------------
/**
 * @brief returns date string
 * @param secs - max_npnts seconds from start Unix time
 * @return always date string (no null anyway)
 */
const char *secs_to_gmt_ddmmyyyy_hhmmss(time_t secs, char *out)
{
    char *ptr = out;
    if (!ptr) {
        ptr = innerbuf;
    }
    time_t tt = secs;
    struct tm tm;
    if (!gmt_time(&tt, &tm)) {
        return "00.00.0000";
    }
    if (tm.tm_year < (2024 - 1900) || tm.tm_year > (3000 - 1900)) {
        return "00.00.0000";
    }
    strftime(ptr, TIMEBUFLEN, "%d.%m.%Y-%H:%M:%S", &tm);
    return ptr;
}

//---------------------------------------------------------
struct timespec *timespec_add(struct timespec *ts_sum, const struct timespec *ts_add)
{
    ts_sum->tv_sec += ts_add->tv_sec;
    ts_sum->tv_nsec += ts_add->tv_nsec;
    if (ts_sum->tv_nsec > 999999999) {
        ts_sum->tv_sec++;
        ts_sum->tv_nsec -= 999999999;
    }
    return ts_sum;
}

//---------------------------------------------------------
struct timespec *timespec_add_ns(struct timespec *ts, long nsecs_to_add)
{
    ts->tv_nsec += nsecs_to_add;
    if (ts->tv_nsec > 999999999) {
        ts->tv_sec++;
        ts->tv_nsec -= 999999999;
    }
#if 0
    uint64_t ns = (uint64_t) ts->tv_sec * 1000000000;
    ns += (uint64_t) ts->tv_nsec;
    ns += nsecs_to_add;
    ts->tv_sec = (time_t) (ns / (uint64_t) 1000000000);
    ts->tv_nsec = (int32_t) (ns - (ts->tv_sec * 1000000000));
#endif
    return ts;
}

//---------------------------------------------------------
struct timespec *timespec_add_ms(struct timespec *ts, long nsecs_to_add)
{
    return timespec_add_ns(ts, nsecs_to_add * 1000000);
}

//---------------------------------------------------------
#if 0
static char *rootdir = NULL;
static int rm_callback(const char *fpath, const struct stat *sb, int typeflag)
{
    int ret = 0;
    printf("-- %s\n", fpath);
    (void) sb;
    //(void)typeflag;
    if (typeflag == FTW_D || typeflag == FTW_DNR) {
        //int err = remove_dir(fpath);
        //return err;

        if (strchr(fpath, '/') == NULL && strchr(fpath, '\\') == NULL) {
            return 0;
        }
        rootdir
                ret = ftw(fpath, rm_callback, 1);
        return ret;
    }
    return 0;

    ret = remove(fpath);
    if (ret != 0) {
        perror(fpath);
        return ret;
    }
    return 0;
}

int remove_path(const char *dirname)
{
    if (strcmp(dirname, "/") == 0 || strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        return -1;
    }
    struct stat st;
    if (stat(dirname, &st) != 0 || (st.st_mode & S_IFMT) != S_IFDIR) {
        return -1;
    }
    rootdir = dirname;
    int err = ftw(dirname, rm_callback, 1);
    if (err) {
        return err;
    }
    err = remove(dirname);
    if (err) {
        perror(dirname);
        return err;
    }
    return 0;
}
#endif

//---------------------------------------------------------
int recursive_delete(const char *dirname)
{
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    // Cast needed (in C) because fts_open() takes a "char * const *", instead
    // of a "const char * const *", which is only allowed in C++. fts_open()
    // does not modify the argument.
    char *files[] = {(char *) dirname, NULL};

    // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
    //                in multithreaded programs
    // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
    //                of the specified directory
    // FTS_XDEV     - Don't cross filesystem boundaries
    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        fprintf(stderr, "%s(): ERROR: fts_open failed %s - %s\n", __func__, dirname, strerror(errno));
        return -1;
    }

    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
            case FTS_NS:
            case FTS_DNR:
            case FTS_ERR:
                fprintf(stderr, "%s(): ERROR: fts_read error %s - %s\n", __func__,
                        curr->fts_accpath, strerror(curr->fts_errno));
                break;

            case FTS_DC:
            case FTS_DOT:
            case FTS_NSOK:
                // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
                // passed to fts_open()
                break;

            case FTS_D:
                // Do nothing. Need depth-first search, so directories are deleted
                // in FTS_DP
                break;

            case FTS_DP:
            case FTS_F:
            case FTS_SL:
            case FTS_SLNONE:
            case FTS_DEFAULT:
                if (remove(curr->fts_accpath) < 0) {
                    fprintf(stderr, "%s(): ERROR: Failed to remove %s - %s\n", __func__,
                            curr->fts_path, strerror(curr->fts_errno));
                    ret = -1;
                }
                break;
        }
    }
    fts_close(ftsp);
    return ret;
}

//---------------------------------------------------------
bool remove_file(const char *filename)
{
    if (filename[0] == '/' && filename[1] == '\0') {
        fprintf(stderr, "%s(): ERROR: '%s' is invalid name.\n", __func__, filename);
        fprintf(stderr, "%s(): ERROR: absolute path starting with / or relative from current ./ is allowed\n",
                __func__);
        return FAIL;
    }
    if (filename[0] == '.' && filename[1] != '/') {
        fprintf(stderr, "%s(): ERROR: '%s' is invalid name.\n", __func__, filename);
        fprintf(stderr, "%s(): ERROR: absolute path starting with / or relative from current ./ is allowed\n",
                __func__);
        return FAIL;
    }
    if (filename[0] != '.' && filename[0] != '/') {
        fprintf(stderr, "%s(): ERROR: '%s' is invalid name.\n", __func__, filename);
        fprintf(stderr, "%s(): ERROR: absolute path starting with / or relative from current ./ is allowed\n",
                __func__);
        return FAIL;
    }

    int ret = recursive_delete(filename);
    return (ret == 0 ? OK : FAIL);
}

//---------------------------------------------------------
enum
{
    WINDOWS_TICK = 10000000,
    SEC_TO_UNIX_EPOCH = 11644473600
};

//---------------------------------------------------------
void windows_filetime_to_secs(uint64_t windows_ticks, uint32_t *unix_sec, uint32_t *unix_ns)
{
    if (!unix_sec) return;
    *unix_sec = (time_t) (windows_ticks / WINDOWS_TICK);
    *unix_sec -= SEC_TO_UNIX_EPOCH;
    if (unix_ns) {
        uint64_t num = *unix_sec + SEC_TO_UNIX_EPOCH;
        num *= WINDOWS_TICK;
        *unix_ns = (long) (windows_ticks - num);
        *unix_ns *= 100;
    }
}

//---------------------------------------------------------
/**
 * Just print obj in hex form
 * @param[in] data - byte sequence
 * @param[in] datalen - length of sequence
 */
void print_hexdump(const char *data, int datalen)
{
    if (!data) {
        DBGF(1, "obj: %p. skip", data);
        return;
    }

    printf("%s(): %d bytes:\n", __func__, datalen);
    printf("0   : ");
    for (int i = 0; i < datalen; i++) {
        if (i > 0) {
            if ((i % 32) == 0) {
                printf("\n%-4d: ", i);
            } else if ((i % 16) == 0) {
                printf("  ");
            }
        }
        printf("%02X ", (unsigned char) data[i]);
    }
    puts("");
#if 0
    for (int i = 0; i < datalen; i++) {
        if (i > 0 && (i % 80) == 0) {
            puts("");
        }
        if (isprint(obj[i])) {
            printf("%c", (unsigned char) obj[i]);
        } else {
            printf(".");
        }
    }
    puts("");
#endif
}

//---------------------------------------------------------
void print_uint32dump(const char *data, int datanum)
{
    if (!data) {
        DBGF(1, "obj: %p. skip", data);
        return;
    }

    uint32_t *ptr = (uint32_t *) data;
    printf("%s(): %d numbers:\n", __func__, datanum);
    printf("0   : ");
    for (int i = 0; i < datanum; i++, ptr++) {
        if (i > 0 && (i % 10) == 0) {
            printf("\n%-4d: ", i);
        }
        printf("%08X ", *ptr);
    }
    puts("");
}

//---------------------------------------------------------
double to_Fahrenheit(double celcium)
{
    double fahrenheit = (9.0 / 5.0) * celcium + 32;
    return fahrenheit;
}

//---------------------------------------------------------
double to_Celcium(double fahrenheit)
{
    double celcium = (5.0 / 9.0) * (fahrenheit - 32);
    return celcium;
}

//---------------------------------------------------------
void replace_dot_to_comma(decimal_delimiter_t delimiter, char *str)
{
    if (delimiter == DECIMAL_COMMA) {
        size_t sz = strlen(str);
        for (size_t i = 0; i < sz; i++) {
            if (str[i] == '.') {
                str[i] = ',';
            }
        }
    }
}

//---------------------------------------------------------
void replace_comma_to_dot(char *str)
{
    /// if (strncasecmp(cfg()->lang_cli, "ru", 2) == 0)
    {
        for (int i = 0; i < (int) strlen(str); i++) {
            if (str[i] == ',') {
                str[i] = '.';
            }
        }
    }
}

//---------------------------------------------------------
char *replace(char *str, const char from, const char to)
{
    char *p = str;
    while (*p) {
        if (*p == from) {
            *p = to;
        }
        p++;
    }
    return str;
}

//---------------------------------------------------------
int generate_uuid_getrandom(char *dst)
{
    unsigned char bytes[16];
    ssize_t res = getrandom(bytes, sizeof(bytes), 0);
    if (res < (ssize_t) sizeof(bytes)) {
        return -1; // Ошибка вызова или прерывание сигналом
    }

    // биты спецификации RFC 4122 v4
    bytes[6] = (bytes[6] & 0x0F) | 0x40; // Версия 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80; // Вариант

    sprintf(dst, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5],
            bytes[6], bytes[7],
            bytes[8], bytes[9],
            bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return 0;
}
