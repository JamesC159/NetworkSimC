/*
Each process (i.e. node) is going to be given the following arguments

1. id of this node (i.e., a number from 0 to 9)
2. the duration, in seconds, that the node should run before it terminates
3. the destination id of a process to which the transport protocol should send data
4. a string of arbitrary text which the transport layer will send to the destination
5. the starting time for the transport layer (explained much later below)
6. a list of id's of neighbors of the process

> foo 0 100 2 "this is a message from 0"  30 1 &

> foo 1 100  2  "this is a message from 1"  30 0 2 &

> foo 2  100  2 1 &

For node 2, since the "destination" is 2 itself, this means 2 should not send a transport level message 
to anyone, and the list of neighbors is just node 1. Note that it does not have a start time 
for the trasnport layer
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/********* Linked List *********/
typedef struct Node {
	int id;
	struct Node *next;
} node;

/********* Function Prototypes ********/
void insert_end(node**, node**, int);
void print_list(node*);
void clear_list(node**);
void datalink_receive_from_network(char*, int, char);
void datalink_receive_from_channel();
void network_receive_from_transport(char*, int, int);
void network_receive_from_datalink(char*, int, int);
void network_route();
void transport_send_string();
void transport_receive_from_network(char*, int, int);
void transport_output_all_received();

int main (int argc, char **argv) {
	
	FILE *ofile;
	FILE *ifile;
	int id, dur, dest, stime, i;
	char *msg;
	node *head = NULL, *tail = NULL;
	
	//Initialization steps
	id = atoi(argv[1]);
	dur = atoi(argv[2]);
	dest = atoi(argv[3]);
	msg = argv[4];
	stime = atoi(argv[5]);
	
	//Build linked list that represents neighbors of this node in the network
	for (i = 6; i < argc; i++) {
		insert_end(&head, &tail, atoi(argv[i]));
	}
	
	//Process will run for dur seconds
	for (i = 0; i < dur; i++) {
		
		sleep(1);
	}
	
	//Free dynamic memory
	clear_list(&head);
		
	return 0;
}

/********* Datalink Layer Functions *********/
void datalink_receive_from_network(char *msg, int len, char next_hop) {
	
}
	

void datalink_receive_from_channel() {
	
}

/********* Network Layer Functions *********/
void network_receive_from_transport(char *msg, int len, int dest) {
	
}

void network_receive_from_datalink(char *msg, int len, int neighbor_id) {
	
}

void network_route() {
	
}

/********* Transport Layer Functions *********/
void transport_send_string() {
	
}
void transport_receive_from_network(char *msg, int len, int source) {
	
}
void transport_output_all_received() {
	
}

/********* Linked List Functions *********/
void insert_end (node **head, node **tail, int node_id) {
	if (*head == NULL) {
		*head = malloc(sizeof(node));
		(*head)->id = node_id;
		(*head)->next = NULL;
		(*tail) = (*head);
	}
	else {
		node *temp = malloc(sizeof(node));
		temp->id = node_id;
		(*tail)->next = temp;
		(*tail) = temp;
		(*tail)->next = NULL;
	}
}

void print_list(node *head) {
	node *temp = head;
	int i = 1;
	while(temp != NULL) {
		printf("Neighbor %d: %d\n", i++, temp->id);
		temp = temp->next;
	}
}

void clear_list(node **head) {
	node *temp = *head;
	node *next;
	while(temp != NULL) {
		next = temp->next;
		free(temp);
		temp = next;
	}
	*head = NULL;
}