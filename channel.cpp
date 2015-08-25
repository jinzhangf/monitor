/*
 * channel.cpp
 *
 *  Created on: 2015-8-25
 *      Author: jz
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <strings.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include "gl_header.h"

#include <iostream>
#include <map>
using namespace std;

#define EPOLLEVENTS 100
#define FDSIZE      1000
#define BUF_SIZE 1024

static char buf[BUF_SIZE];
static map<int, int> s2a; //server to app
static map<int, int> a2s; //app to server
const char X11_LOCAL_PATH[] = "/tmp/.X11-unix/X0";

void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd);
void handle_accpet(int epollfd,int listenfd);
void do_trans(int epollfd, int fd);
void add_event(int epollfd, int fd, int state);
void delete_event(int epollfd, int fd, int state);

#ifdef TCP_CHANNEL
int tcp_socket_bind()
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1){
        perror("socket error ");
        exit(1);
    }
    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    int port = 6000 + DISPLAY_OFFSET;
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        perror("bind error ");
        exit(1);
    }
    return listenfd;
}

#else
int local_socket_bind()
{
	int fd, len;
	struct sockaddr_un un;
	char X11_LOCAL_SERVER[] = "/tmp/.X11-unix/X0";
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("local socket error ");
		exit(1);
	}
	X11_LOCAL_SERVER[sizeof(X11_LOCAL_SERVER) - 2] += DISPLAY_OFFSET;
	unlink(X11_LOCAL_SERVER);
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, X11_LOCAL_SERVER);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(X11_LOCAL_SERVER);
	if (bind(fd, (struct sockaddr *)&un, len) < 0) {
		perror("local socket bind error ");
		exit(1);
	}
	return fd;
}
#endif

void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int num;

    epollfd = epoll_create(FDSIZE);
    add_event(epollfd, listenfd, EPOLLIN);
    for (;;) {
#ifdef TEST_REPAINT
    	repain_onces();
#endif
        num = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd,events,num,listenfd);
    }
    close(epollfd);
}

void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd)
{
    for (int i = 0;i < num; ++i) {
        int fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
        if ((fd == listenfd) && (events[i].events & EPOLLIN))
            handle_accpet(epollfd, listenfd);
        else if (events[i].events & EPOLLIN)
            do_trans(epollfd, fd);
        else {
        	perror("epoll event error:");
        	exit(1);
        }
    }
}

void handle_accpet(int epollfd,int listenfd)
{
	int clifd;
	socklen_t  len;
#ifdef TCP_CHANNEL
    struct sockaddr_in cliaddr;
#else
    struct sockaddr_un cliaddr;
#endif
    len = sizeof(cliaddr);
    clifd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

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

    if (clifd == -1) {
        perror("accpet error ");
        exit(1);
    }else {
#ifdef TCP_CHANNEL
    	char ip[16];
    	const char *client_ip = inet_ntop(AF_INET, &cliaddr.sin_addr, ip, 16);
    	int client_port = htons(cliaddr.sin_port);
    	cout << "accept a new client: " << client_ip << ":" << client_port << ", clifd=" << clifd << ", x11fd=" << x11fd << endl;
#else
    	cout << "accept a new client" << endl;
#endif

        //添加一个客户描述符和事件
        add_event(epollfd, clifd, EPOLLIN);
        add_event(epollfd, x11fd, EPOLLIN);
        s2a[x11fd] = clifd;
        a2s[clifd] = x11fd;
    }
}

void do_trans(int epollfd, int infd)
{
    int nread;
    int outfd;
    if (s2a.count(infd) > 0) outfd = s2a[infd];
    else if (a2s.count(infd) > 0) outfd = a2s[infd];
    else return;

    nread = read(infd, buf, BUF_SIZE);
    if (nread <= 0) {
    	cout << "closing infd=" << infd << ", outfd=" << outfd << endl;
        close(infd);
        close(outfd);
        if (s2a.count(infd) > 0) {
        	s2a.erase(infd);
        	a2s.erase(outfd);
        }else {
        	a2s.erase(infd);
        	s2a.erase(outfd);
        }
        delete_event(epollfd, infd, EPOLLIN);
        delete_event(epollfd, outfd, EPOLLIN);
#ifdef TEST_REPAINT
        set_replain();
#endif
    }else {
        write(outfd, buf, nread);
#ifdef TEST_REPAINT
        store_request_for_replain(buf, nread, a2s.count(infd) > 0);
#endif
    }
}

void add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}
