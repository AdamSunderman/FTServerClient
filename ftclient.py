#!/usr/bin/python
import socket
import sys 
import errno
import os


# File Transfer Client class
# see member function decriptions below
class FTClient(object):

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

	def __del__(self):
		if self.clientsocket != "NONE":
			print "Closing clientsocket..."
			self.clientsocket.close()
		if self.datasocket != "NONE":
			print "Closing datasocket..."
			self.datasocket.close()

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
					print count
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

if __name__ == '__main__':
	testClient = FTClient()
	testClient.isValid()
	testClient.connect(False)
	testClient.sendCommand()
	testClient.connect(True)
	testClient.recieveFile()
	del testClient








