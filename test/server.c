/*************************************************************************
#	 FileName	: server.c
#	 Author		: fengjunhui 
#	 Email		: 18883765905@163.com 
#	 Created	: 2018年12月29日 星期六 13时44分59秒
 ************************************************************************/

#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>

#include "common.h"

sqlite3 *db;  //仅服务器使用

int process_user_or_admin_login_request(int acceptfd,MSG *msg)
{
	//封装sql命令，表中查询用户名和密码－存在－登录成功－发送响应－失败－发送失败响应	
	char sql[DATALEN] = {0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;

	msg->info.usertype =  msg->usertype;
	strcpy(msg->info.name,msg->username);
	strcpy(msg->info.passwd,msg->passwd);
	
	printf("usrtype: %#x-----usrname: %s---passwd: %s.\n",msg->info.usertype,msg->info.name,msg->info.passwd);
	sprintf(sql,"select * from usrinfo where usertype=%d and name='%s' and passwd='%s';",msg->info.usertype,msg->info.name,msg->info.passwd);
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("---****----%s.\n",errmsg);		
	}else{
		//printf("----nrow-----%d,ncolumn-----%d.\n",nrow,ncolumn);		
		if(nrow == 0){
			strcpy(msg->recvmsg,"name or passwd failed.\n");
			send(acceptfd,msg,sizeof(MSG),0);
		}else{
			strcpy(msg->recvmsg,"OK");
			send(acceptfd,msg,sizeof(MSG),0);
		}
	}
	return 0;	
}

int process_user_modify_request(int acceptfd,MSG *msg)
{
	int nrow,ncolumn;
	char *errmsg, **resultp;
	int num,row;
	char sql[DATALEN] = {0};	
	char ready[DATALEN]={0};
	sprintf(ready,"select staffno from usrinfo where name='%s';",msg->username);
	if(sqlite3_get_table(db,ready,&resultp,&row,&num,&errmsg)!= SQLITE_OK){
		printf("change failed\n");
		return -1;
	}else{
		msg->info.no = atoi(resultp[num]);
	}
	switch (msg->recvmsg[0])
	{
	case 'P':
		sprintf(sql,"update usrinfo set phone='%s' where staffno=%d;",msg->info.phone,msg->info.no);
		break;
	case 'D':
		sprintf(sql,"update usrinfo set addr='%s' where staffno=%d;",msg->info.addr, msg->info.no);
		break;	
	case 'M':
		sprintf(sql,"update usrinfo set passwd='%s' where staffno=%d;",msg->info.passwd, msg->info.no);
		break;
	}	

	//调用sqlite3_exec执行sql命令
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
		sprintf(msg->recvmsg,"修改失败！%s\n", errmsg);
	}else{
		sprintf(msg->recvmsg, "修改成功!\n");
	}

	//通知用户信息修改成功
	send(acceptfd,msg,sizeof(MSG),0);
	return 0;
}



int process_user_query_request(int acceptfd,MSG *msg)
{
	char sql[DATALEN] = {0};
	char **resultp;
	int nrow,ncolumn;
	char *errmsg;
	sprintf(sql,"select * from usrinfo where name='%s';",msg->username);
	if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("----------searching--------\n");	
		msg->info.no = atoi(resultp[11]);
		sprintf(msg->info.name,"%s",resultp[13]);
		msg->info.age = atoi(resultp[15]);
		sprintf(msg->info.phone,"%s",resultp[16]);
		sprintf(msg->info.addr,"%s",resultp[17]);
		sprintf(msg->info.work,"%s",resultp[18]);
		sprintf(msg->info.date,"%s",resultp[19]);
		msg->info.level = atoi(resultp[20]);
		msg->info.salary = atoi(resultp[21]);
		send(acceptfd,msg,sizeof(MSG),0);
		usleep(1000);

		sqlite3_free_table(resultp);
	}
}


int process_admin_modify_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	char sql[DATALEN] = {0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	recv(acceptfd,msg,sizeof(MSG),0);
	switch(msg->recvmsg[0]){
	case 1:
		sprintf(sql,"update usrinfo set name='%s' where staffno=%d;",msg->info.name,msg->info.no);
		break;
	case 2:
		sprintf(sql,"update usrinfo set age='%d' where staffno=%d;",msg->info.age,msg->info.no);
		break;
	case 3:
		sprintf(sql,"update usrinfo set addr='%s' where staffno=%d;",msg->info.addr,msg->info.no);
		break;
	case 4:
		sprintf(sql,"update usrinfo set phone='%s' where staffno=%d;",msg->info.phone,msg->info.no);
		break;
	case 5:
		sprintf(sql,"update usrinfo set work='%s' where staffno=%d;",msg->info.work,msg->info.no);
		break;
	case 6:
		sprintf(sql,"update usrinfo set salary='%lf' where staffno=%d;",msg->info.salary,msg->info.no);
		break;
	case 7:
		sprintf(sql,"update usrinfo set date='%s' where staffno=%d;",msg->info.date,msg->info.no);
		break;
	case 8:
		sprintf(sql,"update usrinfo set level='%d' where staffno=%d;",msg->info.level,msg->info.no);
		break;
	case 9:
		sprintf(sql,"update usrinfo set passwd='%s' where staffno=%d;",msg->info.passwd,msg->info.no);
		break;
	case 10:
		return 0;
	}
	memset(msg->recvmsg,0,DATALEN);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		strcpy(msg->recvmsg,"FAIL");
	}else{
		strcpy(msg->recvmsg,"OK");
	}
	send(acceptfd,msg,sizeof(MSG),0);

	return 0;

}


int process_admin_adduser_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);

}



int process_admin_deluser_request(int acceptfd,MSG *msg)
{

	return 0;

}


int process_admin_query_request(int acceptfd,MSG *msg)
{
	int i = 0,j = 0;
	char sql[DATALEN] = {0};
	char **resultp;
	int nrow;
	int ncolumn = 0;
	char *errmsg;
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	if(msg->flags==1){
		sprintf(sql,"select * from usrinfo where name='%s';",msg->info.name);
	}else{
		sprintf(sql,"select * from usrinfo;");
	}
	if(sqlite3_get_table(db, sql, &resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
			if(msg->flags==1){
				msg->info.no = atoi(resultp[ncolumn]);
				msg->info.usertype = atoi(resultp[ncolumn+1]);
				sprintf(msg->info.name,"%s",resultp[ncolumn+2]);
				sprintf(msg->info.passwd,"%s",resultp[ncolumn+3]);
				msg->info.age = atoi(resultp[ncolumn+4]);
				sprintf(msg->info.phone,"%s",resultp[ncolumn+5]);
				sprintf(msg->info.addr,"%s",resultp[ncolumn+6]);
				sprintf(msg->info.work,"%s",resultp[ncolumn+7]);
				sprintf(msg->info.date,"%s",resultp[ncolumn+8]);
				msg->info.level = atoi(resultp[ncolumn+9]);
				msg->info.salary = atoi(resultp[ncolumn+10]);
				send(acceptfd,msg,sizeof(MSG),0);
				usleep(1000);
			}else{
				for(i=0;i<nrow;i++){
					msg->info.no = atoi(resultp[ncolumn]);
					msg->info.usertype = atoi(resultp[ncolumn+1]);
					sprintf(msg->info.name,"%s",resultp[ncolumn+2]);
					sprintf(msg->info.passwd,"%s",resultp[ncolumn+3]);
					msg->info.age = atoi(resultp[ncolumn+4]);
					sprintf(msg->info.phone,"%s",resultp[ncolumn+5]);
					sprintf(msg->info.addr,"%s",resultp[ncolumn+6]);
					sprintf(msg->info.work,"%s",resultp[ncolumn+7]);
					sprintf(msg->info.date,"%s",resultp[ncolumn+8]);
					msg->info.level = atoi(resultp[ncolumn+9]);
					msg->info.salary = atoi(resultp[ncolumn+10]);
					send(acceptfd,msg,sizeof(MSG),0);
					usleep(1000);
					ncolumn=ncolumn+11;
					}
				}
			}

	/*for(i=0;i<nrow;i++){
		printf("%s    %s     %s     %s     %s     %s     %s     %s     %s     %s     %s.\n",resultp[ncolumn],resultp[ncolumn+1],\
			resultp[ncolumn+2],resultp[ncolumn+3],resultp[ncolumn+4],resultp[ncolumn+5],resultp[ncolumn+6],\
			resultp[ncolumn+7],resultp[ncolumn+8],resultp[ncolumn+9],resultp[ncolumn+10]);
			
		sprintf(msg->recvmsg,"%s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s,    %s;",resultp[ncolumn],resultp[ncolumn+1],\
			resultp[ncolumn+2],resultp[ncolumn+3],resultp[ncolumn+4],resultp[ncolumn+5],resultp[ncolumn+6],\
			resultp[ncolumn+7],resultp[ncolumn+8],resultp[ncolumn+9],resultp[ncolumn+10]);
		send(acceptfd,msg,sizeof(MSG),0);
		ncolumn=ncolumn+11;
		usleep(1000);

	}*/
	if(msg->flags != 1){  //全部查询的时候不知道何时结束，需要手动发送结束标志位，但是按人名查找不需要
		//通知对方查询结束了
		strcpy(msg->recvmsg,"over*");
		send(acceptfd,msg,sizeof(MSG),0);
	}
	
	sqlite3_free_table(resultp);
	printf("sqlite3_get_table successfully.\n");


}

int process_admin_history_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);

}


int process_client_quit_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);

}


int process_client_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	switch (msg->msgtype)
	{
		case USER_LOGIN:
		case ADMIN_LOGIN:
			process_user_or_admin_login_request(acceptfd,msg);
			break;
		case USER_MODIFY:
			process_user_modify_request(acceptfd,msg);
			break;
		case USER_QUERY:
			process_user_query_request(acceptfd,msg);
			break;
		case ADMIN_MODIFY:
			process_admin_modify_request(acceptfd,msg);
			break;

		case ADMIN_ADDUSER:
			process_admin_adduser_request(acceptfd,msg);
			break;

		case ADMIN_DELUSER:
			process_admin_deluser_request(acceptfd,msg);
			break;
		case ADMIN_QUERY:
			process_admin_query_request(acceptfd,msg);
			break;
		case ADMIN_HISTORY:
			process_admin_history_request(acceptfd,msg);
			break;
		case QUIT:
			process_client_quit_request(acceptfd,msg);
			break;
		default:
			break;
	}

}


int main(int argc, const char *argv[])
{
	//socket->填充->绑定->监听->等待连接->数据交互->关闭 
	int sockfd;
	int acceptfd;
	ssize_t recvbytes;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t addrlen = sizeof(serveraddr);
	socklen_t cli_len = sizeof(clientaddr);

	MSG msg;
	//thread_data_t tid_data;
	char *errmsg;

	if(sqlite3_open(STAFF_DATABASE,&db) != SQLITE_OK){
		printf("%s.\n",sqlite3_errmsg(db));
	}else{
		printf("the database open success.\n");
	}

	if(sqlite3_exec(db,"create table usrinfo(staffno integer,usertype integer,name text,passwd text,age integer,phone text,addr text,work text,date text,level integer,salary REAL);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}

	if(sqlite3_exec(db,"create table historyinfo(time text,name text,words text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{ //华清远见创客学院         嵌入式物联网方向讲师
		printf("create historyinfo table success.\n");
	}

	//创建网络通信的套接字
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed.\n");
		exit(-1);
	}
	printf("sockfd :%d.\n",sockfd); 

	
	/*优化4： 允许绑定地址快速重用 */
	int b_reuse = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof (int));
	
	//填充网络结构体
	memset(&serveraddr,0,sizeof(serveraddr));
	memset(&clientaddr,0,sizeof(clientaddr));
	serveraddr.sin_family = AF_INET;
//	serveraddr.sin_port   = htons(atoi(argv[2]));
//	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port   = htons(5001);
	serveraddr.sin_addr.s_addr = inet_addr("192.168.5.220");


	//绑定网络套接字和网络结构体
	if(bind(sockfd, (const struct sockaddr *)&serveraddr,addrlen) == -1){
		printf("bind failed.\n");
		exit(-1);
	}

	//监听套接字，将主动套接字转化为被动套接字
	if(listen(sockfd,10) == -1){
		printf("listen failed.\n");
		exit(-1);
	}

	//定义一张表
	fd_set readfds,tempfds;
	//清空表
	FD_ZERO(&readfds);
	FD_ZERO(&tempfds);
	//添加要监听的事件
	FD_SET(sockfd,&readfds);
	int nfds = sockfd;
	int retval;
	int i = 0;

#if 0 //添加线程控制部分
	pthread_t thread[N];
	int tid = 0;
#endif

	while(1){
		tempfds = readfds;
		//记得重新添加
		retval =select(nfds + 1, &tempfds, NULL,NULL,NULL);
		//判断是否是集合里关注的事件
		for(i = 0;i < nfds + 1; i ++){
			if(FD_ISSET(i,&tempfds)){
				if(i == sockfd){
					//数据交互 
					acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&cli_len);
					if(acceptfd == -1){
						printf("acceptfd failed.\n");
						exit(-1);
					}
					printf("ip : %s.\n",inet_ntoa(clientaddr.sin_addr));
					FD_SET(acceptfd,&readfds);
					nfds = nfds > acceptfd ? nfds : acceptfd;
				}else{
					recvbytes = recv(i,&msg,sizeof(msg),0);
					printf("msg.type :%#x.\n",msg.msgtype);
					if(recvbytes == -1){
						printf("recv failed.\n");
						continue;
					}else if(recvbytes == 0){
						printf("peer shutdown.\n");
						close(i);
						FD_CLR(i, &readfds);  //删除集合中的i
					}else{
						process_client_request(i,&msg);
					}
				}
			}
		}
	}
	close(sockfd);

	return 0;
}



