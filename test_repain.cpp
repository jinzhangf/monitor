/*
 * test_repain.cpp
 *
 *  Created on: 2015-8-25
 *      Author: jz
 */



#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
using namespace std;

#include "gl_header.h"

#ifdef TEST_REPAINT

static char is_testing = 'n';
static const int test_size = 4096;
static int test_index = 0;
static char test_buf[test_size];
static vector<string> display;


void repain_onces()
{
	if (is_testing != 'y') return;

    int x11fd;
    struct sockaddr_un x11addr;
    x11fd = socket(AF_UNIX, SOCK_STREAM, 0);
    x11addr.sun_family = AF_UNIX;
    strcpy(x11addr.sun_path, X11_LOCAL_PATH);
    /* connect to x11 server */
    if (connect(x11fd, (struct sockaddr *)&x11addr, sizeof(x11addr)) == -1) {
    	perror("connect to x11 server failed ");
    	exit(1);
    }
    const int splice = 1024;
    for (int i = 0; i < display.size(); ++i) {
    	int j;
    	for (j = 0; j + splice <= display[i].size(); j+=splice) {
    		write(x11fd, display[i].c_str() + j, splice);
    	}
    	if (j < display[i].size()) write(x11fd, display[i].c_str() + j, display[i].size() - j);
    	usleep(5000);
    }
    close(x11fd);
    display.clear();
    is_testing = 'n';
}

void set_replain() {
	is_testing = 'y';
}

void store_request_for_replain(char buf[], int nread, bool app_server) {
	if (app_server && test_index + nread < test_size) {
		memcpy(test_buf + test_index, buf, nread);
		test_index += nread;
	}else if (app_server && test_index + nread >= test_size) {
    	display.push_back(string (test_buf, test_index));
    	test_index = 0;
    	memcpy(test_buf + test_index, buf, nread);
    	test_index += nread;
	}else {
    	display.push_back(string (test_buf, test_index));
    	test_index = 0;
	}
}
#endif

