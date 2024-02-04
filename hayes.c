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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "utils.h"
#include "state.h"

int connectToServer(int out, const char *addr, state_t *state)
{
    int ret;
    char hoststr[256];
    const char *host;
    const char *port = "23"; // Default to 23, but there's really no sensible default here
    char *p;
    struct addrinfo *hostai, *ai;
    int sock;

    strncpy(hoststr, addr, sizeof(hoststr));
    host = hoststr;
    while(*host == ' ') host++;
    if(*host == '[')
    {
        // ipv6 [foo:bar:bza]:port
        host++;
        p=strchr(host, ']');
        if(p) *p='\0';
    }
    p=strrchr(host, ':');
    if(p)
    {
        *p='\0';
        port = p+1;
    }    

    ret = getaddrinfo(host, port, NULL, &hostai);
    if(ret < 0)
    {
        writeString(out, "ERROR ");
        writeString(out, gai_strerror(ret));
        writeString(out, "\n");
        return ret;
    }

    for (ai=hostai; ai != NULL; ai = ai->ai_next) {
        if ((sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
            close(sock);
            sock = -1;
            continue;
        }

        // Connected succesfully!
        break;
    }

    if(sock == -1)
    {
        writeString(out, "ERROR ");
        writeString(out, gai_strerror(errno));
        writeString(out, "\n");
        return -1;
    }

    state->remote = sock;
    return 0;
}

int hayes(int in, int out, state_t *state)
{
    char line[256];
    const char *p;
    int ret;
    int op, val;
    bool err;

    rawMode(in, false);
    enableEcho(in, true);

    if(!state->quiet)
        writeString(out, "OK\n");
 
    while((ret = readLine(in, line, sizeof(line))) > 0)
    {
        ret = 0;
        err = false;
        if(strncmp(line, "AT", 2) == 0)
        {
            p = line+2;
            while(*p)
            {
                op = *(p++);
                if(op == '&') op = *(p++) + 128;
                if((op != 'D') && ((*p) >='0' && (*p) <= '9'))
                  val = *(p++) - '0';
                else 
                  val = 0;

                switch(op)
                {
                    case 'D':
                        if(*p=='T') p++;
                        if(connectToServer(out, p, state) == 0)
                          ret = 1;
                        p+=strlen(p);
                        break;
                    case 'E':
                        if(val == 0) state->localEcho = false;
                        else state->localEcho = true;
                        break;
                    case 'Z':
                    case 'F'+128:
                        state->localEcho = true;
                        break;
                    case 'O':
                        ret = 1;
                        break;
                    case 'H':
                        if(!val)
                        {
                            close(state->remote);
                            state->remote = -1;
                        }
                        break;
                    case 'I':
                        writeString(out, "PiModem 0.1 by Tony Hoyle, 2024\n");
                        break;
                    case 'Q':
                        if(val == 0) state->quiet = false;
                        else state->quiet = true;
                        break;
                    case 'A':
                    case 'L':
                    case 'M':
                    case 'S':
                    case 'V':
                    case 'X':
                        break;
                    default:
                        err = true;
                        break;
                }
            }

            if(ret == 0 && !state->quiet)
                writeString(out, err ? "ERROR\n" : "OK\n");
        }

        if(ret != 0)
            break;
    } 

    return ret;
}