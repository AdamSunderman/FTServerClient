/*
   ftserver.c - ftserver is a TCP file transfer program that operates in conjuncture with ftclient.py.
                ftserver will wait on <CONTROL_PORT_NUM> specified at launch for incoming connections from
                ftclient. When a connection is made ftserver will parse the incoming command and fulfill
                the request if possible. The options allowed are <-g> <FILENAME> to get a file and <-l> 
                to list ftserver's directory contents. ftserver will continue to listen on <CONTROL_PORT_NUM>
                until the SIG_INT signal is sent (cntl^c).

        usage - ./ftserver <CONTROL_PORT_NUM>
      example - ./ftserver 30020
*/

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
#include <signal.h>

//----------------------------------------------------------------------
//         Define boolean values (not needed, just a habit).
//----------------------------------------------------------------------
typedef enum{FALSE, TRUE}bool;

//----------------------------------------------------------------------
//                Connection Return Data (CRD) struct. 
//----------------------------------------------------------------------
// members: struct addrinfo * ai -> connection addrinfo struct
//          int sfd -> connection socket file descriptor
//----------------------------------------------------------------------
//Holds the needed socket info after a call to initializeConnection(), 
//which is defined below.
//----------------------------------------------------------------------
typedef struct ConnectionReturnData{
    struct addrinfo * ai;
    int sfd;
}CRD;

//----------------------------------------------------------------------
//                      void validateArgs(int ac)
//----------------------------------------------------------------------
// params: int ac -> expects argc 
//         const char* av -> expects argv
//----------------------------------------------------------------------
//Validates that there are the correct number of agruments to run
//ftserver [arg[0]=ftserver, arg[1]=<CONTROL_PORT_NUM>]. Also validates 
//that <CONTROL_PORT_NUM> in the range {1025,...,65534} inclusive. 
//----------------------------------------------------------------------
void validateArgs(int ac, char const* av){

    if(ac != 2){
        fprintf(stderr, "Need two arguments(%d)\n", ac);
        exit(1);
    }

    int v = stoi(av[1]);
    if( v <= 1024 || v >= 65535){
        fprintf(stderr, "Need port number in range {1025,...,65534} (%d)\n", v);
        exit(1);
    }
}

//-------------------------------------------------------------------------------
//       CRD initializeConnection(char* ip, char* port, bool isListener)
//-------------------------------------------------------------------------------
//Params:  char* ip -> expects pointer to string with <HOST_NAME> 
//         char* port -> expects pointer to string with <CONTROL_PORT_NUM>
//         bool isListener -> True, if this connection is the control connection
//                            False, if this is the data connection
//-------------------------------------------------------------------------------
//Creates a socket then connects or binds depending on the value of isListener. 
//Will exit the program upon any connect(), bind() or getaddrinfo() errors.
//-------------------------------------------------------------------------------
//Returns: a CRD struct with the new connections information.
//-------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------
//                          void buildDir()
//-------------------------------------------------------------------------------
//Builds a list of the directory contents and saves it to 'DIR.txt'. This file 
//will be created and sent when ftclient requests the current directory (-l).
//Only lists files no directories.  
//-------------------------------------------------------------------------------
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
		while((ent = readdir(dir))){
			char e = ent->d_name[0]; 
            //Only print file names
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

//-------------------------------------------------------------------------------
//         void mSendFile(char* ip, char* port, char* file, int sockfd)
//-------------------------------------------------------------------------------
// params: char* ip -> expects pointer to string with <HOST_NAME> 
//         char* port -> expects pointer to string with <CONTROL_PORT_NUM>
//         char* file -> expects pointer to string with <FILENAME>
//         int sockfd -> expects socket file descriptor for control connection
//-------------------------------------------------------------------------------
//Creates a new data connection then sends the requested file if it is present. If 
//the file doesn't exist then a message is sent to ftclient on the control 
//connection. And the send iss aborted. Files sent can include 'DIR.txt' if the 
//command from ftclient was (-l) list.
//-------------------------------------------------------------------------------
void mSendFile(char* ip, char* port, char* file, int sockfd){
    //sleep to allow ftclient to connect
    sleep(5);
    CRD dataconnection = initializeConnection(ip, port, FALSE);
    FILE *fp;
    //open the file
    int temp = open(file, O_RDONLY);
    if(temp <= 0){
        exit(1);
    }
    fp = fdopen(temp, "rb");
    //if the file doesn't exist
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
        //get file length and prepare to read/write until file sent
        fseek(fp, 0, SEEK_END);
        size_t fpsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* buffer = malloc(fpsize + 1);
        size_t read, wrote, total = 0;
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

//-------------------------------------------------------------------------------
//                  void requestHandler(int sockfd, char* ip)
//-------------------------------------------------------------------------------
// params: int sockfd -> expects socket file descriptor for control connection
//         char* ip -> expects pointer to string with <HOST_NAME> 
//-------------------------------------------------------------------------------
//Parses and validates new incoming requests on the control connection. If the
//request is good a verification message is sent to ftclient and the request 
//contents are sent to mSendFile(). If the request is bad an error message is
//sent and ftserver will go back to waiting for requests on <CONTROL_PORT_NUM>. 
//-------------------------------------------------------------------------------
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
    //if there is a filename get it, else set filename to 'NA'.
    if(token == NULL){
        memcpy(file, "NA", strlen("NA"));
        file[strlen("NA")] = '\0';
    }
    else{
        memcpy(file, token, strlen(token));
        file[strlen(token)] = '\0';
    }
    //If the command is (-l) list, build the directory file and 
    //send the request confirmation. Then set the filename 
    //to 'DIR.txt' and send the directory file to ftclient. 
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
    //If the command is (-g) get, send the request confirmation
    //then send the requested file. 
    else if(strcmp(cmd, "-g") == 0){
        write(sockfd, goodReq , strlen(goodReq));
        printf("File '%s' requested on port %s\n", file, port);
        mSendFile(ip, p, f, sockfd);
        printf("File '%s' sent to %s:%s\n", file, ip, port);
    }
    //If the command was bad send the bad request message
    else{
        write(sockfd, badReq , strlen(badReq));
    }

    free(buffer);
}

//-------------------------------------------------------------------------------
//       void listenWait(int sockfd)
//-------------------------------------------------------------------------------
// params: int sockfd -> expects socket file descriptor for control connection
//-------------------------------------------------------------------------------
//Main listener loop. ftserver waits on <CONTROL_PORT_NUM> for incoming connections
//from ftclient. When a new connection is established ftclient's <hostname> is 
//extracted and sent to requestHandler() to complete the request. If a SIG_INT is
//sent it will be caught by accept(). When this happens the loop is broken and 
//all files and sockets will be closed before ftserver terminates.
//-------------------------------------------------------------------------------
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
        if(errno == EINTR){
        	break;
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

int main(int argc, char const *argv[]){
    //validate args
    validateArgs(argc, argv);
    //setup control port
    CRD listenConnection = initializeConnection(NULL, (char*) argv[1], TRUE);
    //wait for ftclient
    listenWait(listenConnection.sfd);
    //clean up
    freeaddrinfo(listenConnection.ai);
    close(listenConnection.sfd);
    return 0;
}









