/** ---------------------------------------------------------------------------------
 *  Class to handle Keithley 2400 and 2410 devices
 *
 *  Date: 	April 24th 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef EUDAQ_KEITHLEY24XX_H
#define EUDAQ_KEITHLEY24XX_H

#include "Keithley.h"

class Keithley24XX: public Keithley {

public:
  Keithley24XX(const uint16_t, const bool, const eudaq::Configuration &);

  void setComplianceAbort(std::string);
  void setSourceOutput();
  void setAverageFilterCount(uint16_t);
  void setConcurrentMeasurement(std::string);
  void setCompliance(float current)         { writeCmd(":SENS:CURR:PROT " + Config.Get("compliance", eudaq::to_string(current))); }
  void setFilterType(std::string type)      { writeCmd(":SENS:AVER:TCON " + type); }
  void setSourceOutput(std::string out)     { writeCmd(":ROUT:TERM " + out); }
  void setBias(float voltage)               { writeCmd(":SOUR:VOLT "      + eudaq::to_string(validateVoltage(voltage))); }
  void setBeeper(std::string status)        { writeCmd(":SYST:BEEP:STAT " + status) ;}
  void setVoltageMode(std::string mode)     { writeCmd(":SOUR:VOLT:MODE " + mode) ;}
  void setAverageFilter(std::string status) { writeCmd(":SENS:AVER:STAT " + status) ;}

};

#endif //EUDAQ_KEITHLEY24XX_H
