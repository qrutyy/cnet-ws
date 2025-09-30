#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h> // for inet_pton()
#include <netdb.h> // for gethostbyname()

struct cmd_args {
	int port;
	char *hostname;
};

static struct cmd_args *parse_cmd_args(int argc, char* argv[]) {
	int opt;

	struct cmd_args *args = malloc(sizeof(struct cmd_args));
	if (!args) {
		fprintf(stderr, "Failed to allocate mem\n");
		return NULL;
	}
	memset(args, 0, sizeof(struct cmd_args));

	while ((opt = getopt(argc, argv, "n:p:")) != -1) {
		switch (opt) {
			case 'n':
				printf("Hostname %s\n", optarg);
				args->hostname = optarg;
				break;
			case 'p':
				printf("Port %d\n", atoi(optarg));
				args->port = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Unknown option\n");
				return NULL;
		}
	}

	return args;
}

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
	char buffer[4096];
	FILE *fp = fopen("output.png", "w+");
	struct sockaddr_in srv_addr; 
	/* describes an ipv4 internet domain socket addr. 
	 * the sin_port and sin_addr are stored in network byte order, 
	 * thats why we need to conver by using htons.
	 */
	struct hostent *target_srv;
	struct cmd_args *args;

	args = parse_cmd_args(argc, argv);
	if (!args)
		return -2;
	if (!args->hostname || !strcmp(args->hostname, "") || args->port == 0) {
		fprintf(stderr, "-p, -n args are necessary\n");
		return -3;
	}


	/* A SOCK_STREAM type provides sequenced, reliable, two-way connection based byte streams. SOCK_DGRAM - for udp connection
	 * domain specifies the protocol family that will be used for communication
	 * AF_INET - ipv4
	 */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Failed to create a socket\n");
		return -1;
	}

	fprintf(stdout, "Socket desc %d\n", sockfd);

	target_srv = gethostbyname(args->hostname); /* resolve the ip-addr by the dns */
	if (!target_srv) {
		fprintf(stderr, "Failed to hesolve the host by name %s\n", args->hostname);
		return -1;
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(args->port); /* convert port to network format */
	memcpy(&srv_addr.sin_addr.s_addr, target_srv->h_addr, target_srv->h_length);

	/* SOL_SOCKET - to manipulate options on the socket lvl, 
	 * SO_RCVTIMEO - timeout for input 
	 */
	while (bitmap != 127) {
		if (sendto(sockfd, "miaou\r\n\r\n", strlen("miaou\r\n\r\n"), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
			fprintf(stderr, "Error sending the request\n");
			return -1;
		}

		n = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
		part = (n < 1000) ? 6 : (n - 1000);
		
		if (bitmap & (1 << part))
			continue;

		bitmap |= (1 << part);
		printf("bitmap %d part %d n %d\n", bitmap, part, n);

		if (fseek(fp, get_offset(part), SEEK_SET) != 0) {
			fprintf(stderr, "fseek failed\n");
			fclose(fp);
			return -1;
		}
		if (fwrite(buffer, sizeof(char), n, fp) == 0) {
			fprintf(stderr, "Failed to write\n");
		}
		fflush(fp);
	}

	fclose(fp);
	close(sockfd);
	return 0;
}

