/**
  \file   dtr_packet.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The Depth Tracker packet making procedure
*/

#include <malloc.h>
#include <string.h>

#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "utils.h"
#include "siu.h"
#include "dump_siu_text_packet.h"

//-------------------------------------------------------//
void dtr_packet_free()
{
    DBG(4);
    if (dtr()->packet.bytes) {
        free(dtr()->packet.bytes);
    }
    dtr()->packet.bytes = NULL;
    dtr()->packet.nbytes = 0;
    dtr()->packet.memsz = 0;
}

//---------------------------------------------------------
void dtr_data_create_packet()
{
    const char *msg[1];
    int msglen[1] = {0};
    int nmgs = sizeof(msg)/sizeof(msg[1]);

    //---------------------------------
    // формирование записей с данными по необходимому формату
    msg[0] = make_DSD_record();
    msglen[0] = (int)strlen(msg[0]);

    //---------------------------------
    // выделение памяти для отправки данных
    dtr()->packet.nbytes = 0;
    for(int i = 0; i < nmgs; i++) {
        dtr()->packet.nbytes += msglen[i];
    }

    if (dtr()->packet.nbytes > dtr()->packet.memsz) {
        dtr()->packet.memsz = (dtr()->packet.nbytes < 256 ? 256 : dtr()->packet.nbytes * 2);
        dtr()->packet.bytes = realloc(dtr()->packet.bytes, dtr()->packet.memsz);
    }

    //---------------------------------
    // заполнение памяти для отправки.
    // отправка в "клиентском" потоке по сигналу thr_wakeup()
    size_t off = 0;
    for(int i = 0; i < nmgs; i++) {
        memcpy(dtr()->packet.bytes + off, msg[i], msglen[i]);
        off += msglen[i];
    }
    DBGF(3, "len:%d %s", dtr()->packet.nbytes, dtr()->packet.bytes);
    dump_text_packet(2, dtr()->packet.bytes, dtr()->packet.nbytes);
}
