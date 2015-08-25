/*
 * gl_header.h
 *
 *  Created on: 2015-8-25
 *      Author: jz
 */

#ifndef GL_HEADER_H
#define GL_HEADER_H

#include "define.h"

extern const int DISPLAY_OFFSET;
extern const char X11_LOCAL_PATH[];

#ifdef TCP_CHANNEL
int tcp_socket_bind();
#else
int local_socket_bind();
#endif

void do_epoll(int listenfd);

#ifdef TEST_REPAINT
void repain_onces();
void set_replain();
void store_request_for_replain(char buf[], int nread, bool app_server);
#endif

#endif



