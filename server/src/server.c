#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"
#include "commands.h"

#define COMMAND_BUF_SIZE 4096

char lip[32] = "";
int hport;
int listenfd;

int serve(int connfd, int seed);

int main(int argc, char** argv) {
	int connfd;
	struct sockaddr_in addr;

  // handle -port
  int i;
  char* arg;
  char middle[128];
  for (i = 0; i < argc; ++i) {
    arg = argv[i];
    if (arg[0] == '-' && arg[1] != '-') {
      memcpy(middle, arg, strlen(arg));
      argv[i] = malloc(strlen(argv[i]) + 2);
      argv[i][0] = '-';
      memcpy(argv[i] + 1, middle, strlen(middle));
    }
  }

  // parse arguments
  char hport_str[16];
  char root_dir[64];
  parse_argv(argc, argv, lip, hport_str, root_dir);
  hport = atoi(hport_str);
  if (chdir(root_dir) == -1) {
    sprintf(error_buf, ERROR_PATT, "chdir", "main");
    perror(error_buf);
    exit(1);
  }

  // connect
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    sprintf(error_buf, ERROR_PATT, "socket", "main");
    perror(error_buf);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(hport);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    sprintf(error_buf, ERROR_PATT, "bind", "main");
    perror(error_buf);
		return 1;
	}

	if (listen(listenfd, 10) == -1) {
    sprintf(error_buf, ERROR_PATT, "listen", "main");
    perror(error_buf);
		return 1;
	}

  printf("FTP server start running\n");
  printf("IP address %s\n", lip);
  printf("port number %d\n", hport);
  getcwd(root_dir, sizeof(root_dir));
  printf("working directory %s\n", root_dir);

  // loop
  unsigned seed = 9608; // some lucky number
	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
      sprintf(error_buf, ERROR_PATT, "accept", "main");
      perror(error_buf);
			continue;
		} else {
			if (fork() == 0) {
				printf("Connection accepted.\n");
			} else {
				serve(connfd, seed++);
				return 0;
			}
		}
	}

	close(listenfd);
	return 0;
}

int serve(int connfd, int seed)
{
  srand(seed);
  close(listenfd);
  struct ServerState state;
  state.command_fd = connfd;
  state.data_fd = -1;
  state.listen_fd = -1;
  state.trans_mode = -1;
  state.logged = 0;
  state.hport = hport;
  state.binary_flag = 1;
  state.offset = 0;
  state.thread = 1;
  state.encrypt = 0;
  state.pub_exp = NULL;
  state.pub_mod = NULL;
  state.priv_exp = NULL;
  state.priv_mod = NULL;

  get_conn_info(connfd, state.lip, state.rip);
  printf("local ip %s, remote ip %s\n", state.lip, state.rip);

  int c_code = 0;
  int len = 0;
  char message[COMMAND_BUF_SIZE];
  char arg[COMMAND_BUF_SIZE];
  int flag;

  flag = send_msg(&state, RES_READY);

  // loop routine
  while ((len = read_msg(&state, message))) {
    printf("%s\n", message);
    c_code = parse_command(message, arg);

    if (!state.logged && c_code != USER_CODE && c_code != PASS_CODE) {
      flag = send_msg(&state, RES_WANTUSER);
      continue;
    }

    switch (c_code) {
      case USER_CODE:
        command_user(&state, arg);
        break;

      case PASS_CODE:
        command_pass(&state, arg);
        break;

      case XPWD_CODE:
        flag = send_msg(&state, RES_WANTUSER);
        break;

      case QUIT_CODE:
      case ABOR_CODE:
        command_quit(&state);
        return 0;

      case PORT_CODE:
        command_port(&state, arg);
        break;

      case PASV_CODE:
        command_pasv(&state);
        break;

      case RETR_CODE:
        command_retr(&state, arg);
        break;

      case STOR_CODE:
        command_stor(&state, arg);
        break;

      case SYST_CODE:
        flag = send_msg(&state, RES_SYSTEM);
        break;

      case TYPE_CODE:
        command_type(&state, arg);
        break;

      case LIST_CODE:
        command_list(&state, arg, 1);
        break;

      case NLST_CODE:
        command_list(&state, arg, 0);
        break;

      case MKD_CODE:
        command_mkd(&state, arg);
        break;

      case CWD_CODE:
        command_cwd(&state, arg);
        break;

      case RMD_CODE:
        command_rmd(&state, arg);
        break;

      case REST_CODE:
        command_rest(&state, arg);
        break;

      case MULT_CODE:
        command_mult(&state);
        break;

      case ENCR_CODE:
        command_encr(&state);
        break;

      case SIZE_CODE:
        command_size(&state, arg);
        break;

      case PWD_CODE:
        command_pwd(&state);
        break;

      default:
        command_unknown(&state);
        break;
    }

    memset(message, 0, COMMAND_BUF_SIZE);

    if (flag == -1) {
      printf("remote disconnected\n");
      break;
    }
  }

  close(state.command_fd);
  return 0;
}