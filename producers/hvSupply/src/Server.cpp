// Created by micha on 26.04.18.

#include "Server.h"

#include <cstdlib>
#include <unistd.h>
#include "cstring"
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdio>

using namespace std;

Server::Server(uint32_t port_number): PortNumber(port_number) {

  SockFD = socket(AF_INET, SOCK_STREAM, 0);
  if (SockFD < 0)
    error("ERROR opening socket");
  bzero((char *) &ServerAddress, sizeof(ServerAddress));
  ServerAddress.sin_family = AF_INET;
  ServerAddress.sin_addr.s_addr = INADDR_ANY;
  ServerAddress.sin_port = htons(PortNumber);
  if (bind(SockFD, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress)) < 0)
    error("ERROR on binding");
  listen(SockFD, 5);
  CliLen = sizeof(ClientAddress);
  NewSockFD = accept(SockFD, (struct sockaddr *) &ClientAddress, &CliLen);
  if (NewSockFD < 0)
    error("ERROR on accept");
}
Server::~Server() {

  close(NewSockFD);
  close(SockFD);
}

std::string Server::readx() {

  char buffer[256];
  bzero(buffer, 256);
  n = read(NewSockFD, buffer, 255);
  if (n < 0)
    error("ERROR reading from socket");
  return string(buffer);
}

void Server::writex(std::string msg) {

  n = write(NewSockFD, msg.c_str(), 18);
  if (n < 0)
    error("ERROR writing to socket");
}

void Server::error(string msg) {

  perror(msg.c_str());
  exit(1);
}