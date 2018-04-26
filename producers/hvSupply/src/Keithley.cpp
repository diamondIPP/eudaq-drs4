/** ---------------------------------------------------------------------------------
 *  Mother class of all Keithley Devices
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#include "Keithley.h"
#include <cstring>
#include <fcntl.h>      // File control definitions
#include <unistd.h>     // UNIX standard function definitions

using namespace std;
using namespace eudaq;

Keithley::Keithley(const uint16_t device_nr, const bool hot_start, const Configuration & conf): HVInterface(device_nr, hot_start, conf), ReadSleepTime(200),
                                                                                                       WriteSleepTime(100), EndChar("\r\n") {

  Producer = "Keithley";
  Baudrates["57600"] = B57600;
  Baudrates["9600"] = B9600;
  Baudrate = Baudrates[conf.Get("baudrate", "")];
  SerialPortName = conf.Get("address", "");

  openSerialPort();
  clearBuffer();
  setModelName();
  setMaxVoltage();
  clearErrorQueue();

}

void Keithley::setOutput(const string &status) {

  cout << "Set Output of " + Name + " to " + status << endl;
  Status.at(0) = status;
  writeCmd(":OUTP " + status);
}

bool Keithley::openSerialPort() {

  USB = open(("/dev/" + SerialPortName).c_str(), O_RDWR | O_NONBLOCK | O_NDELAY);
  if ( USB < 0 ) {
    cout << "Error " << errno << " opening " << "/dev/" + SerialPortName << ": " << strerror (errno) << endl;
    return false;
  }
  memset (&Serial, 0, sizeof Serial);
  
  cfsetospeed (&Serial, Baudrate);
  cfsetispeed (&Serial, Baudrate);
  Serial.c_cflag     &=  ~PARENB;                         // Make 8n1
  Serial.c_cflag     &=  ~CSTOPB;
  Serial.c_cflag     &=  ~CSIZE;
  Serial.c_cflag     |=  CS8;
  Serial.c_cflag     &=  ~CRTSCTS;                        // no flow control
  Serial.c_lflag     =   0;                               // no signaling chars, no echo, no canonical processing
  Serial.c_oflag     =   0;                               // no remapping, no delays
  Serial.c_cc[VMIN]  =   0;                               // read doesn't block
  Serial.c_cc[VTIME] =   5;                               // 0.5 seconds read timeout
  Serial.c_cflag     |=  CREAD | CLOCAL;                  // turn on READ & ignore ctrl lines
  Serial.c_iflag     &=  ~(IXON | IXOFF | IXANY);         // turn off s/w flow ctrl
  Serial.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  Serial.c_oflag     &=  ~OPOST;                          // make raw
  tcflush(USB, TCIFLUSH);
  eudaq::mSleep(300);

  if (tcsetattr(USB, TCSANOW, &Serial) != 0) {
    cout << "Error " << errno << " from tcsetattr" << endl;
    return false;
  }
  return true;
}

ssize_t Keithley::writeCmd(std::string command) {

  eudaq::mSleep(WriteSleepTime);
  string cmd = command + EndChar;
  return write(USB, cmd.c_str(), cmd.size());
}

string Keithley::readOut() {

  eudaq::mSleep(ReadSleepTime);
  char buf [256];
  memset (&buf, '\0', sizeof buf);

  ssize_t n = read(USB, &buf , sizeof buf);
  if (n < 0) {
//    cout << "Error reading: " << strerror(errno) << endl;
    throw "Error reading: " + string(strerror(errno));
  }
  return string(buf);
}

std::string Keithley::query(std::string cmd) {

  writeCmd(cmd);
  return readOut();
}

void Keithley::setModelName() {

  string id = identify();
  vector<string> bla = split(id, ",");
  for (const auto &i:bla)
    if (i.find("MODEL") != string::npos)
      Name = Producer + " " + split(i, " ").at(1);
  print_banner("Connected to device " + to_string(DeviceNumber) + ": " + Name);
}

vector<pair<float, float>> Keithley::readIV() {

  try{
    vector<float> tmp;
    for (const auto &str: eudaq::split(query(":READ?"), ","))
      tmp.push_back(stof(str));
    return {make_pair(tmp.at(0), tmp.at(1))};
  } catch (...){
    return {make_pair(INVALID, INVALID)};
  }
}
