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

class Client{

public:
  Client(uint32_t port_number, std::string host_name);
  ~Client();
  void writex(std::string);
  std::string readx();
  void init();
  void setHostName(std::string host_name) { HostName = host_name; }
  std::string getHostName() { return HostName; }

private:
  std::string HostName;
  int32_t SockFD, PortNumber;
  ssize_t n;
  struct hostent * Server;
  socklen_t CliLen;
  void error(std::string);
  struct sockaddr_in ServerAddress, ClientAddress;
};


#endif //EUDAQ_SERVER_H
