/*
    pimodem - simple hayes->internet gateway
    Copyright (C) 2025 Tony Hoyle <tony@hoyle.me.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <unistd.h>
git
#include "utils.h"

void enableEcho(int channel, bool enable)
{
    struct termios ti;

    tcgetattr(channel, &ti);
    if(enable)
        ti.c_lflag |= ECHO;
    else
        ti.c_lflag &= ~ECHO;
    tcsetattr(channel, 0, &ti);
}

void rawMode(int channel, bool enable)
{
    struct termios ti;

    tcgetattr(channel, &ti);
    if(enable)
        ti.c_lflag &= ~ICANON;
    else
        ti.c_lflag |= ICANON;
    tcsetattr(channel, 0, &ti);
}

tcflag_t getTermFlags(int channel)
{
    struct termios ti;

    tcgetattr(channel, &ti);
    return ti.c_lflag;
}

void setTermFlags(int channel, tcflag_t flags)
{
    struct termios ti;

    tcgetattr(channel, &ti);
    ti.c_lflag = flags;
    tcsetattr(channel, 0, &ti);
}

void writeString(int channel, const char *str)
{
    write(channel, str, strlen(str));
}

int readLine(int channel, char *line, int maxlen)
{
    char ch;
    char *p = line;
    int ret;

    while((ret = read(channel, &ch, 1)) > 0)
    {
        if(ch == '\n')
            break;
        *(p++) = ch;
        if(p-line == maxlen-1)
            break;
    }

    *p = '\0';
    if(ret > 0)
        return p-line;
    else
        return ret ? ret : -1;
}
