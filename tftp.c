#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

int get_file(int sockfd, struct sockaddr *server_addr, const char *filename);

int main(int argc, char *argv[])
{
	int server_sockfd, client_sockfd;
	struct sockaddr_in client_addr, server_addr;
	WSADATA wsaData;
	int ret,len,i,blk_id=0;
	char buf[1024]={0};

	if (argc<3) {
		printf("参数错误！\n");
		return -1;
	}
	WSAStartup(MAKEWORD(2,2), &wsaData);

	//创建套接字
	client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_sockfd == INVALID_SOCKET) {
		printf("创建套接字失败!\n");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("192.168.0.101");
	server_addr.sin_port = htons(69);

	if (!strcmp(argv[1], "get")) {
		ret = get_file(client_sockfd, (struct sockaddr *)&server_addr, argv[2]);
		if (ret<0) {
			printf("网络错误！\n");
		}
	} else if (!strcmp(argv[1], "put")) {
		ret = put_file(client_sockfd, (struct sockaddr *)&server_addr, argv[2]);
		if (ret<0) {
			printf("网络错误！\n");
		}
	}
	
	system("PAUSE");
	closesocket(client_sockfd);
	WSACleanup();
	return 0;
}

int send_ack(int sockfd, struct sockaddr *address, unsigned short block_id)
{
	int ret;
	unsigned short buf[]={htons(4), htons(block_id)};

	ret = sendto(sockfd, (char *)buf, sizeof(buf), 0, address, sizeof(struct sockaddr_in));

	return ret;
}

int tftp_get_ack(int sockfd, struct sockaddr_in *address, unsigned short *cmd, unsigned short *arg)
{
	int ret;
	unsigned int len;
	unsigned short buf[2]={0};

	printf("receive ack length %d\n", ret);
	if (ret ==4) {
		*cmd = buf[0]<<8 | buf[1];
		*arg = buf[2]<<8 | buf[3];
	}

	return ret;
}

/*
 *
 *
 *
 *
 *
 *	Write @ 2010-12-09
 */
int put_file(int sockfd, struct sockaddr *server_addr, const char *filename)
{
	unsigned char buf[550]={0};
	unsigned char recv_buf[4]={0};
	struct sockaddr_in address;
	int ret;
	unsigned int len;
	unsigned short blk_id=0;
	char *p;
	FILE *fp;

	fp = fopen(filename, "w+b");
	buf[1]=2;//WRQ
	len = 2;
	strcpy(buf+len, filename);
	len += strlen(filename)+1;
	strcpy(buf+len, "octet");
	len += strlen("octet")+1;

	//RRQ、filename、mode
	sendto(sockfd, buf, len, 0, server_addr, sizeof(struct sockaddr_in));
	ret = recvfrom(sockfd, recv_buf, 4, 0, (struct sockaddr *)&address, &len);
	if (ret <0) {
		return -1;
	}

	if ((recv_buf[0]<<8 | recv_buf[1])!=4)
			return -1; 
	if((recv_buf[2]<<8 | recv_buf[3])!=blk_id)
			return -1; 

	blk_id++;

	buf[0] = 0;//DATA
	buf[1] = 3;//DATA
	printf("开始发送文件(%d)\n", len);

	unsigned short length=516;
	while (1) {
		unsigned short cmd=0, arg=0;

		buf[2] = blk_id>>8;
		buf[3] = blk_id;
		memset(buf+4, blk_id, sizeof(buf));
		sendto(sockfd, buf, length, 0, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
		ret = recvfrom(sockfd, recv_buf, 4, 0, (struct sockaddr *)&address, &len);
		if (ret<0) {
			return -1;
		}
		if (((recv_buf[0]<<8 | recv_buf[1])!=4) || ((recv_buf[2]<<8 | recv_buf[3])!=blk_id)) {
			printf("失去#%d响应\n");
			continue;
		}
		printf("已发送%d\n",blk_id);
		blk_id++;
		if(length!=516)	break;
		if(blk_id>=60)	length=510;
	}
	printf("传输完成！\n");
	fclose(fp);

	return 0;
}

/*
 *
 *
 *
 *
 *
 *	Write @ 2010-12-08
 */
int get_file(int sockfd, struct sockaddr *server_addr, const char *filename)
{
	short cmd=0x01;
	unsigned char buf[550]={0};
	struct sockaddr address;
	int ret;
	unsigned int len;
	char *p;
	FILE *fp;

	fp = fopen(filename, "w+b");
	buf[1]=1;//RRQ
	len = 2;
	strcpy(buf+len, filename);
	len += strlen(filename)+1;
	strcpy(buf+len, "octet");
	len += strlen("octet")+1;

	//RRQ、filename、mode
	sendto(sockfd, buf, len, 0, server_addr, sizeof(struct sockaddr_in));

	printf("开始接收文件(%d)\n", len);
	while (1) {
		ret = recvfrom(sockfd, (char *)buf, sizeof(buf), 0, &address, &len);
		if (ret <1) {//网络错误
			return -1;
		}
		if (ret < 512) {//最后一个包
			fwrite(buf+4, 1, ret-4, fp);
			break;
		} else {
			send_ack(sockfd, &address, (buf[2]<<8) | buf[3]);
			fwrite(buf+4, 1, ret-4, fp);
			printf("收到包%d\n", buf[2]<<8 | buf[3]);
		}
	}
	printf("传输完成！\n");
	fclose(fp);

	return 0;
}


