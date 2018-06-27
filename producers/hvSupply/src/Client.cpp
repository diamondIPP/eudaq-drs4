// Created by micha on 26.04.18.

#include "Client.h"
#include <unistd.h>
#include "cstring"
#include <netdb.h>
#include "iostream"

#include <utility>

using namespace std;

Client::Client(uint32_t port_number, const string host_name): PortNumber(port_number), HostName(host_name) {

}

Client::~Client() {

  close(SockFD);
}

void Client::init() {

  SockFD = socket(AF_INET, SOCK_STREAM, 0);
  if (SockFD < 0)
    error("ERROR opening socket");
  Server = gethostbyname(HostName.c_str());
  if (Server == nullptr) {
    cout << stderr << "ERROR, no such host" << endl;
    exit(0);
  }
  bzero((char *) &ServerAddress, sizeof(ServerAddress));
  ServerAddress.sin_family = AF_INET;
  bcopy((char *)Server->h_addr, (char *)&ServerAddress.sin_addr.s_addr, Server->h_length);
  ServerAddress.sin_port = htons(PortNumber);
  if (connect(SockFD, (struct sockaddr *) & ServerAddress, sizeof(ServerAddress)) < 0)
    error("ERROR connecting");
  cout << "Connected to server of host " << HostName << " with port number " << PortNumber << endl;
}

std::string Client::readx() {

  char buffer[256];
  bzero(buffer, 256);
  n = read(SockFD, buffer, 255);
  if (n < 0)
    error("ERROR reading from socket");
  return string(buffer);
}

void Client::writex(std::string msg) {

  n = write(SockFD, msg.c_str(), msg.size());
  if (n < 0)
    error("ERROR writing to socket");
}

void Client::error(string msg) {

  perror(msg.c_str());
  exit(1);
}