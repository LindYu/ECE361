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
void packetmaker(FILE *file,char* fileName, int socketfd);
//char* formatter(struct packet* unformatted_pac);
int finddigs(int num);
struct timeval start,end;

struct packet {
	unsigned int total_frag;
	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};


int finddigs(int num)
{
    int n = 0;
    while(num) {
        num /= 10;
        n++;
    }
    return n;
}

/*
char* formatter(struct packet* unformatted_pac){
	unsigned int total_frag = unformatted_pac -> total_frag;
	unsigned int frag_no = unformatted_pac -> frag_no;
	unsigned int size = unformatted_pac -> size;
	char* filename = unformatted_pac -> filename;
	char filedata[1000] = unformatted_pac -> filedata;
	
	//find total size of formatted str with 100 being filename size, 1000 being data size & 4 being the semicolons
	int formatted_str_size = fiddigs(total_frag) + finddigs(frag_no) + finddigs(size)+ 100 + 1000 + 4;

	char formatted_str [formatted_str_size];
	int n = sprintf(formatted_str, "%d:%d:%d:%s:", total_frag, frag_no, size, filename);
	for (int x = 0; x < 1000; x++){
		formatted_str[n+x] = filedata[x];
	}

	return *formatted_str;
}
*/

void packetmaker(FILE *file, char* fileName, int sockfd){
	struct packet *new_pac = (struct packet*)malloc(sizeof(struct packet));

   // char *code;
  //  size_t n = 0;
    int c;

    fseek(file, 0, SEEK_END);
    long f_size = ftell(file);
	
	//basically saying total_pac is ceil (f_size/1000.0)
	float t_pac_l = f_size/1000.0;
	//printf("long pac %d", t_pac_l);
	unsigned int total_pac = (int) f_size/1000;
	if (total_pac != t_pac_l){
		total_pac++;	
	}
	
    fseek(file, 0, SEEK_SET);

   // code = malloc(f_size);

	unsigned int count = 0, pac_num= 1;

	//initialize first packet
	new_pac->total_frag = total_pac;
	new_pac->frag_no = 1;
	new_pac-> size = 0;
	new_pac -> filename = fileName;



    while ((c = fgetc(file)) != EOF) {

		//if counter >= 1000 send current pac & make a new packet
		if(count >= 1000){
			//send current packet (get rid of formatter fn as cannot return char array, only char ptr)
			int formatted_str_size = finddigs(new_pac -> total_frag) + finddigs(new_pac -> frag_no) + finddigs(new_pac -> size)+ 100 + 1000 + 4;

			char formatted_str [formatted_str_size];
			int n = sprintf(formatted_str, "%d:%d:%d:%s:", new_pac -> total_frag, new_pac -> frag_no, new_pac -> size, fileName);
			for (int x = 0; x < 1000; x++){
				formatted_str[n+x] = new_pac -> filedata[x];
			}
			
			//code to send to server, currently only prints
			//printf ("%s", formatted_str);
			int numbytes;
			char buf[256];
			

			//send first packet without receiving server ACK 
			send(sockfd, formatted_str,n+1000,0); 	
			
			if ((numbytes = recv(sockfd, buf, 255, 0)) == -1) {
		    	perror("recv");
				close(sockfd);
		    	exit(1);
			}
			
			buf[numbytes] = '\0';
			//printf("%d",atoi(buf));
		
			// if ACK is different, send the packet again 

			if(atoi(buf) != new_pac->frag_no)
				send(sockfd, formatted_str,strlen(formatted_str),0); 
			
			
			//delete current packet
			//free (new_pac);

			//create new packet
			new_pac = (struct packet*)malloc(sizeof(struct packet));
			new_pac->total_frag = total_pac;
		
			//next pac
			pac_num++;
			new_pac->frag_no = pac_num;
			new_pac-> size = 0;
			new_pac->filename = fileName;

			count=0; 
			new_pac-> filedata[count] = c;
			count ++;
			
		}
	
		// if counter < 1000, continue to populate data for current packet
		else{
			//struct packet *new_pac = (struct packet*)malloc(sizeof(struct packet));
			new_pac-> filedata[count] = c;
			new_pac-> size = count;
			count ++;
		}
	}

	//new_pac still exists, if its size is >0, send the final packet, otherwise just delete it
	if (new_pac -> size > 0){
		int formatted_str_size = finddigs(new_pac -> total_frag) + finddigs(new_pac -> frag_no) + finddigs(new_pac -> size)+ 100 + 1000 + 4;

		char formatted_str [formatted_str_size];
		int n = sprintf(formatted_str, "%d:%d:%d:%s:", new_pac -> total_frag, new_pac -> frag_no, new_pac -> size, fileName);
		for (int x = 0; x < new_pac->size; x++){
			formatted_str[n+x] = new_pac -> filedata[x];
		}
		
		int numbytes;
		char buf[256];
		send(sockfd, formatted_str,n+new_pac->size,0); 
		//code to send to server, currently only prints
		if ((numbytes = recv(sockfd, buf, 255, 0)) == -1) {
        	perror("recv");
			close(sockfd);
        	exit(1);
   		}

		buf[numbytes] = '\0';
		//only send packet when the ACK has been received 

		//printf("%d",atoi(buf));
		//printf(frag_no);
		if(atoi(buf) != new_pac->frag_no) send(sockfd, formatted_str,strlen(formatted_str),0); 
	
	}
		close(sockfd);
		
		// report time 
		gettimeofday(&end,NULL);
   		printf("Round-trip delay = %ld ms.\n",delay(start,end));

	//	printf ("%s", formatted_str);
	
	//delete final packet
	free (new_pac);	
	
}




int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[256];
    struct addrinfo hints, *servinfo, *p;
    int rv;
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
        // file exists
		packetmaker(file, filename, sockfd); 
		
      //  if (send (sockfd, "ftp", 3, 0) == -1) {
     //       perror("send");
       // } 
    } else {
		close(sockfd);
		exit(1);
    }

/*
    if ((numbytes = recv(sockfd, buf, 255, 0)) == -1) {
        perror("recv");
	close(sockfd);
        exit(1);
    }

    buf[numbytes] = '\0';

    gettimeofday(&end,NULL);
    printf("Round-trip delay = %ld ms.\n",delay(start,end));

    if (strcmp(buf, "yes") == 0) {
        printf("A file transfer can start.\n");
	 	packetmaker(file ,filename);
    }
*/

  //  close(sockfd);
}

long delay (struct timeval t1, struct timeval t2)
{
	long d;
	
	d = (t2.tv_sec - t1.tv_sec) * 1000;
	//printf ("%ld",d);
	d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
	//printf ("%ld",d);
	return (d); 
}
