/**
  \file   utils.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Some helper tools
*/

#ifndef UTILS__H_
#define UTILS__H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "cfg.h"

#define TIMEBUFLEN 64

/*
 * NOTES: TODO for Windows
 struct timespec { long tv_sec; long tv_nsec; };    //header part
int clock_gettime(int, struct timespec *spec)      //C-file part
{  __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -=116444736000000000i64;  //1jan1601 to 1jan1970
   spec->tv_sec  =wintime / 10000000i64;           //seconds
   spec->tv_nsec =wintime % 10000000i64 *100;      //nano-seconds
   return 0;
}
 */

void ltrim(char *str);
void rtrim(char *str);
void alltrim(char *str);

uint8_t channel_to_hex(int channel);
int hex_to_channel(uint8_t one_hot_value);

void print_percent_line(int percent);
bool make_file_backup(const char *filename);

struct tm clock_get_local_time(struct timespec *ts);

void ns_to_timespec(uint64_t ns, struct timespec *ts);
uint64_t timespec_to_ns(const struct timespec *ts);

const char *timespec_to_str(const struct timespec *ts, char *out);
void timespec_diff(struct timespec *ts_diff, const struct timespec *tsbeg, const struct timespec *tsend, int *minus);
bool test_diff_timespec();

struct timespec * timespec_add(struct timespec *ts_sum, const struct timespec *ts_add);
struct timespec *timespec_add_ms(struct timespec *ts, long nsecs_to_add);
struct timespec *timespec_add_ns(struct timespec *ts, long nsecs_to_add);
const char *timespec_to_yymmdd_hhmmss_zzzzzz(const struct timespec *ts);
const char *timespec_to_yymmdd_hhmmss_zzz(const struct timespec *ts);
const char *timespec_to_yymmdd_hhmmss(const struct timespec *ts);

const char *timespec_to_gmt_yymmdd_hhmmss_nz(const struct timespec *ts, int nz);
const char *timespec_to_dd_mm_yyyy_hh_mm_ss_nz(const struct timespec *ts, int nz, char *out);
const char *timespec_to_hh_mm_ss_zzz(const struct timespec *ts);

struct tm *gmt_time(const time_t *secs, struct tm *tm);
struct tm *local_time(const time_t *secs, struct tm *tm);
const char *secs_to_gmt_yymmdd_hhmmss(time_t secs);
const char *secs_to_gmt_ddmmyyyy_hhmmss(time_t secs, char *out);
const char *get_date_time();
int nsleep(long int nsec);
int ssleep(long int sec);

bool remove_file(const char *filename);
void print_hexdump(const char *data, int datalen);
void print_uint32dump(const char *data, int datanum);
void windows_filetime_to_secs(uint64_t windows_ticks, uint32_t *unix_sec, uint32_t *unix_ns);

double to_Fahrenheit(double temperature);
unsigned char check_byteorder();

void replace_dot_to_comma(decimal_delimiter_t delimiter, char *str);
void replace_comma_to_dot(char *str);
char *replace(char *str, char from, char to);

uint8_t channel_to_onehot(int channel);

int generate_uuid_getrandom(char *dst);

#endif //UTILS__H_
