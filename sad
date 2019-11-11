/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.c
 * Author: yutiany3
 *
 * Created on November 10, 2017, 9:08 AM
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_NAME 100
#define MAX_DATA 1000
#define BACKLOG 6
#define MAX_PORT 10


char* Type[] = {"LOGIN", "LO_ACK", "LO_NAK", "EXIT", "JOIN", "JN_ACK", "JN_NAK", "LEAVE_SESS", "NEW_SESS", "NS_ACK", "MESSAGE", "QUERY", "QU_ACK"};

typedef enum state {
    LOGIN, LO_ACK, LO_NAK, EXIT, JOIN, JN_ACK, JN_NAK, LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK, LAST
} State;


// also need their IP and port address

typedef struct client {
    unsigned char name[MAX_NAME];
    char password[100];
    bool isloggedin;
    char sessionName[1000];
    int portNum;
    char IPaddr[100];
} clientInfo;

struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

typedef struct session {
    int sessionID;
    char sessionName[1000];
    int numberofppl;
    bool isdead;
} sessionInfo;

/*A hardcoded record of the client information*/
clientInfo clientlist[6] = {
    {"trezan", "trezannn", 0, "", -2, ""},
    {"derekrocks12345", "derek12345", 0, "", -2, ""},
    {"meow", "meowmeow", 0, "", -2, ""},
    {"pandaman6477", "brown142756", 0, "", -2, ""},
    {"yayforalex", "alex123", 0, "", -2, ""},
    {"ranoutofideas", "imsad", 0, "", -2, ""}
};



/*Array of sessions*/
/* The session is designed as follows: maximum 100 sessions, when a session dies 
   a tombstone is left for the session*/
sessionInfo *sessions[100];
int currentAllSessionNum = 0;

char* formatPacket(struct lab3message *mes) {
    char* formattedStr;

    asprintf(&formattedStr, "%d:%d:%s:%s", mes->type, mes->size, mes->source, mes->data);
    printf("FormattedStr: %s", formattedStr);
    return formattedStr;
}

struct lab3message* packet_demultiplexer(char* message) {
    struct lab3message *pkt = (struct lab3message*) malloc(sizeof (struct lab3message));
    int cnt = 0;
    int cnt_field = 0;
    char field[1000];

    while (cnt < 2) {
        if (message[cnt_field] == ':') {
            field[cnt_field] = '\0';
            if (cnt == 0) {
                // Number the type starting from 0!!! Assume the sent packet won't contain invalid strings
                for (int a = 0; a < 14; a++) { // 13 states in total
                    if (strcmp(field, Type[a]) == 0) {
                        pkt->type = a; // start from 0
                        break;
                    }
                }
                cnt++;
            } else if (cnt == 1) {
                pkt->size = atoi(field);
                cnt++;
            }
            // Moved pointer to next field, excluding the colon
            message = message + cnt_field + 1;
            cnt_field = 0;
        }
        field[cnt_field] = message[cnt_field];
        cnt_field++;
    }

    for (int k = 0; message[k] != ':'; k++) {
        pkt->source[k] = message[k];
        if (message[k + 1] == ':') pkt->source[k + 1] = '\0';
    }
    message = message + strlen(pkt->source) + 1;

    for (int j = 0; message[j] != '\0'; j++) {
        pkt->data[j] = message[j];
        if (message[j + 1] == ':') pkt->data[j + 1] = '\0';
    }

    return pkt;

}

/*
 *
 */

int main(int argc, char *argv[]) {
    int status, client_socket[MAX_PORT], main_port;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage their_addr;

    socklen_t addr_size;
    int sockfd, new_fd, nbytes, max_sd, activity;
    fd_set readfds;

    char buf[1500];
    char *bufpt;

    // error checking
    if (argc != 2) {
        fprintf(stderr, "usage: server portNumber\n");
        return 1;
    }
    

    // load up address structs
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // call getaddrinfo and error checking
    if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    // make a socket, bind and listen

    for (p = res; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &res, sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    main_port = sockfd;

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(main_port, BACKLOG) == -1) {
        perror("listen");
        close(main_port);
        exit(1);
    }

    // accept incoming connection
    addr_size = sizeof their_addr;

	for (int i = 0; i < MAX_PORT; i++) {
        client_socket[i] = 0;
    }

     freeaddrinfo(res);

    while (1) {
		char theirIP[100];

        FD_ZERO(&readfds);
		FD_SET(main_port, &readfds);
        max_sd = main_port;
        printf("Socket number is %d, Main port is %d\n", sockfd, main_port);

		        
        // setting all the current ports, and checking on incoming requests
        for (int i = 0; i < MAX_PORT; i++) {
            sockfd = client_socket[i];
            if (sockfd > 0) FD_SET(sockfd, &readfds);
            if (sockfd > max_sd) max_sd = sockfd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }
               
        if (FD_ISSET(main_port, &readfds)) {
            new_fd = accept(main_port, (struct sockaddr *) &their_addr, &addr_size);

            if (new_fd == -1) {
                close(new_fd);
                perror("accept");
                // continue;
                exit(1);
            }
		  // record client IP and port # 
         
          strcpy(theirIP, inet_ntoa((((struct sockaddr_in *) &their_addr)->sin_addr)));
          printf("Client IP is %s, port number is %d\n", theirIP, new_fd);

            for (int i = 0; i < MAX_PORT; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_fd;
                    printf("adding to socket as %d\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_PORT; i++) {
            sockfd = client_socket[i];

            if (FD_ISSET(sockfd, &readfds)) {
                printf("Currently checking socket: socket id is %d, sockedfd is %d\n", i,new_fd);
                /*if ((nbytes = read(new_fd, buf, 1499) == 0)) {
                    printf("Host disconnected\n");
                    close(new_fd);
                    client_socket[i] = 0;
                } 
                else if{*/
                    printf("Checking socket %d\n", sockfd);
                    // receive data from client
                    if ((nbytes = recv(sockfd, buf, sizeof(buf), 0)) <= 0) {
                        // error
                        if (nbytes == 0) {
                            printf("server: socket closed\n");
                        } else {  
                            perror("recv");
                        }
                        close(sockfd);
                        client_socket[i] = 0;
                        exit(1);
                    }

                    buf[nbytes] = '\0';

					printf("%s", buf);

                    struct lab3message* pkt;
                    pkt = packet_demultiplexer(buf);

                    char* formatted;
                    bool found = 0;
                    struct lab3message* reply = (struct lab3message*) malloc(sizeof (struct lab3message));

                    // variables to be used in switch statement
                    int sender;
                    int len, bytes_sent;
                    char sessiontobeJoined[100];


                    switch (pkt->type) {

                            /********for login the IP port and addr is not saved in client yet! need to test first********/
                        case LOGIN:
                        {
                            for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                if (strcmp(pkt->source, clientlist[i].name) == 0) { // found client
                                    found = 1;
                                    if (clientlist[i].isloggedin) {//NACK
                                        char* loggedin = "Error: client is already logged in.\n";
                                        perror(loggedin);

                                        // Dumb question: shouldn't the source here be the server IP? - change to IP when integrating

                                        strcpy(reply->data, loggedin);
                                        reply->size = strlen(loggedin);
                                        strcpy(reply->source, pkt->source);

                                        reply->type = LO_NAK;

                                        formatted = formatPacket(reply);
                                        printf("%s", formatted);
                                        break;
                                    }
                                    // parse to get password!
                                    char* password = pkt->data + strlen(pkt->source) + 1;
                                    //   printf("%c", password);
                                    if (strcmp(clientlist[i].password, password) != 0) {
                                        char* wrongpass = "Error: wrong password.\n";
                                        perror(wrongpass);

                                        // Dumb question: shouldn't the source here be the server IP? - change to IP when integrating

                                        strcpy(reply->data, wrongpass);
                                        reply->size = strlen(wrongpass);
                                        strcpy(reply->source, pkt->source);
                                        reply->type = LO_NAK;

                                        formatted = formatPacket(reply);
                                        printf("%s", formatted);
                                    } else { //ACK                        
                                        strcpy(reply->data, "");
                                        reply->size = 0;
                                        strcpy(reply->source, pkt->source);
                                        reply->type = LO_ACK;
                                        clientlist[i].portNum = sockfd;
										strcpy(clientlist[i].IPaddr,theirIP);

                                        formatted = formatPacket(reply);
                                        printf("Successfully login: %s", formatted);
                                        clientlist[i].isloggedin = 1;
                                    }
                                    break;
                                }
                            }
                            if (!found) {
                                char* notFound = "Error: client not found.\n";
                                perror(notFound);

                                // Dumb question: shouldn't the source here be the server IP? - change to IP when integrating
                                strcpy(reply->data, notFound);
                                reply->size = strlen(notFound);
                                strcpy(reply->source, pkt->source);
                                reply->type = LO_NAK;
                                formatted = formatPacket(reply);
                            }

                            // send package 
                            len = strlen(formatted);
                            bytes_sent = send(new_fd, formatted, len, 0);

                            // error checking
                            if (bytes_sent == -1) {
                                perror("send");
                            } else if (bytes_sent != len) {
                                printf("not completely sent\n");
                            }


                            free(reply);
                            free(pkt);
                            break;
                        }

                        case MESSAGE:
                        {
                            // get sender ID                   
                            for (int i = 0; i = MAX_PORT; i++) {
                                if (strcmp(pkt->source, clientlist[i].name) == 0) {
                                    sender = i;
                                    break;
                                }
                            }
                            // output data: Client(sender) ID, message 
                            strcpy(reply->source, pkt->source);
                            char msg[] = "Client ID: ";
                            strcat(msg, pkt->source);
                            char a[] = " Message: ";
                            strcat(msg, a);
                            strcat(msg, pkt->data);
                            strcpy(reply->data, msg);
                            reply->type = MESSAGE;
                            reply->size = strlen(reply->data);
                            formatted = formatPacket(reply);

                            // broadcast to all other members in the session
                            for (int i = 0; i = MAX_PORT; i++) {
                                if (strcmp(clientlist[sender].sessionName, clientlist[i].sessionName) == 0 && i != sender) {
                                    len = strlen(formatted);
                                    bytes_sent = send(clientlist[i].portNum, formatted, len, 0);
                                    // error checking
                                    if (bytes_sent == -1) {
                                        perror("send");
                                    } else if (bytes_sent != len) {
                                        printf("not completely sent\n");
                                    }
                                }
                            }
                            free(reply);
                            free(pkt);

                            break;
                        }
                        case QUERY:
                        {
                            // also traverse the list of sessions and list of clients... need to send multiple since packet size is small?
                            reply->type = QU_ACK;
                            char s[] = "S:";

                            printf("S:");
                            char name[100];
                            for (int i = 0; i < currentAllSessionNum; i++) {
                                if (sessions[i]->isdead != 0) {
                                    strcpy(name, sessions[i]->sessionName);
                                    strcat(s, name);
                                    printf("%s:", sessions[i]->sessionName);

                                    for (int j = 0; j < MAX_PORT; j++) {
                                        if (strcmp(clientlist[j].sessionName, sessions[i]->sessionName) == 0 && clientlist[j].isloggedin != 0) {
                                            strcpy(name, clientlist[j].name);
                                            strcat(s, "C:");
                                            strcat(s, name);
                                            strcat(s, ":");
                                            printf("C:%s:", clientlist[j].name);
                                        }
                                    }
                                }
                            }

                            printf("N:");
                            strcat(s, "N:");
                            for (int i = 0; i < MAX_PORT; i++) {
                                if (strcmp(clientlist[i].sessionName, "") == 0 && clientlist[i].isloggedin != 0) {
                                    printf("C:%s:", clientlist[i].sessionName);
                                    strcpy(name, clientlist[i].name);
                                    strcat(s, "C:");
                                    strcat(s, name);
                                    strcat(s, ":");
                                }
                            }
                            strcpy(reply->data, s);
                            strcpy(reply->source, pkt->source);
                            reply->size = strlen(s);

                            //need to write data string!!! - to be safe print out strlen for debugging
                            printf("Length of query package: %d, S is %s ", pkt->size, s);

                            // send package 
                            formatted = formatPacket(reply);
                            len = strlen(formatted);
                            bytes_sent = send(new_fd, formatted, len, 0);

                            // error checking
                            if (bytes_sent == -1) {
                                perror("send");
                            } else if (bytes_sent != len) {
                                printf("not completely sent\n");
                            }

                            free(reply);
                            free(pkt);
                            break;
                        }
                        case NEW_SESS:
                        {
                            sessions[currentAllSessionNum]->isdead = 0;
                            sessions[currentAllSessionNum]->numberofppl = 1;
                            sessions[currentAllSessionNum]->sessionID = currentAllSessionNum + 1; // sessionID starts from 1!
                            strcpy(sessions[currentAllSessionNum]->sessionName, pkt->data);

                            for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                                    strcpy(clientlist[i].sessionName, pkt->data);
                                }
                                break;
                            }
                            currentAllSessionNum++;

                            //send ACK                
                            strcpy(reply->data, pkt->data);
                            reply->size = strlen(reply->data);
                            strcpy(reply->source, pkt->source);
                            reply->type = NS_ACK;
                            formatted = formatPacket(reply);

                            // send package 
                            len = strlen(formatted);
                            bytes_sent = send(new_fd, formatted, len, 0);

                            // error checking
                            if (bytes_sent == -1) {
                                perror("send");
                            } else if (bytes_sent != len) {
                                printf("not completely sent\n");
                            }

                            free(reply);
                            free(pkt);
                            break;
                        }

                        case JOIN:
                        {
                            strcpy(sessiontobeJoined, pkt->data);
                            bool found;
                            for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                if (strcmp(pkt->source, clientlist[i].name) == 0) {
                                    //Already joined a session, NACK 
                                    if (strcmp(clientlist[i].sessionName, "") != 0) {
                                        char* outofBound = ",Error: Client already joined a session.\n";
                                        perror("Error: Client already joined a session.\n");
                                        strcat(pkt->data, outofBound);
                                        strcpy(reply->data, pkt->data);
                                        reply->size = strlen(reply->data);
                                        strcpy(reply->source, pkt->source);
                                        reply->type = JN_NAK;
                                        formatted = formatPacket(reply);
                                        break;
                                    }
                                }
                            }
                            for (int i = 0; i < currentAllSessionNum; i++) {
                                // ACK to join a session
                                if (strcmp(sessiontobeJoined, sessions[i]->sessionName) == 0) {
                                    strcpy(clientlist[i].sessionName, pkt->data);
                                    strcpy(reply->data, pkt->data);
                                    reply->size = pkt->size;
                                    strcpy(reply->source, pkt->source);
                                    reply->type = JN_ACK;
                                    formatted = formatPacket(reply);
                                    found = 1;
                                    sessions[i]->numberofppl++;
                                    break;
                                }
                            }
                            // if session doesn't exist, NACK
                            if (!found) {
                                char* outofBound = ",Error: Session does not exist.\n";
                                perror("Error: Session does not exist.\n");
                                strcpy(pkt->data, outofBound);
                                strcpy(reply->data, pkt->data);
                                reply->size = strlen(reply->data);
                                strcpy(reply->source, pkt->source);
                                reply->type = JN_NAK;
                                formatted = formatPacket(reply);
                            }

                            // send package 
                            len = strlen(formatted);
                            bytes_sent = send(new_fd, formatted, len, 0);

                            // error checking
                            if (bytes_sent == -1) {
                                perror("send");
                            } else if (bytes_sent != len) {
                                printf("not completely sent\n");
                            }

                            free(reply);
                            free(pkt);
                            break;
                        }
                        case LEAVE_SESS:
                        {
                            //record new # of ppl in session, if no people mark session dead
                            for (int i = 0; i < currentAllSessionNum; i++) {
                                if (strcmp(pkt->data, sessions[i]->sessionName) == 0) {
                                    sessions[i]->numberofppl--;
                                    if (sessions[i]->numberofppl == 0) {
                                        sessions[i]->isdead = 1;
                                    }
                                    break;
                                }
                            }
							//mark the client as joined no session
							 for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
									 strcpy(clientlist[i].sessionName, "");
									 break;
									}
							 }
                            free(reply);
                            free(pkt);
                            break;
                        }
                        case EXIT:
                        {
                            for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                                    clientlist[i].isloggedin = 0;
                                    if (strcmp(clientlist[i].sessionName, "") != 0) {
                                        for (int j = 0; j < currentAllSessionNum; j++) {
                                            if (strcmp(clientlist[i].sessionName, sessions[j]->sessionName) == 0) {
                                                if (sessions[j]->numberofppl == 1) {
                                                    sessions[j]->numberofppl = 0;
                                                    sessions[j]->isdead = 1;
                                                } else sessions[j]->numberofppl--;
                                            }
                                        }
                                    }
                                    strcpy(clientlist[i].sessionName, "");
									strcpy(clientlist[i].IPaddr,"");
                                    clientlist[i].portNum = -2;
                                }
                                break;
                            }

							/*GET RID OF THIS SOCKET*/
							
							for(int i = 0; i < MAX_PORT;i++){	
								if(client_socket[i] == new_fd){
									client_socket[i] = 0;
									break;
								}
							}
                            close(new_fd);
							FD_CLR(new_fd, &readfds);
                            free(reply);
                            free(pkt);
                            break;
                        }
                    }
                };
            }
        }
    //}

    return 0;
}


