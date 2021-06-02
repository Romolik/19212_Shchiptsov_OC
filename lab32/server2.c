#include <aio.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>

#define MAX_CONNECTIONS 1

void close_socks (struct aiocb** clientsList) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		close(clientsList[i]->aio_fildes);
		clientsList[i] = NULL;
	}
}

int main() {
	struct sockaddr_un un;
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	char* name = "hype.socket\0";
	strcpy(un.sun_path, name);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		perror ("Socket creation exception");
		return -1;
	}

	unlink (un.sun_path);

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

	char messages[MAX_CONNECTIONS][BUFSIZ];
	struct aiocb** clientsList = (struct aiocb**)malloc(sizeof(struct aiocb*) * MAX_CONNECTIONS);
	if (clientsList == NULL) {
		perror("Error allocation memory");
		close (sock);
		unlink (un.sun_path);
		return -1;
	}

	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		int socketClient = accept(sock, NULL, NULL);
		if (socketClient == -1) {
			perror("Accept error.\n");
			close (sock);
			close_socks (clientsList);
			unlink (un.sun_path);
			return -1;
		}

		struct aiocb* clientIO  = (struct aiocb*) malloc(sizeof(struct aiocb));
		if (clientIO == NULL) {
			perror("Error allocation memory");
			close (sock);
			close_socks (clientsList);
			unlink (un.sun_path);
			free (clientsList);
			return -1;
		}
		clientIO->aio_fildes = socketClient;
		clientIO->aio_buf = messages[i];
		clientIO->aio_nbytes = BUFSIZ;
		clientsList[i] = clientIO;
	}

	while (1) {
		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			if (clientsList[i] == NULL) {
				continue;
			}
			int code = aio_read(clientsList[i]);
			if (code == -1) {
				perror("Aio_read error.\n");
				close (sock);
				close_socks (clientsList);
				unlink (un.sun_path);
				return -1;
			}
		}

		if (aio_suspend((const struct aiocb**)clientsList, MAX_CONNECTIONS, NULL) == -1) {
			perror("Aio_suspend error.\n");
			close (sock);
			close_socks (clientsList);
			unlink (un.sun_path);
			return -1;
		}

		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			int code = aio_error(clientsList[i]);
			if (code > 0) {
				perror("Aio_error error.\n");
				close (sock);
				close_socks (clientsList);
				unlink (un.sun_path);
				return -1;
			}
			if (code == 0) {
				int messageLength = (int)aio_return(clientsList[i]);
				if (messageLength == 0) {
					close(clientsList[i]->aio_fildes);
					clientsList[i] = NULL;
				}
				for (int j = 0; j < messageLength; j++) {
					printf("%c", toupper(messages[i][j]));
				}
				printf("\n");
			}
		}
	}
	return 0;
}
