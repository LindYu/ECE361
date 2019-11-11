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

#define MAX_NAME 1000
#define MAX_DATA 1000

char* Type[] = {"LOGIN", "LO_ACK", "LO_NAK", "EXIT", "JOIN", "JN_ACK", "JN_NAK", "LEAVE_SESS", "NEW_SESS", "NS_ACK", "MESSAGE", "QUERY", "QU_ACK"};

typedef enum state {
    LOGIN, LO_ACK, LO_NAK, EXIT, JOIN, JN_ACK, JN_NAK, LEAVE_SESS, NEW_SESS, NS_ACK, MESSAGE, QUERY, QU_ACK, LAST
} State;


// also need their IP and port address

typedef struct client {
    char name[100];
    char password[100];
    bool isloggedin;
    int sessionNum;

} clientInfo;

struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

typedef struct session {
    int sessionID;
    int numberofppl;
    bool isdead;
} sessionInfo;

/*A hardcoded record of the client information*/
clientInfo clientlist[6] = {
    {"trezan", "trezannn", 0, -1},
    {"derekrocks12345", "derek12345", 0, -1},
    {"meow", "meowmeow", 0, -1},
    {"pandaman6477", "brown142756", 0, -1},
    {"yayforalex", "alex123", 0, -1},
    {"ranoutofideas", "imsad", 0, -1}
};



/*Array of sessions*/
/* The session is designed as follows: maximum 100 sessions, when a session dies 
   a tombstone is left for the session, all sessions freed at the end*/
sessionInfo *sessions = (sessionInfo*) malloc(100 * sizeof (sessionInfo));
int currentsessionNum = 0;

char* formatPacket(struct lab3message *mes) {
    char* formattedStr;

    asprintf(&formattedStr, "%d:%d:%s:%s", mes->type, mes->size, mes->source, mes->data);
    printf("FormattedStr: %s", formattedStr);
    return formattedStr;
}

struct lab3message* packet_demultiplexer(char message[]) {
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
int main(int argc, char** argv) {
    char command[1000];
 
   /* printf("Enter command>");
    scanf("%[^\n]s", command);
    unsigned int len = (unsigned) strlen(command);

    for (unsigned int i = 0; i < len; i++) {
        if (command[i] == ' ') command[i] = ':';
    }*/

    // call demultiplexer
    struct lab3message* pkt;

    pkt = packet_demultiplexer(command);

    char* formatted;
    bool found = 0;

    struct lab3message* reply = (struct lab3message*) malloc(sizeof (struct lab3message));

    switch (pkt->type) {
        case LOGIN:

            for (int i = 0; i < 6; i++) { // size of list = 6
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

                        formatted = formatPacket(reply);
                        printf("Successfully logined in: %s", formatted);
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

            free(reply);
            free(pkt);
            break;

        case QUERY:
            // also traverse the list of sessions and list of clients... need to send multiple since packet size is small?


            break;
        case NEW_SESS:
            sessions[currentsessionNum].isdead = 0;
            sessions[currentsessionNum].numberofppl = 1;
            sessions[currentsessionNum].sessionID = currentsessionNum + 1; // sessionID starts from 1!
            for (int i = 0; i < 6; i++) { // size of list = 6
                if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                    clientlist[i].sessionNum = currentsessionNum + 1;
                }
                break;
            }
            currentsessionNum++;

            //send ACK
            char* number;
            asprintf(&number, "%d", currentsessionNum);
            strcpy(reply->data, number);
            reply->size = strlen(number);
            strcpy(reply->source, pkt->source);
            reply->type = NS_ACK;
            formatted = formatPacket(reply);



            free(reply);
            free(pkt);
            break;
        case JOIN:
            int joinID = atoi(pkt->data);

            // if session doesn't exist, NACK
            if (joinID > currentsessionNum) {
                char* outofBound = ",Error: Session does not exist.\n";
                perror("Error: Session does not exist.\n");
                strcat(pkt->data, outofBound);
                strcpy(reply->data, pkt->data);
                reply->size = strlen(reply->data);
                strcpy(reply->source, pkt->source);
                reply->type = JN_NAK;
                formatted = formatPacket(reply);



            } else {
                for (int i = 0; i < 6; i++) { // size of list = 6
                    if (strcmp(pkt->source, clientlist[i].name) == 0) {
                        //Already joined a session, NACK 
                        if (clientlist[i].sessionNum != -1) {
                            char* outofBound = ",Error: Client already joined a session.\n";
                            perror("Error: Client already joined a session.\n");
                            strcat(pkt->data, outofBound);
                            strcpy(reply->data, pkt->data);
                            reply->size = strlen(reply->data);
                            strcpy(reply->source, pkt->source);
                            reply->type = JN_NAK;
                            formatted = formatPacket(reply);



                        } else { //ACK
                            clientlist[i].sessionNum = joinID;
                            strcpy(reply->data, pkt->data);
                            reply->size = pkt->size;
                            strcpy(reply->source, pkt->source);
                            reply->type = JN_ACK;
                            formatted = formatPacket(reply);


                        }
                        break;
                    }
                }

            }
            free(reply);
            free(pkt);
            break;
        case EXIT:
            for (int i = 0; i < 6; i++) { // size of list = 6
                if (strcmp(pkt->source, clientlist[i].name) == 0 && clientlist[i].isloggedin != 0) {
                    clientlist[i].isloggedin = 0;
                    if (clientlist[i].sessionNum != -1) {
                        if (sessions[clientlist[i].sessionNum].numberofppl == 1) {
                            // session dead
                            sessions[clientlist[i].sessionNum].numberofppl = 0;
                            session[clientlist[i].sessionNum].isdead = 1;
                        } else sessions[clientlist[i].sessionNum].numberofppl--;

                    }
                    clientlist[i].sessionNum = -1;
                }
                break;
            }
            free(reply);
            free(pkt);
            break;

    };


 

    return 0;
}


