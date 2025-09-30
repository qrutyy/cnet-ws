#include <stdio.h>
//#include <netinet.in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h> // for inet_pton()
#include <netdb.h> // for gethostbyname()


#define PORT 80

const char *hostname = "miminet.ru";

int main(int argc, char* argv[]) {
	int sockfd;
	int opt = 1;
	struct sockaddr_in addr; // describes an ipv4 internet domain socket addr. the sin_port and sin_addr are stored in network byte order, thats why we need to conver by using htons.
	struct hostent *server;

	// A SOCK_STREAM type provides sequenced, reliable, two-way connection based byte streams.
	// domain specifies the protocol family that will be used for communication
	// AF_INET - ipv4
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "failed to create a socket\n");
		return -1;
	}

	server = gethostbyname(hostname); // resolbe the ip-addr by the dns
	if (!server) {
		fprintf(stderr, "failed to hesolve the host by name %s\n", hostname);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT); // convert port to network format
	memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error connecting");
		exit(1);
	}

	const char *request = "GET / HTTP/1.0\r\nHost: miminet.ru\r\n\r\n";
	if (send(sockfd, request, strlen(request), 0) < 0) {
		perror("Error writing to socket");
		exit(1);
	}

	char buffer[4096];
	int n;
	while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
		buffer[n] = '\0';
		printf("%s", buffer);
	}
	

	close(sockfd);
	return 0;
}
