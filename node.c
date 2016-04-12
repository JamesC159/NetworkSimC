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

//A Map will be used as the routing table and cost table
typedef struct RTB {
	node *key;
	node *next_hop;
}RTB;

typedef struct CTB {
	int cost;
	node *dest;
}CTB;

/********* Function Prototypes ********/
void insert_end(node**, node**, int*,  int);
void print_list(node*);
void clear_list(node**);
void open_files(int, node**);
void datalink_receive_from_network(char*, int, char);
void datalink_receive_from_channel(node**);
void network_receive_from_transport(char*, int, int);
void network_receive_from_datalink(char*, int, int);
void network_route();
void transport_send_string(char**, int, int, int, char **);
void transport_receive_from_network(char*, int, int);
void transport_output_all_received();
void write_channel();

/* NOTE - NOT dealing with memory management completely right now and also not closing file descriptors in list of nodes */


int main (int argc, char **argv) {
	int id, dur, dest, stime, i, ncount;
	char *msg, *seq_num;
	node *head = NULL, *tail = NULL;

	if (argc < 5) {
		fprintf(stderr, "Error: invalid number of command line arguments.\n");
		exit(1);
	}

	seq_num = (char*)malloc(2*sizeof(char));
	seq_num[0] = '0';
	seq_num[1] = '0';

	int j = 0;
	//Build linked list that represents neighbors of this node in the network
	for (i = 6; i < argc; i++) {
		insert_end(&head, &tail, &ncount, atoi(argv[i]));
	}

	//Initiaization steps
	id = atoi(argv[1]);
	dest = atoi(argv[3]);

	//Open Files for reading and writing based on neighboring nodes. Assuming 2 way communication
	open_files(id, &head);

	//If id is not equal to the destination, then we have a message to deliver and a transport layer start time.
	if (!(id == dest)) {
		msg = argv[4];
		stime = atoi(argv[5]);
	}	

	dur = atoi(argv[2]);
	
	//Process will run for dur seconds
	for (i = 0; i < dur; i++) {
		//datalink_receive_from_channel(&head);
		if(i > stime) {
			transport_send_string(&msg, id, dest, strlen(msg), &seq_num);
		}
		sleep(1);
	}
	
	//Free dynamic memory
	clear_list(&head);
	free(seq_num);

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
		while((bytes_read = read(temp->ichannel, buf, 25)) == 0 && i < 2) {
			sleep(1);
			printf("Reader waiting for file contents\n");
			i++;
		}
		printf("Bytes read: %d which were %s\n", bytes_read, buf);
		temp = temp->next;
	}
}

/********* Network Layer Functions *********/
void network_receive_from_transport(char *msg, int len, int dest) {
	printf("NEWORK PACKET: %s\n", msg);	
}

void network_receive_from_datalink(char *msg, int len, int neighbor_id) {
	
}

void network_route() {
	
}

/********* Transport Layer Functions *********/
void transport_send_string(char **msg, int source, int dest, int len, char **seq_num) {
	const int data_size = 6;
	int msg_size = strlen(*msg);
	int i = 0, j = 0, sn = 0;
	char *data_msg = (char*)malloc(10*sizeof(char));
	
	if (*msg != NULL) {
		printf("Message is not NULL\n");
		//Break segment up into multiple segments if msg is too large
		if (msg_size > data_size) {
			printf("Message size %d is bigger than %d\n", msg_size, data_size);
			while (msg_size > data_size) {
				char *packet = (char *)malloc(6*sizeof(char));
				for (i = 0; i < data_size - 1; i++) {
					if (msg_size == 1)
							break;
					packet[i] = (*msg)[j++];
					printf("packet[i] = %c\n", packet[i]);
					msg_size--;
					printf("j: %d  msg_size: %d\n", j, msg_size);
				}
				
				packet[i] = NULL;
				sprintf(data_msg, "D%d%d%s%s", source, dest, *seq_num, packet);
				printf("Data being sent to network layer: %s\n", data_msg);
				network_receive_from_transport(data_msg, data_size, dest);
				
				//Increase the sequence number for next packet and start over.
				sn = atoi(*seq_num);
				sn++;
				sprintf(*seq_num, "%02d", sn);
				free(packet);
			}
			//Send the last bit of the segment
			if (msg_size > 0 && msg_size < data_size) {
				//Copy rest of message over to temp string;
				char *temp = (char*)malloc(sizeof(*msg)*sizeof(char) + 1);
				i = 0;
				while((*msg)[j] != NULL)
						temp[i++] = (*msg)[j++];
				sprintf(data_msg, "D%d%d%s%s", source, dest, *seq_num, temp);
				printf("Last bit of data being sent: %s\n", data_msg);
				network_receive_from_transport(data_msg, strlen(data_msg), dest);
				free(temp);
			}
		}
		else {
			sprintf(data_msg, "D%d%d%s%s", source, dest, *seq_num, *msg);
			printf("Message size %d is less than or equal to %d\n Data message being sent to network layer:  %s\n", msg_size, data_size, data_msg);
			network_receive_from_transport(data_msg, len, dest);
		}
	}
	else {
		printf("Data Message is NULL\n");
	}
	
	//Always incrememnt the sequence number one last time for next transmission
	sn = atoi(*seq_num);
	sn++;
	sprintf(*seq_num, "%02d", sn);
	printf("%seq_num: s\n", *seq_num);
	free(data_msg);
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

void open_files(int id, node **head) {
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
