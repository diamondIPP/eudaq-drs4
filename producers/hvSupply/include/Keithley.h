/** ---------------------------------------------------------------------------------
 *  Mother class of all Keithley Devices
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef EUDAQ_KEITHLEY_H
#define EUDAQ_KEITHLEY_H

#include "HVInterface.h"
#include "termios.h"
#include "deque"
#include <map>
#include "Constants.h"


class Keithley: public HVInterface {

public:
  Keithley(const uint16_t, const bool, const eudaq::Configuration &);
  bool IsOpen;
  std::string SerialPortName;
  const uint16_t WriteSleepTime;
  const uint16_t ReadSleepTime;
  std::map<std::string, speed_t> Baudrates;
  speed_t Baudrate;
  std::string EndChar;
  std::deque<float> data;
  float LastVoltage;
  struct termios Serial;
  int32_t USB;
  std::string Producer;
  void setOutput(const std::string &);
  ssize_t writeCmd(std::string);
  std::string readOut();
  std::string query(std::string);

  void setModelName();

  void updateStatus()                     { Status.at(0) = stoi(query(":OUTP?")) > 0 ? ON : OFF; }
  void clearBuffer()                      { writeCmd(":TRAC:CLEAR"); }
  void clearErrorQueue()                  { writeCmd("*CLS"); }
  void reset()                            { writeCmd("*RST"); }
  std::string identify()                  { return query("*IDN?"); }
  void setMeasurementSpeed(float value)   { writeCmd(":SENS:CURR:NPLC " + eudaq::to_string(value)); }
  void setOutputFormat(std::string form)  { writeCmd(":FORM:ELEM " + form); }

  std::vector<std::pair<float, float>> readIV();

private:
  bool openSerialPort();

};

#endif //EUDAQ_KEITHLEY_H
