#ifndef __UTILS_H__
#define __UTILS_H__

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
#include <time.h>
#include "rsa.h"

#define ERROR_PATT             "Error %s() in %s()"
#define DATA_BUF_SIZE 8192
#define DATA_BUF_SIZE_LARGE 1048576
#define PORT_MODE 0
#define PASV_MODE 1

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
  int encrypt;
  char hip[32];
  bignum* pub_exp;
  bignum* pub_mod;
  bignum* priv_exp;
  bignum* priv_mod;
  int bytes;
  struct sockaddr_in target_addr;
};

struct write_para
{
  int des_fd;
  char* buf;
  int size;
};

char error_buf[128];

int send_msg(struct ServerState* state, char* str);
int read_msg(struct ServerState* state, char* message);
int send_file(int des_fd, int src_fd, int offset);
int send_file_mt(int des_fd, int src_fd, int offset);
int connect_by_mode(struct ServerState* state);
int parse_addr(char* arg, char* ip_buf);
int parse_argv(int argc, char** argv, char* hip, char* hport, char* root);
int get_random_port(int* p1, int* p2);
int recv_file(int des_fd, int src_fd);
int close_connections(struct ServerState* state);
void write_thread(void* arg);
void str_lower(char* str);
void str_replace(char* str, char src, char des);
void split_command(char* message, char* command, char* arg);
void strip_crlf(char* uname);

#endif