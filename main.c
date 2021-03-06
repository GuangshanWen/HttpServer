#include<string.h>
#include "request.h"
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#define MAXEVENTS 65
static int create_and_bind(char *port)
{
	struct addrinfo hints;
	struct addrinfo *result,*rp;
	int s,sfd;

	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	s = getaddrinfo(NULL,port,&hints,&result);
	
	if(s != 0 )
	{
		fprintf(stderr,"getaddrinfo : %s\n",gai_strerror(s));
		return -1;
	}

	for (rp = result;rp!=NULL;rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);

		if(sfd == -1)
			continue;

		s = bind(sfd,rp->ai_addr,rp->ai_addrlen);
		if(s == 0)
		{
			break;
		}
		close(sfd);
	}

	if(rp == NULL)
	{
		fprintf(stderr,"Could not bind \n");
		return -1;
	}

	freeaddrinfo(result);
	return sfd;
}

static int make_socket_non_blocking(int sfd)
{
	int flags, s;
	flags = fcntl(sfd,F_GETFL,0);

	if(flags == -1)
	{
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd,F_SETFL,flags);
	struct linger so_linger;

//	so_linger.l_onoff = 1;
//	so_linger.l_linger = 30;
//
	setsockopt(s,SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger);
	if(s == -1)
	{
		perror("fcntl");
		return -1;
	}

	return 0;
}
/*
int c = 0;
static void  do_request(int fd)
{
	char buf[512];
	char sbuf[512];
	int count = read(fd,buf,sizeof(buf));
	
	write(1,buf,count);
	write(fd,buf,count);
	c += 1;
	printf("c : %d\n",c);
	close(fd);

}*/


int main(int argc,char* argv[])
{
	int sfd, s;
	int efd;

	struct epoll_event event;
	struct epoll_event *events;

	if(argc !=2 )
	{
		fprintf(stderr,"Usage :  [port] \n");
		exit(EXIT_FAILURE);
	}
	sfd = create_and_bind(argv[1]);
	if(sfd == -1)
		abort();
	s = make_socket_non_blocking(sfd);
	if(s == -1)
		abort();
	
	printf("SOMAXCONN:%d\n",SOMAXCONN);
	s = listen(sfd,SOMAXCONN);
	if(s == -1)
	{
		perror("LIsten");
		abort();
	}

	efd = epoll_create1(0);
	if(efd == -1)
	{
		perror("epoll_create");
		abort();
	}

	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&event);
	
	if(s == -1)
	{
		perror("epoll error");
		abort();
	}

	events = calloc(MAXEVENTS,sizeof event);

	while(1)
	{
		int n,i;
		n = epoll_wait(efd,events,MAXEVENTS,-1);
		
		for(i = 0;i<n;i++)
		{
			if((events[i].events & EPOLLERR) /*|| (events[i].events & EPOLLHUP) *//*|| (!(events[i].events & EPOLLIN))*/)
			{//检测事件类型
				printf("\ninto for loop...\n");
				int error = 0;
				socklen_t errlen = sizeof error;

				if(getsockopt(events[i].data.fd,SOL_SOCKET,SO_ERROR,(void*)&error,&errlen) == 0)
				{
					printf("\n error = %s\n",strerror(error));
				}
				printf("the socket is : %d",events[i].data.fd);
				fprintf(stderr,"epoll error\n");
				close(events[i].data.fd);
				printf("\n out for loop \n");
				continue;
			}
			else if(sfd == events[i].data.fd)
			{
				while(1)
				{
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];
					in_len = sizeof(in_addr);
					infd = accept(sfd,&in_addr,&in_len);
					if(infd == -1)
					{
						if((errno == EAGAIN) || (errno == EWOULDBLOCK))
						{
							break;
						}
						else
						{
							perror("accept");
							break;
						}
					}

					s = getnameinfo(&in_addr,in_len,hbuf,sizeof hbuf,sbuf,sizeof sbuf, NI_NUMERICHOST|NI_NUMERICSERV);

					if(s == 0)
					{
						printf("Accepted connection on descriptor %d" "(host=%s,port=%s)\n",infd,hbuf,sbuf);
					}
				//	char buf[512];
				//	int count = read(infd,buf);
				//	write(stdout,buf,count);
					s = make_socket_non_blocking(infd);

					if(s == -1)
					{
						perror("make_socket_non_blocking \n");
						abort();
					}
					event.data.fd = infd;
					event.events = EPOLLIN|EPOLLET;
					s = epoll_ctl(efd,EPOLL_CTL_ADD,infd,&event);
					if (s == -1)
					{
						perror("epoll ctl error");
						abort();
					}

				}

				continue;
			}
			else 
			{ // fd 套接口有数据可读写
				epoll_ctl(efd,EPOLL_CTL_DEL,events[i].data.fd,&event);
				struct wen_request *myrequest = (struct wen_request*)malloc(sizeof(struct wen_request));
				//myrequest->wen_fd = events[i].data.fd;	
				init_request(myrequest,events[i].data.fd);	
				printf("in while loop\n");
				printf("\n into do_request func\n");
				do_request(myrequest);
				printf("\n out do_request func\n");
			}
		}
		printf("are you there?\n");
	}
	
	printf("something wrong????\n");
	free(events);
	close(sfd);

	return EXIT_SUCCESS;

}

