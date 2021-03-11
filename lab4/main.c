#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct List {
  struct List *next;
  char *str;
};

int addNode (char *buf, struct List *current) {
	struct List *tmp = (struct List *)malloc (sizeof (struct List));
	if (tmp == NULL) {
		return 1;
	}
	tmp->str = (char *)malloc (sizeof (char) * (strlen (buf) + 1));
	if (tmp->str == NULL) {
		free (tmp);
		return 1;
	}
	strcpy (tmp->str, buf);
	tmp->next = NULL;
	current->next = tmp;
	return 0;
}

void printList (struct List *head) {
	struct List *cur = head;
	while (cur != NULL) {
		puts (cur->str);
		cur = cur->next;
	}
}

void freeList (struct List *head) {
	while (head != NULL) {
		struct List *next = head->next;
		free (head->str);
		free (head);
		head = next;
	}
}

int main () {
	struct List *head = (struct List *)malloc (sizeof (struct List));
	if (head == NULL) {
		printf ("Error allocation memory");
		return 1;
	}
	head->next = NULL;
	head->str = NULL;
	char buf[BUFSIZ];
	fgets (buf, BUFSIZ, stdin);
	head->str = (char *)malloc (sizeof (char) * (strlen (buf) + 1));
	if (head->str == NULL) {
		printf ("Error allocation memory");
		free (head);
		return 1;
	}
	strcpy (head->str, buf);
	head->next = NULL;
	struct List *current = head;
	while (fgets (buf, BUFSIZ, stdin) != NULL) {
		if (buf[0] == '.') {
			break;
		}
		if (addNode (buf, current)) {
			printf ("Error allocation memory");
			freeList (head);
			return 1;
		}
		current = current->next;
	}

	printList (head);
	freeList (head);
	return 0;
}
