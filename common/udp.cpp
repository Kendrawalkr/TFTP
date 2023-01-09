#include "udp.hpp"

// int main(){
//   return 0;
// }

void UDP::addTimeout() { timeouts++; }

int UDP::getTimeout() { return timeouts; }

int UDP::setConnection(int endpoint, struct sockaddr *sAdd, int sLen) {
  Endpoint = endpoint;
  ServAddP = sAdd;
  ServLen = (unsigned int)sLen;
  return 1;
}

bool UDP::changePort(int newNum) {
  // check?? 1024 -49151 or ->65135
  PORTNUM = newNum;
  return true;
}

void UDP::changePort() {
  int randPort = rand() % 10;
  PORTNUM = PORTNUM + randPort;
  //PORTNUM++;
}

int UDP::getPortNum() { return PORTNUM; }

int UDP::getPackLen() { return PACKLEN; }

void UDP::setPackLen(int len) { PACKLEN = len; }

void UDP::setEndpoint(int sock) { Endpoint = sock; }

// build and send ack packet
int UDP::sendAck(int blocknum) {
  // build ACK packet
  int opCode = htons(OP_CODE_ACK);
  bcopy(&opCode, &PACKET, 2);
  int block = htons(blocknum);
  bcopy(&block, &PACKET[2], 2);

  // send ACK packet
  int sent = sendto(Endpoint, PACKET, ACKSIZE, 0, ServAddP, ServLen);
  std::cout << "Sent ack packet of " << sent << " bytes" << std::endl;
  std::cout << "Timer started\n";
  alarm(3);
  return sent;
}

int UDP::resendTimeout(int rcvlen) {
  while (rcvlen < 1) {
    std::cout << "recvfrom error \n";
    // if interrupt error
    if (errno == EINTR) {
      std::cout << "Timeout triggered \n";
      std::cout << "Resetting timer & resending last packet\n";
      alarm(0);

      int sent = sendto(Endpoint, PACKET, PACKLEN, 0, ServAddP, ServLen);
      std::cout << "Resent packet of " << sent << " bytes\n";
      int alarmTime = alarm(3);
      std::cout << "Timer started \n";
      rcvlen =
          recvfrom(Endpoint, PACKET, sizeof(PACKET), 0, ServAddP, &ServLen);
      // Simulate packet loss for timeout termination
    }
  }
  return rcvlen;
}

void UDP::checkFile(std::fstream &fileStream, std::string filename){
  const char *fileP = filename.c_str();
    // if not open
    if (!fileStream.is_open()) {
      // check for access
      if (access(fileP,R_OK) ==-1) { // -1 is returned from access if do not have permission
       std::cout << "No permission to access the requested file: " << filename << std::endl;
      exit(0);
      }
      // check for existence
      else if (access(fileP, F_OK) == 0) { // 
      std::cerr << filename << " already exists." << std::endl;
      fileStream.close();
      }
      fileStream.close();
      std::cerr << filename << " does not exist." << std::endl;
      fileStream.close();
      exit(0);
  }

}



void UDP::createSocket() {
  if ((Endpoint = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("can't open datagram socket\n");
    exit(1);
  }
}

void UDP::setLocalUpAddress() {
  bzero((char *)&ServAdd, sizeof(ServAdd));
  ServAdd.sin_family = AF_INET;
  ServAdd.sin_addr.s_addr = htonl(INADDR_ANY);
  ServAdd.sin_port = htons(getPortNum());
  ServLen = sizeof(ServAdd);
}

void UDP::bindSocket() {
  if (bind(Endpoint, ServAddP, ServLen) < 0) {
    printf("can't bind local address\n");
    exit(2);
  }
}

void UDP::bindSocket(struct sockaddr *servAddr, unsigned int servLen) {
  if (bind(Endpoint, servAddr, servLen) < 0) {
    printf("can't bind local address\n");
    exit(2);
  }
}