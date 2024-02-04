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
#include <stdio.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>

#include "utils.h"
#include "state.h"

const int guardTime = 1;

int terminal(int in, int out, state_t *state)
{
    int plus = 0;
    time_t lastCharTime;
    struct pollfd fds[2] = { { in, POLLIN}, { state->remote, POLLIN } };

    writeString(out, "CONNECT\n");

    rawMode(in, true);
    rawMode(out, true);
    enableEcho(in, state->localEcho);

    while(poll(fds, 2, 1000) >= 0)
    {
        time_t charTime = time(NULL);  
        char ch;

        if((fds[0].revents & (POLLERR|POLLHUP)) ||
           (fds[1].revents & (POLLHUP|POLLHUP)))
            break;

        if(fds[0].revents & POLLIN)
        {
            if(read(in, &ch, 1) < 1)
                break;

            if(ch == '+')
            {
                if(plus == 0 && charTime - lastCharTime > guardTime) plus = 1;
                else if(plus > 0 && plus < 3) plus++;
            }
            else
              plus = 0;
            write(state->remote, &ch, 1);
            lastCharTime = charTime;
        }
        else if(plus == 3 && charTime - lastCharTime > guardTime)
            break;

        if(fds[1].revents & POLLIN)
        {
            if(read(state->remote, &ch, 1) < 1)
                break;
            write(out, &ch, 1);
        }
    }

    return 0;
}
