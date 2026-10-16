/**
  \file   m_epl.h
  \author Aleksey Lekhtin
  \date   22.07.2024
  \brief   I/O event notification facility 
 */

#ifndef EPOLL__H_
#define EPOLL__H_

#include <stdbool.h>
#include <sys/epoll.h>

#define ADC_EVENT_FILE1 "/sys/devices/soc0/amba/43c00000.esz7/event1"
#define ADC_EVENT_FILE2 "/sys/devices/soc0/amba/43c00000.esz7/event2"
#define ADC_EVENT_FILE3 "/sys/devices/soc0/amba/43c00000.esz7/event3"
#define ADC_EVENT_FILE4 "/sys/devices/soc0/amba/43c00000.esz7/event4"

#define EVENTS_EXPECTED 4

typedef struct epol_file_t
{
    int channel;
    int file_fd;
    const char *fname;
} epol_file_t;

typedef struct epol_t
{
    int number_of_epol_files;
    epol_file_t file[4];

    int fd;
} epol_t;

epol_t *epl();
bool epl_open();
void epl_close();
bool epl_checkin();

int epl_find_event_file(int event_fd);
bool epl_read_event_file(int event_fd);
bool epl_write_event_files();
const char *epl_event_name(const struct epoll_event *event);

#endif
