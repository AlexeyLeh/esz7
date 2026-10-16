/**
  \file   epl.c
  \author Aleksey Lekhtin
  \date   22.07.2024
  \brief   I/O event notification facility 
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "cfg.h"
#include "dbg.h"
#include "epl.h"
#include "log.h"

static epol_t m_epl = {
        .number_of_epol_files = sizeof(m_epl.file) / sizeof(m_epl.file[0]),
        .fd = -1,
        .file = {
                {.fname = ADC_EVENT_FILE1, .file_fd = -1},
                {.fname = ADC_EVENT_FILE2, .file_fd = -1},
                {.fname = ADC_EVENT_FILE3, .file_fd = -1},
                {.fname = ADC_EVENT_FILE4, .file_fd = -1}
        },
};

static pthread_mutex_t epl_mutex;
static void epl__close();

//---------------------------------------------------------
inline epol_t *epl()
{
    return &m_epl;
}

//---------------------------------------------------------
static void epl__close()
{
    DBG(8);
    int err;
    epl_write_event_files();
    DBG(8, "close event files");
    for (int i = 0; i < m_epl.number_of_epol_files; i++) {
        DBGF(8, "close epoll event file[%d]: %d%s", i,
             m_epl.file[i].file_fd, (m_epl.file[i].file_fd == -1 ? " - skip" : ""));
        if (m_epl.file[i].file_fd > -1) {
            err = close(m_epl.file[i].file_fd);
            if (err) {
                int e = errno;
                LOG(ERROR, "event fd[%d]: %d. err: %d - %s", i, m_epl.file[i].file_fd, e, strerror(e));
                DBGF(0, "ERROR: event fd[%d]: %d. err: %d - %s", i, m_epl.file[i].file_fd, e, strerror(e));
            }
        }
        m_epl.file[i].file_fd = -1;
    }

    DBGF(8, "close epoll fd:%d%s", m_epl.fd, (m_epl.fd == -1 ? " - skip" : ""));
    if (m_epl.fd > -1) {
        err = close(m_epl.fd);
        if (err) {
            int e = errno;
            LOG(ERROR, "epoll fd:%d. err: %d - %s", m_epl.fd, e, strerror(e));
            DBGF(0, "ERROR: epoll fd:%d. err: %d - %s", m_epl.fd, e, strerror(e));
        }
    }
    m_epl.fd = -1;
}

//---------------------------------------------------------
void epl_close()
{
    DBG(5, "lock epl_mutex");
    pthread_mutex_lock(&epl_mutex);
    epl__close();
    DBG(5, "unlock epl_mutex");
    pthread_mutex_unlock(&epl_mutex);
}

//---------------------------------------------------------
bool epl_checkin()
{
    const bool ok = epl_open();
    epl_close();
    if (!ok) {
        return FAIL;
    }
    return OK;
}

//---------------------------------------------------------
bool epl_open()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&epl_mutex, &attr);

    char buf[64] = {0};
    for (int i = 0; i < m_epl.number_of_epol_files; i++) {
        m_epl.file[i].channel = i + 1;
        m_epl.file[i].file_fd = open(m_epl.file[i].fname, O_RDWR); // O_RDONLY
        if (m_epl.file[i].file_fd == -1) {
            int e = errno;
            LOG(CRITICAL, "can't open '%s' (%s)", m_epl.file[i].fname, strerror(e));
            DBGF(0, "\nERROR: can't open '%s'", m_epl.file[i].fname);
            DBGF(0, "%s", strerror(e));
            DBGF(0, "%s\n", msg_esz7_not_loaded);
            epl__close();
            return FAIL;
        }
        DBGF(2, "open %s (event_file_fd:%d)", m_epl.file[i].fname, m_epl.file[i].file_fd);

        // need to make an idle read event after opening file
        lseek(m_epl.file[i].file_fd, 0, SEEK_SET);
        int ret = read(m_epl.file[i].file_fd, buf, sizeof(buf));
        if (ret == -1) {
            LOG(ERROR, "read. errno:%d (%s)\n", errno, strerror(errno));
            DBGF(0, "ERROR: read. errno:%d (%s)\n", errno, strerror(errno));
            epl__close();
            return FAIL;
        }
    }

    m_epl.fd = epoll_create1(0);
    if (m_epl.fd == -1) {
        LOG(ERROR, "epoll_create1(). errno:%d (%s)", errno, strerror(errno));
        DBGF(0, "ERROR: epoll_create1(). errno:%d (%s)\n", errno, strerror(errno));
        epl__close();
        return FAIL;
    }
    DBGF(3, "epoll fd:%d", m_epl.fd);

    struct epoll_event events;
    for (int i = 0; i < m_epl.number_of_epol_files; i++) {
        events.events = EPOLLPRI | EPOLLERR; // EPOLLET
        events.data.fd = m_epl.file[i].file_fd;
        int ret = epoll_ctl(m_epl.fd, EPOLL_CTL_ADD, events.data.fd, &events);
        if (ret == -1) {
            LOG(ERROR, "epoll_ctl(EPOLL_CTL_ADD, m_event_file_fd:%d) errno:%d (%s)\n", events.data.fd, errno,
                strerror(errno));
            DBGF(0, "epoll_ctl(EPOLL_CTL_ADD, m_event_file_fd:%d) errno:%d (%s)\n", events.data.fd, errno,
                 strerror(errno));
            epl__close();
            return FAIL;
        }
    }
    return OK;
}

//---------------------------------------------------------
inline int epl_find_event_file(int event_fd)
{
    // DBG(9, "lock epl_mutex");
    // pthread_mutex_lock(&epl_mutex); // TODO а нужна-ли блокировка?
    int channel = -1;
    if (epl()->file[0].file_fd == event_fd) {
        channel = epl()->file[0].channel;
    } else if (epl()->file[1].file_fd == event_fd) {
        channel = epl()->file[1].channel;
    } else if (epl()->file[2].file_fd == event_fd) {
        channel = epl()->file[2].channel;
    } else if (epl()->file[3].file_fd == event_fd) {
        channel = epl()->file[3].channel;
    }
    // DBGF(9, "channel: %d", channel);
    // DBG(9, "unlock epl_mutex");
    // pthread_mutex_unlock(&epl_mutex);
    return channel;
}

//---------------------------------------------------------
bool epl_read_event_file(int event_fd)
{
    // read event to reset him activation
    static char buf[256] = {0};
    if (event_fd < 0) {
        LOG(ERROR, "event_fd: %d", event_fd);
        DBGF(0, "event_fd: %d", event_fd);
        return OK;
    }
    const int ret = read(event_fd, buf, sizeof(buf));
    if (ret != -1) {
        DBGF(9, "event_fd:%d len: %d, '%s'", event_fd, ret, buf);
        return OK;
    }

    LOG(ERROR, "event_fd:%d - (%d) %s", event_fd, errno, strerror(errno));
    DBGF(0, "ERROR: event_fd:%d - (%d) %s", event_fd, errno, strerror(errno));
    // event_fd = -1; // TODO - нужно-ли отключать файл, если быда ошибка на нем?
    return FAIL;
}

//---------------------------------------------------------
bool epl_write_event_files()
{
    bool ok = OK;
    for (int i = 0; i < m_epl.number_of_epol_files; i++) {
        if (m_epl.file[i].file_fd == -1) {
            continue;
        }
        DBGF(4, "epol file[%d] id: %d", i, m_epl.file[i].file_fd);
        lseek(m_epl.file[i].file_fd, 0, SEEK_SET);
        const char *buf = "xa5ebaca\0";
        const int buflen = (int) strlen(buf);
        const int ret = write(m_epl.file[i].file_fd, buf, buflen);
        if (ret == -1 || ret != buflen) {
            LOG(ERROR, "epl[%d] write fd:%d. errno:%d (%s)", i, m_epl.file[i].file_fd, errno, strerror(errno));
            DBGF(0, "\nERROR: epl[%d] write fd:%d. errno:%d (%s)", i, m_epl.file[i].file_fd, errno, strerror(errno));
            ok = FAIL;
        }
    }
    DBGF(4, "%s", (ok ? "done" : "failed"));
    return ok;
}

//---------------------------------------------------------
const char *epl_event_name(const struct epoll_event *event)
{
    if (event->events & EPOLLIN) {
        return "EPOLLIN";
    }
    if (event->events & EPOLLOUT) {
        return "EPOLLOUT";
    }
    if (event->events & EPOLLPRI) {
        return "EPOLLPRI";
    }
    if (event->events & EPOLLHUP) {
        return "EPOLLHUP";
    }
    if (event->events & (~EPOLLERR)) {
        return "EPOLLERR";
    }
    return "UNKNW";
}
