/**
  \file   signals.h
  \author Aleksey Lekhtin
  \date   April 4, 2024
  \brief  catch signals
*/

#ifndef SIGNALS__H
#define SIGNALS__H

#include <signal.h>
#include <stdbool.h>

bool set_sig_action();
void sigaction_handler(int snum);

#endif // SIGNALS__H
