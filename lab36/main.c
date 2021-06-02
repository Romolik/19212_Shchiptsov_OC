#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#define MAXSLEEP 128
#define BUFFER_SIZE 1024 * 1024

uint16_t PORT = 80;

struct termios tty, tty_backup;

int terminal;

int tty_height;

int connect_retry (struct sockaddr_in *addr) {
	int numsec;
	int sock;
	for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
		printf ("Trying to connect\n");
		if ((sock = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
			return -1;
		}

		if (connect (sock, (struct sockaddr *)addr, sizeof (*addr)) == 0) {
			printf ("OK\n");
			return sock;
		}

		close (sock);
		if (numsec <= MAXSLEEP / 2) {
			sleep (numsec);
		}
	}
	return -1;
}

int term_set_attr () {
	if (terminal = open ("/dev/tty", O_RDONLY) == -1) {
		perror ("Opening file error! ");
		return -1;
	}

	if (isatty (fileno (stdin)) == 0) {
		fprintf (stderr, "Stdin is not a terminal\n");
		return -1;
	}

	if (tcgetattr (terminal, &tty) == -1) {
		perror ("Terminal attributes getting error!");
		return -1;
	}

	tty_backup = tty;
	tty.c_lflag &= ~(ICANON | ECHO);
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	if (tcsetattr (terminal, TCSAFLUSH, &tty) == -1) {
		perror ("Terminal attributes setting error!");
		return -1;
	}
}

int term_set_default () {
	if (tcsetattr (terminal, TCSAFLUSH, &tty_backup) == -1) {
		perror ("Terminal attributes setting error!");
		return -1;
	}
}

void read_response (int socket) {
	tty_height = 25;

	if (term_set_attr () == -1) {
		return;
	}
	char buffer[BUFFER_SIZE];

	struct pollfd fds[2];
	fds[0].fd = socket;
	fds[0].events = POLLIN;
	fds[1].fd = terminal;
	fds[1].events = POLLIN;

	int not_all_read = 1;
	int not_all_written = 1;

	int lines_count = 0;

	int buffer_position = 0;
	int buffer_length = 0;
	int read_iter = 0;
	int write_iter = 0;
	while (not_all_read || not_all_written) {
		if (poll (fds, 2, 0) < 0) {
			perror ("Poll error");
			return;
		}

		if (fds[0].revents == POLLIN) {
			int readed;
			if (!((write_iter < read_iter) && (buffer_length + 256 <= buffer_position))) {
				if ((readed = read (socket, buffer + buffer_length, 256)) == 0) {
					not_all_read = 0;
				} else if (readed < 0) {
					perror ("Reading error");
					return;
				}
				buffer_length += readed;
				if (buffer_length == BUFFER_SIZE) {
					buffer_length = 0;
					read_iter++;
				}
			}
		}

		if ((fds[1].revents == POLLIN)) {
			char trash;
			if (read (terminal, &trash, 1) < 0) {
				perror ("Reading error");
				return;
			}
			if (lines_count == tty_height) {
				lines_count = 0;
			}
		}

		while (lines_count < tty_height) {

			if (!not_all_read) {
				if (buffer_length == buffer_position) {
					not_all_written = 0;
				}
			}

			if (buffer_length == buffer_position) {
				break;
			}

			while ((buffer[buffer_position] != '\n') && (buffer_length != buffer_position)) {
				putchar (buffer[buffer_position]);
				buffer_position++;
				if (buffer_position == BUFFER_SIZE) {
					buffer_position = 0;
					write_iter++;
				}
			}

			if (buffer[buffer_position] == '\n') {
				printf ("\n");
				buffer_position++;
				lines_count++;
			}

			if (lines_count == tty_height) {
				printf ("Press any key to scroll down\n");
			}
		}
	}

	term_set_default ();
}

void get_data (int socket, char *url, char *data) {
	FILE *file_socket = fdopen (socket, "w");
	fprintf (file_socket, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n\r\n", data,
			 url);
	fflush (file_socket);
	printf ("Request sent\n");
	read_response (socket);
}

int main (int argc, char **argv) {
	if (argc != 2) {
		printf ("Please enter destination URL\n");
		return 0;
	}
	struct addrinfo *ailist;
	struct addrinfo *aip;
	struct addrinfo hint;
	int sock;
	memset(&hint, 0, sizeof (hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;

	char url_first[BUFSIZ];
	char *url_second;
	int parsing_flag = 1;
	int parsing_index = 0;
	while (parsing_flag) {
		if (argv[1][parsing_index] == '/') {
			parsing_flag = 0;
			continue;
		}
		url_first[parsing_index] = argv[1][parsing_index];
		parsing_index++;
	}

	url_first[parsing_index] = '\0';
	if (argv[1][parsing_index + 1] != '\0') {
		url_second = &(argv[1][parsing_index + 1]);
	} else {
		url_second = "";
	}
	printf ("Connecting to %s\n", url_first);
	printf ("Getting %s\n", url_second);

	if (getaddrinfo (url_first, NULL, &hint, &ailist) != 0) {
		printf ("Error getting address info\n");
		perror ("Getting address");
		return 0;
	}

	struct sockaddr_in socket_addr;
	aip = ailist;
	memcpy(&socket_addr, (void *)aip->ai_addr, sizeof (socket_addr));
	socket_addr.sin_port = htons(PORT);
	if ((sock = connect_retry (&socket_addr)) < 0) {
		return -1;
	} else {
		get_data (sock, url_first, url_second);
		printf ("\n");
		close (sock);
	}
	printf ("Unable to connect :(\n");
	return 0;
}

