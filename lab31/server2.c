#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>

#define MAX_CONNECTIONS 5

void close_socks (int clients [MAX_CONNECTIONS]) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		close (clients[i]);

	}
}

int main() {
	struct sockaddr_un un;
	memset(&un, 0, sizeof (un));
	un.sun_family = AF_UNIX;
	char *name = "hype.socket\0";
	strcpy(un.sun_path, name);

	int sock;
	if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror ("Socket creation exception");
		return -1;
	}

	unlink(un.sun_path);

	if (bind (sock, (struct sockaddr *)&un, sizeof (un)) < 0) {
		perror ("Binding exception");
		close (sock);
		unlink (un.sun_path);
		return -1;
	}

	if (listen (sock, MAX_CONNECTIONS) != 0) {
		perror ("Listening exception");
		close (sock);
		unlink (un.sun_path);
		return -1;
	}

	int clients[MAX_CONNECTIONS];
	memset(clients, 0, sizeof(int)*MAX_CONNECTIONS);
	int clientsNum = 0;
	int maxDescriptor = sock;
	fd_set set;

	while (1) {
		FD_ZERO(&set);
		FD_SET(sock, &set);
		for (int i = 0; i < clientsNum; i++) {
			if (clients[i] != -1) {
				FD_SET(clients[i], &set);
			}
		}

		int event = select(maxDescriptor + 1, &set, NULL, NULL, NULL);
		if (event == -1) {
			perror("Select error.\n");
			break;
		}

		if (FD_ISSET(sock, &set)) {
			int socketClient = accept(sock, NULL, NULL);
			if (socketClient == -1) {
				perror("Accept error.\n");
				close_socks (clients);
				unlink (un.sun_path);
				exit(1);
			}

			FD_SET(socketClient, &set);
			clients[clientsNum] = socketClient;
			clientsNum++;
			maxDescriptor = socketClient > maxDescriptor ? socketClient : maxDescriptor;
		} else {
			for (int i = 0; i < clientsNum; i++) {
				if (clients[i] == -1) {
					continue;
				}

				if (FD_ISSET(clients[i], &set)) {
					char lines[BUFSIZ];
					int length = read(clients[i], lines, BUFSIZ);

					if (length == 0) {
						FD_CLR(clients[i], &set);
						clients[i] = -1;
						close(clients[i]);
					}

					for (int j = 0; j < length; j++) {
						printf("%c", toupper(lines[j]));
					}
					printf("\n");
				}
			}
		}
	}
	close(sock);
	close_socks (clients);
	unlink (un.sun_path);
	return 0;
}
