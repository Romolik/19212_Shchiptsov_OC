#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main() {
	struct sockaddr_un un;
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	char* name = "hype.socket\0";
	strcpy(un.sun_path, name);

	int sock;
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("Socket creation exception");
		return -1;
	}

	if (connect(sock, (struct sockaddr* )&un, sizeof(un)) == -1){
		perror("Connection exception");
		close(sock);
		return -1;
	}

	char test_str[] = "gtv Jrehdc vfcjnDFCvcx";
	size_t test_str_length = strlen(test_str);

	ssize_t written;
	while((written = send(sock, test_str, test_str_length, MSG_NOSIGNAL)) > 0 && test_str_length > 0) {
		test_str_length -= written;
	}
	sleep(10);
	if(written == -1) {
		perror("Writing error");
		close(sock);
		return -1;
	}

	close(sock);
	return 0;
}