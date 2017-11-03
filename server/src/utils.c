#include "utils.h"

int send_msg(struct ServerState* state, char* str)
{
  int len = strlen(str);
  char* message;
  if (state->encrypt) {
    message = encodeBytes(str, len, state->bytes, state->priv_exp, state->priv_mod);
    len = get_len_after_encoding(len, state->bytes);
  } else {
    message = str;
  }
  // printf("sending %d bytes\n", len);
  int p = 0;
  while (p < len) {
    int n = write(state->command_fd, message + p, len - p);
    if (n < 0) {
      sprintf(error_buf, ERROR_PATT, "write", "send_msg");
      perror(error_buf);
      return -1;
    } else {
      p += n;
    }     
  }
  if (state->encrypt) {
    free(message);
    // print('sent encrypted message %d bytes', )
  }
  return 0;
}

int send_file(int des_fd, int src_fd, struct ServerState* state)
{
  printf("preparing to send file in single-thread mode...\n");
  int offset = state->offset;

  struct stat stat_buf;
  fstat(src_fd, &stat_buf);
  lseek(src_fd, offset, SEEK_SET);

  int to_read, fin_read;
  char buf[DATA_BUF_SIZE];
  char* middle;

  int remain = stat_buf.st_size - offset;
  printf("Start file transfer...\n");
  printf("%d bytes to send...\n", remain);

  while (remain > 0) {
    if (state->encrypt) {
      to_read = remain < DATA_BUF_SIZE_SMALL ? remain : DATA_BUF_SIZE_SMALL;
    } else {
      to_read = remain < DATA_BUF_SIZE ? remain : DATA_BUF_SIZE;
    }
    fin_read = read(src_fd, buf, to_read);
    // printf("read %d bytes...\n", fin_read);
    remain -= fin_read;
    if (fin_read < 0) {
      sprintf(error_buf, ERROR_PATT, "read", "send_file");
      perror(error_buf);
      return -1;
    }
    if (state->encrypt) {
      middle = encodeBytes(buf, fin_read, state->bytes, state->priv_exp, state->priv_mod);
      fin_read = get_len_after_encoding(fin_read, state->bytes);
      memcpy(buf, middle, fin_read);
      free(middle);
      // printf("sending %d bytes...\n", fin_read);
    }
    if (write(des_fd, buf, fin_read) == -1) {
      sprintf(error_buf, ERROR_PATT, "write", "send_file");
      perror(error_buf);
      return -1;
    }
  }
  printf("Transfer success!\n");
  return 0;
}

void write_thread(void* arg)
{
  struct write_para* para = (struct write_para*)arg;
  write(para->des_fd, para->buf, para->size);
}

int send_file_mt(int des_fd, int src_fd, struct ServerState* state)
{
  printf("preparing to send file in multi-thread mode...\n");
  int offset = state->offset;

  struct stat stat_buf;
  fstat(src_fd, &stat_buf);
  lseek(src_fd, offset, SEEK_SET);

  int to_read;
  int fin_read_1;
  int fin_read_2;
  char buf_1[DATA_BUF_SIZE_LARGE];
  char buf_2[DATA_BUF_SIZE_LARGE];

  int remain = stat_buf.st_size - offset;

  printf("Start file transfer...\n");
  printf("%d bytes to send...\n", remain);

  pthread_t pid;

  int ret;

  struct write_para arg;

  to_read = remain < DATA_BUF_SIZE ? remain : DATA_BUF_SIZE;
  fin_read_1 = read(src_fd, buf_1, to_read);
  if (fin_read_1 < 0) {
    sprintf(error_buf, ERROR_PATT, "read", "send_file");
    perror(error_buf);
    return -1;
  }
  remain -= fin_read_1;

  arg.des_fd = des_fd;
  // at least run once to sendout buf_1
  while (remain > 0) {
    // new thread to send buf_1
    arg.buf = buf_1;
    arg.size = fin_read_1;
    ret = pthread_create(&pid, NULL, (void*)write_thread, (void *)&arg);
    if (ret) {
      sprintf(error_buf, ERROR_PATT, "pthread_create", "send_file_mt");
      perror(error_buf);
      return -1;
    }

    // at the same time fill buf_2
    to_read = remain < DATA_BUF_SIZE ? remain : DATA_BUF_SIZE;
    fin_read_2 = read(src_fd, buf_2, to_read);
    if (fin_read_2 < 0) {
      sprintf(error_buf, ERROR_PATT, "read", "send_file_mt");
      perror(error_buf);
      return -1;
    }
    remain -= fin_read_2;

    // sync
    pthread_join(pid, NULL);

    // new thread to send buf_2
    arg.buf = buf_2;
    arg.size = fin_read_2;
    ret = pthread_create(&pid, NULL, (void*)write_thread, (void *)&arg);
    if (ret) {
      sprintf(error_buf, ERROR_PATT, "pthread_create", "send_file_mt");
      perror(error_buf);
      return -1;
    }

    // at the same time fill buf_1
    to_read = remain < DATA_BUF_SIZE ? remain : DATA_BUF_SIZE;
    fin_read_1 = read(src_fd, buf_1, to_read);
    if (fin_read_1 < 0) {
      sprintf(error_buf, ERROR_PATT, "read", "send_file_mt");
      perror(error_buf);
      return -1;
    }
    remain -= fin_read_1;

    // sync
    pthread_join(pid, NULL);
  }
  write(des_fd, buf_1, fin_read_1);

  printf("Transfer success!\n");
  return 0;
}

int recv_file(int des_fd, int src_fd, struct ServerState* state, int file_sz)
{
  // printf("receiving file...\n");
  int rcvd;
  int stored;
  int written;
  int to_decode;
  int decoded_len;
  int to_write;
  int flag;
  char* middle;
  char cache[DATA_BUF_SIZE + BLOCK_LENGTH_BYTES];
  char buf[DATA_BUF_SIZE];

  if (state->encrypt) {
    if (file_sz <= 0) {
      printf("error in recv_file: invalid file size\n");
      return -1;
    }
    // printf("ready to receive %d bytes\n", file_sz);
    written = 0;
    stored = 0;
    while((rcvd = read(src_fd, buf, DATA_BUF_SIZE_SMALL)) > 0) {
      // printf("stored %d, new received %d\n", stored, rcvd);
      memcpy(cache + stored, buf, rcvd);
      stored += rcvd;
      // printf("now store %d bytes\n", stored);
      to_decode = stored - (stored % BLOCK_LENGTH_BYTES);
      // printf("start decoding...\n");
      middle = decodeBytes(cache, to_decode, state->bytes, state->priv_exp, state->priv_mod);
      decoded_len = to_decode / BLOCK_LENGTH_BYTES * BLOCK_SIZE;
      // printf("decode %d bytes to %d bytes\n", to_decode, decoded_len);
      to_write = (file_sz - written) > decoded_len ? decoded_len : (file_sz - written);
      flag = write(des_fd, middle, to_write);
      // printf("write %d bytes\n", to_write);
      written += to_write;
      // printf("%d bytes written in total\n", written);
      free(middle);
      if (flag == -1) {
        sprintf(error_buf, ERROR_PATT, "write", "recv_file");
        perror(error_buf);
        return -1;
      }
      stored -= to_decode;
      memcpy(buf, cache + to_decode, stored);
      memcpy(cache, buf, stored);
      // printf("flushed cache\n");
    }
    if (rcvd == 0) {
      // printf("to process final packet: by now written %d bytes, file size %d bytes\n", written, file_sz);
      middle = decodeBytes(cache, stored, state->bytes, state->priv_exp, state->priv_mod);
      flag = write(des_fd, middle, file_sz - written);
      // printf("write last %d bytes\n", file_sz - written);
      free(middle);
      return 0;
    } else {
      sprintf(error_buf, ERROR_PATT, "read", "recv_file");
      perror(error_buf);
      return -1;
    }
  } else {
    while((rcvd = read(src_fd, buf, DATA_BUF_SIZE)) > 0) {
      if (write(des_fd, buf, rcvd) == -1) {
        sprintf(error_buf, ERROR_PATT, "write", "recv_file");
        perror(error_buf);
        return -1;
      }
    }
    if (rcvd == 0) {
      return 0;
    } else {
      sprintf(error_buf, ERROR_PATT, "read", "recv_file");
      perror(error_buf);
      return -1;
    }
  }
}

int read_msg(struct ServerState* state, char* str)
{
  int n;

  n = read(state->command_fd, str, DATA_BUF_SIZE);

  if (n < 0) {
    sprintf(error_buf, ERROR_PATT, "read", "read_msg");
    perror(error_buf);
    close(state->command_fd);
    return -1;
  }

  if (state->encrypt) {
    char* message = decodeBytes(str, n, state->bytes, state->priv_exp, state->priv_mod);
    strcpy(str, message);
    free(message);
  }

  strip_crlf(str);
  return strlen(str);
}

void str_lower(char* str)
{
  int p = 0;
  int len = strlen(str);
  for (p = 0; p < len; p++) {
    str[p] = tolower(str[p]);
  }
}

void str_replace(char* str, char src, char des)
{
  char* p = str;
  while (1) {
    p = strchr(p, src);
    if (p) {
      *p = des;
    } else {
      break;
    }
  }
}

void split_command(char* message, char* command, char* arg)
{
  char* blank = strchr(message, ' ');

  if (blank != NULL) {
    strncpy(command, message, (int)(blank - message));
    command[(int)(blank - message)] = '\0';
    strcpy(arg, blank + 1);
  } else {
    strcpy(command, message);
    arg[0] = '\0';
  }
}

int connect_by_mode(struct ServerState* state)
{
  if (state->trans_mode == PORT_MODE) {
    if (connect(
          state->data_fd,
          (struct sockaddr*)&(state->target_addr),
          sizeof(state->target_addr)
        ) < 0) {
      sprintf(error_buf, ERROR_PATT, "connect", "command_stor");
      perror(error_buf);
      return -1;
    }
  } else if (state->trans_mode == PASV_MODE){
    if ((state->data_fd = accept(state->listen_fd, NULL, NULL)) == -1) {
      sprintf(error_buf, ERROR_PATT, "accept", "command_stor");
      perror(error_buf);
      return -1;
    }
    close(state->listen_fd);
  } else {
    return -1;
  }
  return 0;
}

int close_connections(struct ServerState* state)
{
  close(state->data_fd);
  state->data_fd = -1;
  state->trans_mode = -1;
  return 0;
}

int parse_addr(char* arg, char* ip)
{

  int a1, a2, a3, a4, p1, p2;
  int i;
  int len = strlen(arg);
  for (i = 0; i < len; ++i) {
    if (arg[i] >= '0' && arg[i] <= '9') {
      break;
    }
  }

  sscanf(arg + i, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
  sprintf(ip, "%d.%d.%d.%d", a1, a2, a3, a4);
  return p1 * 256 + p2;

  // str_replace(arg, ',', '.');
  // int i = 0;
  // char* dot = arg;
  // for (i = 0; i < 4; ++i) {
  //   dot = strchr(++dot, '.');
  // }

  // // retrieve ip address
  // strncpy(ip, arg, (int)(dot - arg));
  // strcat(ip, "\0");

  // // retrieve port 1
  // ++dot;
  // char* dot2 = strchr(dot, '.');
  // char buf[32];
  // strncpy(buf, dot, (int)(dot2 - dot));
  // strcat(buf, "\0");
  // int p1 = atoi(buf);

  // //retrieve port 2
  // int p2 = atoi(strcpy(buf, dot2 + 1));

  // return (p1 * 256 + p2);
  // // return 0;
}

int parse_argv(int argc, char** argv, char* hip, char* hport, char* root)
{
  hip[0] = '\0';
  hport[0] = '\0';
  root[0] = '\0';
  static struct option opts[] = {
    {"address",    required_argument,   NULL,   'a'},
    {"port",       required_argument,   NULL,   'p'},
    {"root",       required_argument,   NULL,   'r'},
    {0, 0, 0, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "a:p::r::", opts, NULL)) != -1) {
    switch (opt) {
      case 'a':
        strcpy(hip, optarg);
        break;

      case 'p':
        strcpy(hport, optarg);
        break;

      case 'r':
        strcpy(root, optarg);
        break;

      case '?':
       printf("Unknown option: %c\n", (char)optopt);
       break;

      default:
        sprintf(error_buf, ERROR_PATT, "getopt_long", "parse_argv");
        perror(error_buf);
        break;
    }
  }

  if (strlen(hport) == 0) {
    strcpy(hport, "21");
  }

  if (strlen(root) == 0) {
    strcpy(root, "/tmp");
  }

  if (strlen(hip) == 0) {
    strcpy(hip, "127.0.0.1");
  }

  return 0;
}

void strip_crlf(char* str)
{
  int len = strlen(str);
  if (len < 1) {
    return;
  }
  if (str[len - 1] == '\n' || str[len - 1] == '\r') {
    str[len - 1] = '\0';
    if (len < 2) {
      return;
    }
    if (str[len - 2] == '\n' || str[len - 2] == '\r') {
      str[len - 2] = '\0';
    }
  }
}

int get_random_port(int* p1, int* p2)
{
  int port = rand() % (65535 - 20000) + 20000;
  *p1 = port / 256;
  *p2 = port % 256;
  return port;
}

int get_len_after_encoding(int len, int bytes)
{
  int pck_num;
  get_encode_info(len, bytes, &pck_num);
  return pck_num * BLOCK_LENGTH * (sizeof(int) / sizeof(char));
}

