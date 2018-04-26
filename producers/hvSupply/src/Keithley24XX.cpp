/** ---------------------------------------------------------------------------------
 *  Class to handle Keithley 2400 and 2410 devices
 *
 *  Date: 	April 24th 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#include <Keithley24XX.h>
#include "eudaq/Logger.hh"


using namespace std;

Keithley24XX::Keithley24XX(const uint16_t device_nr, const bool hot_start, const eudaq::Configuration & conf): Keithley(device_nr, hot_start, conf) {


  if (not hot_start){
    setOutput(OFF);
    reset();
    setSourceOutput();
    setVoltageMode(FIXED);
    setOutputFormat("CURR,VOLT,RES,TIME,STAT");
    setConcurrentMeasurement(ON);
    setFilterType(REPEAT);
    setAverageFilter(ON);
    setAverageFilterCount(3);   // < 100
    setMeasurementSpeed(2);     // value in [0.01, 10]
    setCompliance(105e-6);      // argument is only default if none is provided in the config
    setComplianceAbort(EARLY);
  }
  setBeeper(ON);
  clearErrorQueue();
  setBias(150);
}

void Keithley24XX::setComplianceAbort(std::string level) {

  if (not eudaq::in(level, hvc::AbortLevels)) {
    EUDAQ_WARN("Could not set compliance abort level!");
    return;
  }
  writeCmd(":SOURCE:SWEEP:CABort " + level);
}

void Keithley24XX::setSourceOutput() {

  string out = Config.Get("output", "rear");
  std::transform(out.begin(), out.end(), out.begin(), ::toupper);
  setSourceOutput(out.find("FRONT") != string::npos ? FRONT : REAR);
}

void Keithley24XX::setAverageFilterCount(uint16_t count) {

  if (count > 100) {
    EUDAQ_WARN("Maximum trigger count is 100");
    count = 100;
  }
  writeCmd(":SENS:AVER:COUN " + to_string(count));
}

void Keithley24XX::setConcurrentMeasurement(std::string status) {

  writeCmd(":FUNC:CONC " + status);
  if (status == ON) {
    writeCmd(":SENS:FUNC \'VOLT:DC\'");
    writeCmd(":SENS:FUNC \'CURR:DC\'");
  }
}
