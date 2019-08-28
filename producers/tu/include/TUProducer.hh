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

class trigger_control;


class TUProducer:public eudaq::Producer{

public:
	TUProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity);
	void OnConfigure(const eudaq::Configuration &conf) override;
	virtual void MainLoop();
	void OnStartRun(unsigned param) override;
	void OnStopRun() override;
	void OnTerminate() override;
	void OnReset() override;
	void OnStatus() override;
	template <typename Q>
	float CalculateAverage(std::deque<Q> & d, Q value, bool=false, unsigned short max_size=10);
	void UpdateBeamCurrent(tuc::Readout_Data*);
	void UpdateScaler(tuc::Readout_Data*);
	float CalculateBeamCurrent();
	float TimeDiff() { return (time_stamps.second - time_stamps.first) / 1000.; } /** time difference in seconds */

private:
	std::string event_type;
	unsigned m_run;
	std::pair<unsigned, unsigned> m_event;
	bool done, TUStarted, TUJustStopped, TUConfigured;
	trigger_control *tc; //class for TU control from trigger_control.h
	Trigger_logic_tpc_Stream *stream; //class for handling communication from triger_logic_tpc_stream.h
//	unsigned int error_code;
	int trg_mask;
	float beam_current_now;
	std::deque<float> beam_currents;
  std::deque<float> coincidence_rate;
  std::deque<float> handshake_rate;
  float average_beam_current;
  unsigned average_coincidence_rate;
  unsigned average_handshake_rate;
  unsigned short n_scaler;
  std::vector<std::deque<unsigned>> scaler_deques;

	/** TU DATA */
	unsigned long trigger_counts[10];
	unsigned int prev_trigger_counts[10];
	unsigned int input_frequencies[10];
	unsigned int avg_input_frequencies[10];
	unsigned int trigger_counts_multiplicity[10];
	unsigned int coincidence_count_no_sin;
	std::pair<unsigned, unsigned> beam_current_scaler; //first entry = old, second entry = new
	std::pair<unsigned, unsigned> coincidence_count; //first entry = old, second entry = new
	std::pair<unsigned, unsigned> handshake_count; //first entry = old, second entry = new
	std::pair<unsigned long, unsigned long> time_stamps; //first entry = old, second entry = new
	unsigned int prescaler_count;
	unsigned int prescaler_count_xor_pulser_count;
	unsigned int accepted_pulser_events;
	unsigned int accepted_prescaled_events;
};

int main(int /*argc*/, const char ** argv);

#endif