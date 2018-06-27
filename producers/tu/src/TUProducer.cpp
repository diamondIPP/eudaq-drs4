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
#include "trigger_controll.h"
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
#include <math.h>

#define BOLDRED "\33[1m\033[31m"
#define BOLDGREEN "\33[1m\033[32m"
#define CLEAR "\033[0m\n"

static const std::string EVENT_TYPE = "TU";

TUProducer::TUProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity):eudaq::Producer(name, runcontrol),
	event_type(EVENT_TYPE),
	m_run(0), m_ev(0),
	done(false), TUStarted(false), TUJustStopped(false),
	tc(NULL), stream(NULL){

  try{
    std::fill_n(trigger_counts, 10, 0);
    std::fill_n(prev_trigger_counts, 10, 0);
		std::fill_n(input_frequencies, 10, 0);
		std::fill_n(avg_input_frequencies, 10, 0);
    std::fill_n(trigger_counts_multiplicity, 10, 0);
    std::fill_n(time_stamps, 2, 0);
    std::fill_n(beam_current, 2, 0);
    avg.assign(20,0); //allow the average for the beam current to hold 20 values
    error_code = 0;

    //average for the scaler inputs - 10 values here
    scaler_deques.resize(10);
    for (auto i = scaler_deques.begin(); i != scaler_deques.end(); i++)
        i->assign(20,0);

    tc = new trigger_controll(); //does not talk to the box
    stream = new Trigger_logic_tpc_Stream(); //neither this
    if (tc->enable(false) != 0){throw(-1);}

    TUServer = new Server(44444);
  }catch (...){
    std::cout << BOLDRED << "TUProducer::TUProducer: Could not connect to TU!" << CLEAR;
    EUDAQ_ERROR(std::string("Error in the TUProducer class constructor."));
    SetStatus(eudaq::Status::LVL_ERROR, "Error in the TUProducer class constructor.");
  }
}


void TUProducer::MainLoop(){
	//variables for trigger-fallover check:
	const unsigned int bit_28 = 268435456; //28bit counter
	const unsigned int limit = bit_28/2;

	do{
	  	if(!tc){
			eudaq::mSleep(500);
			continue;}

		if(TUJustStopped){
			tc->enable(false);
			eudaq::mSleep(1000);
		}
			    	
		if(TUStarted || TUJustStopped){
			

			tuc::Readout_Data *rd;
			rd = stream->timer_handler();
			if (rd){

				/******************************************** start get all the data ********************************************/
				coincidence_count_no_sin = rd->coincidence_count_no_sin;
				coincidence_count = rd->coincidence_count;
				prescaler_count = rd->prescaler_count;
				prescaler_count_xor_pulser_count = rd->prescaler_count_xor_pulser_count;
				accepted_prescaled_events = rd->prescaler_xor_pulser_and_prescaler_delayed_count;
				accepted_pulser_events = rd->pulser_delay_and_xor_pulser_count;
				handshake_count = rd->handshake_count; //equivalent to event count
				beam_current[0] = beam_current[1]; //save old beam current monitor count for frequency calculations
				beam_current[1] = rd->beam_curent; //save new
				time_stamps[0] = time_stamps[1]; //save old timestamp for frequency calculations
				time_stamps[1] = rd->time_stamp; //save new

				beam_curr = CorrectBeamCurrent(1000*(0.01*((beam_current[1]-beam_current[0])/(time_stamps[1] - time_stamps[0]))));
				cal_beam_current = SlidingWindow(beam_curr);
				

				for(int idx=0; idx<10; idx++){
					//check if there was a fallover
					unsigned int new_tc = rd->trigger_counts[idx]; //data from readout
					unsigned int old_tc = trigger_counts[idx]%bit_28; //data from previous readout	

					//check for fallover
					if (old_tc > limit && new_tc < limit){
						trigger_counts_multiplicity[idx]++;
					}

					prev_trigger_counts[idx] = trigger_counts[idx];
					trigger_counts[idx] = trigger_counts_multiplicity[idx]*bit_28 + new_tc;
					//input_frequencies[idx] = 1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0]);

					input_frequencies[idx] = (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0]));
					avg_input_frequencies[idx] = this->ScalerDeque(idx, (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0])));
					//std::cout << "sending idx: " << idx << " rate: " << (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0])) << std::endl;
				}
				/******************************************** end get all the data ********************************************/

				//check if eventnumber has changed since last readout:
				if(handshake_count != prev_handshake_count){
				
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
					
					//send fake events for events we are missing out between readout cycles
					if(handshake_count > m_ev_prev){
						int tmp =0;
						for (unsigned int id = m_ev_prev; id < handshake_count; id++){
							eudaq::RawDataEvent f_ev(event_type, m_run, id);
							f_ev.SetTag("valid", std::to_string(0));
							SendEvent(f_ev);
							tmp++;
							//std::cout << "----- fake event " << id << " sent." << std::endl;
						}
					}

					uint64_t ts = time_stamps[1];
					m_ev = handshake_count;
					
					//real data event
					//std::cout << "m_run: " << m_run << std::endl;
					eudaq::RawDataEvent ev(event_type, ((unsigned int) m_run), m_ev); //generate a raw event
					ev.SetTag("valid", std::to_string(1));

        			unsigned int block_no = 0;
        			ev.AddBlock(block_no, static_cast<const uint64_t*>(&ts), sizeof(uint64_t)); //timestamp
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&coincidence_count), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&coincidence_count_no_sin), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&prescaler_count), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&prescaler_count_xor_pulser_count), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&accepted_prescaled_events), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&accepted_pulser_events), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&handshake_count), sizeof(uint32_t));
        			block_no++;
        			ev.AddBlock(block_no, static_cast<const float*>(&beam_curr), sizeof(float));
        			block_no++;

        			//also send individual event scalers
        			unsigned long *cnts = trigger_counts;
        			ev.AddBlock(block_no, reinterpret_cast<const char*>(cnts), 10*sizeof(uint64_t));

        			SendEvent(ev);

        			m_ev_prev = m_ev+1;
					prev_handshake_count = handshake_count;
					std::cout << "Event number TUProducer: " << handshake_count << std::endl;
				
				}//end if (prev event count)
			}//end if(rd)
			eudaq::mSleep(500); //only read out every 1/2 second

		//std::cout << BOLDRED << "TUProducer::MainLoop: One event readout returned nothing!" << CLEAR;

		}//end if(TUStarted)


		if(TUJustStopped){
		    SendEvent(eudaq::RawDataEvent::EORE(event_type, m_run, m_ev));
			OnStatus();
			TUJustStopped = false;
		}
	}while (!done);
}




void TUProducer::OnStartRun(unsigned run_nr) {
	try{
		SetStatus(eudaq::Status::LVL_OK, "Wait");
		std::cout << "--> Starting TU Run " << run_nr << std::endl;
		eudaq::mSleep(5000);
		m_run = run_nr;
		m_ev = 0;

		

		eudaq::RawDataEvent ev(eudaq::RawDataEvent::BORE(event_type, m_run));
		ev.SetTag("FirmwareID", "not given"); //FIXME!
		ev.SetTag("TriggerMask", "0x" + eudaq::to_hex(trg_mask));
		ev.SetTimeStampToNow();
		SendEvent(ev);

		std::cout << "Sent BORE event." << std::endl;

		OnReset();
		TUStarted = true;
		if(tc->enable(true) != 0){throw(-1);}

        EUDAQ_INFO("Triggers are now accepted");
		SetStatus(eudaq::Status::LVL_OK, "Started");
	}catch(...){
		std::cout << BOLDRED << "TUProducer::OnStartRun: Could not start TU Producer." << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
	}
}



void TUProducer::OnStopRun(){
	std::cout << "--> Stopping TU Run." << std::endl;	
	try{
		if(tc->enable(false) != 0){throw(-1);}
		TUStarted = false;
		TUJustStopped = true;
		eudaq::mSleep(2000);
		OnReset();
		SetStatus(eudaq::Status::LVL_OK, "Stopped");

	} catch (...) {
	    std::cout << BOLDRED << "TUProducer::OnStopRun: Could not stop TU Producer." << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
		}
}



void TUProducer::OnTerminate(){
		try{
		std::cout << "--> Terminate (press enter)" << std::endl;
		if(tc->enable(false) != 0){throw(-1);}
		if(tc->reset_counts() != 0){throw(-1);}
		done = true;
		delete tc; //clean up
		delete stream;
		eudaq::mSleep(1000);
		}catch(...){
		    std::cout << BOLDRED << "TUProducer::OnTerminate: Could not terminate TU Producer." << CLEAR;
			SetStatus(eudaq::Status::LVL_ERROR, "Terminate Error");
		}
}



void TUProducer::OnReset(){
		try{
			std::cout << "--> Resetting TU." << std::endl;
			if(tc->reset_counts() != 0){throw(-1);}
			std::fill_n(trigger_counts, 10, 0);
			std::fill_n(prev_trigger_counts, 10, 0);
			std::fill_n(input_frequencies, 10, 0);
    		std::fill_n(trigger_counts_multiplicity, 10, 0);
    		std::fill_n(time_stamps, 2, 0);
    		std::fill_n(beam_current, 2, 0);
    		avg.assign(20,0);
			coincidence_count_no_sin = 0;
			coincidence_count = 0;
			cal_beam_current = 0;
			prescaler_count = 0;
			prescaler_count_xor_pulser_count = 0;
			accepted_pulser_events = 0;
			handshake_count = 0;
			prev_handshake_count = 99999;
			m_ev_prev = 0;
			SetStatus(eudaq::Status::LVL_OK);
		}catch(...){
		    std::cout << BOLDRED << "TUProducer::OnReset: Could not reset TU Producer." << CLEAR;
			SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
		}
}



void TUProducer::OnStatus(){
	if(TUStarted && handshake_count > 0)
		m_status.SetTag("COINC_COUNT", std::to_string(handshake_count));
	else 
		m_status.SetTag("TRIG", std::to_string(0));

	if (tc){
		m_status.SetTag("TIMESTAMP", std::to_string(time_stamps[1]/1000.0));
		m_status.SetTag("LASTTIME", std::to_string((time_stamps[0])/1000.0));

		if(TUStarted)
			m_status.SetTag("STATUS", "OK");
		else
			m_status.SetTag("STATUS", "NOT RUNNING");

		for (int i = 0; i < 10; ++i) {
			m_status.SetTag("SCALER" + std::to_string(i), std::to_string(avg_input_frequencies[i]));
		}

		if(cal_beam_current > 0)
			m_status.SetTag("BEAM_CURR", std::to_string(cal_beam_current));
		else
			m_status.SetTag("BEAM_CURR", std::to_string(0));
	}
}


void TUProducer::OnConfigure(const eudaq::Configuration& conf) {
	try {

		SetStatus(eudaq::Status::LVL_OK, "Wait");

		std::string ip_adr = conf.Get("ip_adr", "192.168.1.120");
		const char *ip = ip_adr.c_str();
		if(tc->enable(false) != 0){throw(-1);}


		stream->set_ip_adr(ip);
   		std::cout << "Opening connection to TU @ " << ip << " ..";
   		if(stream->open() != 0){throw(-1);}

   		std::cout << BOLDGREEN << " [OK] " << CLEAR;


   		std::cout << "Configuring (" << conf.Name() << "), please wait." << std::endl;			
  		//enabling/disabling and getting&setting delays for scintillator and planes 1-8 (same order in array)
  		std::cout << "--> Setting delays for scintillator, planes 1-8 and pad..";
		tc->set_scintillator_delay(conf.Get("scintillator_delay", 100));
		tc->set_plane_1_delay(conf.Get("plane1del", 100));
		tc->set_plane_2_delay(conf.Get("plane2del", 100));
		tc->set_plane_3_delay(conf.Get("plane3del", 100));
		tc->set_plane_4_delay(conf.Get("plane4del", 100));
		tc->set_plane_5_delay(conf.Get("plane5del", 100));
		tc->set_plane_6_delay(conf.Get("plane6del", 100));
		tc->set_plane_7_delay(conf.Get("plane7del", 100));
		tc->set_plane_8_delay(conf.Get("plane8del", 100));
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


  		std::cout << "--> Setting prescaler and delay..";
		int scal = conf.Get("prescaler", 1);
		int predel = conf.Get("prescaler_delay", 5); //must be >4
		if(tc->set_prescaler(scal) != 0){throw(-1);}
		if(tc->set_prescaler_delay(predel) != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;


		std::cout << "--> Setting pulser frequency, width and delay..";
		double freq = conf.Get("pulser_freq", 0);
		int width = conf.Get("pulser_width", 0);
		int puldel = conf.Get("pulser_delay", 5); //must be > 4

		if(tc->set_Pulser_width(freq, width) != 0){throw(-1);}
		if(tc->set_pulser_delay(puldel) != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;


		std::cout << "--> Setting coincidence pulse and edge width..";
		int copwidth = conf.Get("coincidence_pulse_width", 10);
		int coewidth = conf.Get("coincidence_edge_width", 10);
		if(tc->set_coincidence_pulse_width(copwidth) != 0){throw(-1);}
		if(tc->set_coincidence_edge_width(coewidth) != 0){throw(-1);}
		if(tc->send_coincidence_edge_width() != 0){throw(-1);}
		if(tc->send_coincidence_pulse_width() != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

		std::cout << "--> Setting handshake settings..";
		int hs_del = conf.Get("handshake_delay", 0);
		if(tc->set_handshake_delay(hs_del) != 0){throw(-1);}
		if(tc->send_handshake_delay() != 0){throw(-1);}

		int hs_mask = conf.Get("handshake_mask", 0);
		if(tc->set_handshake_mask(hs_mask) != 0){throw(-1);}
		if(tc->send_handshake_mask() != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

		std::cout << "--> Set trigger delays..";
		int trigdel1 = conf.Get("trig_1_delay", 100);
		int trigdel2 = conf.Get("trig_2_delay", 100);
		int trigdel12 = (trigdel1<<12) | trigdel2;
		if(tc->set_trigger_12_delay(trigdel12) != 0){throw(-1);}
		int trigdel3 = conf.Get("trig_3_delay", 100);
		if(tc->set_trigger_3_delay(trigdel3) != 0){throw(-1);}
		if(tc->set_delays() != 0){throw(-1);}
		std::cout << BOLDGREEN << " [OK] " << CLEAR;

		//set current UNIX timestamp
		std::cout << "--> Set current UNIX time to TU..";
   		if(tc->set_time() != 0){throw(-1);}
		eudaq::mSleep(1000);
		std::cout << BOLDGREEN << " [OK] " << CLEAR << std::endl;

		std::cout << "################### Readback ###################" << std::endl;
		std::cout << "Scintillator delay [ns]: " << tc->get_scintillator_delay()*2.5 << std::endl;
		std::cout << "Plane 1 delay [ns]: " << tc->get_plane_1_delay()*2.5 << std::endl;
		std::cout << "Plane 2 delay [ns]: " << tc->get_plane_2_delay()*2.5 << std::endl;
		std::cout << "Plane 3 delay [ns]: " << tc->get_plane_3_delay()*2.5 << std::endl;
		std::cout << "Plane 4 delay [ns]: " << tc->get_plane_4_delay()*2.5 << std::endl;
		std::cout << "Plane 5 delay [ns]: " << tc->get_plane_5_delay()*2.5 << std::endl;
		std::cout << "Plane 6 delay [ns]: " << tc->get_plane_6_delay()*2.5 << std::endl;
		std::cout << "Plane 7 delay [ns]: " << tc->get_plane_7_delay()*2.5 << std::endl;
		std::cout << "Plane 8 delay [ns]: " << tc->get_plane_8_delay()*2.5 << std::endl;
		std::cout << "Pad delay [ns]: " << tc->get_pad_delay()*2.5 << std::endl << std::endl;

		std::cout << "Handshake delay [ns]: " << tc->get_handshake_delay()*2.5 << std::endl;
		std::cout << "Handshake mask: " << tc->get_handshake_mask() << std::endl << std::endl;

		std::cout << "Coincidence pulse width [ns]: " << tc->get_coincidence_pulse_width() << std::endl;
		std::cout << "Coincidence edge width [ns]: " << tc->get_coincidence_edge_width() << std::endl << std::endl;

		std::cout << BOLDGREEN <<  "--> ##### Configuring TU with settings file (" << conf.Name() << ") done. #####" << CLEAR;

		SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf.Name() + ")");

	}catch (...){
		std::cout << BOLDRED << "TUProducer::OnConfigure: Could not connect to TU, try again." << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
	}
}



unsigned TUProducer::ScalerDeque(unsigned nr, unsigned rate){
	scaler_deques.at(nr).pop_back();
	scaler_deques.at(nr).push_front(rate);


	int len = 0;
	unsigned temp = 0;
	for (std::deque<unsigned>::iterator itr=scaler_deques.at(nr).begin(); itr!=scaler_deques.at(nr).end(); ++itr){
		if (*itr != 0){			
			temp += *itr;
			len++;
		}
	}
	if(len>0){
		return (temp/len);}
	return 0;
}


//for beam current, can be implemented in method above (fixme)
float TUProducer::SlidingWindow(float val){
	avg.pop_back();
	avg.push_front(val);
	
	int len = 0;
	float temp = 0;
	for (std::deque<float>::iterator itr=avg.begin(); itr!=avg.end(); ++itr){
		if (*itr != 0){			
			temp += *itr;
			len++;
		}
	}
	return (1.0*temp/len);
}

//values from laboratory measurement with pulser
float TUProducer::CorrectBeamCurrent(float uncorr){
	return (3.01077 + 1.06746*uncorr);
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