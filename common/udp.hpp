//header file for shared udp functions between client and server
#pragma once

#include <fstream>
#include <iostream>
#include <string.h>

// copied from PA1
#include <arpa/inet.h>
#include <errno.h> // for retrieving the error number.
#include <cerrno> 
#include <netinet/in.h>
#include <signal.h> // for the signal handler registration.
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strerror function.
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCKNUMOFFSET 2
#define FILENAMEOFFSET 2
#define ERRORCODEOFFSET 2
#define DATAOFFSET 4
#define ERRMSGOFFSET 4
#define FILEDOESNOTEXIST 1
#define FILEALREADYEXISTS 6
#define NOPERMISSIONTOACCESSFILE 2
#define FAILEDTOPOPEN 3

#define MAXPACKET 516
#define MAXDATA 512
#define ACKSIZE 4

//class includes shared code between Client and Server UDP programs

class UDP{
public:
  void addTimeout(); // adds timeout to timeout counter
  int getTimeout();

  //*************** TODO find out where these are used
  int getPackLen();
  void setPackLen(int len);
  void setEndpoint(int sock);

  // intialize variables for connection
  int setConnection(int endpoint, struct sockaddr *servadd, int servlen);

  // set new PORTNUM from input
  bool changePort(int newNum);
  // generate new PORTNUM
  void changePort();
  // Return PORTNUM
  int getPortNum(); 

  int writeToFile();

  int extractFromFile();
  

protected:

  // build and send ack packet
  int sendAck(int blocknum);

  // handle resending last packet in timeout event
  int resendTimeout(int rcvlen);

  void createSocket();

  void setLocalUpAddress();

  void bindSocket();
  void bindSocket(struct sockaddr* servAddr, unsigned int servLen);

  // checks for file exists and permissions
  void checkFile(std::fstream &filestream, std::string filename);

  //data fields
  char PACKET[MAXPACKET]; // packet for sending data
  int timeouts; // timeout counter for alarm
  int PORTNUM = 51454; // default port number
  int PACKLEN = 0;  // Length of built packet
  int SENDLEN = 0; //length of last buffer sent

  // opcodes
  const static unsigned short OP_CODE_RRQ = 1;
  const static unsigned short OP_CODE_WRQ = 2;
  const static unsigned short OP_CODE_DATA = 3;
  const static unsigned short OP_CODE_ACK = 4;
  const static unsigned short OP_CODE_ERROR = 5;
  const static char null = 0x00;

  //established connection variables
  int Endpoint = 0;
  sockaddr_in ServAdd, CliAdd;
  struct sockaddr *ServAddP = (struct sockaddr*)&ServAdd;
  struct sockaddr *CliAddP = (struct sockaddr*)&CliAdd;
  unsigned int ServLen, CliLen;
  
  // NETASCII string null terminated
  const char NETASCII[9] = {'n', 'e', 't', 'a', 's', 'c', 'i', 'i', 0x00};

private:
};