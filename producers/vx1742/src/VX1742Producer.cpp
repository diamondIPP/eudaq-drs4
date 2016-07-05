/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework
** 
**
** <VX1742Producer>.cpp
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

//wishlist:
//software trigger
//trigger on signal
//calibration
//send info in bore event!
//give channels names!



#include "VX1742Producer.hh"
#include "VX1742Interface.hh"
#include "VX1742Event.hh"
#include "VX1742DEFS.hh"


#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"

#include <unistd.h> //usleep


static const std::string EVENT_TYPE = "VX1742";

VX1742Producer::VX1742Producer(const std::string & name, const std::string & runcontrol, const std::string & verbosity)
:eudaq::Producer(name, runcontrol),
  m_producerName(name),
  m_event_type(EVENT_TYPE),
  m_ev(0), 
  m_run(0), 
  m_running(false){

  try{
    caen = new VX1742Interface();
    caen->openVME();
  }

  catch (...){
    EUDAQ_ERROR(std::string("Error in the VX1742Producer class constructor."));
    SetStatus(eudaq::Status::LVL_ERROR, "Error in the VX1742Producer class constructor.");}
}




void VX1742Producer::OnConfigure(const eudaq::Configuration& conf) {
  std::cout << "###Configure VX1742 board with: " << conf.Name() << "..";

  m_config = conf;
  
  try{
    sampling_frequency = conf.Get("sampling_frequency", 0);
    post_trigger_samples = conf.Get("post_trigger_samples", 0);
    trigger_source = conf.Get("trigger_source", 0);
    groups[0] = conf.Get("group0", 0);
    groups[1] = conf.Get("group1", 0);
    groups[2] = conf.Get("group2", 0);
    groups[3] = conf.Get("group3", 0);
    custom_size = conf.Get("custom_size", 0);
    m_group_mask = (groups[3]<<3) + (groups[2]<<2) + (groups[1]<<1) + groups[0];

    trn_enable[0] = conf.Get("TR01_enable", 0);
    trn_enable[1] = conf.Get("TR23_enable", 0);
    trn_threshold[0] = conf.Get("TR01_threshold", 0x51C6);
    trn_threshold[1] = conf.Get("TR23_threshold", 0x51C6);
    trn_offset[0] = conf.Get("TR01_offset", 0x8000);
    trn_offset[1] = conf.Get("TR23_offset", 0x8000);
    trn_polarity = conf.Get("TRn_polarity", 1);


    if(caen->isRunning())
      caen->stopAcquisition();

    caen->softwareReset();
    usleep(1000000);

    caen->setSamplingFrequency(sampling_frequency);
    caen->setPostTriggerSamples(post_trigger_samples);
    caen->setTriggerSource(trigger_source);
    caen->toggleGroups(groups);
    caen->setCustomSize(custom_size);
    caen->sendBusyToTRGout();
    caen->setTriggerCount(); //count just accepted triggers
    caen->disableIndividualTriggers(); //count one event only once, not per group
    usleep(10000);
    caen->enableTRn(trn_enable, trn_threshold, trn_offset, trn_polarity);

    caen->initializeDRS4CorrectionTables(sampling_frequency);
    usleep(200000);
    caen->printDRS4CorrectionTables();


  std::cout << " [OK]" << std::endl;

  
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + conf.Name() +")");

  }catch ( ... ){
  EUDAQ_ERROR(std::string("Error in the VX1742 configuration procedure."));
   SetStatus(eudaq::Status::LVL_ERROR, "Error in the VX1742 configuration procedure.");}
}




void VX1742Producer::OnStartRun(unsigned runnumber){
  m_run = runnumber;
  m_ev = 0;
  try{
    this->SetTimeStamp();

    //create BORE
    std::cout<<"VX1742: Create " << m_event_type << "BORE EVENT for run " << m_run <<  " @time: " << m_timestamp << "." << std::endl;
    eudaq::RawDataEvent bore(eudaq::RawDataEvent::BORE(m_event_type, m_run));
    bore.SetTag("timestamp", m_timestamp); //unit64_t
    bore.SetTag("serial_number", caen->getSerialNumber()); //std::string
    bore.SetTag("firmware_version", caen->getFirmwareVersion()); //std::string
    uint32_t n_channels = caen->getActiveChannels();
    bore.SetTag("active_channels", n_channels);
    bore.SetTag("device_name", "VX1742");
    bore.SetTag("group_mask", m_group_mask); //uint32_t


    if(sampling_frequency==0) bore.SetTag("sampling_speed", 5);
    if(sampling_frequency==1) bore.SetTag("sampling_speed", 2.5);
    if(sampling_frequency==2) bore.SetTag("sampling_speed", 1);
    if(sampling_frequency==3) bore.SetTag("sampling_speed", 0);

    uint32_t samples_c = caen->getCustomSize();
    if(samples_c==0) bore.SetTag("samples_per_channel", 1024);
    if(samples_c==1) bore.SetTag("samples_per_channel", 520);
    if(samples_c==2) bore.SetTag("samples_per_channel", 256);
    if(samples_c==3) bore.SetTag("samples_per_channel", 136);

    //fixme - offset for groups other than 0
    for(int ch=0; ch < n_channels; ch++){
      std::string conf_ch = "CH_"+std::to_string(ch);
      std::string ch_name = m_config.Get(conf_ch, conf_ch);
      bore.SetTag(conf_ch, ch_name);
    }

    //bore.SetTag("voltage_range", 1);

    //time_calibration
    usleep(2000000);

    caen->clearBuffers();
    caen->startAcquisition();
    //caen->printAcquisitionStatus();
    //caen->printAcquisitionControl();


    SendEvent(bore);

    SetStatus(eudaq::Status::LVL_OK, "Running");
    m_running = true;
  }
  catch (...){
  EUDAQ_ERROR(std::string("Error in the VX1742 OnStartRun procedure."));
  SetStatus(eudaq::Status::LVL_ERROR, "Error in the VX1742 OnStartRun procedure.");}
}



void VX1742Producer::ReadoutLoop() {
  while(!m_terminated){
    if(!m_running){
      sched_yield();} //if not running deprioritize thread

  while(m_running){
    try{
      //std::cout << "Events stored: " << caen->getEventsStored() << ", size of next event: " << caen->getNextEventSize() << std::endl;
      //usleep(500000);
      if(caen->eventReady()){
        VX1742Event vxEvent;
        caen->BlockTransferEventD64(&vxEvent);

        if(vxEvent.isValid()){
          unsigned int event_counter = vxEvent.EventCounter();
          uint32_t trigger_time_tag = vxEvent.TriggerTimeTag();
          uint32_t n_groups = vxEvent.Groups();
          uint32_t group_mask = vxEvent.GroupMask();
          uint32_t event_size = vxEvent.EventSize();

          //fix first two redneck events
          //if(n_groups==0){
          //  group_mask = m_group_mask;
          //}

          uint32_t block_no = 0;
          eudaq::RawDataEvent ev(m_event_type, m_run, event_counter);          

          ev.AddBlock(block_no, static_cast<const uint32_t*>(&event_size), sizeof(event_size));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&n_groups), sizeof(n_groups));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&group_mask), sizeof(group_mask));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&trigger_time_tag), sizeof(trigger_time_tag));
          block_no++;


          //loop over all groups
          for(uint32_t grp = 0; grp < 4; grp++){
            
            if(group_mask & (1<<grp)){ 

              uint32_t samples_per_channel = vxEvent.SamplesPerChannel(grp);
              uint32_t start_index_cell = vxEvent.GetStartIndexCell(grp);
              uint32_t event_timestamp = vxEvent.GetEventTimeStamp(grp);

              //fix first two redneck events
              //if(n_groups==0){
              //  samples_per_channel = this->SamplesInCustomSize();
              //  start_index_cell = 0;
              //  event_timestamp = 0;
              //}

              #ifdef DEBUG
                std::cout << "***********************************************************************" << std::endl << std::endl;
                std::cout << "Event number: " << event_counter << std::endl;
                std::cout << "Event size: " << event_size << std::endl;
                std::cout << "Groups enabled: " << n_groups << std::endl;
                std::cout << "Group mask: " << group_mask << std::endl;
                std::cout << "Trigger time tag: " << trigger_time_tag << std::endl;
                std::cout << "Samples per channel: " << samples_per_channel << std::endl;
                std::cout << "Start index cell : " << start_index_cell << std::endl;
                std::cout << "Event trigger time tag: " << event_timestamp << std::endl;
                std::cout << "***********************************************************************" << std::endl << std::endl; 
              #endif            

              ev.AddBlock(block_no, static_cast<const uint32_t*>(&samples_per_channel), sizeof(samples_per_channel));
              block_no++;
              ev.AddBlock(block_no, static_cast<const uint32_t*>(&start_index_cell), sizeof(start_index_cell));
              block_no++;
              ev.AddBlock(block_no, static_cast<const uint32_t*>(&event_timestamp), sizeof(event_timestamp));
              block_no++;

              for(u_int ch = 0; ch < 8; ch++){
                uint16_t *payload = new uint16_t[samples_per_channel];
                  
                //first two sucker events have no channel data for whatever reason which then fucks up the OnlineMonitor so just send zeros
                //if(n_groups == 0){
                //  for(int idx = 0; idx<samples_per_channel; idx++){
                //    payload[idx] = 0;
                //  }                   
                //}else{
                vxEvent.getChannelData(grp, ch, payload, samples_per_channel);
                //}
                ev.AddBlock(block_no, reinterpret_cast<const char*>(payload), samples_per_channel*sizeof(uint16_t));
                block_no++;
                delete payload;
              }//end loop over channels
            }//end if
          }//end for
  
          SendEvent(ev);
          
        }// is valid
      }// event is ready

    }catch (...){
    EUDAQ_ERROR(std::string("Readout error................"));
    SetStatus(eudaq::Status::LVL_ERROR, "Readout error................");}

    }// while running
  }// while not terminated
}// ReadoutLoop



void VX1742Producer::OnStopRun(){
  m_running = false;
  //caen->printAcquisitionControl();
  caen->stopAcquisition();
  std::cout << "VX1742 run stopped." << std::endl;
}


void VX1742Producer::OnTerminate(){
  m_running = false;
  m_terminated = true;
  caen->closeVME();
  delete caen;
  std::cout << "VX1742 producer terminated." << std::endl; 
}


int main(int /*argc*/, const char ** argv){
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("VX1742 Producer", "0.0", "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
    "tcp://localhost:44000", "address", "The address of the RunControl.");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
    "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "VX1742", "string",
    "The name of this Producer");
  eudaq::Option<std::string> verbosity(op, "v", "verbosity mode", "INFO", "string");

  try{
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());
    // Create a producer
    VX1742Producer *producer = new VX1742Producer(name.Value(), rctrl.Value(), verbosity.Value());
    // And set it running...
    producer->ReadoutLoop();
    // When the readout loop terminates, it is time to go
    std::cout << "VX1742: Quitting" << std::endl;

  }
  catch(...){
    // This does some basic error handling of common exceptions
    return op.HandleMainException();}

  return 0;
}


void VX1742Producer::SetTimeStamp(){
  std::chrono::high_resolution_clock::time_point epoch;
  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = now - epoch;
  m_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()/100u;
}


//uint32_t VX1742Producer::SamplesInCustomSize(){
//  uint32_t csizes[4] = {1024, 520, 256, 136};
//  return csizes[custom_size];
//}