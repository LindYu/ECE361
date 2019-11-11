/* 
 *
 */
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_NAME 100
#define MAX_DATA 1000
#define BUFLEN 1025

typedef enum state {LOGIN, LO_ACK, LO_NAK, EXIT, JOIN, JN_ACK, JN_NAK, LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK} Type;

bool ackReceived = 0, nackRec = 0;

struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct lab3message *recvmes;

char* formatPacket(struct lab3message *mes) {
    char* formattedStr;
    asprintf(&formattedStr, "%d:%d:%s:%s", mes->type, mes->size, mes->source, mes->data);
    return formattedStr;
}

struct lab3message* packet_demultiplexer(char message[]) {
	//printf("%s\n", message);
    struct lab3message *pkt = (struct lab3message*) malloc(sizeof (struct lab3message));
    int cnt = 0;
    int cnt_field = 0;
    char field[MAX_DATA];
    while (cnt < 2) {
        if (message[cnt_field] == ':') {
            field[cnt_field] = '\0';
            if (cnt == 0) {
                pkt->type = atoi(field);
            } else {
                pkt->size = atoi(field);
            }
            cnt++;
            message += cnt_field + 1;
            cnt_field = 0;
        }
           
        field[cnt_field] = message[cnt_field];
        cnt_field++;
    }
    
    cnt_field = 0;
    while (message[0] != ':') {
        pkt->source[cnt_field] = message[0];
        message++;
        cnt_field++;
    }
    pkt->source[cnt_field] = '\0';
    
    message++;
    strcpy(pkt->data, message);
    return pkt;
}

void *receiveMessage(void* socket) {
    int sockfd, ret;
    char buf[BUFLEN];
    sockfd = *((int*)socket);
	while(1){
		ret = recv(sockfd, buf, BUFLEN-1, 0);
		if (ret == -1) {
		    perror("recv");    
		    close(sockfd);                       
		    exit(1);                      
		} else {
		    // message received
			buf[ret] = '\0';
			free(recvmes);
		    recvmes = packet_demultiplexer(buf);
		    switch (recvmes->type) {
		        case LO_ACK: case JN_ACK: case NS_ACK: case QU_ACK:
		            ackReceived = 1;
		            nackRec = 0;
		            break;
		        case LO_NAK: case JN_NAK:
		            ackReceived = 1;
		            nackRec = 1;
		            break;
		        case MESSAGE:
		            // received message from other client, output directly
		            printf("%s", recvmes->data);
		            break;
		        default:
		            break;
		    } 
		}
	}
}

int main() {
    // input variables
    char command[MAX_NAME];
    char sIP[MAX_NAME], sPort[MAX_NAME];
    char sessionID[MAX_NAME];

    // networking variables
    int sockfd, numbytes;
    char buf[BUFLEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    pthread_t rThread;

    memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct lab3message newmes;
    newmes.source[0] = '\0';

    while (1) {
        printf("> ");
        scanf("%s", command);
       // printf("%s\n", command);
	    if (strcmp(command, "/login") == 0) {
	        scanf("%s%s%s%s", newmes.source, newmes.data, sIP, sPort);

	        // get connection
            rv = getaddrinfo(sIP, sPort, &hints, &servinfo);
            if (rv != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            } // got address info

            bool connected = 0;
            for (p = servinfo; p != NULL && !connected; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }
                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    perror("client: connect");
                    close(sockfd);
                    continue;
                }
                connected = 1;
            }
            if (!connected) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
            }
            freeaddrinfo(servinfo);
            // connected, login
            newmes.type = LOGIN;
            newmes.size = strlen(newmes.data);
            char* message = formatPacket(&newmes);
            // send packet
            send(sockfd, message, strlen(message), 0);
            
            ackReceived = 0;
            pthread_create(&rThread, NULL, receiveMessage, &sockfd);

            // wait for receiving of acknowledgement
            while (!ackReceived) {}
            // acknowledgement received
            if (nackRec) {
                printf("Login not successful.\n%s\n", recvmes->data);
            } else printf("Login successful.\n");
	    } else if (strcmp(command, "/logout") == 0) {
       	    // logout
            if (newmes.source[0] != '\0') {
                newmes.type = EXIT;
                newmes.size = 0;
                newmes.data[0] = '\0';
                char *message = formatPacket(&newmes);
                send(sockfd, message, strlen(message), 0);
                newmes.source[0] = '\0';
                close(sockfd);
                printf("Logged out.\n");
            } else printf("Not logged in.\n");
	    } else if (strcmp(command, "/joinsession") == 0) {
	        scanf("%s", newmes.data);
	        // join session
            if (newmes.source[0] != '\0') {
                newmes.type = JOIN;
                newmes.size = strlen(newmes.data);
                char* message = formatPacket(&newmes);
                ackReceived = 0;
                send(sockfd, message, strlen(message), 0);
                while (!ackReceived) {}
                if (nackRec) {
                    printf("Session not joined.\n%s\n", recvmes->data);
                } else printf("Successfully joined session.\n");
            } else printf("Not Logged in.\n");
	    } else if (strcmp(command, "/leavesession") == 0) {
  	        // leave session
            if (newmes.source[0] != '\0') {
                newmes.type = LEAVE_SESS;
                newmes.size = 0;
                newmes.data[0] = '\0';
                char* message = formatPacket(&newmes);
                send(sockfd, message, strlen(message), 0);
                printf("Leave session request sent.\n");
            } else printf("Not logged in.\n");
	    } else if (strcmp(command, "/createsession") == 0) {
	        scanf("%s", newmes.data);
	        // create session
            if (newmes.source[0] != '\0') {
                newmes.type = NEW_SESS;
                newmes.size = strlen(newmes.data);
                char* message = formatPacket(&newmes);
                ackReceived = 0;
                send(sockfd, message, strlen(message), 0);

                while (!ackReceived) {}
                printf("Successfully created session.\n");
            } else printf("Not logged in.\n");
	    } else if (strcmp(command, "/list") == 0) {
	        //list connected clients and available sessions
            if (newmes.source[0] != '\0') {
                newmes.type = QUERY;
                newmes.size = 0;
                newmes.data[0] = '\0';
                char* message = formatPacket(&newmes);
                ackReceived = 0;
                send(sockfd, message, strlen(message), 0);
				printf("%s\n", message);

                while (!ackReceived){}
                
                // demultiplex query data and output
                printf("List of active sessions and clients: \n");
                char* qdata = recvmes->data;
                while (qdata[0] == 'S') {
                    (qdata) += 2;
                    char clientID [MAX_NAME], sessionID[MAX_NAME];
                    int cnt = 0;
                    while (qdata[0] != ':') {
                        sessionID[cnt++] = qdata[0];
                        qdata++;
                    }
                    sessionID[cnt] = '\0';
                    printf ("Session %s:\n", sessionID);
                    qdata++;
                    while (qdata[0] == 'C') {
                        qdata+=2;
                        cnt = 0;
                        while(qdata[0] != ':') {
                            clientID[cnt++] = qdata[0];
                            qdata++;
                        }
                        clientID[cnt] = '\0';
                        printf (" - %s\n", clientID);
                        qdata++;
                    }
                }
                qdata+=2;
                printf("Free clients:\n");
                while (qdata[0] == 'C') {
                    qdata+=2;
                    int cnt = 0;
                    char clientID[MAX_NAME];
                    while (qdata[0] != ':') {
                        clientID[cnt++] = qdata[0];
                        qdata++;
                    }
                    clientID[cnt] = '\0';
                    printf(" - %s\n", clientID);
					qdata++;
                }
            }
	    } else if (strcmp(command, "/quit") == 0) {
	        // terminate the program
            if (newmes.source[0] != '\0') {
                newmes.type = EXIT;
                newmes.size = 0;
                newmes.data[0] = '\0';
                char *message = formatPacket(&newmes);
                send(sockfd, message, strlen(message), 0);
                printf("Logged out.\n");
            }

            printf("Terminating program.\n");
            close(sockfd);
	        exit(1);
	    } else {
	        // send message
            if (newmes.source[0] != '\0') {
                char* buffer;
                size_t bufsize = 1000*sizeof(char);
                getline(&buffer, &bufsize, stdin);
				sprintf(newmes.data, "%s%s", command, buffer);
                
                newmes.type = MESSAGE;
                newmes.size = strlen(newmes.data);
                char *message = formatPacket(&newmes);
                send(sockfd, message, strlen(message), 0);
            } else printf("Not logged in.\n");
	    }
    }
}