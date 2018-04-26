/** ---------------------------------------------------------------------------------
 *  Main class that initiates the HV supplies
 *
 *  Date: 	April 24th 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef EUDAQ_HVDEVICE_H
#define EUDAQ_HVDEVICE_H

#include <vector>
#include <string>

class HVInterface;
class Configuration;

class HVClient {

public:
  HVClient(const uint16_t, const bool, const eudaq::Configuration &);

  std::string ModelNumber;

  bool IsKilled;
  bool IsBusy;
  bool HotStart;
  HVInterface * Interface;
  /** VALUES */
  std::vector<float> Bias;
  std::vector<float> Current;

  std::vector<std::pair<float, float>> readIV();
  std::vector<uint16_t> getActiveChannels() { return ActiveChannels; }
  std::vector<float> getTargetBias() { return TargetBias; }

private:
  /** CHANNELS */
  std::vector<uint16_t> ActiveChannels;
  size_t NChannels;
  /** CONFIG */
  float RampingSpeed;
  uint16_t MaxStep;
  std::vector<float> TargetBias;
  std::vector<float> MaxBias;


  bool initInterface(const uint16_t, const bool, const eudaq::Configuration &);
  void hotStart();

};

#endif //EUDAQ_HVDEVICE_H
