#include "server.hpp"

char *progname;

Server server;

void sigHandler(int sig) {
  server.timeouts++;
  std::cout << "** Timeout Count " << server.timeouts << std::endl;
  if (server.timeouts >= 10) {
    std::cout << "Program Terminating: Connection Timeout\n";
    exit(0);
  }
  return;
}

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

/* Main driver program. Initializes server's socket and calls the  */
int main(int argc, char *argv[]) {

  int argumentsError = 0;

  if (argc == 3) {
    if (strcmp(argv[1], "-p") == 0) {
      int newPort = std::stoi(argv[2]);
      server.changePort(newPort);
    } else { // error, -p was not in correct location or not an argument
      argumentsError = 1;
    }
  } else if (argc == 1) {
  }      // do nothing
  else { // error, inproper number of arguments
    argumentsError = 1;
  }

  if (argumentsError) {
    // error message
    std::cerr << "Incorrect input format\n";
    return 0;
  }

  /* General purpose socket structures are accessed using an         */
  /* integer handle.                                                 */

  int sockfd;

  /* The Internet specific address structure. We must cast this into */
  /* a general purpose address structure when setting up the socket. */

  struct sockaddr_in serv_addr, cli_addr;

  /* argv[0] holds the program's name. We use this to label error    */
  /* reports.                                                        */

  progname = argv[0];

  /* Create a UDP socket (an Internet datagram socket). AF_INET      */
  /* means Internet protocols and SOCK_DGRAM means UDP. 0 is an      */
  /* unused flag byte. A negative value is returned on error.        */

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("%s: can't open datagram socket\n", progname);
    exit(1);
  }

  /* Abnormal termination using the exit call may return a specific  */
  /* integer error code to distinguish among different errors.       */

  /* To use the socket created, we must assign to it a local IP      */
  /* address and a UDP port number, so that the client can send data */
  /* to it. To do this, we fisrt prepare a sockaddr structure.       */

  /* The bzero function initializes the whole structure to zeroes.   */

  bzero((char *)&serv_addr, sizeof(serv_addr));

  /* As sockaddr is a general purpose structure, we must declare     */
  /* what type of address it holds.                                  */

  serv_addr.sin_family = AF_INET;

  /* If the server has multiple interfaces, it can accept calls from */
  /* any of them. Instead of using one of the server's addresses,    */
  /* we use INADDR_ANY to say that we will accept calls on any of    */
  /* the server's addresses. Note that we have to convert the host   */
  /* data representation to the network data representation.         */

  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* We must use a specific port for our server for the client to    */
  /* send data to (a well-known port).                               */

  serv_addr.sin_port = htons(server.getPortNum());

  /* We initialize the socket pointed to by sockfd by binding to it  */
  /* the address and port information from serv_addr. Note that we   */
  /* must pass a general purpose structure rather than an Internet   */
  /* specific one to the bind call and also pass its size. A         */
  /* negative return value means an error occured.                   */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("%s: can't bind local address\n", progname);
    exit(2);
  }

  // register the alarm() handler for timeout
  if (register_handler() != 0) {
    printf("Failed to register timeout\n");
  }

  /* We can now start the server's main loop. We only pass the       */
  /* local socket to dg_echo, as the client's data are included in   */
  /* all received datagrams.                                         */

  unsigned int serAddrLen = sizeof(serv_addr);
  unsigned int cliAddrLen;
  while (true) {
    std::cout << " Main server Loop" << std::endl;
    // reclen to reset at start of each new iteration
    int reclen = 0;
    reclen = recvfrom(sockfd, server.PACKET, sizeof(server.PACKET), 0,
                      (struct sockaddr *)&cli_addr, &cliAddrLen);

    if (reclen > 0) {
      // update new port number for new child to use
      server.changePort();
      
      // returns ID num to parent, 0 to child, and - for error
      int forkID = fork();
      if (forkID < 0) {
        std::cerr << "Fork error: Program terminating\n";
        exit(1);
      }

      if (forkID == 0) { // the child process
      
        server.packLen = reclen;

        // create a new socket
        int sock_fd;

        close(sockfd);
    
        if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
          printf("%s: can't open datagram socket\n", progname);
          exit(1);
        }

        struct sockaddr_in servAddr;
    
        bzero((char *)&servAddr, sizeof(serv_addr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        //std::cout << "NEW PORT NUM: " << server.getPortNum() << std::endl;
        servAddr.sin_port = htons(server.getPortNum());

        if (bind(sock_fd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
          printf("%s: can't bind local address\n", progname);
          exit(2);
        }

        server.endpoint = sock_fd;
        server.cliAdd = (struct sockaddr *)&cli_addr;
        server.clilen = sizeof(cli_addr);

        // unpack request packet
        unsigned short opcode;
        bcopy(server.PACKET, &opcode, 2);
        opcode = ntohs(opcode);
        std::string fileName(server.PACKET + FILENAMEOFFSET);

        switch (opcode) {
        case server.OP_CODE_RRQ:
          std::cout << "Server: Received read request packet of "
                    << server.packLen << " bytes" << std::endl;
          server.read(fileName);
          break;
        case server.OP_CODE_WRQ:
          std::cout << "Server: Received write request packet of "
                    << server.packLen << " bytes" << std::endl;
          server.write(fileName);
          break;
        default:
          break; // TODO: add error case here
        }
        
        // child process will then terminate
        close(sock_fd);
        exit(0);
      }
    } // if (reclen > 0)
    
    // will not execute unless a buffer has been received
    // if (reclen > 0) {
    //   server.packLen = reclen;
      // returns ID num to parent, 0 to child, and - for error
      // int forkID = fork();

      // if (forkID < 0) {
      //   std::cerr << "Fork error: Program terminating\n";
      //   exit(1);
      // }

      // if (forkID == 0) { // the child process executes functions

      //   // create a new socket
      //   int sock_fd;
      //   // struct sockaddr_in serv_add; // may not need to allocate
      //   // new variable just use existing one
      //   if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      //     printf("%s: can't open datagram socket\n", progname);
      //     exit(1);
      //   }
      //   // clear values
      //   // bzero((char *)&serv_add, sizeof(serv_add));
      //   // serv_add.sin_family = AF_INET;
      //   // serv_add.sin_addr.s_addr = htonl(INADDR_ANY);
      //   server.changePort();
      //   serv_addr.sin_port = htons(server.getPortNum());
      //   if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
      //       0) {
      //     printf("%s: can't bind local address\n", progname);
      //     exit(2);
      //   }
      //   // new socket sock_fd created
      //   // update server socket data members
      //   server.endpoint = sock_fd;
      //   server.cliAdd = (struct sockaddr *)&serv_addr;
      //   server.clilen = sizeof(serv_addr);

      //   // unpack request packet
      //   unsigned short opcode;
      //   bcopy(server.PACKET, &opcode, 2);
      //   opcode = ntohs(opcode);
      //   std::string fileName(server.PACKET + FILENAMEOFFSET);

      //   switch (opcode) {
      //   case server.OP_CODE_RRQ:
      //     std::cout << "Server: Received read request packet of "
      //               << server.packLen << " bytes" << std::endl;
      //     server.read(fileName);
      //     break;
      //   case server.OP_CODE_WRQ:
      //     std::cout << "Server: Received write request packet of "
      //               << server.packLen << " bytes" << std::endl;
      //     server.write(fileName);
      //     break;
      //   default:
      //     break; // TODO: add error case here
      //   }
      //   // child fork will then terminate
      //   close(sock_fd);
      //   exit(0);
      // }
      // // parent thread will restart main server loop & can continue using its og
      // // portnum
      // else {
      //   exit(0); //test kill parent
      // }
    // }
  }
}

bool Server::changePort(int newNum) {
  // check?? 1024 -49151 or ->65135
  PORTNUM = newNum;
  return true;
}

void Server::changePort() {
  // int randPort = rand() % 10;
  // PORTNUM = PORTNUM + randPort;
  // return PORTNUM;
  PORTNUM++;
}

int Server::getPortNum() { return PORTNUM; }

int Server::read(std::string filename) {
  unsigned int serverLen = (unsigned int)clilen;

  std::fstream fileStream;

  // check file exists
   
  fileStream.open(filename, std::fstream::in);
  
  if (!fileStream.is_open()) {
    const char *CFileName = filename.c_str();
    // check for permissions
    if (access(CFileName, R_OK) == -1) { // -1 is returned from access if do not have permission
      //construct and send error packet to client
      error(NOPERMISSIONTOACCESSFILE, endpoint, cliAdd, clilen);
      fileStream.close();
      return 0;
    }
    error(FILEDOESNOTEXIST, endpoint, cliAdd, clilen);
    fileStream.close();
    return 0;
  }


  // get size of file
  fileStream.seekg(0, std::ios::end); // sets file pointer to end of file
  int endFile = fileStream.tellg();
  fileStream.seekg(0,
                   std::ios::beg); // sets file pointer to beginning of file
  int beginFile = fileStream.tellg();
  int fileSize = endFile - beginFile;
  int fileCount = 0;

  std::cout << "Filesize: " << fileSize << " Packets: " << fileSize / 516
            << std::endl;

  unsigned short opcode = htons(OP_CODE_DATA);
  int block = 1;
  // ***************************************** timeout test variable
  // int timecount = 0;
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

    // send data packet
    // dataSize is last array slot filled
    int sent = sendto(endpoint, PACKET, count, 0, cliAdd, clilen);
    std::cout << "Server: Sent read data packet of " << sent << " bytes"
              << std::endl;
    // begin timout
    std::cout << "Timer started\n";
    int alarmTime = alarm(3);

    // clear packet
    bzero(PACKET, sizeof(PACKET));
    // wait for ack packet
    packLen =
        recvfrom(endpoint, PACKET, sizeof(PACKET), 0, cliAdd, &serverLen);

    // ********************************************************************
    // Timeout test if (timecount == 2) {
    //   std::cout << "Simulate timeout\n";
    //   sleep(5);
    //   packLen = 0;
    // }
    // timecount++;

    // Begin timeout logic
    while (packLen < 1) {
      std::cout << "recvfrom error \n";
      // if interupt error
      if (errno == EINTR) {
        std::cout << "Timeout triggered \n";
        std::cout << "Resetting timer & resending last packet\n";
        alarm(0);

        int sent = sendto(endpoint, PACKET, count, 0, cliAdd, clilen);
        std::cout << "Resent packet of " << sent << " bytes\n";
        int alarmTime = alarm(3);
        std::cout << "Timer started \n";
        packLen =
            recvfrom(endpoint, PACKET, sizeof(PACKET), 0, cliAdd, &serverLen);
        // std::cout << "packlen after recvfrom()\n";
      }
    }
    std::cout << "Response packet received from client. Clearing timeout alarm "
                 "& count\n";
    alarm(0);
    timeouts = 0;
    // End timeout logic

    std::cout << "Server: Received read ack packet of " << packLen << " bytes"
              << std::endl;

    // TODO error checking
  }

  fileStream.close();

  // TODO: return meaningful value
  return 0;
}

int Server::write(std::string filename) {
  std::fstream fileStream;

  // check file does not exist
  // fileStream.open(filename, std::fstream::out);
  if (fileStream.is_open()) {
    error(FILEALREADYEXISTS, endpoint, cliAdd, clilen);
    fileStream.close();
    return 0;
  }

  fileStream.close();

  unsigned int serverLen = (unsigned int)clilen;
  // init file name and file stream
  std::ofstream OutFileStream(filename);
  // check file actually opened
  if (!OutFileStream) {
    std::cerr << "Failed to open " << filename << '\n';
    error(FAILEDTOPOPEN, endpoint, cliAdd, clilen);
  }

  // send ack packet in response to wrq
  unsigned short opcode = htons(OP_CODE_ACK);
  bcopy(&opcode, &PACKET, 2);
  int blocknum = htons(0);
  bcopy(&blocknum, &PACKET[2], 2);
  SENDLEN = sendto(endpoint, PACKET, ACKSIZE, 0, cliAdd, clilen);
  std::cout << "Send ack packet of " << SENDLEN << " bytes\n";
  std::cout << "Timer started \n";
  alarm(3);

  bool moreData = true;
  while (moreData) {
    // recieve data packet
    packLen =
        recvfrom(endpoint, PACKET, sizeof(PACKET), 0, cliAdd, &serverLen);
    // Begin timeout logic
    while (packLen < 1) {
      std::cout << "recvfrom error \n";
      // if interupt error
      if (errno == EINTR) {
        std::cout << "Timeout triggered \n";
        std::cout << "Resetting timer & resending last packet\n";
        alarm(0);

        // testing simulaneous clients
        sleep(1);
        
        int sent = sendto(endpoint, PACKET, SENDLEN, 0, cliAdd, clilen);
        std::cout << "Resent packet of " << sent << " bytes\n";
        int alarmTime = alarm(3);
        std::cout << "Timer started \n";
        packLen =
            recvfrom(endpoint, PACKET, sizeof(PACKET), 0, cliAdd, &serverLen);
      }
    }
    std::cout << "Response packet received from client. Clearing timeout alarm "
                 "& count\n";
    alarm(0);
    timeouts = 0;
    // End timeout logic

    std::cout << "Server: Received data packet of " << packLen << " bytes"
              << std::endl;
    // check for last packet to end loop
    if (packLen < MAXPACKET) {
      moreData = false;
    }

    // unpack packet
    unsigned short opCode = 0;
    // copy bytes from first two packet slots
    bcopy(PACKET, &opCode, 2);
    opCode = ntohs(opCode);

    unsigned short blockNum = 0;
    bcopy(&PACKET[BLOCKNUMOFFSET], &blockNum, 2);
    blockNum = ntohs(blockNum);

    std::string data(&PACKET[DATAOFFSET]); // converting c string to std::string

    // check op code
    if (opCode == OP_CODE_DATA) {
      // write packet to file
      OutFileStream << data;

      // empty PACKET
      bzero(PACKET, sizeof(PACKET));

      // build ACK packet
      opCode = htons(OP_CODE_ACK);
      bcopy(&opCode, &PACKET, 2);
      blockNum = htons(blockNum);
      bcopy(&blockNum, &PACKET[BLOCKNUMOFFSET], 2);

      // send ACK packet
      int sent = sendto(endpoint, PACKET, ACKSIZE, 0, cliAdd, clilen);
      std::cout << "Server: Sent ack packet of " << sent << " bytes"
                << std::endl;
      // begin timout
      if (moreData) {
        std::cout << "Timer started\n";
        int alarmTime = alarm(3);
      }

    } else {
      // TODO: ERROR packet
    }
  }

  OutFileStream.close();

  return 0; // TODO: return meaningful value
}

int Server::error(unsigned short errorCode, int endpoint,
                  struct sockaddr *cliAdd, int clilen) {
  int returnVal = 0;

  std::string ErrMsg;
  switch (errorCode) {
  case FILEDOESNOTEXIST:
    ErrMsg = "File does not exist.";
    std::cerr << "Error: File does not exist. \n";
    break;
  case FILEALREADYEXISTS:
    ErrMsg = "File already exists.";
    std::cerr << "Error: File already exists. \n";
    break;
  case NOPERMISSIONTOACCESSFILE:
    ErrMsg = "No permission to access the requested file.";
    std::cerr << "Error: No permission to access the requested file. \n";
    break;
  case FAILEDTOPOPEN:
    ErrMsg = "Unable to open file.\n";
    std::cerr << "Error: Unable to open file. \n";
    break;
  default:
    returnVal = 1; // invalid errorCode
    break;
  }

  // empty PACKET
  bzero(PACKET, sizeof(PACKET));

  // build error packet
  unsigned short opCode = htons(OP_CODE_ERROR);
  bcopy(&opCode, &PACKET, 2);
  errorCode = htons(errorCode);
  bcopy(&errorCode, &PACKET[ERRORCODEOFFSET], 2);
  ErrMsg.copy(&PACKET[ERRMSGOFFSET], ErrMsg.length());
  PACKET[ERRMSGOFFSET + ErrMsg.length()] = null;

  // send ERROR packet, TODO: use sent value to check if successfully sent or
  // not
  int sent = sendto(endpoint, PACKET, ERRMSGOFFSET + ErrMsg.length() + 1, 0,
                    cliAdd, clilen);
  std::cout << "Server: Sent error packet of " << sent << " bytes" << std::endl;

  exit(0);
}