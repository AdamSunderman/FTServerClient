#!/usr/bin/python
import socket
import sys 
import errno
import os

#-------------------------------------------------------------------------------------------
#            File Transfer Client class (FTClient)     Written By: Adam Sunderman                        
#-------------------------------------------------------------------------------------------
# ftclient.py: ftclient.py is a TCP file transfer program that operates in conjuncture with 
#              ftserver.c. ftserver waits on <CONTROL_PORT_NUM> specified at it's launch 
#              for incoming connections from ftclient. When a connection is made ftserver 
#              will parse ftclient's incoming command and fulfill the request if possible. 
#              The options allowed are <-g> <FILENAME> to get a file from ftserver and <-l> 
#              to list ftserver's directory contents. If a command entered is invalid ftclient
#              will display a message and close. If a filename entered is invalid ftserver 
#              will send an error message and ftclient will close. If a command is valid 
#              ftclient will execute the command and then close diplaying status messages
#              throughout execution. If the command was -l, list the directory of ftserver,
#              ftserver will send a file 'DIR.txt' to ftclient. ftclient will print out the 
#              files contents then delete the file before closing as this is meant to be a 
#              temp file. If the command was -g, get file from ftserver, ftclient will save 
#              the requested file in the current working directory and close, diplaying status 
#              messages during execution.              
#-------------------------------------------------------------------------------------------
# attributes: FTClient.clientsocket - control connection socket file descriptor 
#             FTClient.datasocket - data connection socket file descriptor        
#             FTClient.serverport - control connection port number
#             FTClient.dataport - data connection port number 
#			  FTClient.serverhost - ftservers hostname
#             FTClient.command - command to send 
#             FTClient.filename - file to request ('NA' if command is -l list)
#-------------------------------------------------------------------------------------------
# functions:  FTClient.__init__(self)
#             FTClient.__del__(self)
#             FTClient.isValid(self)
#             FTClient.changeFileName(self)
#             FTClient.connect(self, isDataSocket)
#             FTClient.sendCommand(self)
#             FTClient.recieveFile(self)
#        ** see member function decriptions below **
#-------------------------------------------------------------------------------------------
#
# usage:  ./ftclient.py <HOST_NAME> <CONTROL_PORT_NUM> <COMMAND> <FILENAME> <DATA_PORT_NUM>
#
# example(list): ./ftclient.py flip1 30020 -l 30021
#
# example(get):  ./ftclient.py flip1 30020 -g file.pdf 30021
#
#  ** May need to run the command 'chmod +x ftclient.py' to get executable permissions **
#-------------------------------------------------------------------------------------------
#
#        <HOST_NAME>: Host name of ftserver. Should be a fully qualified domain name, 
#                     localhost or ip.
# 
# <CONTROL_PORT_NUM>: Control port number to connect to ftserver. Should be the port number
#                     that ftserver is currently recieving connections on. Must be in range
#                     {1024,...,65535}, inclusive.
#
#          <COMMAND>: The command to send to ftserver. Must be either -l (list) or -g (get).
#                     When the command is -l the parameter <FILENAME> should be excluded. When
#                     the command is -g the parameter <FILENAME> must be included. 
#
#         <FILENAME>: The name of the file for ftserver to send on a -g (get) request. Should
#                     be a full file name including extention but without path information. No
#                     quotations.
# 
#    <DATA_PORT_NUM>: Data port number for ftserver to send the request back to. Must be in 
#                     range {1024,...,65535}, inclusive.
#-------------------------------------------------------------------------------------------
class FTClient(object):

	#===============================FTClient.__init__()===========================
	# Creates a new FTClient object with the command arguments
	# provided at launch
	#=============================================================================
	def __init__(self):
		self.clientsocket = "NONE"
		self.datasocket = "NONE"
		self.serverhost = sys.argv[1]
		self.serverport = sys.argv[2]
		self.command = sys.argv[3]
		if len(sys.argv) == 6:
			self.filename = sys.argv[4]
			self.dataport = sys.argv[5]
		elif len(sys.argv) == 5:
			self.filename = "NA"
			self.dataport = sys.argv[4]
		else:
			print "Invalid number of arguments"
			exit(1)

	#===============================FTClient.__del__()===========================
	# Deletes an existing FTClient object while closing sockets.
	# Used on exit.
	#============================================================================
	def __del__(self):
		if self.clientsocket != "NONE":
			print "Closing clientsocket..."
			self.clientsocket.close()
		if self.datasocket != "NONE":
			print "Closing datasocket..."
			self.datasocket.close()

	#===============================FTClient.isValid()===========================
	# validates FTClient's port numbers and commands from launch
	#============================================================================
	def isValid(self):
		sp = int(self.serverport)
		if sp >= 65535 or sp <= 1024:
			print os.strerror(errno.EINVAL) + "(serverport)"
			exit(1)
		cmd = self.command
		if cmd != "-l" and cmd != "-g":
			print os.strerror(errno.EINVAL) + "(command)"
			exit(1)
		dp = int(self.dataport)
		if dp >= 65535 or dp <= 1024:
			print os.strerror(errno.EINVAL) + "(dataport)"
			exit(1)

	#===============================FTClient.changeFileName()===========================
	# Changes the filename for the file recieved from ftserver if needed.
	# Appends (1),(2),etc...  Ex. 'file.txt' --> 'file(1).txt' 
	#=================================================================================== 
	def changeFileName(self):
		count = 1;
		fname = self.filename
		while(os.path.isfile(fname)):
			num = "(" + str(count) + ")."
			if fname.find("("):
				s1, s2 = fname.split(".")
				s3 = s1.split("(")
				newname = str(str(s3[0]) + num + s2)
			else:
				newname = fname.replace(".", num)
			self.filename = newname
			count = count + 1
			fname = self.filename

	#===============================FTClient.connect()==================================
	# Handles the connection of sockets. If isDataSocket equals false
	# the program connects to self.clientsocket, if true the connection
	# is self.datasocket.
	#===================================================================================
	def connect(self, isDatasocket):
		if isDatasocket == True:
			try:
				s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				s.bind(('', int(self.dataport)))
				s.listen(1)
				self.datasocket, self.misc = s.accept()
			except socket.error as msg:
				print str(msg[0]) + ":" + str(msg[1]) + "(datasocket create/bind/listen)"
		else:
			try:
				self.clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			except socket.error as msg:
				print str(msg[0]) + ":" + str(msg[1]) + "(socket create)"
			try:
				self.clientsocket.connect((self.serverhost, int(self.serverport)))
			except socket.error as msg:
				print str(str(msg[0]) + ":" + str(msg[1]) + "(socket connect)")

	#===============================FTClient.sendCommand()==================================
	# Handles the sending of commands to ftserver. If the command is -l self.filename is 
	# changed to 'DIR.txt' to read the directory file when it arrives. If the command was 
	# accepted by ftserver then a message will be displayed and FTClient.recieveFile() will
	# handle the transfer. If a -g command fails the program will exit.
	#======================================================================================== 	
	def sendCommand(self):
		#If the command is list
		if self.command == "-l":
			self.filename = "DIR.txt"
			message = str(self.dataport + " " + self.command + " \n")
			mlen = len(message)
			sent = self.clientsocket.send(message)
			assert(sent == mlen)
			recieved = self.clientsocket.recv(1024)
			if recieved == "ACCEPTED":
				print str("Recieving directory structure from " + self.serverhost + ":" + self.serverport)
			else:
				print str(self.serverhost + ":" + self.serverport + " says: " + recieved)
		#If the command is get
		else:
			message = str(self.dataport + " " + self.command + " " + self.filename + " \n")
			mlen = len(message)
			sent = self.clientsocket.send(message)
			assert(sent == mlen)
			recieved = self.clientsocket.recv(1024)
			if recieved == "ACCEPTED":
				print "Recieving '" + self.filename + "'from " + self.serverhost + ":" + self.serverport
			else:
				print str(self.serverhost + ":" + self.serverport + " says: " + recieved)
				exit(1)

	#===============================FTClient.recieveFile()===================================
	# Handles the recieving of files from ftserver. If the current filename is a duplicate 
	# of a file already on disk then the file name will be changed as per one of the following 
	# examples: (1):'my_file.txt'->'my_file(1).txt' (2):'my_file(1).txt'->'my_file(2).txt'.
	# If the recieved file is not a directory request then the file is only saved to disk. 
	# If the recived file is from a directory request then the file will be read, printed and 
	# then deleted from disk.Write files are opened according to type. Extentions .text and 
	# .txt files are opened in normal mode, but all others will be opened in binary mode.
	#======================================================================================== 
	def recieveFile(self):
		#Open the file for writing based on what type of extention it has.
		#If this is a duplicate file append a unique numbering to the name
		if self.filename.find(".txt") == -1 or self.filename.find(".text") == -1:
			self.changeFileName()
			f = open(self.filename, "wb")
		else:
			self.changeFileName()
			f = open(self.filename, "w")
		#Recieve and write the file provided we didn't recieve the FILE NOT FOUND error
		cntmes = self.clientsocket.recv(20)
		if cntmes.find("FILE-NOT-FOUND",0,16) > 0:
			print str(self.serverhost + ":" + self.serverport + " says FILE NOT FOUND")
			f.close()
			os.remove(self.filename+".out")
			exit(1)
		else:
			count = 0
			while 1:
				buf = self.datasocket.recv(1024)
				if buf == "":
					count = count + 1
					if count > 3:
						break
					else:
						continue
				f.write(buf)	
			f.close()
		#If the file we just recieved was the directory then open, read and print the directory file
		#contents then delete the file
		if self.filename == "DIR.txt":
			self.changeFileName()
			fr = open("DIR.txt", "r")
			c = fr.read()
			print c
			fr.close()
			os.remove("DIR.txt")
		else:
			print "File transfer complete."	

#-----------Main--------------
if __name__ == '__main__':
	testClient = FTClient()
	testClient.isValid()
	testClient.connect(False)
	testClient.sendCommand()
	testClient.connect(True)
	testClient.recieveFile()
	del testClient
	exit(0)
