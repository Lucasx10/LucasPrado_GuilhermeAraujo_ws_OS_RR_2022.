#ifndef LINKEDLIST_DOT_H	
#define LINKEDLIST_DOT_H	

struct node {
	char identifier[5];
	int x_solution;
	int y_solution;

	struct node *next;
	struct node *last;
};


int isEmpty();

void insertHead(char identifier[], int x_solution, int y_solution);

void deleteNode(struct node *to_delete);

void printList();

#endif				
