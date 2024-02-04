#ifndef UTILS__H
#define UTILS__H

#include <termios.h>

typedef enum { false=0, true=1 } bool;

void enableEcho(int channel, bool enable);
void rawMode(int channel, bool enable);
tcflag_t getTermFlags(int channel);
void setTermFlags(int channel, tcflag_t flags);
void writeString(int channel, const char *str);
int readLine(int channel, char *line, int maxlen);

#endif