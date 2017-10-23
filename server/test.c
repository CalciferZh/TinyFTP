#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

void split_command_test()
{
  char message[] = "USER Calcifer";
  char command[16] = "";
  char command_truth[] = "user";
  char content[128] = "";
  char content_truth[] = "Calcifer";
  split_command(message, command, content);

  // int i = 0;
  // for (i = 0; i < 16; ++i) {
  //   putchar(command[i]);
  // }
  // printf("\n-------------------------------\n");
  // for (i = 0; i < 16; ++i) {
  //   putchar(content[i]);
  // }

  if (strcmp(command, command_truth) != 0) {
    printf("Split command test: wrong command:%s.\n", command);
  } else if (strcmp(content, content_truth) != 0) {
    printf("Split command test: wrong content:%s\n", content);
  } else {
    printf("Split command test passed.\n");
  }
}

int main()
{
  printf("================================================================\n");
  split_command_test();
  return 0;
}

