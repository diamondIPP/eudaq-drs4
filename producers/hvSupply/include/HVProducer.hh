/** ---------------------------------------------------------------------------------
 *  Interface to several HV supplies
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/


#ifndef HVPRODUCER_HH
#define HVPRODUCER_HH

//Readout_Data struct defined here:
#include "eudaq/Producer.hh"
#include <deque>
#include <vector>
#include "HVClient.h"

class Configuration;
class trigger_controll;


class HVProducer:public eudaq::Producer{

public:
	HVProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity);
	virtual void OnConfigure(const eudaq::Configuration &conf);
	virtual void MainLoop();
	virtual void OnStartRun(unsigned param);
	virtual void OnStopRun();
	virtual void OnTerminate();
	virtual void OnReset();
	virtual void OnStatus();
  bool Done;
  void sendRawEvent();
  void sendFakeEvent();

private:
  bool HVStarted;
  uint32_t m_ev, m_run;
	std::string m_event_type;
  std::vector<HVClient *> Clients;
  size_t NClients;
  std::vector<std::string> getNames();
  std::vector<float> getBiases();
};

int main(int /*argc*/, const char ** argv);

#endif