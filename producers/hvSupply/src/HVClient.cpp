/** ---------------------------------------------------------------------------------
 *  Main class that initiates the HV supplies
 *
 *  Date: 	April 24th 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#include <eudaq/Configuration.hh>
#include "HVClient.h"
#include "Keithley24XX.h"

using namespace std;
using namespace eudaq;

HVClient::HVClient(const uint16_t device_number, const bool hot_start, const eudaq::Configuration & conf): IsKilled(false), HotStart(hot_start) {

  conf.SetSection("HV" + eudaq::to_string(device_number));
  ModelNumber = conf.Get("model", "None");
  initInterface(device_number, hot_start, conf);

  RampingSpeed = conf.Get("ramp", float(1.0));  // [V/s]
  MaxStep = conf.Get("max_step", uint16_t(2));
  ActiveChannels = Interface->ActiveChannels;
  NChannels = Interface->NChannels;
  TargetBias = stovec(conf.Get("bias", "0"), float(0));

  IsBusy = false;
  if (HotStart)
    hotStart();
}

bool HVClient::initInterface(const uint16_t device_nr, const bool hot_start, const eudaq::Configuration & conf) {

  if (ModelNumber == "2400" or ModelNumber == "2410")
    Interface = new Keithley24XX(device_nr, hot_start, conf);

}

void HVClient::hotStart() {

  Interface->updateStatus();
  TargetBias = Interface->readBias();
}

vector<pair<float, float>> HVClient::readIV() { return Interface->readIV(); }