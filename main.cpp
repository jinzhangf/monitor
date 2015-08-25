//============================================================================
// Name        : monitor.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include "gl_header.h"


const int DISPLAY_OFFSET = 1;


int main()
{
	if (DISPLAY_OFFSET <= 0 || DISPLAY_OFFSET > 9) {
		perror("DISPLAY_OFFSET error");
		exit(1);
	}
	int  listenfd;
#ifdef TCP_CHANNEL
	listenfd = tcp_socket_bind();
#else
	listenfd = local_socket_bind();
#endif
	listen(listenfd, 100);
	do_epoll(listenfd);

	return 0;
}


