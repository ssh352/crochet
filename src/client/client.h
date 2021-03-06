#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <exchanges/exchanges.h>
#include <finmath/linear_equation.h>
#include <globals/globals.h>
#include <math.h>
#include <pprint/pprint.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int
client_start();
int
client_getdisplay(Display **display);
int
client_getwindow(Window *window);
void
client_redraw();

#endif
