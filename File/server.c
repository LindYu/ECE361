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
#define MAX_PORT 20


char* Type[] = {"LOGIN", "LO_ACK", "LO_NAK", "EXIT", "JOIN", "JN_ACK", "JN_NAK", "LEAVE_SESS", "NEW_SESS", "NS_ACK", "MESSAGE", "QUERY", "QU_ACK", "INVITE", "INV_ACK", "INV_NACK"};

typedef enum state {
    LOGIN, LO_ACK, LO_NAK, EXIT, JOIN, JN_ACK, JN_NAK, LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK, INVITE, INV_ACK, INV_NACK, LAST
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
sessionInfo sessions[100];
int currentAllSessionNum = 0;

char* formatPacket(struct lab3message *mes) {
    char* formattedStr;

    asprintf(&formattedStr, "%d:%d:%s:%s", mes->type, mes->size, mes->source, mes->data);
    printf("FormattedStr: %s", formattedStr);
    return formattedStr;
}

struct lab3message* packet_demultiplexer(char* message) {
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
    fd_set master;

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

    FD_SET(main_port, &master);
    max_sd = main_port;

    // accept incoming connection
    addr_size = sizeof their_addr;

    freeaddrinfo(res);
	int target_port = 0;

    char theirIP[100];
    while (1) {
        readfds = master;
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= max_sd; i++) {
		    target_port = i;

            if (FD_ISSET(i, &readfds)) {
                printf("Currently checking socket: socket id is %d\n", i);
                if (i == main_port) {
                    // handle new connections
					printf("i is %d, main port is %d",i,main_port);
                    new_fd = accept(main_port, (struct sockaddr *) &their_addr, &addr_size);
                    
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(new_fd, &master); // add to master set
                        if (new_fd > max_sd) { // keep track of the max
                            max_sd = new_fd;
                        }
						strcpy(theirIP, inet_ntoa((((struct sockaddr_in *) &their_addr)->sin_addr)));
                        printf("Client IP is %s, port number is %d\n", theirIP, new_fd);
                    }
                } else {
                    // receive data from client
					printf("Here");
                    if ((nbytes = recv(i, buf, 1499, 0)) <= 0) {
                        // error
                        if (nbytes == 0) {
                            printf("server: socket closed\n");
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);                  
                    } else {

                        buf[nbytes] = '\0';
						printf("hi");

                        printf("%s\n", buf);
						printf("bye");

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

                                            strcpy(reply->data, loggedin);
                                            reply->size = strlen(loggedin);
                                            strcpy(reply->source, pkt->source);

                                            reply->type = LO_NAK;

                                            formatted = formatPacket(reply);
                                            printf("%s", formatted);
                                            break;
                                        } 
                                        char* password = pkt->data;
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
                                            clientlist[i].portNum = target_port;
                                            strcpy(clientlist[i].IPaddr, theirIP);

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
                                for (int i = 0; i < MAX_PORT; i++) {
								
                                    if (strcmp(pkt->source, clientlist[i].name) == 0) {
                                        sender = i;
                                        break;
                                    }
                                }
                                // output data: Client(sender) ID, message 
                                strcpy(reply->source, pkt->source);
                                char msg[] = "";
                                strcat(msg, pkt->source);
                                char a[] = ": ";
                                strcat(msg, a);
                                strcat(msg, pkt->data);
                                strcpy(reply->data, msg);
                                reply->type = MESSAGE;
                                reply->size = strlen(reply->data);
                                formatted = formatPacket(reply);
								// check if client already in session, if not ignore message
								if(clientlist[sender].sessionName[0] != '\0'){
								printf("Here, session name is %s",clientlist[sender].sessionName);

                                // broadcast to all other members in the session
                                for (int i = 0; i < MAX_PORT; i++) {
                                    if (strcmp(clientlist[sender].sessionName, clientlist[i].sessionName) == 0 && i != sender) 											{
										printf("Client port number is %d",clientlist[i].portNum);
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
								}
                                free(reply);
                                free(pkt);

                                break;
                            }
                            case QUERY:
                            {
                                // also traverse the list of sessions and list of clients... need to send multiple since packet size is small?
                                reply->type = QU_ACK;
                                char s[] = "";

                               
                                char name[100];
                                for (int i = 0; i < currentAllSessionNum; i++) {
                                    if (sessions[i].isdead != 0) {
										printf("Currently in session %s\n", sessions[i].sessionName);
                                        strcpy(name, sessions[i].sessionName);
										strcat(s,"S:");
                                        strcat(s, name);   
								                                  

                                        for (int j = 0; j < MAX_PORT; j++) {
                                            if (strcmp(clientlist[j].sessionName, sessions[i].sessionName) == 0 && clientlist[j].isloggedin != 0) {
                                                strcpy(name, clientlist[j].name);
                                                strcat(s, ":C:");
                                                strcat(s, name);                                  
                                                printf("C:%s:", clientlist[j].name);
                                            }
                                        }
										strcat(s,":");
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
                                printf("Length of query package: %d, S is %s\n", reply->size, s);

                                // send package 
                                formatted = formatPacket(reply);
                                len = strlen(formatted);
                                bytes_sent = send(target_port, formatted, len, 0);

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
                                sessions[currentAllSessionNum].isdead = 1;
                                sessions[currentAllSessionNum].numberofppl = 1;
                                sessions[currentAllSessionNum].sessionID = currentAllSessionNum + 1; // sessionID starts from 1!
                                strcpy(sessions[currentAllSessionNum].sessionName, pkt->data);

                                for (int i = 0; i < MAX_PORT; i++) { // size of list = 6
                                    if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                                        strcpy(clientlist[i].sessionName, pkt->data);
										break;
                                    }
                              
                                }
                                currentAllSessionNum++;

								printf("c\n");

                                //send ACK                
                                strcpy(reply->data, pkt->data);
                                reply->size = strlen(reply->data);
                                strcpy(reply->source, pkt->source);
                                reply->type = NS_ACK;
                                formatted = formatPacket(reply);

                                // send package 
                                len = strlen(formatted);
                                bytes_sent = send(target_port, formatted, len, 0);

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
                                    if (strcmp(sessiontobeJoined, sessions[i].sessionName) == 0) {
                                        strcpy(clientlist[i].sessionName, pkt->data);
                                        strcpy(reply->data, pkt->data);
                                        reply->size = pkt->size;
                                        strcpy(reply->source, pkt->source);
                                        reply->type = JN_ACK;
                                        formatted = formatPacket(reply);
                                        found = 1;
                                        sessions[i].numberofppl++;
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
                                bytes_sent = send(target_port, formatted, len, 0);

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
                                    if (strcmp(pkt->data, sessions[i].sessionName) == 0) {
                                        sessions[i].numberofppl--;
                                        if (sessions[i].numberofppl == 0) {
                                            sessions[i].isdead = 0;
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
                                for (int i = 0; i < 6; i++) { // size of list = 6
                                    if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                                        clientlist[i].isloggedin = 0;
									    
                                        if (strcmp(clientlist[i].sessionName, "") != 0) {
                                            for (int j = 0; j < currentAllSessionNum; j++) {
                                                if (strcmp(clientlist[i].sessionName, sessions[j].sessionName) == 0) {
                                                    if (sessions[j].numberofppl == 1) {
                                                        sessions[j].numberofppl = 0;
                                                        sessions[j].isdead = 0;
                                                    } else sessions[j].numberofppl--;
                                                }
                                            }
                                        }
                                        strcpy(clientlist[i].sessionName, "");
                                        strcpy(clientlist[i].IPaddr, "");
                                        clientlist[i].portNum = -2;
                                    	break;
                                    }
                                }
         
                                close(target_port);
                                FD_CLR(target_port, &readfds);

								//select the new max_sd 
								if(max_sd == target_port) max_sd --;								
								//max_sd = main_port;

                                free(reply);
                                free(pkt);
                                break;
                            }
						       case INVITE:
                            {
                                int inviter = 0;
                                int invitedC = 0;


                                //find the session first 
                                for (int i = 0; i < MAX_PORT; i++) {
                                    if (strcmp(pkt->source, clientlist[i].name) == 0) {
                                        inviter = i;
                                        break;
                                    }
                                }
                                //check if invited valid client 
                                for (int i = 0; i < MAX_PORT; i++) {
                                    if (strcmp(pkt->data, clientlist[i].name) == 0) {
                                        found = 1;
                                        // NACK if client not logged in
                                        if (!(clientlist[i].isloggedin)) {
                                            char *notloggedin = "Error: Client is not online.\n";
                                            perror(notloggedin);

                                            reply->type = INV_NACK;
                                            strcpy(reply->data, notloggedin);
                                            strcpy(reply->source, pkt->source);
                                            reply->size = strlen(notloggedin);
                                            break;
                                        }// NACK if client already joined another session 
                                        else if (clientlist[i].sessionName[0] != '\0') {
                                            char *alreadyin = "Error: Client already joined another session.\n";
                                            perror(alreadyin);

                                            reply->type = INV_NACK;
                                            strcpy(reply->source, pkt->source);
                                            strcpy(reply->data, alreadyin);
                                            reply->size = strlen(alreadyin);
                                            break;
                                        }// ACK, need to forward the request to client 
                                        else {
                                            reply->type = INV;
                                            strcpy(reply->source, pkt->source);
                                            reply->size = 0;
                                            strcpy(reply->data, "");
                                            invitedC = clientlist[i].portNum;                                            
                                            break;
                                        }
                                        break;
                                    }
                                }
                                if (!found) {
                                    char* noClient = "Error: client not found.\n";
                                    perror(noClient);
                                    strcpy(reply->data, noClient);
                                    strcpy(reply->source, pkt->source);
                                    reply->size = strlen(noClient);
                                    reply->type = INV_NACK;
                                }// NACK
                                formatted = formatPacket(reply);
                                len = strlen(formatted);

                                // if NACK send back, else forward to client port
                                if (reply->type == INV_NACK) {
                                    bytes_sent = send(target_port, formatted, len, 0);
                                } else if (reply->type == INV_ACK) {
                                    bytes_sent = send(invitedC, formatted, len, 0);
                                }

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
                            case INV_ACK
                            {
                                // client is willing to join!
                                for (int i = 0; i < MAX_PORT; i++) {
                                    if (strcmp(pkt->source, clientlist[i].name) == 0) {
                                        strcpy(clientlist[i].sessionName, pkt->data);
                                        for (int j = 0; j < currentAllSessionNum; j++) {
                                            if (strcmp(clientlist[i].sessionName, sessions[j]->sessionName) == 0) {
                                                sessions[j]->numberofppl++;
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                }
                                printf("Client %s has successfully joined session %s", pkt->source, pkt->data);

                                free(reply);
                                free(pkt);
                                break;
                            };



                        }
                    }

                }
            }
        }
    }

    return 0;
}


