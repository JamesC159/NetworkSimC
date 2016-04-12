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
#include <sys/time.h>
#include <errno.h>

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
void network_route(node**);
void transport_send_string(char**, int, int, int, char **);
void transport_receive_from_network(char*, int, int);
void transport_output_all_received();
void write_channel();
void increment_seq_num(char**);
void close_file(int);

int main (int argc, char **argv) {
	int id, dur, dest, stime, i, ncount, ts = 0;
	char *msg, *seq_num;
	struct timeval tv;
	node *head = NULL, *tail = NULL;

	if (argc < 5) {
		fprintf(stderr, "Error: invalid number of command line arguments.\n");
		exit(1);
	}
	
	//Setup initial sequence number
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
	
	//Grab initial starting time of program before sending messages
	gettimeofday(&tv, NULL);
	ts = tv.tv_sec;
	
	//Process will run for dur seconds
	for (i = 0; i < dur; i++) {
		//datalink_receive_from_channel(&head);
		if(i > stime) {
			//Check to see if 5 seconds have elapsed in order to determine if we need
			//to send routing messages to compute new paths
			gettimeofday(&tv, NULL);
			if (tv.tv_sec - ts >= 5) {
				printf("5 seconds have elapsed since last sent message\nSending 10 routing messages to each neighbor\n");
				network_route(&head);
			}
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
	
	//Check the channel of each node
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
	char *d_msg = (char*)malloc(1+sizeof(msg));
	char *r_msg = (char*)malloc(12*sizeof(char));
	int i = 0, j = 0;

	//Check to see if the segment received from TCP is a data/XOR message
	if(msg[0] == 'D') {
		char *temp = (char*)malloc(1+sizeof(msg));
		for (j, i = 5; i < len; j++, i++)
			temp[j] = msg[i];
		sprintf(d_msg, "D%d%s", dest, temp);
		printf("Network Data Message Being Sent To DataLink Layer: %s\n", d_msg);
		free(temp);
	}
	else if (msg[0] == 'X') {
		printf("network layer received an XOR message\n");
	}
	else {
		fprintf(stderr, "Error: network layer received invalid message\n");
		exit(1);
	}
	
	free(d_msg);
}

void network_receive_from_datalink(char *msg, int len, int neighbor_id) {
	
}

void network_route(node **head) {
	printf("Inside network_route()\n");
	node *temp = *head;
	int i = 0;
	while(temp != NULL) {
		while(i < 10)
			printf("Sending routing message %d to neighbor node %d\n", ++i, temp->id);
		temp = temp->next;
		i = 0;
	}
}

/********* Transport Layer Functions *********/
void transport_send_string(char **msg, int source, int dest, int len, char **seq_num) {
	const int data_size = 6;
	int msg_size = strlen(*msg), p_len = 0;
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
				p_len = strlen(data_msg);
				printf("Data being sent to network layer: %s\n", data_msg);
				network_receive_from_transport(data_msg, p_len, dest);
				
				//Increase the sequence number for next packet and start over.
				increment_seq_num(seq_num);
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
				p_len = strlen(data_msg);
				network_receive_from_transport(data_msg, p_len, dest);
				free(temp);
			}
		}
		else {
			sprintf(data_msg, "D%d%d%s%s", source, dest, *seq_num, *msg);
			printf("Message size %d is less than or equal to %d\n Data message being sent to network layer:  %s\n", msg_size, data_size, data_msg);
			p_len = strlen(data_msg);
			network_receive_from_transport(data_msg, p_len, dest);
		}
	}
	else {
		printf("Data Message is NULL\n");
	}
	
	//Always incrememnt the sequence number one last time for next transmission
	increment_seq_num(seq_num);
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
		close_file(temp->ochannel);
		close_file(temp->ichannel);
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

void close_file(int fd) {
	if (close(fd) == -1) {
		exit(1);
	}
}

void increment_seq_num(char **sn) {
	int temp_sn;
	temp_sn = atoi(*sn);
	if (temp_sn == 99)
		temp_sn = 0;
	else
		temp_sn++; 
	sprintf(*sn, "%02d", temp_sn);
}