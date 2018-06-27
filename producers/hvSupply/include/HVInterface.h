/** ---------------------------------------------------------------------------------
 *  Mother class of all HV Devices
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef EUDAQ_HVINTERFACE_H
#define EUDAQ_HVINTERFACE_H

#include <cstdint>
#include <string>
#include "eudaq/Configuration.hh"
#include "Constants.h"

class HVInterface{

public:
  HVInterface(const uint16_t, const bool, const eudaq::Configuration &);
  std::vector<uint16_t> ActiveChannels;
  size_t NChannels;
  bool HotStart;
  uint16_t DeviceNumber;
  uint16_t MaxVoltage;

  std::string Name;
  std::string ModelNumber;
  std::string Producer;
  std::string SectionName;

  bool CanRamp;
  std::vector<std::string> Status;
  const eudaq::Configuration & Config;

  virtual void setOutput(const std::string &) {};
  void setOn() { setOutput(ON); }
  void setOff() { setOutput(OFF); }
  void setMaxVoltage();
  float validateVoltage(float voltage);
  virtual std::vector<std::pair<float, float>> readIV() { std::cout << "BLA" << std::endl; };
  virtual void updateStatus() {};
  std::vector<float> readBias();

private:
  void init();
  ssize_t writeCmd(std::string);
  std::string readOut();
  std::string query(std::string);


};


#endif //EUDAQ_HVINTERFACE_H
