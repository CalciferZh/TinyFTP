#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

int send_msg(int connfd, char* message)
{
  int p = 0;
  int len = strlen(message);
  while (p < len) {
    int n = write(connfd, message + p, len - p);
    if (n < 0) {
      printf("Error write(): %s(%d)\n", strerror(errno), errno);
      return -1;
    } else {
      p += n;
    }     
  }
  return 0;
}

int read_msg(int connfd, char* message)
{
  int n = read(connfd, message, 8191);
  if (n < 0) {
    printf("Error read(): %s(%d)\n", strerror(errno), errno);
    close(connfd);
    return -1;
  }
  message[n] = '\0';
  return n;
}

void str_lower(char* str)
{
  int p = 0;
  int len = strlen(str);
  for (p = 0; p < len; p++) {
    str[p] = tolower(str[p]);
  }
}

void split_command(char* message, char* command, char* content)
{
  // printf("%s\n", message);
  char* blank = strchr(message, ' ');

  // char* p = message;
  // while(p != blank) {
  //   putchar(*(p++));
  // }

  // printf("\n%d\n", (int)(blank - message));
  strncpy(command, message, (int)(blank - message));
  command[(int)(blank - message)] = '\0';
  // printf("%s\n", command);
  
  strcpy(content, blank + 1);
  // printf("%s\n", content);
}

int parse_command(char* message, char* content)
{
  char command[16]; // actually all commands are 4 bytes or less
  split_command(message, command, content);
  str_lower(command);

  int ret = -1;

  if (strcmp(command, USER_COMMAND) == 0) {
    ret = USER_CODE;
  } else {
    printf("Unknown command: %s\n", command);
  }

  return ret;
}

int handle_command(char* message)
{
  char content[128];
  int code = parse_command(message, content);
  int ret = 0;
  switch (code) {
    case USER_CODE:
      if (strcmp(content, USER_NAME) == 0){

      }
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

int serve(int connfd)
{
  int ret_code = 0;
  int c_code = 0;
  int len = 0;
  char message[4096];
  char content[4096];

  send_msg(connfd, RES_READY);
  // expecting user
  while (len = read_msg(connfd, message)) {
    printf("%s\n", message);
    c_code = parse_command(message, content);
    if (c_code != USER_CODE) {
      send_msg(connfd, RES_WANTUSER);
    } else {
      break;
    }
  }

  close(connfd);
  return ret_code;
}

