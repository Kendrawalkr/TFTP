#include <fstream>
#include <iostream>
#include <string.h>

// copied from PA1
#include <arpa/inet.h>
#include <errno.h> // for retrieving the error number.
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


class Server {

public:
  // Data members
  // Connection Timeout counter
  int timeouts = 0;
  // build data packet from specified file and send to client
  int read(std::string filename);

  // recieves data packet from client to put into specified file
  // sends ACK packet to client
  int write(std::string filename);

  bool changePort(int newNum);
  void changePort();
  int getPortNum(); 

  const static int MAXPACKET = 516;
  const static int MAXDATA = 512;
  const static int ACKSIZE = 4;

  char PACKET[MAXPACKET];

  const static unsigned short OP_CODE_RRQ = 1;
  const static unsigned short OP_CODE_WRQ = 2;
  const static unsigned short OP_CODE_DATA = 3;
  const static unsigned short OP_CODE_ACK = 4;
  const static unsigned short OP_CODE_ERROR = 5;

  const static unsigned int SIGALARMINTERVAL = 5; // sig alarm interval in seconds

  int packLen = 0;

  unsigned int consecTimeouts = 0;

  int endpoint;
  struct sockaddr *cliAdd;
  int clilen;

private:
  int error(unsigned short errorCode, int endpoint, struct sockaddr *servAdd, int servlen);

  int PORTNUM = 51454; // default port number
  //length of last buffer sent
  int SENDLEN = 0;

  const static char null = 0x00;
};