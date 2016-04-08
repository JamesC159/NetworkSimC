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
	int ichannel, ochannel;
	int id;
	struct Node *next;
} node;

typedef struct FILES {
	FILE *file;
} file;

/********* Function Prototypes ********/
void insert_end(node**, node**, int*,  int);
void print_list(node*);
void clear_list(node**);
void open_files(int, node**, FILE***);
void datalink_receive_from_network(char*, int, char);
void datalink_receive_from_channel(node**);
void network_receive_from_transport(char*, int, int);
void network_receive_from_datalink(char*, int, int);
void network_route();
void transport_send_string();
void transport_receive_from_network(char*, int, int);
void transport_output_all_received();
void write_channel();

int main (int argc, char **argv) {
	
	FILE **file_list;
	int id, dur, dest, stime, i, ncount;
	char *msg;
	node *head = NULL, *tail = NULL;

	file_list = malloc(20 * sizeof(FILE));

	if (argc < 5) {
		fprintf(stderr, "Error: invalid number of command line arguments.\n"); 
	}

	int j = 0;
	//Build linked list that represents neighbors of this node in the network
	for (i = 6; i < argc; i++) {
		insert_end(&head, &tail, &ncount, atoi(argv[i]));
	}

	//Initiaization steps
	id = atoi(argv[1]);
	dest = atoi(argv[3]);

	//Open Files for reading and writing based on neighboring nodes. Assuming 2 way communication
	open_files(id, &head, &file_list);

	//If id is not equal to the destination, then we have a message to deliver and a transport layer start time.
	if (!(id == dest)) {
		msg = argv[4];
		stime = atoi(argv[5]);
	}	

	dur = atoi(argv[2]);
	
	//Process will run for dur seconds
	for (i = 0; i < dur; i++) {
		datalink_receive_from_channel(&head);
		if(i > stime) {
			transport_send_string();
		}
		sleep(1);
	}
	
	//Free dynamic memory
	clear_list(&head);
	free(file_list);
		
	return 0;
}

/********* Datalink Layer Functions *********/
void datalink_receive_from_network(char *msg, int len, char next_hop) {
	
}
	

void datalink_receive_from_channel(node **head) {
	node *temp = *head;
	int bytes_read, i;
	char buf[1000];
	while(temp != NULL) {
		while((bytes_read = read(temp->ichannel, buf, 25)) == 0 && i < 10) {
			sleep(2);
			printf("Reader waiting for file contents\n");
			i++;
		}
		printf("Bytes read: %d which were %s\n", bytes_read, buf);
		temp = temp->next;
	}
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
void insert_end (node **head, node **tail, int *ncount,  int node_id) {
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

	(*ncount)++;
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

void open_files(int id, node **head, FILE ***file_list) {
	node *temp = *head;
	int iflag = (O_CREAT | O_RDONLY);
	int oflag = (O_TRUNC | O_CREAT | O_WRONLY);
	int ifd, ofd;
	char *ichan = (char*)malloc(9 * sizeof(char));
	char *ochan = (char*)malloc(9 * sizeof(char));

	while (temp != NULL) {
		sprintf(ochan, "from%dto%d.txt", id, temp->id);
		sprintf(ichan, "from%dto%d.txt", temp->id, id);
		
		ofd = open(ochan, oflag, 0x1c0);
		ifd = open(ichan, iflag, 0x1c0);
		if (ofd < 0) {
			fprintf(stderr, "Error: Failed to open %s\n", ochan);
			exit(1);
		}
		else if (ifd < 0) {
			fprintf(stderr, "Error: Failed to open %s\n", ichan);
			exit(1);
		}
		
		//Store the file descriptors in the nodes themselves for convenience.
		temp->ichannel = ifd;
		temp->ochannel = ofd;

		temp = temp->next;
	}
}
