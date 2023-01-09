
#pragma once

#include "../common/udp.cpp"
#include <fstream>
#include <iostream>


#define SERV_UDP_PORT PORTNUM            // replaced port number
//#define SERV_HOST_ADDR "127.0.0.1" // loopback IP address 
//#define SERV_HOST_ADDR "10.158.82.131" // CSS lab20
//#define SERV_HOST_ADDR "10.158.82.127" // CSS lab16
#define SERV_HOST_ADDR "10.158.82.11" // CSS lab14

#define WRITE 1
#define READ 2


class Client : public UDP {

public:
  // Data Members********************
  // length of received buffer
  int rcvlen = 0;
  //int timeouts = 0;
 
  // read request
  // build rrq packet & sends to server
  // receives data packets from server, sends data to read()
  int rrq(char file[]);

  // write request
  // build wrq packet & send to server
  // receives ack packet from server
  int wrq(char file[]);



private:
  // creates file and stores data from server
  int read(std::string filename);

  // read from data file and send to server
  // builds write packets & receives ack packets
  int write(std::string filename);

  // error handling
  int error();

  // checks for file already exists error
  void fileExists(char filename[]);

};
