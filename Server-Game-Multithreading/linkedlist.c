#include "linkedlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct node *head = NULL;
struct node *current = NULL;


int isEmpty()
{
	return head == NULL;
}


void insertHead(char identifier[5], int x_solution, int y_solution)
{
	struct node *new_node = (struct node *)malloc(sizeof(struct node));
	strncpy(new_node->identifier, identifier, 4);
	new_node->x_solution = x_solution;
	new_node->y_solution = y_solution;

	if (isEmpty()) {
		head = new_node;
		head->next = head;
		head->last = head;
	} else {

		new_node->next = head;
		head->last->next = new_node;
		new_node->last = head->last;
		head->last = new_node;
		head = new_node;
	}
}


void deleteNode(struct node *to_delete)
{
    if (to_delete == head && to_delete->next == NULL) {
        free(head);
    } else if (to_delete == head && to_delete->next->next == head) {
        head = to_delete->next;
        head->next = NULL;
        head->last = NULL;
        free(to_delete);
    } else if (to_delete == head) {
        head = to_delete->next;
	    to_delete->last->next = to_delete->next;
	    to_delete->next->last = to_delete->last;
	    free(to_delete);
    } else {
        to_delete->last->next = to_delete->next;
        to_delete->next->last = to_delete->last;
        free(to_delete);
    }
}


void printList()
{
	struct node *iter = head;
	printf("(Identifier: %s, Solution x value: %d, Solution y value: %d\n) ",
	       iter->identifier, iter->x_solution, iter->y_solution);

	while (iter->next != head) {
		printf
		    ("(Identifier: %s, Solution x value: %d, Solution y value: %d\n) ",
		     iter->identifier, iter->x_solution, iter->y_solution);
		iter = iter->next;
	}
}
