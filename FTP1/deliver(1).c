#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
/*
 * deliver serveraddr serverportnumber
 * ask the user to input a message follows the format ftp filename
 * check the existence of the file
 * if exist, send a message "ftp" to server
 * else exit
 * receive a message from server
 * if yes, print out "A file transfer can start."
 * else exit
 */


long delay (struct timeval t1, struct timeval t2);
void packetmaker(FILE *file,char* fileName,int* num_of_pac);
char* formatter(struct packet* unformatted_pac);

char* formatter(struct packet* unformatted_pac){
	unsigned int frag_no = unformatted_pac -> frag_no;
	unsigned int size = unformatted_pac -> size;
	char* filename = unformatted_pac -> filename;
	char* filedata = unformatted_pac -> filedata;
	
 //	const char* firstformat= strcat(formatted_str); 
//	char formatted_str = unformatted_pac -> formatted_str;
	unsigned int total_frag = unformatted_pac -> total_frag;
 	
	char *str;
	asprintf(&str,"%d:%d:%d:%s:%s",total_frag,frag_no,size,filename,filedata);
	
	return str;
}

char** packetmaker(FILE *file, char* fileName, int sockfd, int* num_of_pac){
	// stores an array of strings 
	char** all_msgs;
	struct packet *new_pac = (struct packet*)malloc(sizeof(struct packet));
    char *code;
    size_t n = 0;
    int c;

    fseek(file, 0, SEEK_END);
    long f_size = ftell(file);
	unsigned int total_pac = ceil(f_size/1000.0);
	*num_of_pac = total_pac
	
    fseek(file, 0, SEEK_SET);

    code = malloc(f_size);


	unsigned int count = 0, pac_num= 1;

    while ((c = fgetc(file)) != EOF) {

	//if counter > 1000 make multiple packets
	if(count >= 1000){
		// format and store the newest package
		all_msgs[(new_pac-> frag_no)-1] = formatter(new_pac);
		
		//construct new packet
		new_pac = (struct packet*)malloc(sizeof(struct packet));
		new_pac->total_frag = total_pac;
		pac_num++;
		new_pac->frag_no = pac_num;
		
		new_pac->filename = fileName;
		count=0; 
		new_pac-> filedata[count] = c;

		char *message = formatter(new_pac);

		if (send (sockfd, message, strlen(message)+1, 0) == -1) {
            perror("send");
			return;
        }
	}
	
	// if counter <= 1000
	else{
		//struct packet *new_pac = (struct packet*)malloc(sizeof(struct packet));
		new_pac->filename = file; 
		new_pac-> filedata[count] = c;
		count ++;
		new_pac->frag_no = frag; 
	}
	}
	
	if (count!=0) {
		new_pac = (struct packet*)malloc(sizeof(struct packet));
		new_pac->total_frag = total_pac;
		pac_num++;
		new_pac->frag_no = pac_num;
		
		new_pac->filename = fileName;

		char *message = formatter(new_pac);

		if (send (sockfd, message, strlen(message)+1, 0) == -1) {
            perror("send");
			return;
        }
	}

	return all_msgs; 

}


struct packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
}


int main(int argc, char *argv[]) {
	char** msgs; 
    int sockfd, numbytes;
    char buf[256];
    struct addrinfo hints, *servinfo, *p;
    int rv;
	int* num_of_pac;
    struct timeval start,end;

    if (argc != 3) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset (&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //printf("Got address info\n");

    // connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

      //  printf("socket\n");

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        //printf("connect\n");

        break;
    }

    //printf("connected");

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    
    // input
    char ftp[100], filename[100];
    printf("> ");
    scanf("%s%s", ftp, filename);
    while(strcmp(ftp, "ftp") != 0) {
        printf("Invalid input.\n> ");
        scanf("%s%s", ftp, filename);
    }

    gettimeofday(&start,NULL);

    // check if file exists
    FILE *file;
    file = fopen(filename, "r");
    if (file) {
        // file exists, make into an array of strings 
        msgs = packetmaker(file, filename, sockfd);
    } else {
	close(sockfd);
	exit(1);
    }

    buf[numbytes] = '\0';

    gettimeofday(&end,NULL);
    printf("Round-trip delay = %ld ms.\n",delay(start,end));

    if (strcmp(buf, "yes") == 0) {
        printf("A file transfer can start.\n");
    }
	
	/*Start transferring*/
	
	//first packet
	if (numbytes = send(sockfd, msgs[0],sizeof(msgs[0]),0)){
		perror("recv");
		close(sockfd);
        exit(1);
    }
	
	for(int i = 1; i <= &num_of_pac;i++){
	
	if ((numbytes = recv(sockfd, buf, 255, 0)) == -1) {
        perror("recv");
		close(sockfd);
        exit(1);
    }

	if(strcmp(buf,i) == 0){
		send(sockfd, msgs[i],1499,0); 
		}
	}

    close(sockfd);
}

long delay (struct timeval t1, struct timeval t2)
{
	long d;
	
	d = (t2.tv_sec - t1.tv_sec) * 1000;
	printf ("%ld",d);
	d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
	printf ("%ld",d);
	return (d); 
}
