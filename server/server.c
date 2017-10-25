#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

char hip[32] = "";

int serve(int connfd);

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in addr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	get_local_ip(listenfd, hip);
	printf("host ip address(eth0): %s\n", hip);

	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		} else {
			if (fork() == 0) {
				printf("Connection accepted.\n");
			} else {
				serve(connfd);
				return 0;
			}
		}
	}

	close(listenfd);
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
  int datafd;
  struct sockaddr_in addr;

  send_msg(connfd, RES_READY);

  // loop routine
  while ((len = read_msg(connfd, message))) {
    printf("%s", message);
    c_code = parse_command(message, content);

    if (!logged && c_code != USER_CODE && c_code != PASS_CODE) {
      send_msg(connfd, RES_WANTUSER);
      continue;
    }
    // else if (logged && want_pwd &&
    //          c_code != PASS_CODE && c_code != XPWD_CODE) {
    //   send_msg(connfd, RES_WANTPASS);
    //   continue;
    // }

    switch (c_code) {
      case USER_CODE:
        command_user(connfd, content);
        break;

      case PASS_CODE:
        if (command_pass(connfd, content)) {
          send_msg(connfd, RES_ACCEPT_PASS);
          logged = 1;
        } else {
          send_msg(connfd, RES_REJECT_PASS);
        }
        break;

      case XPWD_CODE:
        send_msg(connfd, RES_WANTUSER);
        break;

      case QUIT_CODE:
        command_quit(connfd);
        return ret_code;
        break;

      case PORT_CODE:
        datafd = command_port(connfd, content, &addr);
        break;

      case PASV_CODE:
        datafd = command_pasv(connfd, hip, &addr);
        break;

      default:
        command_unknown(connfd);
        break;
    }
  }

  close(connfd);
  return ret_code;
}