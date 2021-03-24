#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N 100

int main(int argc, char *argv[]) {
	long fileOffsets[N] = {0};
	int fileDescriptor;
	int lineLength[N] = {-1};
	char symbols = 0;
	int bufSize = 512;
	char *buf = (char*)malloc (sizeof(char) * bufSize);
	if (buf == NULL) {
		perror("Error memory allocation ");
		exit(1);
	}
	if((fileDescriptor = open("test.txt", O_RDONLY)) == -1) {
		perror("File doesn't open");
		free (buf);
		exit(1);
	}

	int i = 1, j = 0;
	while(read(fileDescriptor, &symbols, 1)){
		++j;
		if(symbols == '\n' && i < N) {
			lineLength[i] = j;
			fileOffsets[++i] = lseek(fileDescriptor, 0L, SEEK_CUR);
			j = 0;
		}
	}

	int lineNumber = 0;
	printf("Enter line number: ");
	while(scanf("%d", &lineNumber)) {

		if(!lineNumber) {
			free(buf);
			if (close (fileDescriptor) != 0)
			{
				fprintf (stderr, "Cannot close file (descriptor=%d)\n", fileDescriptor);
				exit (1);
			}
			exit (0);
		}

		if(lineNumber < 0 || lineNumber > (N - 1) || (fileOffsets[lineNumber
			+ 1] == 0)) {
			fprintf(stderr, "wrong line number \n");
			printf("Enter line number: ");
			continue;
		}

		lseek(fileDescriptor, fileOffsets[lineNumber], SEEK_SET);
		if (lineLength[lineNumber] > bufSize) {
			if (realloc (buf, lineLength[lineNumber] * sizeof(char)) == NULL) {
				perror("Error memory allocation");
				free (buf);
				if (close (fileDescriptor) != 0)
				{
					fprintf (stderr, "Cannot close file (descriptor=%d)\n", fileDescriptor);
				}
				exit(1);
			}
			bufSize = lineLength[lineNumber];
		}

		if(read(fileDescriptor, buf, lineLength[lineNumber]) > 0) {
			write (STDOUT_FILENO, buf, lineLength[lineNumber]);
		}
		else {
			fprintf(stderr, "Error read occurred\n");
		}
		printf("Enter line number: ");

	}
	return 0;
}