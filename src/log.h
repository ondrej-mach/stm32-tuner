#ifndef _LOG_H
#define _LOG_H

#ifdef DEBUG
#include <stdio.h>
#include "usb.h"
#define LOG(str) usb_debug_log(str)
#else
#define LOG(str) // empty statement
#endif //DEBUG


#endif //_LOG_H
