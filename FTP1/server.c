#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BACKLOG 10

struct packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

int main(int argc, char *argv[]) {
    int status;
    struct addrinfo hints, *res;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int sockfd, new_fd, nbytes;

    char buf[1500];
	char * bufpt;

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
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(status));

        return 2;
    }

    //printf("Got address info\n");

    // make a socket, bind and listen
    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &res, sizeof(int)) == -1) {
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
    
    //printf("Binded\n");

/*    // output server address
    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, p->ai_addr, ipstr, sizeof ipstr);
    printf("server address: %s\n", ipstr);
 */

    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
       perror("listen");
       close(sockfd);
       exit(1);
    }

  //   printf("listening\n");

    // accept incoming connection
    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

    if (new_fd == -1) {
	close(sockfd);
        perror("accept");
        // continue;
        exit(1);
    }

//    printf("accepted\n");

	struct packet pkt;
	char* data;

	FILE* file;
	while(1){
    	// receive data from client
    	if ((nbytes = recv(new_fd, buf, sizeof(buf), 0))<=0) {
        	// error
        	if (nbytes == 0) {
            	printf("server: socket closed\n");
        	} else {
           	 	perror("recv");
      	  	}	
        	close(sockfd);
			close(new_fd);
			exit(1);
    	}

    	//  printf("received\n");
		
    	char *msg;
    	buf[nbytes] = '\0';
		bufpt = buf;
    
		/*if (strcmp(buf, "ftp") == 0) {
        	msg = "yes";
    	} else {
			msg = "no";
		}*/

		int cnt = 0;
		int cnt_field = 0;
		char field[50];
		int field_val[3];
		char filename[100];
		while (cnt < 4) {
			if (bufpt[0] == ':') {
				field[cnt_field] = '\0';
				if (field[0] >= '0' && field[0] <= '9') {
					field_val[cnt] = atoi(field);
				} else {
					for (int i = 0; field[i] != '\0'; i++) {
						filename[i] = field[i];
						if (field[i+1]=='\0') filename[i+1] = '\0';
					}
				}
				//printf("%d\n",field_val[cnt]);
				cnt_field = 0;
				cnt++;
			} else {
				field[cnt_field] = bufpt[0];
				cnt_field++;
			}
			// increment bufpt means bufpt[0] starts at original bufpt[1]
			bufpt = bufpt+1;
			nbytes--;
		}

		pkt.total_frag = field_val[0];
		pkt.frag_no = field_val[1];
		pkt.size = field_val[2];
		pkt.filename = filename;

		for (int i = 0; bufpt[i] != '\0'; i++) {
			pkt.filedata[i] = bufpt[i];
			if (bufpt[i+1]=='\0') pkt.filedata[i+1] = '\0';
		}
		
		if (pkt.frag_no == 1) {
			file = freopen(filename, "w", stdout);
		}

		// print remaining string to file
		//printf("%s", bufpt);
		for (int i = 0; i < nbytes; i++) {
			printf("%c", bufpt[i]);
		}
	
    	// send reply
		asprintf(&msg, "%d", pkt.frag_no);
		//printf("ACK%d\n",pkt.frag_no);


    	int len = strlen(msg);
    	int bytes_sent = send(new_fd, msg, len, 0);

    	// error checking
    	if (bytes_sent == -1) {
        	perror("send");
    	} else if (bytes_sent != len) {
        	printf("not completely sent\n");
    	}

		if (pkt.total_frag == pkt.frag_no) break;
	}
    close(sockfd);
    close(new_fd);
	fclose(file);

    return 0;
}
