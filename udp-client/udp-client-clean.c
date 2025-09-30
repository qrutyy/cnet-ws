#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h> // for inet_pton()
#include <netdb.h> // for gethostbyname()

#define PORT 8011
#define HOST "task.miminet.ru"

static long get_offset(int part) {
    long offset = 0;
	int i;

    for (i = 0; i < part; i++) {
        offset += (1000 + i);
    }
    return offset;
}

int main(int argc, char* argv[]) {
	int sockfd, part, bitmap = 0;
	int n = 0;
	char buffer[1006];
	FILE *fp = fopen("output.png", "wb+");
	struct sockaddr_in srv_addr; 
	struct hostent *target_srv;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Failed to create a socket\n");
		return -1;
	}

	fprintf(stdout, "Socket desc %d\n", sockfd);

	target_srv = gethostbyname(HOST);
	if (!target_srv) {
		fprintf(stderr, "Failed to resolve the host by name %s\n", HOST);
		return -1;
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	memcpy(&srv_addr.sin_addr.s_addr, target_srv->h_addr, target_srv->h_length);

	while (bitmap != ((1 << 7) - 1)) {

		if (sendto(sockfd, "miaou\r\n\r\n", strlen("miaou\r\n\r\n"), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
			fprintf(stderr, "Error sending the request\n");
			return -1;
		}

		n = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
		part = (n < 1000) ? 6 : (n - 1000);
		
		if (bitmap & (1 << part))
			continue;

		bitmap |= (1 << part);
		printf("bitmap 0x%x part %d n %d\n", bitmap, part, n);

		if (fseek(fp, get_offset(part), SEEK_SET) != 0) {
			fprintf(stderr, "fseek failed\n");
			fclose(fp);
			return -1;
		}

		if (fwrite(buffer, 1, n, fp) == 0)
			fprintf(stderr, "Failed to write\n");

		fflush(fp);
	}

	fclose(fp);
	close(sockfd);
	return 0;
}

