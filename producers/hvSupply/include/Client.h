/** ---------------------------------------------------------------------------------
 *  Server class to open a socket to read trigger numbers
 *  A simple server in the internet domain using TCP. The port number is passed as an argument
 *
 *  Date: 	April 26th 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef EUDAQ_SERVER_H
#define EUDAQ_SERVER_H

#include <netinet/in.h>
#include <string>

class Server{

public:
  Server(uint32_t port_number);
  ~Server();
  void writex(std::string);
  std::string readx();

private:
  int32_t SockFD, NewSockFD, PortNumber;
  ssize_t n;
  socklen_t CliLen;
  void error(std::string);
  struct sockaddr_in ServerAddress, ClientAddress;
};


#endif //EUDAQ_SERVER_H
