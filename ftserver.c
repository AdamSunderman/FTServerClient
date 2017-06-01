#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

typedef enum{FALSE, TRUE}bool;

typedef struct ConnectionReturnData{
    struct addrinfo * ai;
    int sfd;
}CRD;

void validateArgs(int ac){
    if(ac != 2){
        fprintf(stderr, "Need two arguments(%d)\n", ac);
        exit(1);
    }
}

CRD initializeConnection(char* ip, char* port, bool isListener){
    CRD conReturn;
    struct addrinfo hints;
    int status;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //Setup addrinfo struct. If ip is null then this is the servers listener connection
    //so leave out the ip. Else, pass the ip to getaddrinfo().
    if(ip == NULL){
        hints.ai_flags = AI_PASSIVE;
        if((status = getaddrinfo(NULL, port, &hints, &conReturn.ai)) != 0){
            fprintf(stderr, "Getaddrinfo Error(%s)\n", gai_strerror(status));
            exit(1);
        }
    }
    else{
        if((status = getaddrinfo(ip, port, &hints, &conReturn.ai)) != 0){
            fprintf(stderr, "Getaddrinfo Error(%s)\n", gai_strerror(status));
            exit(1);
        }
    }

    //Setup the socket
    if((conReturn.sfd = socket(((struct addrinfo *)conReturn.ai)->ai_family, conReturn.ai->ai_socktype, conReturn.ai->ai_protocol)) == -1){
        fprintf(stderr, "Socket Error(%s)\n", strerror(errno));
        exit(1);
    }

    //If this is the servers listener connection then bind and listen,
    //Else, just connect to prepare for transfers.
    if(isListener == TRUE){
        if((status = bind(conReturn.sfd, conReturn.ai->ai_addr, conReturn.ai->ai_addrlen)) == -1){
            close(conReturn.sfd);
            fprintf(stderr, "Bind Error(%s)\n", strerror(errno));
            exit(1);
        }
        if(listen(conReturn.sfd, 5) == -1){
            close(conReturn.sfd);
            fprintf(stderr, "Listen Error(%s)\n", strerror(errno));
            exit(1);
        }
        printf("Server open on %s\n", port);
    }
    else{
        if((status = connect(conReturn.sfd, conReturn.ai->ai_addr, conReturn.ai->ai_addrlen)) == -1){
            close(conReturn.sfd);
            fprintf(stderr, "Connect Error(%s)\n", strerror(errno));
            exit(1);
        }
    }

    return conReturn;
}

void buildDir(){
	DIR *dir;
	struct dirent *ent;
	dir = opendir("./");
	FILE* text = fopen("DIR.txt", "w");
	if(text == NULL){
		printf("Couldn't open DIR.txt for write.\n");
		exit(1);
	}
	if(dir != NULL){
		while(ent = readdir(dir)){
			char e = ent->d_name[0]; 
			if( e == '.' || e == '_'){
				continue;
			}
			else{
				fprintf(text, "%s\n", ent->d_name);	
			}
		}
		(void) closedir(dir);
	}
	else{
		perror("Couldn't open DIR for read");
	}
	fclose(text);
}

void mSendFile(char* ip, char* port, char* file, int sockfd){
    sleep(5);
    CRD dataconnection = initializeConnection(ip, port, FALSE);
    FILE *fp;
    
    int temp = open(file, O_RDONLY);
    if(temp <= 0){
        exit(1);
    }

    fp = fdopen(temp, "rb");
    if(!fp){
        printf("Error %d\n", errno);
        fflush(stdout);
        char* mesb= "FILE-NOT-FOUND\0";
        write(sockfd, mesb, strlen(mesb));
        fflush(stdout);
        close(dataconnection.sfd);
        freeaddrinfo(dataconnection.ai);
        exit(1);
    }
    else{
        char* mesg= "FILE-FOUND\0";
        write(sockfd, mesg, strlen(mesg));
        fseek(fp, 0, SEEK_END);
        size_t fpsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* buffer = malloc(fpsize + 1);
        size_t read, wrote, total =0;
        while(!feof(fp)){
            read = fread(buffer, 1, 1024, fp);
            buffer[read] = '\0';
            wrote = write(dataconnection.sfd, buffer, read);
            while(wrote < read){
                wrote += write(dataconnection.sfd, buffer+wrote, read-wrote);
            }
            total += wrote;
        }
        sleep(3);
        free(buffer);
        fclose(fp);
        close(dataconnection.sfd);
        freeaddrinfo(dataconnection.ai);
    }
}

void requestHandler(int sockfd, char* ip){
    char* buffer = malloc(1025);
    char port[64] = {'\0'};
    char *p = port;
    char cmd[4] = {'\0'};
    char *c = cmd;
    char file[64] = {'\0'};
    char *f = file;
    char* token;
    char* goodReq = "ACCEPTED\0";
    char* badReq = "REJECTED\0";
    ssize_t recieved = 0;

    //Get the command and break it into sections {portnumber, command, filename}. If the
    //command is good send a message back indicating that the data connection will proceed.
    recieved = read(sockfd, buffer, 1024);
    buffer[recieved] = '\0';
    if(recieved <= 0){
        exit(1);
    }

    token = strtok(buffer, " \0\n");
    memcpy(port, token, strlen(token));
    port[strlen(token)] = '\0';
    token = strtok(NULL, " \0\n");
    memcpy(cmd, token, strlen(token));
    cmd[strlen(token)] = '\0';
    token = strtok(NULL, " \0\n");
    if(token == NULL){
        memcpy(file, "NA", strlen("NA"));
        file[strlen("NA")] = '\0';
    }
    else{
        memcpy(file, token, strlen(token));
        file[strlen(token)] = '\0';
    }
    if(strcmp(cmd, "-l") == 0){
        buildDir();
        write(sockfd, goodReq , strlen(goodReq));
        sleep(1);
        printf("List directory requested on port %s\n", port);
        memcpy(file, "DIR.txt", strlen("DIR.txt"));
        file[strlen("DIR.txt")] = '\0';
        mSendFile(ip, p, f, sockfd);
        printf("Directory contents sent to %s:%s\n", ip, port);
    }
    else if(strcmp(cmd, "-g") == 0){
        write(sockfd, goodReq , strlen(goodReq));
        printf("File '%s' requested on port %s\n", file, port);
        mSendFile(ip, p, f, sockfd);
        printf("File '%s' sent to %s:%s\n", file, ip, port);
    }
    else{
        write(sockfd, badReq , strlen(badReq));
    }

    free(buffer);
}

void listenWait(int sockfd){
    struct sockaddr_in callerAddr;
    socklen_t addrSize;
    int fd, res;
    char * ip = (char*) malloc(128);

    while(1){
        addrSize = sizeof(callerAddr);
        fd = accept(sockfd, (struct sockaddr *)&callerAddr, &addrSize);
        if(fd == -1){
            continue;
        }
        else{
            res = getpeername(fd, (struct sockaddr *)&callerAddr, &addrSize);
            strcpy(ip, inet_ntoa(callerAddr.sin_addr));
            printf("Connection from %s\n", ip);
            requestHandler(fd, ip);
            close(fd);
        }

    }
    free(ip);
}

int main(int argc, char const *argv[])
{
    validateArgs(argc);
    CRD listenConnection = initializeConnection(NULL, (char*) argv[1], TRUE);
    listenWait(listenConnection.sfd);
    freeaddrinfo(listenConnection.ai);
    return 0;
}









