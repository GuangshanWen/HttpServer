#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include"response.h"
#include<stdlib.h>
#include<stdio.h>
#include<sys/socket.h>

static void parse_http_uri(char*,char*);
static void io_write(char*,int);
static void http_header(int,int);

int http_response(struct wen_request* request,char* filename)
{
	// first check the request url
//	char filename[100];
//
//	parse_http_uri(request->wen_url,filename);
//	
//	io_write(filename,request->wen_fd);
	

//	exit(1);

//	wen_free(request);
//	free(request);

	return 1; 
}

static void parse_http_uri(char* url,char* filename)
{
	if(strcmp(url,"/") == 0)
		sprintf(filename,"index.html");
	else 
		return ;
}

static void io_write(char* filename,int fd)
{
	char path[100] = "html/";
	strcat(path,filename);
	char buf[1024];
	printf(path);
	printf("\n");
	int file = open((char*)path,O_RDONLY);
	if(file == -1)
	{
		http_header(fd,-1);
		int c = sprintf(buf,"cannot find this file in the server,please check again!\n");
		send(fd,buf,c,0);
		printf("hello \n");
		return ;
	}
//	printf("end of not find\n");
	http_header(fd,1);
	int count = read(file,buf,sizeof buf);
	
	while(count != 0)
	{
		write(fd,buf,count);
		count = read(file,buf,sizeof buf);
	}
	write(fd,"\0",1);

	close(file);
}

static void http_header(int fd,int flag)
{
	int count = 0;
	char buf[1024];
	if(flag == 1)count = sprintf(buf,"HTTP/1.1 200 OK\r\n");
	else if(flag == -1)
	{
		count = sprintf(buf,"HTTP/1.1 404 CANNOT FIND FILE\r\n");
	}
//	printf("in header\n");
	send(fd,buf,count,0);
//	printf("out of header\n");
	count = sprintf(buf,"content-type: text/html\r\n");
	send(fd,buf,count,0);
	
	count = sprintf(buf ,"hello world!\n");

	count = sprintf(buf,"\r\n");
	send(fd,buf,count,0);
}


