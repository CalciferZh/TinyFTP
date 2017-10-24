#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#define USER_CODE 0
#define PASS_CODE 1
#define XPWD_CODE 2
#define QUIT_CODE 3
#define PORT_CODE 4


#define USER_COMMAND "user"
#define PASS_COMMAND "pass"
#define XPWD_COMMAND "xpwd"
#define QUIT_COMMAND "quit"
#define PORT_COMMAND "port"


#define RES_READY              "220 Anonymous FTP server ready.\r\n"
#define RES_UNKNOWN            "500 Unknown command.\r\n"

#define RES_WANTUSER           "500 Command USER is expected.\r\n"
#define RES_ACCEPT_USER        "331 Please enter password.\r\n"
#define RES_REJECT_USER        "503 Unknown user.\r\n"

#define RES_WANTPASS           "500 Command PASS is expected.\r\n"
#define RES_ACCEPT_PASS        "220 Password accepted.\r\n"
#define RES_REJECT_PASS        "503 Wrong password.\r\n"

#define RES_CLOSE              "421 Bye.\r\n"

#define USER_NAME              "anonymous"
#define PASSWORD               "some_password"

// a secured method to send message
int send_msg(int connfd, char* message);

// a secured method to receive message
int read_msg(int connfd, char* message);

void str_lower(char* str);

void split_command(char* message, char* command, char* content);

// parse the command from client
int parse_command(char* message, char* content);

int command_user(int connfd, char* uname);

void strip_crlf(char* uname);

// return 1 if password is correct
int command_pass(int connfd, char* pwd);

int command_unknown(int connfd);

int command_port(int connfd, struct sockaddr_in* des);

int command_quit(int connfd);

int serve(int connfd);



