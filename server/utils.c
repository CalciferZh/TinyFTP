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
    int n = write(connfd, message + p, len + 1 - p);
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
    int p = 0;
    while (1) {
      int n = read(connfd, message + p, 8191 - p);
      if (n < 0) {
        printf("Error read(): %s(%d)\n", strerror(errno), errno);
        close(connfd);
        return -1;
      } else if (n == 0) {
        break;
      } else {
        p += n;
      }
    }
    if (message[p - 1] != '\0') {
      message[p] = '\0';
      p += 1;
    }
    return p;
}

void str_lower(char* str)
{
  int p = 0;
  int len = strlen(str);
  for (p = 0; p < len; p++) {
    str[p] = toupper(str[p]);
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
  command[strlen(command)] = '\0';
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
