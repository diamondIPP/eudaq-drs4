/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework
** 
**
** <VX1742Producer>.hh
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#ifndef VX1742PRODUCER_HH
#define VX1742PRODUCER_HH

class VX1742Interface;

#include "eudaq/Producer.hh"
#include "eudaq/Configuration.hh"

class VX1742Producer: public eudaq::Producer{
public:
  VX1742Producer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  void OnConfigure(const eudaq::Configuration & config);
  void OnStartRun(unsigned runnumber);
  void OnStopRun();
  void OnTerminate();
  void SetTimeStamp();
  void ReadoutLoop();
  uint32_t SamplesInCustomSize();

private:
  VX1742Interface *caen;

  //config values
  u_int sampling_frequency;
  u_int post_trigger_samples;
  u_int trigger_source;
  u_int active_groups;
  u_int groups[4];
  u_int custom_size;

  std::string m_event_type, m_producerName;
  uint32_t m_ev;
  uint64_t m_timestamp;
  unsigned int m_run;
  bool m_running, m_terminated;
  uint32_t m_group_mask;
  eudaq::Configuration m_config;

};

int main(int /*argc*/, const char ** argv);

#endif