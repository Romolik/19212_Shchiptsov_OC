#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <aio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

#define MAX_CLIENTS 5

sigjmp_buf env;

int disconnected_client;

void SIGIO_handler (int signo, siginfo_t *siginfo, void *context) {
	struct aiocb *aio_pointer;
	aio_pointer = (struct aiocb *)siginfo->si_value.sival_ptr;
	if (aio_error (aio_pointer) != EINPROGRESS) {
		int size = aio_return (aio_pointer);
		if (size == 0) {
			disconnected_client = aio_pointer->aio_fildes;
			siglongjmp (env, 4);
		}
		for (int k = 0; k < size; k++) {
			if (islower (((char *)aio_pointer->aio_buf)[k])) {
				((char *)aio_pointer->aio_buf)[k] = toupper (((char *)aio_pointer->aio_buf)[k]);
			}
		}

		write (STDOUT_FILENO, (const void *)aio_pointer->aio_buf, size);
		aio_read (aio_pointer);
	}
}

int main () {
	struct sockaddr_un un;
	memset(&un, 0, sizeof (un));
	un.sun_family = AF_UNIX;
	char *name = "hype.socket";
	strcpy(un.sun_path, name);

	int sock;
	if ((sock = socket (AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0) {
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

	if (listen (sock, 1) != 0) {
		perror ("Listening exception");
		close (sock);
		unlink (un.sun_path);
		return -1;
	}

	char aio_buffers[MAX_CLIENTS][BUFSIZ];
	struct aiocb *aio_list[MAX_CLIENTS];
	int is_alive[MAX_CLIENTS];

	for (int i = 0; i < MAX_CLIENTS; i++) {
		aio_list[i] = (struct aiocb *)malloc (sizeof (struct aiocb));
		is_alive[i] = 0;
		aio_list[i]->aio_fildes = -1;
		aio_list[i]->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aio_list[i]->aio_sigevent.sigev_signo = SIGIO;
		aio_list[i]->aio_sigevent.sigev_value.sival_ptr = aio_list[i];
		aio_list[i]->aio_buf = aio_buffers[i];
		aio_list[i]->aio_offset = 0;
		aio_list[i]->aio_nbytes = BUFSIZ;
	}

	int clients_connected = 0;
	int alive = 1;
	ssize_t readed = 0;
	int clients_discon_during_iter = 0;

	sigset_t sigio;
	sigaddset(&sigio, SIGIO);
	struct sigaction sigio_action;
	memset(&sigio_action, 0, sizeof (sigio_action));
	sigio_action.sa_flags = SA_SIGINFO;
	sigio_action.sa_sigaction = SIGIO_handler;
	sigio_action.sa_mask = sigio;
	sigaction (SIGIO, &sigio_action, NULL);
	int new_client = 0;

	if (sigsetjmp (env, 1) != 0) {
		for (int client = 0; client < clients_connected; client++) {
			if (aio_list[client]->aio_fildes != disconnected_client) {
				continue;
			}
			if (client = clients_connected - 1) {
				aio_list[client]->aio_fildes = -1;
			} else {
				aio_list[client] = aio_list[clients_connected - 1];
			}
			is_alive[clients_connected - 1] = 0;
			aio_list[client]->aio_fildes = -1;
			clients_connected--;
			printf ("Client disconnected\n");
			if (clients_connected == 0) {
				printf ("No clients are on server, shutting down\n");
				for (int i = 0; i < MAX_CLIENTS; i++) {
					free (aio_list[i]);
				}
				close (sock);
				unlink (un.sun_path);
				return 0;
			}
			break;
		}
	}

	while (alive) {
		if ((new_client = accept (sock, NULL, NULL)) == -1) {
			errno = 0;
		} else {
			if (clients_connected == MAX_CLIENTS) {
				printf ("Not enough available client slots\n");
				continue;
			}
			aio_list[clients_connected]->aio_fildes = new_client;
			is_alive[clients_connected] = 1;
			aio_read (aio_list[clients_connected]);
			clients_connected++;
			printf ("New client accepted\n");
		}
	}

	return 0;
}
