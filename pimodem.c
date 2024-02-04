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
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "utils.h"
#include "hayes.h"
#include "terminal.h"
#include "state.h"

int createListeningSocket(const char *useAddress, const char *usePort, bool v4only, bool v6only)
{
    int s, res;
    int v6 = 0;

    struct addrinfo hints = 
    {
        ai_flags: AI_PASSIVE | AI_NUMERICSERV,
        ai_family: v4only ? AF_INET : v6only ? AF_INET6 : AF_UNSPEC,
        ai_socktype:  SOCK_STREAM,
        ai_protocol: IPPROTO_TCP
    };

    struct addrinfo *result, *ai;
    if((res = getaddrinfo(useAddress, usePort, &hints, &result)) != 0 )
    {
        fprintf(stderr, "Couldn't get socket info: %s\n",gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    /* For some reason glibc AF_UNSPEC/AI_PASSIVE returns v4 first, which is backwards to */
    /* the sensible way.. so this just points to the right one.  We can't just  request AF_INET6 */
    /* because an installation might not have the ipv6 module loaded */
    ai = result;
    if(ai->ai_next && ai->ai_next->ai_family == AF_INET6) ai = ai->ai_next;

    if((s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
    {
        fprintf(stderr, "Couldn't create socket: %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }

    v6 = v6only ? 1 : 0; 
    setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&v6, sizeof(v6)); 
    
    if(bind(s, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        fprintf(stderr, "Couldn't bind socket: %s\n", strerror(errno));
        close(s);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    if(listen(s, 64) < 0)
    {
        fprintf(stderr, "Couldn't listen on socket: %s\n",strerror(errno));
        close(s);
        exit(EXIT_FAILURE);
    }
    
    return s;
}

int main(int argc, char *argv[])
{
    int opt;
    int in, out;
    bool useSocket = false;
    bool v4only = false, v6only = false;
    const char *usePort = "255";
    const char *useAddress = NULL;
    state_t state;

    while ((opt = getopt(argc, argv, "h46a:sp:")) != -1) 
    {
        switch (opt) 
        {
            case 's':
                useSocket = true;
                break;
            case '4':
                v4only = true;
                useSocket = true;
                break;
            case '6':
                v6only = true;
                useSocket = true;
                break;
            case 'a':
                useAddress = optarg;
                useSocket = true;
                break;
            case 'p':
                usePort = optarg;
                useSocket = true;
                break;
            case 'h':
                fprintf(stderr, "PiModem 0.4, by Tony Hoyle 2024\n\n");
                fprintf(stderr, "Usage: %s [-s] [-4] [-6] [-a bindAddress] [-p port]\n", argv[0]);
                fprintf(stderr, "-s              Server mode\n");
                fprintf(stderr, "-4              ipv4 only\n");
                fprintf(stderr, "-6              ipv6 only\n");
                fprintf(stderr, "-a bindAddress  Bind to address (default all)\n");
                fprintf(stderr, "-p port         Bind to port (default 255)\n");
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Usage: %s [-h] [-s] [-4] [-6] [-a bindAddress] [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(v4only && v6only)
    {
        v4only = false;
        v6only = false;
    }

    if(useSocket)
    {
        int sock = createListeningSocket(useAddress, usePort, v4only, v6only);
        int pid;

        if(daemon(0, 0) != 0)
        {
                fprintf(stderr, "Couldn't deamonize: %s\n", strerror(errno));
                close(sock);
                exit(EXIT_FAILURE);
        }

        signal(SIGCHLD, SIG_IGN);

        while(true)
        {
            in = accept(sock, NULL, NULL);
            if(in == -1)
            {
                fprintf(stderr, "Couldn't accept: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            pid = fork();
            if(pid == -1)
            {
                fprintf(stderr, "Couldn't fork: %s\n",strerror(errno));
                exit(EXIT_FAILURE);
            }
            if(pid == 0)
            {
                state.remote = -1;
                while(true)
                {
                    if(hayes(in, in, &state) < 0)
                        break;
                    if(terminal(in, in, &state) < 0)
                        break;
                }
                exit(EXIT_SUCCESS);
            }
        }
    }
    else
    {
        in = 0;
        out = 1;
        state.remote = -1;

        while(true)
        {
            if(hayes(in, out, &state) < 0)
                break;
            if(terminal(in, out, &state) < 0)
                break;
        }
    }
    return 0;
}