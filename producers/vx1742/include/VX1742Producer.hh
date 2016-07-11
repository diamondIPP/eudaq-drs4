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
  void CAENPeakCorrection(uint32_t channels, uint32_t nsamples);
  void CAENTimeCorrection(uint32_t grp, uint32_t channels, uint32_t nsamples, uint32_t freq, uint32_t st_index);


private:
  VX1742Interface *caen;

  //config values
  uint32_t sampling_frequency;
  uint32_t post_trigger_samples;
  uint32_t trigger_source;
  uint32_t active_groups;
  uint32_t groups[4];
  uint32_t custom_size;

  uint32_t cell_offset;
  uint32_t index_sampling;
  uint32_t time_correction;
  int16_t cell_corr[36][1024];
  int8_t index_corr[36][1024];
  float time_corr[4][1024];
  uint16_t wf_storage[9][1024]; //for peak correction according to CAEN

  uint32_t trn_enable[2];
  uint32_t trn_threshold[2];
  uint32_t trn_offset[2];
  uint32_t trn_polarity;
  uint32_t trn_readout;

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