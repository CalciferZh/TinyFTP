#include "utils.h"

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
  else {
    printf("Unknown command: %s\n", command);
  }

  return ret;
}

int parse_addr(char* content, char* ip)
{
  str_replace(content, ',', '.');

  int i = 0;
  char* dot = content;
  for (i = 0; i < 4; ++i) {
    dot = strchr(++dot, '.');
  }

  // retrieve ip address
  strncpy(ip, content, (int)(dot - content));
  strcat(ip, "\0");

  // retrieve port 1
  ++dot;
  char* dot2 = strchr(dot, '.');
  char buf[32];
  strncpy(buf, dot, (int)(dot2 - dot));
  strcat(buf, "\0");
  int p1 = atoi(buf);

  //retrieve port 2
  int p2 = atoi(strcpy(buf, dot2 + 1));

  return (p1 * 256 + p2);
  // return 0;
}

void strip_crlf(char* str)
{
  int len = strlen(str);
  if (len < 2) {
    return;
  }
  if (str[len - 2] == '\n' || str[len - 2] == '\r') {
    str[len - 2] = '\0';
    if (len < 3) {
      return;
    }
    if (str[len - 3] == '\n' || str[len - 3] == '\r') {
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

int command_port(int connfd, char* content, struct sockaddr_in* addr)
{
  // create socket
  int datafd;
  if ((datafd = socket(AF_INET, SOCK_STREAM,  IPPROTO_TCP)) == -1) {
    printf("Error socket(): %s(%d)\n", strerror(errno), errno);
    send_msg(connfd, RES_REJECT_PORT);
    return -1;
  }

  // check
  if (!strlen(content)) {
    send_msg(connfd, RES_ACCEPT_PORT);
    return datafd;
  }

  char ip[64];
  int port = parse_addr(content, ip);
  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  // translate the decimal IP address to binary
  if (inet_pton(AF_INET, ip, &(addr->sin_addr)) != 0) {
    printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
    send_msg(connfd, RES_REJECT_PORT);
    return -1;
  }

  // if (connect(datafd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
  //   printf("Error connect(): %s(%d)\n", strerror(errno), errno);
  //   ret = -1;
  // }

  send_msg(connfd, RES_ACCEPT_PORT);
  return datafd;
}

int command_pasv(int connfd, char* hip, struct sockaddr_in* addr)
{
  int datafd;
  if ((datafd = socket(AF_INET, SOCK_STREAM,  IPPROTO_TCP)) == -1) {
    printf("Error socket(): %s(%d)\n", strerror(errno), errno);
    send_msg(connfd, RES_REJECT_PASV);
    return -1;
  }

  int p1, p2;
  int port = get_random_port(&p1, &p2);

  memset(addr, 0, sizeof(*addr));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(datafd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
    printf("Error bind(): %s(%d)\n", strerror(errno), errno);
    send_msg(connfd, RES_REJECT_PASV);
    return -1;
  }

  if (listen(datafd, 10) == -1) {
    printf("Error listen(): %s(%d)\n", strerror(errno), errno);
    send_msg(connfd, RES_REJECT_PASV);
    return -1;
  }

  char hip_cp[32] = "";
  strcpy(hip_cp, hip);
  str_replace(hip_cp, '.', ',');
  char buffer[32] = "";
  sprintf(buffer, RES_ACCEPT_PASV, hip_cp, p1, p2);
  send_msg(connfd, buffer);

  return datafd;
}

int command_quit(int connfd)
{
  send_msg(connfd, RES_CLOSE);
  close(connfd);
  return 0;
}

int get_local_ip(int sock, char* buf)
{
  // char *temp = NULL;
  // struct ifreq ifr;
  // char ifname[] = "eth0";

  // memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
  // memcpy(ifr.ifr_name, ifname, strlen(ifname));

  // if((0 != ioctl(sock, SIOCGIFADDR, &ifr)))
  // {   
  //   perror("ioctl error");
  //   return -1;
  // }

  // temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);     
  // memcpy(buf, temp, strlen(temp));

  memcpy(buf, "123.206.56.140", strlen("123.206.56.140"));

  return 0;
}

int get_random_port(int* p1, int* p2)
{
  int port = rand() % (65535 - 20000) + 20000;
  *p1 = port / 256;
  *p2 = port % 256;
  return port;
}



