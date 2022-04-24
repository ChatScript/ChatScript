#ifndef DISCARDSERVER
#ifdef EVSERVER

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wbitwise-op-parentheses"
 
#define EV_STANDALONE 1
#define EV_CHILD_ENABLE 1
#include "evserver/ev.c"

#pragma GCC diagnostic pop

#endif
#endif
