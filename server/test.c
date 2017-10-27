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
  char command_truth[] = "USER";
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

void parse_addr_test()
{
  char content[] = "166,111,81,14,215,10";
  char ip[64] = "";
  int port = parse_addr(content, ip);

  if (strcmp(ip, "166.111.81.14") != 0) {
    printf("Parse addr test: wrong ip addr:%s\n", ip);
  } else if (port != 55050) {
    printf("Parse addr test: wrong port:%d\n", port);
  } else {
    printf("Parse addr test passed.\n");
  }
}

void command_list_test()
{
  FILE *fp;
  fp = popen("ls -l", "r");
  char buffer[256];
  while (fgets(buffer, 256, fp)) {
    printf("%s", buffer);
  }
  pclose(fp); 
}

int main()
{
  printf("================================================================\n");
  command_list_test();
  // split_command_test();
  // parse_addr_test();
  return 0;
}

