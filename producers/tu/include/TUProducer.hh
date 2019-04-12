/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUProducer>.hh
** 
** Date: March 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#ifndef TUPRODUCER_HH
#define TUPRODUCER_HH

//Readout_Data struct defined here:
#include "eudaq/Producer.hh"
#include "trigger_logic_tpc_stream.h"
#include "Server.h"
#include <deque>
#include <vector>

class Configuration;
class trigger_controll;


class TUProducer:public eudaq::Producer{
public:
	TUProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity);
	virtual void OnConfigure(const eudaq::Configuration &conf);
	virtual void MainLoop();
	virtual void OnStartRun(unsigned param);
	virtual void OnStopRun();
	virtual void OnTerminate();
	virtual void OnReset();
	virtual void OnStatus();
	float SlidingWindow(float);
	unsigned ScalerDeque(unsigned scaler_nr, unsigned rate);
	float CalculateBeamCurrent();


private:
	std::string event_type;
	unsigned int m_run, m_ev, m_ev_prev, prev_handshake_count; //run & event number
	bool done, TUStarted, TUJustStopped;
	trigger_controll *tc; //class for TU control from trigger_controll.h
	Trigger_logic_tpc_Stream *stream; //class for handling communication from triger_logic_tpc_stream.h
	unsigned int error_code;
	int trg_mask;
	float beam_curr;
	float cal_beam_current;
	std::deque<float> avg;
	std::vector<std::deque<unsigned>> scaler_deques;

	//data read back from TU
	unsigned long trigger_counts[10];
	unsigned int prev_trigger_counts[10];
	unsigned int input_frequencies[10];
	unsigned int avg_input_frequencies[10];
	unsigned int trigger_counts_multiplicity[10];
	unsigned int coincidence_count_no_sin;
	unsigned int coincidence_count;
	unsigned int beam_current[2]; //first entry = old, second entry = new
	unsigned int prescaler_count;
	unsigned int prescaler_count_xor_pulser_count;
	unsigned int accepted_pulser_events;
	unsigned int accepted_prescaled_events;
	unsigned int handshake_count;
	unsigned long time_stamps[2]; //first entry = old, second entry = new timestamp

protected:
	Server * TUServer;

};

int main(int /*argc*/, const char ** argv);

#endif