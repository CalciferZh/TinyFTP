#include "commands.h"


int parse_command(char* message, char* arg)
{
  char command[16]; // actually all commands are 4 bytes or less
  split_command(message, command, arg);
  // strip_crlf(command);
  // strip_crlf(arg);
  str_lower(command);

  int ret = -1;

  if (strcmp(command, USER_COMMAND) == 0) {
    ret = USER_CODE;
  } 
  else if (strcmp(command, PASS_COMMAND) == 0) {
    ret = PASS_CODE;
  }
  else if (strcmp(command, XPWD_COMMAND) == 0) {
    ret = XPWD_CODE;
  }
  else if (strcmp(command, QUIT_COMMAND) == 0) {
    ret = QUIT_CODE;
  }
  else if (strcmp(command, PORT_COMMAND) == 0) {
    ret = PORT_CODE;
  }
  else if (strcmp(command, PASV_COMMAND) == 0) {
    ret = PASV_CODE;
  }
  else if (strcmp(command, RETR_COMMAND) == 0) {
    ret = RETR_CODE;
  }
  else if (strcmp(command, SYST_COMMAND) == 0) {
    ret = SYST_CODE;
  }
  else if (strcmp(command, STOR_COMMAND) == 0) {
    ret = STOR_CODE;
  }
  else if (strcmp(command, TYPE_COMMAND) == 0) {
    ret = TYPE_CODE;
  }
  else if (strcmp(command, ABOR_COMMAND) == 0) {
    ret = ABOR_CODE;
  }
  else if (strcmp(command, LIST_COMMAND) == 0) {
    ret = LIST_CODE;
  }
  else if (strcmp(command, NLST_COMMAND) == 0) {
    ret = NLST_CODE;
  }
  else if (strcmp(command, MKD_COMMAND) == 0) {
    ret = MKD_CODE;
  }
  else if (strcmp(command, CWD_COMMAND) == 0) {
    ret = CWD_CODE;
  }
  else if (strcmp(command, RMD_COMMAND) == 0) {
    ret = RMD_CODE;
  }
  else if (strcmp(command, REST_COMMAND) == 0) {
    ret = REST_CODE;
  }
  else if (strcmp(command, MULT_COMMAND) == 0) {
    ret = MULT_CODE;
  }
  else if (strcmp(command, ENCR_COMMAND) == 0) {
    ret = ENCR_CODE;
  }
  else if (strcmp(command, SIZE_COMMAND) == 0) {
    ret = SIZE_CODE;
  }
  else if (strcmp(command, PWD_COMMAND) == 0) {
    ret = PWD_CODE;
  }
  return ret;
}

int command_user(struct ServerState* state, char* uname)
{
  int ret = 0;
  //if (strcmp(uname, USER_NAME) == 0) {
  if (1) {
    send_msg(state, RES_ACCEPT_USER);
    ret = 1;
  } else {
    send_msg(state, RES_REJECT_USER);
  }

  return ret;
}

int command_pass(struct ServerState* state, char* pwd)
{
  //if (strcmp(pwd, PASSWORD) == 0) {
  if (1) {
    send_msg(state, RES_ACCEPT_PASS);
    state->logged = 1;
  } else {
    send_msg(state, RES_REJECT_PASS);
    state->logged = 0;
  }

  return state->logged;
}

int command_unknown(struct ServerState* state)
{
  send_msg(state, RES_UNKNOWN);
  return 0;
}

int command_port(struct ServerState* state, char* arg)
{
  struct sockaddr_in* addr = &(state->target_addr);

  if ((state->data_fd = socket(AF_INET, SOCK_STREAM,  IPPROTO_TCP)) == -1) {
    sprintf(error_buf, ERROR_PATT, "scoket", "aommand_port");
    perror(error_buf);
    send_msg(state, RES_REJECT_PORT);
    return -1;
  }

  // check
  if (!strlen(arg)) {
    send_msg(state, RES_ACCEPT_PORT);
    return 1;
  }

  char ip[64];
  int port = parse_addr(arg, ip);
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  // translate the decimal IP address to binary
  if (inet_pton(AF_INET, ip, &(addr->sin_addr)) <= 0) {
    sprintf(error_buf, ERROR_PATT, "inet_pton", "command_port");
    perror(error_buf);
    send_msg(state, RES_REJECT_PORT);
    return -1;
  }

  send_msg(state, RES_ACCEPT_PORT);
  state->trans_mode = PORT_MODE;
  return 1;
}

int command_pasv(struct ServerState* state)
{
  char* hip = state->hip;
  struct sockaddr_in* addr = &(state->target_addr);

  if ((state->listen_fd = socket(AF_INET, SOCK_STREAM,  IPPROTO_TCP)) == -1) {
    sprintf(error_buf, ERROR_PATT, "scoket", "command_pasv");
    perror(error_buf);
    send_msg(state, RES_REJECT_PASV);
    return -1;
  }



  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(INADDR_ANY);

  int p1, p2;
  int port = get_random_port(&p1, &p2);
  addr->sin_port = htons(port);

  int max_try = 16;
  while (bind(state->listen_fd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
    if (--max_try < 0) {
      sprintf(error_buf, ERROR_PATT, "bind", "command_pasv");
      perror(error_buf);
      send_msg(state, RES_REJECT_PASV);
      return -1;
    }
    port = get_random_port(&p1, &p2);
    addr->sin_port = htons(port);
  }

  if (listen(state->listen_fd, 10) == -1) {
    sprintf(error_buf, ERROR_PATT, "listen", "command_pasv");
    perror(error_buf);
    send_msg(state, RES_REJECT_PASV);
    return -1;
  }

  char hip_cp[32] = "";
  strcpy(hip_cp, hip);
  str_replace(hip_cp, '.', ',');
  char buffer[32] = "";
  sprintf(buffer, RES_ACCEPT_PASV, hip_cp, p1, p2);
  send_msg(state, buffer);
  state->trans_mode = PASV_MODE;

  return 1;
}

int command_quit(struct ServerState* state)
{
  send_msg(state, RES_CLOSE);
  close(state->command_fd);
  return 0;
}

int command_retr(struct ServerState* state, char* path)
{
  if (access(path, 4) != 0) { // 4: readable
    send_msg(state, RES_TRANS_NOFILE);
    return -1;
  }

  int src_fd;
  if ((src_fd = open(path, O_RDONLY)) == 0) {
    send_msg(state, RES_TRANS_NREAD);
    return -1;
  }

  if (connect_by_mode(state) != 0) {
    return -1;
    if (state->trans_mode == PORT_MODE) {
      send_msg(state, RES_FAILED_CONN);
    } else if (state->trans_mode == PASV_MODE) {
      send_msg(state, RES_FAILED_LSTN);
    } else {
      send_msg(state, RES_WANTCONN);
    }
  }

  send_msg(state, RES_TRANS_START);

  int flag;
  struct timespec t1, t2;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  if (state->thread == 1) {
    flag = send_file(state->data_fd, src_fd, state);
  } else {
    flag = send_file_mt(state->data_fd, src_fd, state);
  }
  clock_gettime(CLOCK_MONOTONIC, &t2);
  int delta = (int)((t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_nsec - t1.tv_nsec) / 1000000);

  printf("finish sending, time cost %dms\n", delta);

  if (flag == 0) {
    send_msg(state, RES_TRANS_SUCCESS);
  } else {
    send_msg(state, RES_TRANS_FAIL);
  }

  close_connections(state);
  state->offset = 0;
  return 0;
}

int command_stor(struct ServerState* state, char* path)
{
  int des_fd;
  int file_size = -1;
  char path_buf[256];
  char length_buf[256];
  if (state->encrypt) { // one more arg for file size
    split_command(path, path_buf, length_buf);
    strcpy(path, path_buf);
    file_size = atoi(length_buf);
  }

  if ((des_fd = open(path, O_WRONLY | O_CREAT)) == 0) {
    send_msg(state, RES_TRANS_NCREATE);
    return -1;
  }

  if (connect_by_mode(state) != 0) {
    return -1;
  }

  send_msg(state, RES_TRANS_START);
  printf("start receiving...\n");
  if (recv_file(des_fd, state->data_fd, state, file_size) == 0) {
    send_msg(state, RES_TRANS_SUCCESS);
  } else {
    send_msg(state, RES_TRANS_FAIL);
  }
  printf("finished receiving...\n");
  close_connections(state);
  return 0;
}

int command_type(struct ServerState* state, char* arg)
{
  str_lower(arg);
  if (arg[0] == 'i' || arg[0] == 'l') {
    state->binary_flag = 1;
    send_msg(state, RES_ACCEPT_TYPE);
  } else if (arg[0] == 'a') {
    state->binary_flag = 0;
    send_msg(state, RES_ACCEPT_TYPE);
  } else {
    send_msg(state, RES_ERROR_ARGV);
  }
  return 0;
}

int command_list(struct ServerState* state, char* path, int is_long)
{
  if (strlen(path) == 0) {
    path = "./";
  }
  
  if (access(path, 0) != 0) { // 0: existence
    send_msg(state, RES_TRANS_NOFILE);
    return -1;
  }

  char cmd[128];
  if (is_long) {
    sprintf(cmd, "ls -l %s", path);
  } else {
    sprintf(cmd, "ls %s", path);
  }
  
  FILE* fp = popen(cmd, "r");

  if (!fp) {
    sprintf(error_buf, ERROR_PATT, "popen", "command_list");
    perror(error_buf);
    send_msg(state, RES_TRANS_NREAD);
  }

  if (connect_by_mode(state) != 0) {
    return -1;
  }

  send_msg(state, RES_TRANS_START);

  char buf[256];
  if (is_long) {
    fgets(buf, sizeof(buf), fp); // remove the first line
  }
  while (fgets(buf, sizeof(buf), fp)) {
    strip_crlf(buf);
    strcat(buf, "\r\n");
    // if (state->encrypt) {
    //   char* data = encodeBytes(buf, strlen(buf), state->priv_exp, state->priv_mod);

    // } else {
    write(state->data_fd, buf, strlen(buf));
    // }
  }

  printf("Command list finish transfer.\n");
  send_msg(state, RES_TRANS_SUCCESS);
  pclose(fp);
  close_connections(state);
  return 0;
}

int command_mkd(struct ServerState* state, char* path)
{
  int flag = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (flag == 0) {
    send_msg(state, RES_ACCEPT_MKD);
  } else {
    send_msg(state, RES_REJECT_MKD);
  }
  return flag;
}

int command_cwd(struct ServerState* state, char* path)
{
  if (chdir(path) == -1) {
    send_msg(state, RES_REJECT_CWD); 
  } else {
    send_msg(state, RES_ACCEPT_CWD);
  }
  return 0;
}

int command_rmd(struct ServerState* state, char* path)
{
  char cmd[32] = "rm -rf ";
  strcat(cmd, path);
  if (system(cmd) == 0) {
    send_msg(state, RES_ACCEPT_RMD);
  } else {
    send_msg(state, RES_REJECT_RMD);
  }
  return 0;
}

int command_rest(struct ServerState* state, char* arg)
{
  state->offset = atoi(arg);
  if (state->offset >= 0) {
    send_msg(state, RES_ACCEPT_REST);
  } else {
    state->offset = 0;
    send_msg(state, RES_REJECT_REST);
  }
  return 0;
}

int command_mult(struct ServerState* state)
{
  if (state->thread == 1) {
    state->thread = 2;
    send_msg(state, RES_MULTIT_ON);
  } else {
    state->thread = 1;
    send_msg(state, RES_MULTIT_OFF);
  }
  return 0;
}

int command_encr(struct ServerState* state)
{
  if (state->encrypt == 1) {
    state->encrypt = 0;
    send_msg(state, RES_ENCR_OFF);
    bignum_deinit(state->pub_exp);
    bignum_deinit(state->pub_mod);
    bignum_deinit(state->priv_exp);
    bignum_deinit(state->priv_mod);
    state->pub_exp = NULL;
    state->pub_mod = NULL;
    state->priv_exp = NULL;
    state->priv_mod = NULL;
  } else {
    printf("Generating RSA key...\n");
    gen_rsa_key(&(state->pub_exp), &(state->pub_mod),
      &(state->priv_exp), &(state->priv_mod), &(state->bytes));
    printf("finished rsa-key generation\n");

    char* pub_exp = bignum_tostring(state->pub_exp);
    char* pub_mod = bignum_tostring(state->pub_mod);
    char buf[1024];
    sprintf(buf, RES_ENCR_ON, pub_exp, pub_mod, state->bytes);
    send_msg(state, buf);
    free(pub_exp);
    free(pub_mod);

    state->encrypt = 1;
    send_msg(state, "200 Hello, here is an encrypted message.");
  }
  return 0;
}

int command_size(struct ServerState* state, char* path)
{
  int src_fd;
  if ((src_fd = open(path, O_RDONLY)) == 0) {
    send_msg(state, RES_REJECT_SIZE);
    return -1;
  }

  struct stat stat_buf;
  fstat(src_fd, &stat_buf);
  close(src_fd);

  char buf[128];
  sprintf(buf, RES_ACCEPT_SIZE, (int)stat_buf.st_size);
  send_msg(state, buf);
  return 0;
}

int command_pwd(struct ServerState* state)
{
  char buf1[1024], buf2[1024];
  getcwd(buf1, 1024);
  sprintf(buf2, RES_ACCPET_PWD, buf1);
  send_msg(state, buf2);
  return 0;
}
