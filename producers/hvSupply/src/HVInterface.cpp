/** ---------------------------------------------------------------------------------
 *  Mother class of all HV Devices
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/


#include <Constants.h>
#include "HVInterface.h"

using namespace std;
using namespace eudaq;

HVInterface::HVInterface(const uint16_t device_nr, const bool hot_start, const eudaq::Configuration & conf): DeviceNumber(device_nr), HotStart(hot_start), Config(conf) {

  SectionName = "HV" + eudaq::to_string(DeviceNumber);
  ActiveChannels = stovec(conf.Get("active_channels", "[0]"), uint16_t(0));
  NChannels = ActiveChannels.size();
  Status.resize(NChannels, OFF);
}

void HVInterface::setMaxVoltage() { MaxVoltage = hvc::MaxVoltages.at(Name); }

float HVInterface::validateVoltage(float voltage) {

  if (fabs(voltage) > MaxVoltage){
    cout << BOLDRED << "Setting Voltage to maximum allowed " << to_string(MaxVoltage) << CLEAR << endl;
    return MaxVoltage * (signbit(voltage) ? -1 : 1);
  }
  return voltage;
}

vector<float> HVInterface::readBias() {

  vector<float> tmp = {INVALID};
  while (tmp.at(0) == INVALID){
    tmp.clear();
    for (auto iv: readIV())
      tmp.push_back(iv.second);
  }
  return tmp;
}