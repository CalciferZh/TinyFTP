#include "utils.h"
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

int send_msg(int connfd, char* sentence, int len)
{
  int p = 0;
  while (p < len) {
    int n = write(connfd, sentence + p, len + 1 - p);
    if (n < 0) {
      printf("Error write(): %s(%d)\n", strerror(errno), errno);
      return -1;
    } else {
      p += n;
    }     
  }
  return 0;
}

int read_msg(int connfd, char* sentence)
{
    int p = 0;
    while (1) {
      int n = read(connfd, sentence + p, 8191 - p);
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
    if (sentence[p - 1] != '\0') {
      sentence[p] = '\0';
      p += 1;
    }
    return p;
}
