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
#include <limits.h>

/********* Linked List *********/
typedef struct Node {
	int ichannel, ochannel;
	int id;
	int cost;
	struct Node *next;
} node;

/********* Routing Table *********/
typedef struct RTB {
	int nid[10];
	char pvector[10][11];
}RTB;

/********* Cost Vector *********/
typedef struct CTB {
	int cost;
	node *dest;
}CTB;

/********* Function Prototypes ********/
void insert_end(int);
void clear_list();
void open_files(int);
void datalink_receive_from_network(char*, int, char);
void datalink_receive_from_channel();
void network_receive_from_transport(char*, int, int);
void network_receive_from_datalink(char*, int, int);
void network_route(int);
void network_encapsulate(char*, int, int, char);
void transport_send_string(char**, int, int, int);
void transport_receive_from_network(char*, int, int);
void transport_output_all_received();
char* build_packet(char**, int*, int*, int);
void init_pvector();
void write_channel();
void increment_seq_num();
void close_file(int);
void broadcast(char*);

RTB *r_table;
int id, dur, dest;
node *head, *tail;
char *seq_num;
int ifd, ofd;

int main (int argc, char **argv) {
	int stime = 0, i = 0, j = 0, ts = 0;
	char *msg;
	struct timeval tv;
	
	//Make sure minimum number of command line arguments are met
	if (argc < 5) {
		fprintf(stderr, "Error: invalid number of command line arguments.\n");
		exit(1);
	}
	
	id = (int)strtol(argv[1], NULL, 10);
	dest = (int)strtol(argv[3], NULL, 10);
	dur = (int)strtol(argv[2], NULL, 10);
	
	//Initialize the routing table and path vector
	r_table = (RTB*)malloc(sizeof(RTB));
	init_pvector();
	
	//Setup initial sequence number
	seq_num = (char*)malloc(2*sizeof(char));
	seq_num[0] = '0';
	seq_num[1] = '0';
		
	//Build Linked List of known neighbors
	if (id == dest)
		for (i = 4; i < argc; i++)
			insert_end((int)strtol(argv[i], NULL, 10));
	else
		for (i = 6; i < argc; i++)
			insert_end((int)strtol(argv[i], NULL, 10));
		
	//Open Files for reading and writing based on neighboring nodes. Assuming 2 way communication
	open_files(id);
	
	//If id is not equal to the destination, then we have a message to deliver and a transport layer start time.
	if (id != dest) {
		msg = argv[4];
		stime = (int)strtol(argv[5], NULL, 10);
	}
	else
		msg = NULL;
	
	//Grab initial starting time of program before sending messages
	gettimeofday(&tv, NULL);
	ts = tv.tv_sec;	
	for (i = 0; i < dur; i++) {
		//datalink_receive_from_channel(&head);
		if(i > stime) {
			datalink_receive_from_channel();
			
			//Check to see if 5 seconds have elapsed in order to determine if we need
			//to send routing messages to compute new paths
			gettimeofday(&tv, NULL);
			if (tv.tv_sec - ts >= 5) {
				//printf("\nID: %d - 5 seconds have elapsed since last sent message\nSending 10 routing messages to each neighbor\n", id);
				network_route(dest);
				ts = tv.tv_sec;
			}

			if (msg != NULL)
				//Send msg accross transport layer
				transport_send_string(&msg, id, dest, (int)strlen(msg));
		}
		//datalink_receive_from_channel();
		sleep(1);
	}
	
	//Free dynamic memory
	clear_list();
	printf("%d is done\n", id);
	free(seq_num);
	free(r_table);
	return 0;
}

/********* Datalink Layer Functions *********/
void datalink_receive_from_network(char *msg, int len, char next_hop) {
	char *dlink_msg = (char*)calloc(100, sizeof(char));
	int i = 0, fidx = 0, midx = 0;
	char *frame = (char*)calloc(100, sizeof(char));
	
	//Byte Insertion for beginning and end of frame markers
	frame[0] = 'F';
	for (fidx = 1, midx = 0; midx < len; midx++, fidx++) {
		switch(msg[midx]) {
			case 'E': {
				frame[fidx] = 'X';
				frame[fidx + 1] = 'E';
				fidx++;
				break;
			}
			case 'F': {
				frame[fidx] = 'X';
				frame[fidx + 1] = 'F';
				fidx++;	
				break;
			}
			case 'X': {
				frame[fidx] = 'X';
				frame[fidx + 1] = 'X';
				fidx++;
				break;
			}
			default:
				frame[fidx] = msg[midx];
		}
	}
	
	frame[(int)strlen(frame)] = 'E';
	sprintf(dlink_msg, "data %d %s %s", 2, seq_num, frame);
	//printf("ID: %d - Datalink Layer received message from Network Layer: %s\n\n", id, dlink_msg);	
	
	//Output to the correct destination output channel
	node *neighbor = head;
	if (next_hop == 'X')
		while(neighbor != NULL) {
			write(neighbor->ochannel, dlink_msg, (int)strlen(dlink_msg));
			neighbor = neighbor->next;
		}
	else
		while(neighbor != NULL) {
			if (next_hop == '0' + neighbor->id) {
				write(neighbor->ochannel, dlink_msg, (int)strlen(dlink_msg));
				break;
			}
			
			neighbor = neighbor->next;
		}
		
	free(frame);
	free(dlink_msg);
}
	

void datalink_receive_from_channel() {
	node *neighbor = head;
	int bytes_read, i = 0;
	char *buf = (char*)calloc(25, sizeof(char));
	char *full_msg = (char*)calloc(100, sizeof(char));

	//Check the channel of each node
	while(neighbor != NULL) {
		while((bytes_read = read(neighbor->ichannel, buf, 25)) == 0 && i < 10) {
			i++;
		}
		//full_msg[(int)strlen(full_msg)] = NULL;
		//if (bytes_read != 0) {
			network_receive_from_datalink(buf, (int)strlen(buf), neighbor->id);
		//}

		neighbor = neighbor->next;
	}
	
	sleep(1);
	free(buf);
	free(full_msg);
}

/********* Network Layer Functions *********/
void network_receive_from_transport(char *msg, int len, int dest) {
	char *r_msg = (char*)calloc(50, sizeof(char));
	int i = 0, j = 0;
	
	network_encapsulate(msg, dest, len, '0' + dest);
}

void network_encapsulate(char *msg, int dest, int len, char next_hop) {
	char *d_msg = (char*)calloc(100, sizeof(char));
	int i = 0, j = 0, hop_count = 0, is_path = 0;
	char original_source;
	
	sprintf(d_msg, "D%d%s", dest, msg);
	next_hop = (r_table->pvector)[dest][1];
	datalink_receive_from_network(d_msg, (int)strlen(d_msg), next_hop);
	free(d_msg);
}

void network_receive_from_datalink(char *msg, int len, int neighbor_id) {
	int i, j, k, rtb_count = 0, msg_count = 0, hop_count = 0, is_path = 0;
	char original_source;

	for(i = 0; i < 25; i++) {		
		//Check to see if the message is destined for this node
		if(msg[i] == 'D' && msg[i+1] == '0' + id) {
				transport_receive_from_network(msg, len, neighbor_id);
				break;
		}
		
		//If it is a routing message, then it is destined for this node
		//Currently, this only looks for one complete routing message within the packet received
		//Need to account for partial routing messages in a packet.
		//BAD IMPLEMENTATION, I JUST WANT THINGS TO WORK.. HAHA
		else if(msg[i] == 'R') {
			if(i+1 < len) {
				original_source = msg[i+1];
				
				//check to see if we have a full path to update
				//Or check to see if we have the entire path vector
				if (i+2 < len) {
					j = i+2;
				}
				else {
					j = i+1;
				}

				for(j; j < len; j++) {
					if(j >= i + 2 && msg[j] != 'X') {
						hop_count++;
					}
					else {
						break;
					}
				}
				
				//If we at least counted one hop, then we know there was a path to the destination of the routing message from
				//the original source.
				if(hop_count > 0) {
					dest = msg[j-1];
					is_path = 1;
				}
				else {
					is_path = 0;
				}
			}
			
			//this does not account for broken routing messages
			if(is_path) {
				printf("Source of routing message is: %c\nDestination of routing message is: %c\nHop count = %d\n\n", original_source, dest, hop_count);
			}
			else {
				printf("Hop count was 0, there was no path from %c\n\n", original_source);
			}



			transport_receive_from_network(msg, len, neighbor_id);			
			break;			
		}
	}
}

void network_route(int dest) {
	node *neighbor = head;
	int i, j;
	char *temp_pv = (char*)calloc(20, sizeof(char));
	
	while(neighbor != NULL) {
		
		//No need for byte insertion protocol since frame markers will not
		//appear in the routing messages by code design
		temp_pv[0] = 'R';
		
		for(i = 0; i < 10; i++) {
			for(j = 1; j < 12; j++) {
				temp_pv[j] = (r_table->pvector)[i][j-1];
			}

			write(neighbor->ochannel, temp_pv, (int)strlen(temp_pv));
		}
		
		neighbor = neighbor->next;
	}
}

/********* Transport Layer Functions *********/
void transport_send_string(char **msg, int source, int dest, int len) {
	const int data_size = 6;
	int msg_size = (int)strlen(*msg), p_len = 0, i = 0, j = 0, sn = 0, msg_count = 0;
	char *data_msg = (char*)malloc(10*sizeof(char));
	char *prev_msg1 = (char*)malloc(10*sizeof(char)), *prev_msg2 = (char*)malloc(10*sizeof(char));
	char *xor = (char*)malloc(10*sizeof(char));

		//Break segment up into multiple segments if msg is too large
		if (msg_size > data_size) {
			while (msg_size > data_size) {
				char *packet = build_packet(msg, &msg_size, &j, data_size);
				sprintf(data_msg, "D%d%d%s%s", source, dest, seq_num, packet);
				
				//Send packet to network layer
				network_receive_from_transport(data_msg, (int)strlen(data_msg), dest);
				
				//Store pairs of messages
				(msg_count++ % 2 == 0 ? strcpy(prev_msg1, packet) 
						: strcpy(prev_msg2, packet));
						
				//Logical XOR contents of each message once a pair is obtained
				if(msg_count == 2) {
					char *temp_xor = (char*)malloc(6*sizeof(char));
					
					for (i = 0; i < 6; i++)
						temp_xor[i] = (char)(prev_msg1[i] ^ prev_msg2[i]);
					
					sprintf(xor, "X%d%d%s%X", source, dest, seq_num, temp_xor);
					
					//Send xor of packet pair to network layer
					network_receive_from_transport(xor, (int)strlen(xor), dest);
					msg_count = 0;
					free(temp_xor);
				}
				
				//Increase the sequence number for next packet and start over.
				increment_seq_num();
				free(packet);
			}
			//Send the last bit of the segment
			if (msg_size >= 0 && msg_size <= data_size){
				//Copy rest of message over to end_packet
				char *end_packet = (char*)malloc(data_size*sizeof(char));
				
				i = 0;
				while((*msg)[j] != NULL)
						end_packet[i++] = (*msg)[j++];
				end_packet[i] = NULL;
					
				//Send message to network layer
				sprintf(data_msg, "D%d%d%s%s", source, dest, seq_num, end_packet);
				
				//Send packet to network layer
				network_receive_from_transport(data_msg, (int)strlen(data_msg), dest);
				
				//Store pairs of messages
				(msg_count++ % 2 == 0 ? strcpy(prev_msg1, end_packet) 
						: strcpy(prev_msg2, end_packet));
						
				//Logical XOR contents of each message once a pair is obtained
				if(msg_count == 2) {
					char *temp_xor = (char*)malloc(6*sizeof(char));
					
					for (i = 0; i < 6; i++)
						temp_xor[i] = (char)(prev_msg1[i] ^ prev_msg2[i]);
					
					sprintf(xor, "X%d%d%s%X", source, dest, seq_num, temp_xor);
					
					//Send xor of packet pair to network layer
					network_receive_from_transport(xor, (int)strlen(xor), dest);
					msg_count = 0;
					free(temp_xor);
				}
				free(end_packet);
			}
		}
		else {
			//Message is equal to or smaller than one packet, send to network layer
			sprintf(data_msg, "D%d%d%s%s", source, dest, seq_num, *msg);
			network_receive_from_transport(data_msg, (int)strlen(data_msg), dest);
		}
		
	//Always incrememnt the sequence number one last time for next transmission
	increment_seq_num();
	free(prev_msg1);
	free(prev_msg2);
	free(xor);
	free(data_msg);
}

char* build_packet(char **msg, int *msg_size, int *msg_idx, int data_size) {
	int i = 0;

	//Break segment up into multiple segments if msg is too large
	char *packet = (char *)malloc(6*sizeof(char));
	for (i = 0; i < data_size - 1; i++) {
		if (*msg_size == 1)
			break;
		packet[i] = (*msg)[(*msg_idx)++];
		(*msg_size)--;
	}
       	packet[i] = NULL;
	return packet;
}

void transport_receive_from_network(char *msg, int len, int neighbor_id) {
	printf("Transport Layer of Node %d received %s from message from neighbor %d\n\n", id, msg, neighbor_id);
}

void transport_output_all_received() {
}

/********* Linked List Functions *********/
void insert_end (int node_id) {
	if (head == NULL) {
		head = malloc(sizeof(node));
		head->id = node_id;
		head->cost = 1;
		//Enter routing information for my neighbors
		(r_table->nid)[head->id] = head->id;
		(r_table->pvector)[head->id][1] = '0' + head->id;
		(r_table->pvector)[head->id][10] = 'X';
		head->next = NULL;
		tail = head;
	}
	else {
		node *temp = malloc(sizeof(node));
		temp->id = node_id;
		temp->cost = 1;	
		//Enter routing information for my neighbors
		(r_table->nid)[temp->id] = temp->id;
		(r_table->pvector)[temp->id][1] = '0' + temp->id;	
		(r_table->pvector)[temp->id][10] = 'X';
		tail->next = temp;
		tail = temp;
		tail->next = NULL;
	}
}

void clear_list() {
	node *temp = head;
	node *next;
	
	while(temp != NULL) {
		next = temp->next;
		close_file(temp->ochannel);
		close_file(temp->ichannel);
		free(temp);
		temp = next;
	}
	head = NULL;
}

void open_files(int id) {
	node *temp = head;
	int iflag = (O_CREAT | O_RDONLY);
	int oflag = (O_TRUNC | O_CREAT | O_WRONLY);
	char *ichan = (char*)malloc(9 * sizeof(char));
	char *ochan = (char*)malloc(9 * sizeof(char));

	//Open input and ouput channels for each neighbor
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
		
		//Store the file descriptors in the neighbors themselves for convenience.
		temp->ichannel = ifd;
		temp->ochannel = ofd;
		temp = temp->next;
	}
}

/********* Misc Functions *********/
void close_file(int fd) {
	if (close(fd) == -1)
		exit(1);
}

void increment_seq_num() {
	int temp_sn;
	
	temp_sn = (int)strtol(seq_num, NULL, 10);
	(temp_sn == 99 ? temp_sn = 0 : temp_sn++);
	sprintf(seq_num, "%02d", temp_sn);
}

void init_pvector() {
	int i, j;
	for(i = 0; i < 10; i++)
		for(j = 0; j < 11; j++)
			if(j == 0)
				(r_table->pvector)[i][j] = '0' + id;
			else if (j == 10)
				(r_table->pvector)[i][j] = '0' + i;
			else
				(r_table->pvector)[i][j] = 'X';
}

void init_pvector() {
	int i, j;
	for(i = 0; i < 10; i++)
		for(j = 0; j < 10; j++)
			if(j == 0)
				(r_table->pvector)[i][j] = '0' + id;
			else if (j == 9)
				(r_table->pvector)[i][j] = '0' + i;
			else
				(r_table->pvector)[i][j] = 'X';
}
