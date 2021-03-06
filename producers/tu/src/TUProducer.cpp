/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUProducer>.cpp
** 
** Date: March 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

//TU includes
#include "TUProducer.hh"
#include "trigger_control.h"
#include "TUDEFS.h"

//EUDAQ includes
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"

//System Includes
#include <iostream>
#include <string>
#include <cstdint>

#define BOLDRED "\33[1m\033[31m"
#define BOLDGREEN "\33[1m\033[32m"
#define CLEAR "\033[0m\n"
#define BIT28 268435456
#define LIMIT BIT28 / 2

using namespace std;

static const std::string EVENT_TYPE = "TU";

TUProducer::TUProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity):
  eudaq::Producer(name, runcontrol),
  event_type(EVENT_TYPE),
	m_run(0),
	m_event(999, 0),
	done(false),
	TUStarted(false),
	TUJustStopped(false),
	TUConfigured(false),
	tc(nullptr),
	stream(nullptr),
	beam_current_scaler(0, 0),
	coincidence_count(0, 0),
	handshake_count(0, 0),
	average_beam_current(0),
	average_coincidence_rate(0),
	average_handshake_rate(0),
	n_scaler(10),
  time_stamps(0, 0) {

  try{
    std::fill_n(trigger_counts, 10, 0);
    std::fill_n(prev_trigger_counts, 10, 0);
		std::fill_n(input_frequencies, 10, 0);
		std::fill_n(avg_input_frequencies, 10, 0);
    std::fill_n(trigger_counts_multiplicity, 10, 0);
//    error_code = 0;
    scaler_deques.resize(n_scaler);

    tc = new trigger_control(); //does not talk to the box
    stream = new Trigger_logic_tpc_Stream(); //neither this

    //TUServer = new Server(8889);
  } catch (exception & e) {
    std::cout << BOLDRED << "TUProducer::" << e.what() << CLEAR;
    EUDAQ_ERROR(std::string("Error in the TUProducer class constructor."));
    SetStatus(eudaq::Status::LVL_ERROR, "Error in the TUProducer class constructor.");
  }
}


void TUProducer::MainLoop(){
	//variables for trigger-fallover check:
	const unsigned int bit_28 = 268435456; //28bit counter
	const unsigned int limit = bit_28/2;

	do{
    if (tc == nullptr) {
      eudaq::mSleep(500);
      continue;}

    if (TUJustStopped) {
      tc->enable(false);
      eudaq::mSleep(1000);
    }
    if (TUConfigured and not TUStarted){
      tuc::Readout_Data *rd;
      rd = stream->timer_handler();
      time_stamps = std::make_pair(time_stamps.second, rd->time_stamp); /** move old and save new */
      UpdateBeamCurrent(rd);
      UpdateScaler(rd);
      eudaq::mSleep(100); //update faster if it's not running
    }

    else if (TUStarted or TUJustStopped) {
      tuc::Readout_Data *rd;
      rd = stream->timer_handler();
      if (rd != nullptr){

        /******************************************** start get all the data ********************************************/
				coincidence_count_no_sin = rd->coincidence_count_no_sin;
				coincidence_count = make_pair(coincidence_count.second, rd->coincidence_count); /** move old and save new */
        handshake_count = make_pair(handshake_count.second, rd->handshake_count); /** move old and save new. equivalent to event count */
				prescaler_count = rd->prescaler_count;
				prescaler_count_xor_pulser_count = rd->prescaler_count_xor_pulser_count;
				accepted_prescaled_events = rd->prescaler_xor_pulser_and_prescaler_delayed_count;
				accepted_pulser_events = rd->pulser_delay_and_xor_pulser_count;
				time_stamps = std::make_pair(time_stamps.second, rd->time_stamp); /** move old and save new */

				average_coincidence_rate = unsigned(round(CalculateAverage(coincidence_rate, (coincidence_count.second - coincidence_count.first) / TimeDiff())));
				average_handshake_rate = unsigned(round(CalculateAverage(handshake_rate, (handshake_count.second - handshake_count.first) / TimeDiff())));

				UpdateBeamCurrent(rd);
				UpdateScaler(rd);
        /******************************************** end get all the data ********************************************/

        /** check if event number has changed since last readout: */
        if (handshake_count.second != handshake_count.first){
          #ifdef DEBUG
          std::cout << "************************************************************************************" << std::endl;
                        std::cout << "time stamp: " << time_stamps[1] << std::endl;
                        std::cout << "coincidence_count: " << coincidence_count << std::endl;
                        std::cout << "coincidence_count_no_sin: " << coincidence_count_no_sin << std::endl;
                        std::cout << "prescaler_count: " << prescaler_count << std::endl;
                        std::cout << "prescaler_count_xor_pulser_count: " << prescaler_count_xor_pulser_count << std::endl;
                        std::cout << "accepted_prescaled_events: " << accepted_prescaled_events << std::endl;
                        std::cout << "accepted_pulser_events: " << accepted_pulser_events << std::endl;
                        std::cout << "handshake_count: " << handshake_count << std::endl;
                        std::cout << "cal_beam_current: " << cal_beam_current << std::endl;
                        std::cout << "************************************************************************************" << std::endl;
          #endif
          /** send fake events for events we are missing out between readout cycles */
          if (handshake_count.second > m_event.first) {
            for (unsigned int id = m_event.first; id < handshake_count.second; id++){
              eudaq::RawDataEvent f_ev(event_type, m_run, id);
              f_ev.SetTag("valid", std::to_string(0));
              SendEvent(f_ev);
            }
          }

          uint64_t ts = time_stamps.second;
          m_event.second = handshake_count.second;
          //TUServer->writex(eudaq::to_string(m_ev));

          //real data event
          //std::cout << "m_run: " << m_run << std::endl;
          eudaq::RawDataEvent ev(event_type, ((unsigned int) m_run), m_event.second); //generate a raw event
          ev.SetTag("valid", std::to_string(1));

          unsigned int block_no = 0;
          ev.AddBlock(block_no++, static_cast<const uint64_t*>(&ts), sizeof(uint64_t)); //timestamp
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&coincidence_count.second), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&coincidence_count_no_sin), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&prescaler_count), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&prescaler_count_xor_pulser_count), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&accepted_prescaled_events), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&accepted_pulser_events), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const uint32_t*>(&handshake_count.second), sizeof(uint32_t));
          ev.AddBlock(block_no++, static_cast<const float*>(&beam_current_now), sizeof(float));
          ev.AddBlock(block_no, reinterpret_cast<const char*>(&trigger_counts), 10 * sizeof(uint64_t));
          SendEvent(ev);

          m_event.first = m_event.second + 1;
          cout << "TU events: " << handshake_count.second << endl;

        } //end if (prev event count)
      } //end if(rd)
      eudaq::mSleep(500); //only read out every 1/2 second

    } //end if(TUStarted)

    if(TUJustStopped){
      SendEvent(eudaq::RawDataEvent::EORE(event_type, m_run, m_event.second));
      OnStatus();
      TUJustStopped = false;
    }
	} while (!done);
}

void TUProducer::OnStartRun(unsigned run_nr) {

  try{
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    cout << "--> Starting TU Run " << run_nr << endl;
    eudaq::mSleep(5000);
    m_run = run_nr;
    m_event.second = 0;
    eudaq::RawDataEvent ev(eudaq::RawDataEvent::BORE(event_type, m_run));
    ev.SetTag("FirmwareID", "not given"); //FIXME!
    ev.SetTag("TriggerMask", "0x" + eudaq::to_hex(trg_mask));
    ev.SetTimeStampToNow();
    SendEvent(ev);
    cout << "Sent BORE event." << endl;

    OnReset();
    TUStarted = true;
    if (tc->enable(true) != 0) { throw(tu_program_exception("Could not enable.")); }
    EUDAQ_INFO("Triggers are now accepted");
    SetStatus(eudaq::Status::LVL_OK, "Started");
  } catch(exception & e) {
    cout << BOLDRED << "TUProducer::OnStartRun: Could not start TU Producer. " << e.what() << CLEAR << endl;
    SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
  }
}

void TUProducer::OnStopRun(){

  cout << "--> Stopping TU Run." << endl;
  try{
    if (tc->enable(false) != 0) {throw(tu_program_exception("Could not disable."));}
    TUStarted = false;
    TUJustStopped = true;
    eudaq::mSleep(2000);
    OnReset();
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
    OnStatus();
  } catch(exception & e) {
    std::cout << BOLDRED << "TUProducer::OnStopRun: Could not stop TU Producer. " << e.what() << CLEAR << endl;
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}

void TUProducer::OnTerminate(){

  try{
    cout << "--> Terminate" << endl;
    if (tc->enable(false) != 0) { throw(tu_program_exception("Could not disable.")); }
    if (tc->reset_counts() != 0) { throw(tu_program_exception("Could not reset counts.")); }
    done = true;
    delete tc; //clean up
    delete stream;
    //delete TUServer;
    eudaq::mSleep(1000);
  } catch(exception & e) {
    cout << BOLDRED << "TUProducer::OnTerminate: Could not terminate TU Producer. " << e.what() << CLEAR << endl;
    SetStatus(eudaq::Status::LVL_ERROR, "Terminate Error");
  }
}

void TUProducer::OnReset(){
  try{
    std::cout << "--> Resetting TU." << std::endl;
    if (tc->reset_counts() != 0) { throw(tu_program_exception()); }
    std::fill_n(trigger_counts, n_scaler, 0);
    std::fill_n(prev_trigger_counts, n_scaler, 0);
    std::fill_n(input_frequencies, n_scaler, 0);
    std::fill_n(trigger_counts_multiplicity, n_scaler, 0);
    std::fill_n(avg_input_frequencies, n_scaler, 0);
    time_stamps = make_pair(0, 0);
    beam_current_scaler = make_pair(0, 0);
    coincidence_count = make_pair(0, 0);
    handshake_count = make_pair(99999, 0);
    coincidence_count_no_sin = 0;
    prescaler_count = 0;
    prescaler_count_xor_pulser_count = 0;
    accepted_pulser_events = 0;
    m_event.first = 0;
    average_beam_current = 0;
    average_coincidence_rate = 0;
    beam_currents.clear();
    coincidence_rate.clear();
    handshake_rate.clear();
    for (auto i_scaler: scaler_deques) {
      i_scaler.clear();
    }
    SetStatus(eudaq::Status::LVL_OK);
  } catch(...){
    std::cout << BOLDRED << "TUProducer::OnReset: Could not reset TU Producer." << CLEAR;
    SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
  }
}

void TUProducer::OnStatus(){

  if (TUStarted && handshake_count.second > 0) {
    m_status.SetTag("HANDSHAKE_COUNT", to_string(handshake_count.second));
  } else {
    m_status.SetTag("TRIG", std::to_string(0));
  }

	if (tc != nullptr){
	  m_status.SetTag("COINC_RATE", to_string(average_coincidence_rate));
	  m_status.SetTag("HANDSHAKE_RATE", to_string(average_handshake_rate));
		m_status.SetTag("TIMESTAMP", std::to_string(time_stamps.second / 1000.0));
		m_status.SetTag("LASTTIME", std::to_string(time_stamps.first / 1000.0));
    m_status.SetTag("STATUS", TUStarted ? "OK" : "IDLE");
		for (int i = 0; i < n_scaler; ++i) {
			m_status.SetTag("SCALER" + std::to_string(i), std::to_string(avg_input_frequencies[i]));
		}
    m_status.SetTag("BEAM_CURR", TUConfigured ? std::to_string(average_beam_current) : "");
	}
}

void TUProducer::OnConfigure(const eudaq::Configuration& conf) {

	try {
		SetStatus(eudaq::Status::LVL_OK, "Wait");

		std::string ip_adr = conf.Get("ip_adr", "192.168.1.120");
		tc->set_ip_adr(ip_adr);
		const char *ip = ip_adr.c_str();
		if(tc->enable(false) != 0){throw(-1);}
		stream->set_ip_adr(ip);
    std::cout << "Opening connection to TU @ " << ip << " ..";
    if(stream->open() != 0){throw(-1);}
    std::cout << BOLDGREEN << " [OK] " << CLEAR;

    std::cout << "Configuring (" << conf.Name() << "), please wait." << std::endl;
    //enabling/disabling and getting&setting delays for scintillator and planes 1-8 (same order in array)
    std::cout << "--> Setting delays for scintillator, planes 1-8 and pad...";
		tc->set_scintillator_delay(conf.Get("scintillator_delay", 100));
		for (auto i_plane(0); i_plane < tc->n_planes; i_plane++){
		  std::string key = "plane" + to_string(i_plane + 1) + "del";
		  tc->set_plane_delay(i_plane, conf.Get(key, 100));
		}
		tc->set_pad_delay(conf.Get("pad_delay", 100));
		if(tc->set_delays() != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

  		//generate and set a trigger mask
  		std::cout << "--> Generate and set trigger mask..";
  		trg_mask = conf.Get("pad", 0);
  		for (unsigned int idx=8; idx>0; idx--){
  			std::string sname = "plane" + std::to_string(idx);
  			int tmp = conf.Get(sname, 0);
  			trg_mask = (trg_mask<<1)+tmp;
  		}

  		trg_mask = (trg_mask<<1)+conf.Get("scintillator", 0);
  		//std::cout << "Debug: Trigger mask: " << trg_mask << std::endl;
  		if(tc->set_coincidence_enable(trg_mask) != 0){throw(-1);}
  		std::cout << BOLDGREEN << " [OK] " << CLEAR;


  		std::cout << "--> Setting prescaler and delay.." << std::endl;
		int scal = conf.Get("prescaler", 1);
		int predel = conf.Get("prescaler_delay", 5); //must be >4
		EUDAQ_INFO("Setting a prescaler of " + to_string(scal) + " with a delay of " + to_string(predel));
		if(tc->set_prescaler(scal) != 0){throw(-1);}
		if(tc->set_prescaler_delay(predel) != 0){throw(-1);}
		std::cout << "      Prescaler: " << scal << std::endl;
		std::cout << "      Prescaler Delay: " << predel << std::endl;
		std::cout << BOLDGREEN << "... [OK] " << CLEAR;


		std::cout << "--> Setting pulser frequency, width, delay and polarity.. " << std::endl;
		double freq = conf.Get("pulser_freq", 0);
		int width = conf.Get("pulser_width", 0);
		int puldel = conf.Get("pulser_delay", 5); //must be > 4
		bool pol_pulser1 = conf.Get("pulser1_polarity", false);
		bool pol_pulser2 = conf.Get("pulser2_polarity", true);
		std::cout << "     Frequency: " << freq << std::endl;
		std::cout << "     Width: " << width << std::endl;
		std::cout << "     Delay: " << freq << std::endl;
		std::cout << "     Pulser 1/2 Polarities: " << pol_pulser1 << "/" << pol_pulser2 << " (0=negative/1=positive)" << std::endl;

    if (tc->set_pulser(freq, width) != 0) { throw(tu_program_exception("Could not set pulser frequency or pulse width!")); }
		if (tc->set_pulser_delay(puldel) != 0) { throw(tu_program_exception("Could not set pulser delay!")); }
		if (tc->set_pulser_polarity(pol_pulser1, pol_pulser2) != 0) { throw(tu_program_exception("Could not set pulser polarities!")); }
		std::cout << BOLDGREEN << "... [OK] " << CLEAR;


		std::cout << "--> Setting coincidence pulse and edge width..";
		int copwidth = conf.Get("coincidence_pulse_width", 10);
		int coewidth = conf.Get("coincidence_edge_width", 10);
		if(tc->set_coincidence_pulse_width(copwidth) != 0){throw(-1);}
		if(tc->set_coincidence_edge_width(coewidth) != 0){throw(-1);}
		if(tc->send_coincidence_edge_width() != 0){throw(-1);}
		if(tc->send_coincidence_pulse_width() != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

		cout << "--> Setting handshake settings.." << endl;
		int hs_del = conf.Get("handshake_delay", 0);
		if(tc->set_handshake_delay(hs_del) != 0){throw(-1);}

		int hs_mask = conf.Get("handshake_mask", 0);
		if(tc->set_handshake_mask(hs_mask) != 0) { throw(tu_program_exception("Invalid handshake mask!")); }
    cout << "     mask:  " << hs_mask << endl;
    cout << "     delay: " << int(hs_del * 2.5) << " ..." << BOLDGREEN << " [OK] " << CLEAR;
		EUDAQ_INFO("Setting handshake mask to " + to_string(hs_mask) + " with delay of " + to_string(hs_del * 2.5));

		std::cout << "--> Set trigger delays ... ";
		int trigdel1 = conf.Get("trig_1_delay", 40);
		int trigdel2 = conf.Get("trig_2_delay", 40);
    unsigned short trigdel3 = conf.Get("trig_3_delay", 40);
    if (tc->set_trigger_12_delay((trigdel2 << 12) | trigdel1) != 0) { throw(tu_program_exception("Trigger 1/2 Delay")); }
		if (tc->set_trigger_3_delay(trigdel3) != 0) { throw(tu_program_exception("Trigger 3 Delay")); }
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

		cout << "--> Set 40MHz clock phases ... " << endl;
		int clk40_phase1 = conf.Get("clk40_phase1", 0);
		int clk40_phase2 = conf.Get("clk40_phase2", 0);
		cout << "     CLK 1 Phase: " << clk40_phase1 << " [clk cycles]" << endl;
		cout << "     CLK 2 Phase: " << clk40_phase2 << " [clk cycles]" << endl;
    if (tc->set_clk40_phases(clk40_phase1, clk40_phase2) !=0 ) { throw(tu_program_exception("Clock Phases")); }
    cout << BOLDGREEN << "... [OK] " << CLEAR;


		//set current UNIX timestamp
		std::cout << "--> Set current UNIX time to TU..";
    if (tc->set_time() != 0) { throw(tu_program_exception("Could not set time!")); }
		eudaq::mSleep(1000);
		std::cout << BOLDGREEN << " [OK] " << CLEAR << std::endl;

		float delay_width = 2.5;
		std::cout << "################### Readback ###################" << std::endl;
		std::cout << "Scintillator delay [ns]: " << tc->get_scintillator_delay() * delay_width << std::endl;
		for (auto i_delay(0); i_delay < tc->n_planes; i_delay++) {
		  std::cout << "Plane " << i_delay + 1 << " delay [ns]: " << tc->get_plane_delay(i_delay) * delay_width << std::endl;
		}
		std::cout << "Pad delay [ns]: " << tc->get_pad_delay() * delay_width << std::endl << std::endl;

		std::cout << "Coincidence pulse width [ns]: " << tc->get_coincidence_pulse_width() << std::endl;
		std::cout << "Coincidence edge width [ns]: " << tc->get_coincidence_edge_width() << std::endl;

		std::cout << "CLK 1 Phase: [ns]: " << tc->get_clk40_phase1() * delay_width << std::endl;
		std::cout << "CLK 2 Phase: [ns]: " << tc->get_clk40_phase2() * delay_width << std::endl;

		std::string pol1 = ((tc->get_pulser_polarities() & 0b01) != 0) ? "positive" : "negative";
		std::string pol2 = ((tc->get_pulser_polarities() & 0b10) != 0) ? "positive" : "negative";
		std::cout << "Pulser 1 Polarity: " << pol1 << std::endl;
		std::cout << "Pulser 2 Polarity: " << pol2 << std::endl;
		auto trigger_delays = tc->get_trigger_delays();
		for (auto i_trig(0); i_trig < trigger_delays.size(); i_trig++) {
		    cout << "Trigger " << i_trig + 1 << " Delay: " << trigger_delays.at(i_trig) << endl;
		}

		std::cout << BOLDGREEN <<  "--> ##### Configuring TU with settings file (" << conf.Name() << ") done. #####" << CLEAR;

		SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf.Name() + ")");
		TUConfigured = true;

	} catch (...){
		std::cout << BOLDRED << "TUProducer::OnConfigure: Could not connect to TU with ip " << tc->get_ip_adr() << ", try again." << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
	}
}

/** calculate the average of the provided deque */
template <typename Q>
float TUProducer::CalculateAverage(std::deque<Q> & d, Q value, bool execute, unsigned short max_size){

  if (coincidence_count.second > 1 or handshake_count.second > 1 or execute) { d.push_back(value); } /** we need at least two events to calculate the rates */
  if (d.size() > max_size) { d.pop_front(); } /** keep max_size values in the deque */
  return std::accumulate(d.begin(), d.end(), 0.0) / d.size();
}

/** values from laboratory measurement with pulser */
float TUProducer::CalculateBeamCurrent(){
  float rate = 10 * ((beam_current_scaler.second - beam_current_scaler.first) / float(time_stamps.second - time_stamps.first));
	return rate > 0 ? float(3.01077 + 1.06746 * rate) : 0;
}

void TUProducer::UpdateBeamCurrent(tuc::Readout_Data * rd) {
  beam_current_scaler = make_pair(beam_current_scaler.second, rd->beam_curent); /** move old and save new */
  beam_current_now = CalculateBeamCurrent();
  average_beam_current = CalculateAverage(beam_currents, beam_current_now, true);
}

void TUProducer::UpdateScaler(tuc::Readout_Data * rd){
  for (auto idx(0); idx < n_scaler; idx++) {
    unsigned int new_tc = rd->trigger_counts[idx]; //data from readout
    unsigned int old_tc = trigger_counts[idx] % BIT28; //data from previous readout
    if (old_tc > LIMIT && new_tc < LIMIT) { trigger_counts_multiplicity[idx]++; } //check for fall over

    prev_trigger_counts[idx] = trigger_counts[idx];
    trigger_counts[idx] = trigger_counts_multiplicity[idx] * BIT28 + new_tc;

    input_frequencies[idx] = unsigned(round((trigger_counts[idx] - prev_trigger_counts[idx]) / TimeDiff()));
    avg_input_frequencies[idx] = unsigned(round(CalculateAverage(scaler_deques.at(idx), input_frequencies[idx], true)));
  }
}


int main(int /*argc*/, const char ** argv) {
	//FIXME: get rid of useless options here and implement new ones
	eudaq::OptionParser op("EUDAQ TU Producer", "1.0", "The Producer for the Ohio State trigger unit.");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
	eudaq::Option<std::string> name (op, "n", "name", "TU", "string", "The name of this Producer");
	eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());
		//FIXME option parser missing
		TUProducer producer(name.Value(), rctrl.Value(), verbosity.Value());
		producer.MainLoop();
		std::cout << "Quitting" << std::endl;
		eudaq::mSleep(300);
	} catch (...) {
		return op.HandleMainException();
	}
	return 0;
}
