#include <stdio.h>
//#include <netinet.in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h> // for inet_pton()
#include <netdb.h> // for gethostbyname()

struct cmd_args {
	int port;
	char *hostname;
};

struct cmd_args *parse_cmd_args(int argc, char* argv[]) {
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

int main(int argc, char* argv[]) {
	int sockfd;
	int n = 0;
	char buffer[4096];
	char request[512];
	struct sockaddr_in addr; 
	/* describes an ipv4 internet domain socket addr. 
	 * the sin_port and sin_addr are stored in network byte order, 
	 * thats why we need to conver by using htons.
	 */
	struct hostent *server;
	struct cmd_args *args;
	struct timeval timeout;      

	timeout.tv_sec = 4;
	timeout.tv_usec = 0;

	args = parse_cmd_args(argc, argv);
	if (!args) { 
		if (strcmp(args->hostname, "") || args->port == 0) 
			fprintf(stderr, "-p, -n args are necessary\n");
		return -2;
	}

	/* A SOCK_STREAM type provides sequenced, reliable, two-way connection based byte streams.
	 * domain specifies the protocol family that will be used for communication
	 * AF_INET - ipv4
	 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Failed to create a socket\n");
		return -1;
	}

	server = gethostbyname(args->hostname); /* resolve the ip-addr by the dns */
	if (!server) {
		fprintf(stderr, "Failed to hesolve the host by name %s\n", args->hostname);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(args->port); /* convert port to network format */
	memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

	/* SOL_SOCKET - to manipulate options on the socket lvl, 
	 * SO_RCVTIMEO - timeout for input 
	 */ 
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		fprintf(stderr, "Failed setting options (timeout) connecting\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Error connecting\n");
		return -1;
	}

    
	snprintf(request, sizeof(request), "GET / HTTP/1.0\r\nHost: %s\r\n\r\n", args->hostname);
	if (send(sockfd, request, strlen(request), 0) < 0) {
		fprintf(stderr, "Error sending the request\n");
		return -1;
	}

	while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
		buffer[n] = '\0';
		printf("%s", buffer);
	}
	
	close(sockfd);
	return 0;
}
