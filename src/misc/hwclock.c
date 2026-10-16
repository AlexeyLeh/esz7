//
// Created by LAV on 15.06.2026.
//

#include "hwclock.h"
#include "dbg.h"
#include "log.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

bool hwclock()
{
    // 1. Открываем интерфейс внешнего RTC, созданный ядром
    int fd = open("/dev/rtc", O_WRONLY);
    if (fd == -1) {
        LOG(ERROR, "/dev/rtc not found");
        DBG(0, "ERROR: /dev/rtc not found");
        return false;
    }

    // 2. Получаем текущее системное время
    time_t now = time(NULL);
    if (now == (time_t) -1) {
        LOG(ERROR, "can't get system time");
        DBG(0, "ERROR: can't get system time");
        close(fd);
        return false;
    }

    // Преобразуем во время по Гринвичу (UTC), так как RTC обычно хранит UTC
    struct tm *sys_tm = gmtime(&now);
    if (sys_tm == NULL) {
        LOG(ERROR, "can't convert to UTC");
        close(fd);
        return false;
    }

    // 3. Заполняем структуру ядра для RTC
    struct rtc_time rtc_tm;
    memset(&rtc_tm, 0, sizeof(rtc_tm));

    rtc_tm.tm_sec = sys_tm->tm_sec;
    rtc_tm.tm_min = sys_tm->tm_min;
    rtc_tm.tm_hour = sys_tm->tm_hour;
    rtc_tm.tm_mday = sys_tm->tm_mday;
    rtc_tm.tm_mon = sys_tm->tm_mon;         // 0-11
    rtc_tm.tm_year = sys_tm->tm_year;       // Года с 1900

    // 4. Запись времени во внешний RTC по шине (I2C/SPI) через ioctl
    if (ioctl(fd, RTC_SET_TIME, &rtc_tm) == -1) {
        LOG(ERROR, "can't write time to external RTC");
        DBG(0, "ERROR: can't write time to external RTC");
        close(fd);
        return false;
    }

    close(fd);
    return true;
}
