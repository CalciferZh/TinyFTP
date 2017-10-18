#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int listenfd, connfd;
	struct sockaddr_in addr;
	char sentence[8192];
	int p;
	int len;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 6789;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		
		p = 0;
		while (1) {
			int n = read(connfd, sentence + p, 8191 - p);
			if (n < 0) {
				printf("Error read(): %s(%d)\n", strerror(errno), errno);
				close(connfd);
				continue;
			} else if (n == 0) {
				break;
			} else {
				p += n;
				if (sentence[p - 1] == '\n') {
					break;
				}
			}
		}

		sentence[p - 1] = '\0';
		len = p - 1;
		
		for (p = 0; p < len; p++) {
			sentence[p] = toupper(sentence[p]);
		}

		p = 0;
		while (p < len) {
			int n = write(connfd, sentence + p, len + 1 - p);
			if (n < 0) {
				printf("Error write(): %s(%d)\n", strerror(errno), errno);
				return 1;
	 		} else {
				p += n;
			}			
		}

		close(connfd);
	}

	close(listenfd);
}

