/** ---------------------------------------------------------------------------------
 *  Interface to several HV supplies
 *
 *  Date: 	April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

//HV includes
#include "HVProducer.hh"
#include "Constants.h"
#include "HVInterface.h"

//EUDAQ includes
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/RawDataEvent.hh"


static const std::string EVENT_TYPE = "HV";
using namespace std;

HVProducer::HVProducer(const std::string &name, const std::string &runcontrol, const std::string &verbosity): eudaq::Producer(name, runcontrol),
                       m_event_type(EVENT_TYPE), Done(false), HVStarted(false), m_ev(0), m_run(0) {

  DataClient = new Client(44444, "None");
}


void HVProducer::MainLoop(){
//	//variables for trigger-fallover check:
//	const unsigned int bit_28 = 268435456; //28bit counter
//	const unsigned int limit = bit_28/2;
//
  eudaq::print_banner("Starting HV Producer!");

	do{
//	  	if(!tc ){
//			eudaq::mSleep(500);
//			continue;}
//
//		if(TUJustStopped){
//			tc->enable(false);
//			eudaq::mSleep(1000);
//		}
//
//		if(TUStarted || TUJustStopped){
//
//
//			tuc::Readout_Data *rd;
//			rd = stream->timer_handler();
//			if (rd){
//
//				/******************************************** start get all the data ********************************************/
//				coincidence_count_no_sin = rd->coincidence_count_no_sin;
//				coincidence_count = rd->coincidence_count;
//				prescaler_count = rd->prescaler_count;
//				prescaler_count_xor_pulser_count = rd->prescaler_count_xor_pulser_count;
//				accepted_prescaled_events = rd->prescaler_xor_pulser_and_prescaler_delayed_count;
//				accepted_pulser_events = rd->pulser_delay_and_xor_pulser_count;
//				handshake_count = rd->handshake_count; //equivalent to event count
//				beam_current[0] = beam_current[1]; //save old beam current monitor count for frequency calculations
//				beam_current[1] = rd->beam_curent; //save new
//				time_stamps[0] = time_stamps[1]; //save old timestamp for frequency calculations
//				time_stamps[1] = rd->time_stamp; //save new
//
//				beam_curr = CorrectBeamCurrent(1000*(0.01*((beam_current[1]-beam_current[0])/(time_stamps[1] - time_stamps[0]))));
//				cal_beam_current = SlidingWindow(beam_curr);
//
//
//				for(int idx=0; idx<10; idx++){
//					//check if there was a fallover
//					unsigned int new_tc = rd->trigger_counts[idx]; //data from readout
//					unsigned int old_tc = trigger_counts[idx]%bit_28; //data from previous readout
//
//					//check for fallover
//					if (old_tc > limit && new_tc < limit){
//						trigger_counts_multiplicity[idx]++;
//					}
//
//					prev_trigger_counts[idx] = trigger_counts[idx];
//					trigger_counts[idx] = trigger_counts_multiplicity[idx]*bit_28 + new_tc;
//					//input_frequencies[idx] = 1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0]);
//
//					input_frequencies[idx] = (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0]));
//					avg_input_frequencies[idx] = this->ScalerDeque(idx, (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0])));
//					//std::cout << "sending idx: " << idx << " rate: " << (1000*(trigger_counts[idx] - prev_trigger_counts[idx])/(time_stamps[1] - time_stamps[0])) << std::endl;
//				}
//				/******************************************** end get all the data ********************************************/
//
//				//check if eventnumber has changed since last readout:
//				if(handshake_count != prev_handshake_count){
//
//					#ifdef DEBUG
//                        std::cout << "************************************************************************************" << std::endl;
//                        std::cout << "time stamp: " << time_stamps[1] << std::endl;
//                        std::cout << "coincidence_count: " << coincidence_count << std::endl;
//                        std::cout << "coincidence_count_no_sin: " << coincidence_count_no_sin << std::endl;
//                        std::cout << "prescaler_count: " << prescaler_count << std::endl;
//                        std::cout << "prescaler_count_xor_pulser_count: " << prescaler_count_xor_pulser_count << std::endl;
//                        std::cout << "accepted_prescaled_events: " << accepted_prescaled_events << std::endl;
//                        std::cout << "accepted_pulser_events: " << accepted_pulser_events << std::endl;
//                        std::cout << "handshake_count: " << handshake_count << std::endl;
//                        std::cout << "cal_beam_current: " << cal_beam_current << std::endl;
//                        std::cout << "************************************************************************************" << std::endl;
//					#endif
//
//					//send fake events for events we are missing out between readout cycles
//					if(handshake_count > m_ev_prev){
//						int tmp =0;
//						for (unsigned int id = m_ev_prev; id < handshake_count; id++){
//							eudaq::RawDataEvent f_ev(event_type, m_run, id);
//							f_ev.SetTag("valid", std::to_string(0));
//							SendEvent(f_ev);
//							tmp++;
//							//std::cout << "----- fake event " << id << " sent." << std::endl;
//						}
//					}
//
//					uint64_t ts = time_stamps[1];
//					m_ev = handshake_count;
//
//					//real data event
//					//std::cout << "m_run: " << m_run << std::endl;
//					eudaq::RawDataEvent ev(event_type, ((unsigned int) m_run), m_ev); //generate a raw event
//					ev.SetTag("valid", std::to_string(1));
//
//        			unsigned int block_no = 0;
//        			ev.AddBlock(block_no, static_cast<const uint64_t*>(&ts), sizeof(uint64_t)); //timestamp
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&coincidence_count), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&coincidence_count_no_sin), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&prescaler_count), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&prescaler_count_xor_pulser_count), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&accepted_prescaled_events), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&accepted_pulser_events), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const uint32_t*>(&handshake_count), sizeof(uint32_t));
//        			block_no++;
//        			ev.AddBlock(block_no, static_cast<const float*>(&beam_curr), sizeof(float));
//        			block_no++;
//
//        			//also send individual event scalers
//        			unsigned long *cnts = trigger_counts;
//        			ev.AddBlock(block_no, reinterpret_cast<const char*>(cnts), 10*sizeof(uint64_t));
//
//        			SendEvent(ev);
//
//        			m_ev_prev = m_ev+1;
//					prev_handshake_count = handshake_count;
//					std::cout << "Event number TUProducer: " << handshake_count << std::endl;
//
//				}//end if (prev event count)
//			}//end if(rd)
//			eudaq::mSleep(500); //only read out every 1/2 second
//
//		//std::cout << BOLDRED << "TUProducer::MainLoop: One event readout returned nothing!" << CLEAR;
//
//		}//end if(TUStarted)
//
//
//		if(TUJustStopped){
//		    SendEvent(eudaq::RawDataEvent::EORE(event_type, m_run, m_ev));
//			OnStatus();
//			TUJustStopped = false;
//		}
	}while (!Done);
}




void HVProducer::OnStartRun(unsigned run_nr) {

	try{
		SetStatus(eudaq::Status::LVL_OK, "Wait");
		std::cout << "--> Starting HV Run " << run_nr << std::endl;

    m_run = run_nr;
    m_ev = 0;
		eudaq::RawDataEvent ev(eudaq::RawDataEvent::BORE(m_event_type, m_run));
		ev.SetTag("Names", eudaq::to_string(getNames()));
		ev.SetTag("Biases", eudaq::to_string(getBiases()));
		SendEvent(ev);

		cout << "Sent BORE event." << endl;

		OnReset();
		HVStarted = true;
		SetStatus(eudaq::Status::LVL_OK, "Started");
	}catch(...){
		std::cout << BOLDRED << "HVProducer::OnStartRun: Could not start" << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
	}
}



void HVProducer::OnStopRun(){
  try{
	  cout << "--> Stopping HV Run." << endl;
    HVStarted = false;
    OnReset();
    SetStatus(eudaq::Status::LVL_OK, "Stopped");
	} catch (...) {
    std::cout << BOLDRED << "HVProducer::OnStopRun: Could not stop ..." << CLEAR;
    SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
  }
}



void HVProducer::OnTerminate(){

  try {
    EUDAQ_INFO("Terminating HVProducer");
    for (auto client: Clients){
      delete client->Interface;
      delete client;
    }
    Done = true;
    eudaq::mSleep(1000);
    SetStatus(eudaq::Status::LVL_OK);
  } catch(...){
    cout << BOLDRED << "HVProducer::OnTerminate: Could not terminate Producer." << CLEAR;
    SetStatus(eudaq::Status::LVL_ERROR, "Terminate Error");
  }
}



void HVProducer::OnReset(){

  try{
    EUDAQ_INFO("Resetting HV Clients");
    SetStatus(eudaq::Status::LVL_OK);
  }catch(...){
    std::cout << BOLDRED << "HVProducer::OnReset: Could not reset Producer." << CLEAR;
    SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
  }
}



void HVProducer::OnStatus(){

  m_status.SetTag("STATUS", "OK");
}


void HVProducer::OnConfigure(const eudaq::Configuration& conf) {

  if (DataClient->getHostName() == "None"){
    DataClient->setHostName(conf.Get("host_name", "daq"));
    DataClient->init();
  }
  std::cout << BOLDGREEN << "HVProducer::OnConfigure" << CLEAR;
  Clients.clear();
	try {
		SetStatus(eudaq::Status::LVL_OK, "Wait");
    vector<uint16_t> device_numbers = eudaq::stovec(conf.Get("devices", "1"), uint16_t(1));
    for (auto nr: device_numbers)
      Clients.push_back(new HVClient(nr, true, conf));
    NClients = Clients.size();
		SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf.Name() + ")");
    cout << BOLDGREEN <<  "Configuring HV devices with settings file (" << conf.Name() << ") done." << CLEAR;
	} catch (...){
		std::cout << BOLDRED << "HVProducer::OnConfigure: Could not connect to HV Clients, try again." << CLEAR;
		SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
	}
}

std::vector<std::string> HVProducer::getNames() {

  vector<string> tmp;
  for (auto client: Clients){
    for (auto ch: client->getActiveChannels())
      tmp.push_back(client->Interface->Name + "_" + eudaq::to_string(ch));
  }
  return tmp;
}

std::vector<float> HVProducer::getBiases() {

  vector<float> tmp;
  for (auto client: Clients)
    for (auto bias: client->getTargetBias())
      tmp.push_back(bias);
  return tmp;
}

void HVProducer::sendRawEvent() {

  eudaq::RawDataEvent event(m_event_type, m_run, m_ev);
  event.SetTag("valid", true);
  uint32_t block_nr = 0;
//  event.AddBlock(block_nr, static_cast<const uint64_t*>(&), sizeof( trigger_cell));

}

void HVProducer::sendFakeEvent() {

  /**send fake events for events we are missing out between readout cycles */
  eudaq::RawDataEvent fake_event(m_event_type, m_run, m_ev);
  fake_event.SetTag("valid", false);
  SendEvent(fake_event);
}




int main(int /*argc*/, const char ** argv) {
	//FIXME: get rid of useless options here and implement new ones
	eudaq::OptionParser op("EUDAQ TU Producer", "1.0", "The Producer for the Ohio State trigger unit.");
	eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
	eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
	eudaq::Option<std::string> name (op, "n", "name", "HV", "string", "The name of this Producer");
	eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");
	try {
		op.Parse(argv);
		EUDAQ_LOG_LEVEL(level.Value());
		//FIXME option parser missing
    HVProducer producer(name.Value(), rctrl.Value(), verbosity.Value());
		producer.MainLoop();
		std::cout << "Quitting" << std::endl;
		eudaq::mSleep(300);
	} catch (...) {
		return op.HandleMainException();
	}
	return 0;
}