#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <net/if.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define USER_CODE 0
#define PASS_CODE 1
#define XPWD_CODE 2
#define QUIT_CODE 3
#define PORT_CODE 4
#define PASV_CODE 5
#define RETR_CODE 6
#define SYST_CODE 7
#define STOR_CODE 8
#define TYPE_CODE 9
#define ABOR_CODE 10
#define LIST_CODE 11
#define NLST_CODE 12
#define MKD_CODE  13
#define CWD_CODE  14
#define RMD_CODE  15
#define REST_CODE 16
#define MULT_CODE 17

#define USER_COMMAND "user"
#define PASS_COMMAND "pass"
#define XPWD_COMMAND "xpwd"
#define QUIT_COMMAND "quit"
#define PORT_COMMAND "port"
#define PASV_COMMAND "pasv"
#define RETR_COMMAND "retr"
#define SYST_COMMAND "syst"
#define STOR_COMMAND "stor"
#define TYPE_COMMAND "type"
#define ABOR_COMMAND "abor"
#define LIST_COMMAND "list"
#define NLST_COMMAND "nlst"
#define MKD_COMMAND  "mkd"
#define CWD_COMMAND  "cwd"
#define RMD_COMMAND  "rmd"
#define REST_COMMAND "rest"
#define MULT_COMMAND "mult"


#define RES_READY              "220 Anonymous FTP server ready.\r\n"
#define RES_UNKNOWN            "500 Unknown command.\r\n"

#define RES_WANTUSER           "500 Command USER is expected.\r\n"
#define RES_ACCEPT_USER        "331 Please enter password.\r\n"
#define RES_REJECT_USER        "503 Unknown user.\r\n"

#define RES_WANTPASS           "500 Command PASS is expected.\r\n"
#define RES_ACCEPT_PASS        "220 Password accepted.\r\n"
#define RES_REJECT_PASS        "503 Wrong password.\r\n"

#define RES_ACCEPT_PORT        "200 PORT command success.\r\n"
#define RES_REJECT_PORT        "425 PORT command failed.\r\n"

#define RES_ACCEPT_PASV        "227 =%s,%d,%d\r\n"
#define RES_REJECT_PASV        "425 PASV command failed.\r\n"

#define RES_FAILED_CONN        "425 Connection attempt failed.\r\n"
#define RES_FAILED_LSTN        "425 Listen for request failed.\r\n"

#define RES_TRANS_START        "150 Start transfer.\r\n"
#define RES_TRANS_NOFILE       "551 File does not exist.\r\n"
#define RES_TRANS_NREAD        "451 Failed to read.\r\n"
#define RES_TRANS_SUCCESS      "226 Transfer success.\r\n"
#define RES_TRANS_FAIL         "426 Transfer failed.\r\n"

#define RES_ACCEPT_MKD         "250 Directory created succesfully.\r\n"
#define RES_REJECT_MKD         "550 Directory created failed.\r\n"

#define RES_ACCEPT_CWD         "250 Command CWD accepted.\r\n"
#define RES_REJECT_CWD         "550 Command CWD rejected.\r\n"

#define RES_ACCEPT_RMD         "250 Command RMD accepted.\r\n"
#define RES_REJECT_RMD         "550 Command RMD rejected.\r\n"

#define RES_ACCEPT_REST        "350 Command REST accepted.\r\n"
#define RES_REJECT_REST        "500 Command REST rejected.\r\n"

#define RES_MULTIT_ON          "200 Switch to multi-thread mode.\r\n"
#define RES_MULTIT_OFF         "200 Switch to single-thread mode.\r\n"

#define RES_TRANS_NCREATE      "551 Cannot create file.\r\n"

#define RES_WANTCONN           "425 Require PASV or PORT.\r\n"

#define RES_SYSTEM             "215 UNIX Type: L8\r\n"

#define RES_ERROR_ARGV         "504 Illegal argument.\r\n"

#define RES_ACCEPT_TYPE        "200 Type set to I.\r\n"

#define RES_CLOSE              "421 Bye.\r\n"

#define USER_NAME              "anonymous"
#define PASSWORD               "some_password"

#define ERROR_PATT             "Error %s() in %s()"

#define DATA_BUF_SIZE 8192

struct ServerState
{
  int command_fd;
  int data_fd;
  int listen_fd;
  int trans_mode;
  int logged;
  int hport;
  int binary_flag;
  int offset;
  int thread;
  char hip[32];
  struct sockaddr_in target_addr;
};

struct write_para
{
  int des_fd;
  char* buf;
  int size;
};

// a secured method to send message
int send_msg(int connfd, char* message);

// a secured method to receive message
int read_msg(int connfd, char* message);

int send_file(int des_fd, int src_fd, int offset);

void str_lower(char* str);

void str_replace(char* str, char src, char des);

void split_command(char* message, char* command, char* content);

// parse the command from client
int parse_command(char* message, char* content);

int connect_by_mode(struct ServerState* state);

// parse ip address & port
int parse_addr(char* content, char* ip_buf);

int parse_argv(int argc, char** argv, char* hip, char* hport, char* root);

void strip_crlf(char* uname);

int command_user(struct ServerState* state, char* uname);

// return 1 if password is correct
int command_pass(struct ServerState* state, char* pwd);

int command_unknown(struct ServerState* state);

int command_port(struct ServerState* state, char* content);

int command_pasv(struct ServerState* state);

int command_quit(struct ServerState* state);

int command_retr(struct ServerState* state, char* path);

int command_stor(struct ServerState* state, char* path);

int command_type(struct ServerState* state, char* content);

int command_list(struct ServerState* state, char* path, int is_long);

int command_mkd(struct ServerState* state, char* path);

int command_cwd(struct ServerState* state, char* path);

int command_rmd(struct ServerState* state, char* path);

int command_rest(struct ServerState* state, char* content);

int command_mult(struct ServerState* state);

int get_random_port(int* p1, int* p2);

char error_buf[128];

