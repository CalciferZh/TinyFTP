#define USER_CODE 0

#define USER_COMMAND "user"

#define RES_READY              "220 Anonymous FTP server ready.\r\n"
#define RES_WANTUSER           "500 USER command is expected.\r\n"

#define USER_NAME "anonymous"

// a secured method to send message
int send_msg(int connfd, char* message);

// a secured method to receive message
int read_msg(int connfd, char* message);

void str_lower(char* str);

void split_command(char* message, char* command, char* content);

// parse the command from client
int parse_command(char* message, char* content);

int handle_command(char* message);

int serve(int connfd);

