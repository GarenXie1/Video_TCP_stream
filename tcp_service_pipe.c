#define _GNU_SOURCE /* for asprintf */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h> //inet_addr




#define	LOG_TAG	"tcp_service_pipe"

//#define _DEBUG_
#define _ERROR_
#define _APPPRINTF_

#ifdef _DEBUG_
#define DEBUG(format,...) \
						printf("DEBUG: File:%s, Line: %05d: "format"\n",__FILE__,__LINE__, ##__VA_ARGS__);
#else
#define DEBUG(format,...) //printf(""format"\n",##__VA_ARGS__)
#endif

#ifdef _ERROR_
#define ERROR(format,...) \
						perror(""format"",##__VA_ARGS__);
#else
#define ERROR(format,...)
#endif

#ifdef _APPPRINTF_
#define APPPRINTF(format,...) \
						printf("INFO: File:%s, Line: %05d: "format"\n",__FILE__,__LINE__, ##__VA_ARGS__);
#else
#define APPPRINTF(format,...)
#endif

#define SOCKET_DONE		"done"
#define	SOCKET_FAILED	"failed"
#define	SOCKET_EXIT		"exit"


#define PORT 		8899               	// socket port listening by server
#define MAX_BUFFER 	1024         		// max data buff
#define ACK_BUFFER 	10         		// max data buff
static char *IP = "127.0.0.1";
static FILE *m_out_avc_file;
#define OUTPUT_FILE "/etc/Shutdown_ATE/out.avc"

void open_pipe_file() {
	// 以只读方式打开文件, 对应于 open()函数的 flags 参数取值 O_RDONLY
	m_out_avc_file = fopen(OUTPUT_FILE, "r");
	if(m_out_avc_file == NULL){
		ERROR("Open Pipe File Failed !!!");
	}
}

void close_pipe_file() {
	fclose(m_out_avc_file);
}

int read_pipe_file(uint8_t *buf, size_t len) {
	int ret_count;

	// 读取 1个 数据项，该数据项的长度为 len; 返回值: 返回读取到的数据项的个数.
	ret_count = fread(buf, len, 1, m_out_avc_file);
	printf("read -> %d , ret_count -> %d\n", len,ret_count);
	return ret_count;
}

static void sendRespondBySocket(int client_sockfd , unsigned char * buf){

	int write_size = 0;
	char buffer[ACK_BUFFER];
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, buf, sizeof(buf));

	if ((write_size = write(client_sockfd, buffer, sizeof(buffer))) > 0)	 // Send the received data back to the client
	{
		APPPRINTF("Sent '%s' to client successfully!",buffer);
	}
}

static void executeCommand(char * buffer , int client_sockfd)
{

	pid_t status;
	APPPRINTF("run --> %s", buffer);
	status = system(buffer);
	if (-1 == status)
	{
		ERROR("run command Failed!");
		sendRespondBySocket(client_sockfd,SOCKET_FAILED);
	}
	else{

		if (WIFEXITED(status)){

			if (0 == WEXITSTATUS(status)){

				APPPRINTF("run command successfully.");
				sendRespondBySocket(client_sockfd,SOCKET_DONE);
			}else{

				ERROR("run command failed");
				sendRespondBySocket(client_sockfd,SOCKET_FAILED);
			}
		}else{

			APPPRINTF("exit status -->	[%d]", WEXITSTATUS(status));
			sendRespondBySocket(client_sockfd,SOCKET_EXIT);
		}
	}

}

static void *connection_handler(void *socket_desc)
{
	char buffer[MAX_BUFFER];
	int size, write_size , len , ret;
	int client_sockfd = *(int*)socket_desc;

	APPPRINTF("client_sockfd --> %d", client_sockfd);
	open_pipe_file();

	while(1){

		memset(buffer, 0, sizeof(buffer));							   // Clear the data buffer
#if 0
		if ((size = read(client_sockfd, buffer, MAX_BUFFER)) <= 0 )    // block until there are client data to read
		{
			if(size == 0){
         	 	APPPRINTF("Client disconnected");
			}else{
				ERROR("Recv Failed!");
			}
			break;
		}
#endif
		len = sizeof(buffer)/sizeof(buffer[0]);
		ret = read_pipe_file(buffer,len);
		if (ret > 0)
		{
			//buffer[size] = '\0';
			//APPPRINTF("Recv msg from client: %s", buffer);
			//executeCommand(buffer,client_sockfd);
			write(client_sockfd, buffer, len);
		}
	}

    close(client_sockfd);   //close cient Socket
	close_pipe_file();
	return 0;
}


int main(int argc, char **argv)
{
    struct sockaddr_in server_addr, client_addr;
    int size, write_size;
	pid_t status;
	int err;
	static	int server_sockfd, client_sockfd;
	char *CLIENT_IP;
	int CLIENT_PORT;
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)    // create Socket
    {
        ERROR("Socket Created Failed!");
        exit(1);
    }
    APPPRINTF("Socket Create Success!");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	// No IP address specified, the address follows the host address
    //server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(PORT);
    bzero(&(server_addr.sin_zero), 8);

    int opt = 1;
    int res = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));    // Set up address multiplexing
    if (res < 0)
    {
        ERROR("Server reuse address failed!");
        exit(1);
    }

	// 绑定 特定的IP 和 Port , 服务-> 就是需要固定的 IP 和 port
    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        ERROR("Socket Bind Failed!");
        exit(1);
    }
    APPPRINTF("Socket Bind Success!");

    if (listen(server_sockfd, 5) == -1)                 // monitor
    {
        ERROR("Listened Failed!");
        exit(1);
    }
    APPPRINTF("Listening ....");
    socklen_t len = sizeof(client_addr);

	// 主线程 仅仅负责 和 client accept连接，并把 socketfd 传递给 读写client 线程
	// 主线程 不能即负责 和 client accept连接,也负责 读client 数据, 因为 accept 和 read 都是 阻塞操作, 会导致不能正常读数据
    while (1)
    {
		APPPRINTF("waiting connection...");
		// Block until client is connected , return val --> client socket fd
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &len)) == -1)
		{
			ERROR("Accepted Failed!");
			break;
		}
		CLIENT_IP = inet_ntoa(client_addr.sin_addr);	//	获取客户端 IP地址
		CLIENT_PORT = ntohs(client_addr.sin_port);	//	获取客户端 Port , 系统随机分配的
		APPPRINTF("       CLIENT = {%s:%d}", CLIENT_IP, CLIENT_PORT);
		APPPRINTF("connection established , client_sockfd --> %d",client_sockfd);
		APPPRINTF("waiting message...");

		// 启动 读写client 线程
		pthread_t clientReadWrite_thread;
		err = pthread_create(&clientReadWrite_thread, NULL, connection_handler, (void*)&client_sockfd);
		if(err != 0){

			ERROR("create Client read write thread Failed!");
		}

    }

    close(server_sockfd);	// close server  socket

    return 0;
}


