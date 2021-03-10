#include <unistd.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
	FILE *file;
	printf ("Real user id: %d,  effective user id: %d\n", getuid (),
			geteuid ());
	file = fopen (argv[1], "r");
	if (file == NULL) {
		perror ("The file could not be opened for the first time");
		return 1;
	} else {
		printf ("The file is opened for the first time\n");
		fclose (file);
	}
	seteuid (getuid ());
	printf ("New real user id: %d, new effective user id: %d\n", getuid (),
			geteuid ());
	file = fopen (argv[1], "r");
	if (file == NULL) {
		perror ("The file could not be opened for the second time");
		return 1;
	} else {
		printf ("The file is opened for the second time\n");
		fclose (file);
	}
	return 0;
}