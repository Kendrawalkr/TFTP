#include "client.hpp"

Client client;

// timeout singal handler
void sigHandler(int sig) {
  client.addTimeout();
  int count = client.getTimeout();
  std::cout << "** Timeout Count " << count << std::endl;
  if (count >= 10) {
    std::cout << "Program Terminating: Connection Timeout\n";
    exit(0);
  }
  return;
}

// register the timeout signal handler
int register_handler() {
  int rt = 0;
  sig_t prev = signal(SIGALRM, sigHandler);
  if (prev == (SIG_ERR)) {
    std::cout << "Can't register sigHandler function \n";
    std::cout << "signal() error: " << strerror(errno) << std::endl;
    return -1;
  }
  rt = siginterrupt(SIGALRM, 1);
  if (rt == -1) {
    std::cout << "Invalid sig number \n";
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  //./client.cpp -r filename  : 3 arguments
  // ./client.cpp -p port# -(r/w) filename : 5 arguments

  // register the alarm() handler for timeout
  if (register_handler() != 0) {
    printf("Failed to register timeout\n");
  }

  char *file = argv[2];
  int argument = 0;
  int newPort = 0;
  int argumentsError = 0;

  // check for five arguments, -p or-(r/w) in either order
  if (argc == 5) {
    if (strcmp(argv[1], "-w") == 0 || strcmp(argv[3], "-w") == 0) {
      argument = WRITE;
    } else if (strcmp(argv[1], "-r") == 0 || strcmp(argv[3], "-r") == 0) {
      argument = READ;
    } else { // error, -w or -r was either not in correct location or not an
             // argument
      argumentsError = 1;
    }

    // ./ex -p port# -(r/w) filename
    if (strcmp(argv[1], "-p") == 0) {
      newPort = std::stoi(argv[2]);
      file = argv[4];
    }
    // ./ex -(r/w) filename -p port#
    else if (strcmp(argv[3], "-p") == 0) {
      newPort = std::stoi(argv[4]);
      file = argv[2];
    } else { // error, -p was not in correct location or not an argument
      argumentsError = 1;
    }

    client.changePort(newPort);
  }

  // no port num input
  else if (argc == 3) {
    file = argv[2];
    if (strcmp(argv[1], "-w") == 0) {
      argument = WRITE;
    } else if (strcmp(argv[1], "-r") == 0) {
      argument = READ;
    } else { // error, -w or -r was either not in correct location or not an
             // argument
      argumentsError = 1;
    }
  }

  else { // error, inproper number or order of arguments
    argumentsError = 1;
  }

  if (argumentsError) {
    // error message
    std::cerr << "Incorrect input format\n";
    return 0;
  }

  //********************************** set up socket stuff

  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  int sockfd;

  /* Initialize first the server's data with the well-known numbers. */
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  /* The system needs a 32 bit integer as an Internet address, so we */
  /* use inet_addr to convert the dotted decimal notation to it.     */
  serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
  serv_addr.sin_port = htons(client.getPortNum());

  /* Create the socket for the client side.                          */
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("can't open datagram socket\n");
    exit(1);
  }

  /* Initialize the structure holding the local address data to      */
  /* bind to the socket.                                             */
  bzero((char *)&cli_addr, sizeof(cli_addr));
  cli_addr.sin_family = AF_INET;

  /* Let the system choose one of its addresses for you. You can     */
  /* use a fixed address as for the server.                          */
  cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* The client can also choose any port for itself (it is not       */
  /* well-known). Using 0 here lets the system allocate any free     */
  /* port to our program.                                            */
  cli_addr.sin_port = htons(0);

  /* The initialized address structure can be now associated with    */
  /* the socket we created. */
  if (bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
    printf("can't bind local address\n");
    exit(2);
  }

  // initialize global variables
  client.setConnection(sockfd, (struct sockaddr *)&serv_addr,
                       sizeof(serv_addr));

  /* call the function requested from arguments */
  switch (argument) {
  case WRITE:
    client.wrq(file);
    break;
  case READ:
    client.rrq(file);
    break;
  default:
    std::cerr << "Argument case function call error";
    break;
  };

  /* We return here after the function call */
  /* We can now release the socket and exit normally.*/
  close(sockfd);
  exit(0);

  return 0;
}

// checks for existing file error
void Client::fileExists(char filename[]) {
  // check file does not exist
  std::fstream fileStream;
  if (access(filename, F_OK) ==
      0) { // -1 is returned from access if do not have
    std::cerr << filename << " already exists." << std::endl;
    fileStream.close();
  }
  fileStream.close();
}

// construct and send rrq packet - wait for ack
int Client::rrq(char filename[]) {
  // check file does not already exist
  fileExists(filename);

  // Construct Packet ****************************************
  // pointer to current point in packet
  int cur = 0;
  // reset packet contents
  bzero(PACKET, sizeof(PACKET));
  // convert opCode to network order & add to beginning
  unsigned short opCodeNetworkOrd = htons(OP_CODE_RRQ);
  bcopy(&opCodeNetworkOrd, &PACKET[cur], 2);
  cur += 2;

  // filename
  std::string sendFile(filename);
  int len = sendFile.length();
  sendFile.copy(&PACKET[cur], len);

  // zero
  cur += len;
  const static char null = 0x00;
  bcopy(&null, &PACKET[cur], 1);

  // mode (0 is included at end of NETASCII array as '\0' null)
  cur++;
  memcpy(&PACKET[cur], &NETASCII, strlen(NETASCII));
  // size of used packet bytes
  cur += strlen(NETASCII);
  std::string mode(NETASCII);

  PACKLEN = cur;
  // Packet fully constructed ********************************

  // TIMER START *********************************************
  int sent = sendto(Endpoint, PACKET, PACKLEN, 0, ServAddP, ServLen);
  std::cout << "Client: Sent read request packet of " << sent << " bytes"
            << std::endl;
  // begin timout timer for three seconds
  std::cout << "Setting time out alarm \n";
  int alarmTime = alarm(3);

  // read() will wait for server response
  read(filename);

  return 0;
}

int Client::read(std::string filename) {
  // init file name and file stream
  std::ofstream fileStream(filename);
  // check file actually opened
  if (!fileStream) {
    std::cerr << "Failed to open " << filename << '\n';
    return 0;
  }

  std::cout << "Client: Preparing to receive data packets from Server\n";
  bool moreData = true;
  // Timeout test
  // variable***********************************************************TEST int
  // testcount = 0;
  while (moreData) {
    // recieve data packet

    // Timeout test
    // code************************************************************TEST
    // testcount++;
    // std::cout <<"Client Data Reception Loop # ***********" << testcount <<
    // "******\n";

    int rcvlen =
        recvfrom(Endpoint, PACKET, sizeof(PACKET), 0, ServAddP, &ServLen);

    // Timeout test
    // code************************************************************TEST if
    // (testcount == 1 || testcount == 3|| testcount == 5){ std::cout
    // <<"Simulating packet loss: rcvlen set to 0, sleep(5) called\n"; rcvlen =
    // 0; sleep(5);
    // }

    // TIMER END
    // *******************************************************************
    if (rcvlen < 1) {
      resendTimeout(rcvlen);
    }
    // otherwise rcvlen > 0
    std::cout << "Response packet received from server. Clearing timeout alarm "
                 "& count\n";
    alarm(0);
    timeouts = 0;

    // check for last packet to end loop
    if (rcvlen < MAXPACKET) {
      moreData = false;
    }

    // Unpack Packet **************************************************
    unsigned short opCode = 0;
    // copy bytes from first two packet slots
    bcopy(PACKET, &opCode, 2);
    opCode = ntohs(opCode);
    if (opCode == OP_CODE_ERROR) {
      std::cout << "Received error packet of " << rcvlen << " bytes"
                << std::endl;
      error();
      return 1; // error
    }

    std::cout << "Received read data packet of " << rcvlen << " bytes"
              << std::endl;

    // get blocknum from packet
    unsigned short blockNum;
    bcopy(&PACKET[BLOCKNUMOFFSET], &blockNum, 2);
    blockNum = ntohs(blockNum);

    std::string data(&PACKET[DATAOFFSET]); // converting c string to std::string

    // check op code
    if (opCode == OP_CODE_DATA) {
      // write packet to file
      fileStream << data;
      // empty PACKET
      bzero(PACKET, sizeof(PACKET));
      // send ack packet
      
      sendAck(blockNum);
      
      
    } else {
      // TODO: ERROR packet
      std::cout << "!! Error: Did not receive data packet opcode from Server\n "
                   "Received opcode: "
                << opCode << std::endl;
      error();
    }
  }

  fileStream.close();

  return 0; // TODO: return meaningful value
}

int Client::wrq(char filename[]) {
  // check file exists
  std::fstream fileStream;
  fileStream.open(filename, std::fstream::in);

  checkFile(fileStream, filename);

  // if (!fileStream.is_open()) {
  //       if (access(filename,
  //             R_OK) ==
  //       -1) { // -1 is returned from access if do not have permission
  //     std::cout << "Client: No permission to access the requested file: " << filename 
  //               << std::endl;
  //     return 0;
  //   }
  //   std::cerr << filename << " does not exist." << std::endl;
  //   fileStream.close();
  //   return 1;
  // }

  // check for permissions

  fileStream.close();

  // pointer to place in packet
  int cur = 0;

  // reset packet contents
  bzero(Client::PACKET, sizeof(PACKET));

  // convert opCode to network order & add to beginning
  unsigned short opCodeNetworkOrd = htons(OP_CODE_WRQ);
  bcopy(&opCodeNetworkOrd, &PACKET[cur], 2);
  cur += 2;

  // add filename to packet
  std::string sendFile(filename);
  int len = sendFile.length();
  sendFile.copy(&PACKET[cur], len);

  // zero - null termination of filename
  cur += len;
  const static char null = 0x00;
  bcopy(&null, &PACKET[cur], 1);

  // mode (0 is included at end of NETASCII array as '\0' null)
  cur++;
  memcpy(&PACKET[cur], &NETASCII, strlen(NETASCII));
  // size of used packet bytes
  cur += strlen(NETASCII);

  // *** wrq packet fully constructed at this point

  int sent = sendto(Endpoint, PACKET, cur, 0, ServAddP, ServLen);
  std::cout << "Client: Sent write request packet of " << sent << " bytes"
            << std::endl;
  std::cout << "Timer started\n";
  int alarmTime = alarm(3);

  // wait to receive ack or error packet from sever
  int rcvlen =
      recvfrom(Endpoint, PACKET, sizeof(PACKET), 0, ServAddP, &ServLen);

  // if no packet received, resend
  if (rcvlen < 1) {
    resendTimeout(rcvlen);
  }
  std::cout << "Response packet received from server. Clearing timeout alarm & "
               "count\n";
  alarm(0);
  timeouts = 0;
  // End timeout logic

  unsigned short opCode = 0;
  bcopy(PACKET, &opCode, 2);
  opCode = ntohs(opCode);
  if (opCode == OP_CODE_ERROR) {
    std::cout << "Client: Received error packet of " << rcvlen << " bytes"
              << std::endl;
    error();
    return 1; // error
  }

  std::cout << "Client: Received write ack packet of " << rcvlen << " bytes"
            << std::endl;

  write(filename);

  return 0;
}

int Client::write(std::string filename) {
  std::fstream fileStream;
  fileStream.open(filename, std::fstream::in);
  if (!fileStream.is_open()) {
    std::cerr << filename << " does not exist." << std::endl;
    return 0;
  }

  // get size of file
  fileStream.seekg(0, std::ios::end); // sets file pointer to end of file
  int endFile = fileStream.tellg();
  fileStream.seekg(0, std::ios::beg); // sets file pointer to beginning of file
  int beginFile = fileStream.tellg();
  int fileSize = endFile - beginFile;
  int fileCount = 0;

  unsigned short opcode = htons(OP_CODE_DATA);
  unsigned short block = 1;

  bool lastPacket = false;
  while (!lastPacket) {
    // clear packet
    bzero(PACKET, sizeof(PACKET));
    // set packet opcode
    bcopy(&opcode, PACKET, 2);
    // set packet block #
    unsigned short blockNum = htons(block++);
    bcopy(&blockNum, &PACKET[BLOCKNUMOFFSET], 2);

    // extract data from file and fill packet
    int count = DATAOFFSET;
    for (count; count < MAXDATA + DATAOFFSET; count++) {
      PACKET[count] = fileStream.get();
      fileCount++;
      if (fileCount == fileSize) {
        lastPacket = true;
        break;
      }
    }

    // testing simulaneous clients
    sleep(1);

    // count contains packet size
    // send data packet
    int sent = sendto(Endpoint, PACKET, count, 0, ServAddP, ServLen);
    std::cout << "Client: Sent data packet of " << sent << " bytes"
              << std::endl;
    // begin timout timer
    std::cout << "Timer started\n";
    int alarmTime = alarm(3);

    // clear packet
    bzero(PACKET, sizeof(PACKET));
    // wait for ack packet
    int rcvlen =
        recvfrom(Endpoint, PACKET, sizeof(PACKET), 0, ServAddP, &ServLen);

    // if no packet received, resend
    if (rcvlen < 1) {
      resendTimeout(rcvlen);
    }
    std::cout << "Response packet received from server. Clearing timeout alarm "
                 "& count\n";
    alarm(0);
    timeouts = 0;
    // End timeout logic

    std::cout << "Client: Received write ack packet of " << rcvlen << " bytes"
              << std::endl;
  }
  fileStream.close();

  // TODO: return meaningful value
  return 0;
}

int Client::error() {
  // get blocknum from packet
  unsigned short errorCode;
  bcopy(&PACKET[ERRORCODEOFFSET], &errorCode, 2);
  errorCode = ntohs(errorCode);

  switch (errorCode) {
  case FILEDOESNOTEXIST:
    std::cout << "File does not exist at server." << std::endl;
    break;
  case FILEALREADYEXISTS:
    std::cout << "File already exists at server." << std::endl;
    break;
  case NOPERMISSIONTOACCESSFILE:
    std::cout << "No permission to access the requested file from server."
              << std::endl;
    break;
  case FAILEDTOPOPEN:
    std::cout << "Server error: unable to open file\n";
    break;
  default:
    std::cout << "Invalid errorcode from server." << std::endl;
    return 1; // invalid errorCode
  }
  return 0;
}