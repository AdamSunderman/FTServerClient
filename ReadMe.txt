                                              FTSERVER and FTCLIENT 
                                        Written by Adam Sunderman for CS-372 
                                                  June 4, 2017


                       Included in the submission zip file are some various test files to validate 
                      that ftserver and ftclient can handle various file types. All of these files 
                      are named 'test' but with various extentions. There are the following files:
                                   
                               'test.jpeg' : a simple test image
                                'test.pdf' : a simple formatted document 
                                'text.txt' : a simple text file with several lines of text 
                                'test.exe' : a binary executable written in c that simply prints "Hello World" 
                                'test.c'   : test.exe's source code
                                'test.mp4' : a mid-sized video file
                                'test.zip' : a compressed file that contains all (6) files above 

                                                  ** Important **
                      ** Tested on flip1, flip2, flip3 and localhost using various port numbers **
                     ** FTClient will rename recieved files if in the same directory as FTServer **
                    ** Please read below for build and run instructions plus other important info. **


----------------------------------------------------------------------------------------------------------------------------
-----------------------------------------------    FTSERVER.C    -----------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------
      Sources - Beej's Guide To Network Programming.         http://beej.us/guide/bgnet/ 
                Help with printing directories.              http://www.sanfoundry.com/c-program-list-files-directory/
                Various functon and error code lookups       http://man7.org/
----------------------------------------------------------------------------------------------------------------------------
 Extra Credit - ./ftserver can send any file type. Such as: TXT, PDF, JPEG, MP4, ZIP, etc... 
----------------------------------------------------------------------------------------------------------------------------
   ftserver.c - ftserver is a TCP file transfer program that operates in conjuncture with ftclient.py.
                ftserver will wait on <CONTROL_PORT_NUM> specified at launch for incoming connections from
                ftclient. When a connection is made ftserver will parse the incoming command and fulfill
                the request if possible. The options allowed are <-g> <FILENAME> to get a file and <-l> 
                to list ftserver's directory contents. ftserver will continue to listen on <CONTROL_PORT_NUM>
                until the SIG_INT signal is sent (cntl^c).
----------------------------------------------------------------------------------------------------------------------------
     to build - gcc -o ftserver ftserver.c
        usage - ./ftserver <CONTROL_PORT_NUM>   ** <CONTROL_PORT_NUM> must be in range {1024,...,65535}, inclusive **
      example - ./ftserver 30020

----------------------------------------------------------------------------------------------------------------------------
 <CONTROL_PORT_NUM>: Control port number to connect to ftserver. Should be the port number
                     that ftserver is currently recieving connections on. Must be in range
                     {1024,...,65535}, inclusive.
----------------------------------------------------------------------------------------------------------------------------




----------------------------------------------------------------------------------------------------------------------------
-----------------------------------------------    FTCLIENT.C    -----------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------
     Sources - Beej's Guide To Network Programming.         http://beej.us/guide/bgnet/ 
               Python sockets reference.                    https://docs.python.org/2/library/socket.html
               Python class/object reference                https://docs.python.org/2/reference/datamodel.html
----------------------------------------------------------------------------------------------------------------------------
Extra Credit - ./ftclient can recieve any file type. Such as: TXT, PDF, JPEG, MP4, ZIP, etc... 
----------------------------------------------------------------------------------------------------------------------------
            File Transfer Client class (FTClient)     Written By: Adam Sunderman                        
----------------------------------------------------------------------------------------------------------------------------
 ftclient.py: ftclient.py is a TCP file transfer program that operates in conjuncture with 
              ftserver.c. ftserver waits on <CONTROL_PORT_NUM> specified at it's launch 
              for incoming connections from ftclient. When a connection is made ftserver 
              will parse ftclient's incoming command and fulfill the request if possible. 
              The options allowed are <-g> <FILENAME> to get a file from ftserver and <-l> 
              to list ftserver's directory contents. If a command entered is invalid ftclient
              will display a message and close. If a filename entered is invalid ftserver 
              will send an error message and ftclient will close. If a command is valid 
              ftclient will execute the command and then close diplaying status messages
              throughout execution. If the command was -l, list the directory of ftserver,
              ftserver will send a file 'DIR.txt' to ftclient. ftclient will print out the 
              files contents then delete the file before closing as this is meant to be a 
              temp file. If the command was -g, get file from ftserver, ftclient will save 
              the requested file in the current working directory and close, diplaying status 
              messages during execution.              
----------------------------------------------------------------------------------------------------------------------------
 attributes: FTClient.clientsocket - control connection socket file descriptor 
             FTClient.datasocket - data connection socket file descriptor        
             FTClient.serverport - control connection port number
             FTClient.dataport - data connection port number 
             FTClient.serverhost - ftservers hostname
             FTClient.command - command to send 
             FTClient.filename - file to request ('NA' if command is -l list)
----------------------------------------------------------------------------------------------------------------------------
 functions:  FTClient.__init__(self)
             FTClient.__del__(self)
             FTClient.isValid(self)
             FTClient.changeFileName(self)
             FTClient.connect(self, isDataSocket)
             FTClient.sendCommand(self)
             FTClient.recieveFile(self)
        ** see member function decriptions below **
----------------------------------------------------------------------------------------------------------------------------
  ** May need to run the command 'chmod +x ftclient.py' to get executable permissions before running **

 usage:  ./ftclient.py <HOST_NAME> <CONTROL_PORT_NUM> <COMMAND> <FILENAME> <DATA_PORT_NUM>

 example(list): ./ftclient.py flip1 30020 -l 30021

 example(get):  ./ftclient.py flip1 30020 -g file.pdf 30021

----------------------------------------------------------------------------------------------------------------------------

        <HOST_NAME>: Host name of ftserver. Should be a fully qualified domain name, 
                     localhost or ip.
 
 <CONTROL_PORT_NUM>: Control port number to connect to ftserver. Should be the port number
                     that ftserver is currently recieving connections on. Must be in range
                     {1024,...,65535}, inclusive.

          <COMMAND>: The command to send to ftserver. Must be either -l (list) or -g (get).
                     When the command is -l the parameter <FILENAME> should be excluded. When
                     the command is -g the parameter <FILENAME> must be included. 

         <FILENAME>: The name of the file for ftserver to send on a -g (get) request. Should
                     be a full file name including extention but without path information. No
                     quotations.
 
    <DATA_PORT_NUM>: Data port number for ftserver to send the request back to. Must be in 
                     range {1024,...,65535}, inclusive.
----------------------------------------------------------------------------------------------------------------------------


