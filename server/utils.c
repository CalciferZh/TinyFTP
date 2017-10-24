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
  char* blank = strchr(message, ' ');

  if (blank != NULL) {
    strncpy(command, message, (int)(blank - message));
    command[(int)(blank - message)] = '\0';
    strcpy(content, blank + 1);
  } else {
    strcpy(command, message);
    content[0] = '\0';
  }
}

int parse_command(char* message, char* content)
{
  char command[16]; // actually all commands are 4 bytes or less
  split_command(message, command, content);
  strip_crlf(command);
  strip_crlf(content);
  str_lower(command);

  int ret = -1;

  if (strcmp(command, USER_COMMAND) == 0) {
    ret = USER_CODE;
  } 
  else if(strcmp(command, PASS_COMMAND) == 0) {
    ret = PASS_CODE;
  }
  else if(strcmp(command, XPWD_COMMAND) == 0) {
    ret = XPWD_CODE;
  }
  else {
    printf("Unknown command: %s\n", command);
  }

  return ret;
}

void strip_crlf(char* str)
{
  int len = strlen(str);
  if (str[len - 2] == '\n' || str[len - 2] == '\r') {
    str[len - 2] = '\0';
    if (str[len - 2] == '\n' || str[len - 2] == '\r') {
      str[len - 3] = '\0';
    }
  }
}

int command_user(int connfd, char* uname)
{
  int ret = 0;
  if (strcmp(uname, USER_NAME) == 0) {
    send_msg(connfd, RES_ACCEPT_USER);
    ret = 1;
  } else {
    send_msg(connfd, RES_REJECT_USER);
  }

  return ret;
}

int command_pass(int connfd, char* pwd)
{
  int ret = 0;

  if (strcmp(pwd, PASSWORD) == 0) {
    send_msg(connfd, RES_ACCEPT_PASS);
    ret = 1;
  } else {
    send_msg(connfd, RES_REJECT_PASS);
  }

  return ret;
}

int command_unknown(int connfd)
{
  send_msg(connfd, RES_UNKNOWN);
  return 0;
}

int serve(int connfd)
{
  int ret_code = 0;
  int c_code = 0;
  int len = 0;
  int logged = 0;
  char message[4096];
  char content[4096];

  // to implement the strange feature
  // which requires PASS follows USER
  int want_pwd = 0;

  send_msg(connfd, RES_READY);

  // loop routine
  while ((len = read_msg(connfd, message))) {
    printf("%s", message);
    c_code = parse_command(message, content);

    if (!logged && c_code != USER_CODE) {
      send_msg(connfd, RES_WANTUSER);
      continue;
    }
    else if (logged && want_pwd &&
             c_code != PASS_CODE && c_code != XPWD_CODE) {
      send_msg(connfd, RES_WANTPASS);
      continue;
    }

    switch (c_code) {
      case USER_CODE:
        logged = command_user(connfd, content);
        want_pwd = logged;
        break;

      case PASS_CODE:
        if (!want_pwd) {
          send_msg(connfd, RES_WANTUSER);
        } else {
          if (command_pass(connfd, content)) {
            send_msg(connfd, RES_ACCEPT_PASS);
            want_pwd = 0;
          } else {
            send_msg(connfd, RES_REJECT_PASS);
          }
        }
        break;

      case XPWD_CODE:
        if (!want_pwd) {
          send_msg(connfd, RES_WANTUSER);
        } else {
          send_msg(connfd, RES_ACCEPT_USER);
        }
        break;

      default:
        command_unknown(connfd);
        break;
    }
  }

  close(connfd);
  return ret_code;
}

