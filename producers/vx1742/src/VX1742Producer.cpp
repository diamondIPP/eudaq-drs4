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

  //initialize correction arrays
  for(uint32_t grp=0; grp<vmec::VX1742_GROUPS; grp++)
    std::fill_n(time_corr[grp], vmec::VX1742_MAX_SAMPLES, 0);

  for(uint32_t ch=0; ch<vmec::VX1742_CHANNELS; ch++){
    std::fill_n(index_corr[ch], vmec::VX1742_MAX_SAMPLES, 0);
    std::fill_n(cell_corr[ch], vmec::VX1742_MAX_SAMPLES, 0);
  }

  for(uint32_t ch=0; ch<vmec::VX1742_MAX_CHANNEL_SIZE; ch++)
    std::fill_n(wf_storage[ch], vmec::VX1742_MAX_SAMPLES,0);

  try{
    caen = new VX1742Interface();
    caen->openVME();
  }

  catch (...){
    EUDAQ_ERROR(std::string("Error in the VX1742Producer class constructor."));
    SetStatus(eudaq::Status::LVL_ERROR, "Error in the VX1742Producer class constructor.");}
}




void VX1742Producer::OnConfigure(const eudaq::Configuration& conf) {
  std::cout << "###Configure VX1742 board with: " << conf.Name() << "..\n";

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

    cell_offset = conf.Get("cell_offset", 0);
    index_sampling = conf.Get("index_sampling", 0);
    spike_correction = conf.Get("spike_correction", 0);

    trn_enable[0] = conf.Get("TR01_enable", 0);
    trn_enable[1] = conf.Get("TR23_enable", 0);
    trn_threshold[0] = conf.Get("TR01_threshold", 0x51C6);
    trn_threshold[1] = conf.Get("TR23_threshold", 0x51C6);
    trn_offset[0] = conf.Get("TR01_offset", 0x8000);
    trn_offset[1] = conf.Get("TR23_offset", 0x8000);
    trn_polarity = conf.Get("TRn_polarity", 1);
    trn_readout = conf.Get("TRn_readout", 1);


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
    caen->enableTRn(trn_enable, trn_threshold, trn_offset, trn_polarity, trn_readout);

    caen->initializeDRS4CorrectionTables(sampling_frequency);
    usleep(400000);

    //copy currently used correction tables
    for(uint32_t grp=0; grp<vmec::VX1742_GROUPS; grp++){
      //time correction values
      for(uint32_t idx=0; idx<vmec::VX1742_MAX_SAMPLES; idx++)
        time_corr[grp][idx] = caen->getTimingCorrectionValues(grp, sampling_frequency, idx);
      
      //load cell and index correction
      uint32_t ch_grp = vmec::VX1742_CHANNELS_PER_GROUP + trn_readout;
      for(uint32_t ch=0; ch<ch_grp; ch++){
        for(uint32_t idx=0; idx<vmec::VX1742_MAX_SAMPLES; idx++){
          cell_corr[ch_grp*grp+ch][idx] = caen->getCellCorrectionValues(grp, sampling_frequency, ch, idx);
          index_corr[ch_grp*grp+ch][idx] = caen->getNSamplesCorrectionValues(grp, sampling_frequency, ch, idx);
        }
      } 
    }

    //caen->printDRS4CorrectionTables();
    
    std::cout << "###Configuration [OK] - Ready to run!" << std::endl;
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
    uint32_t trn_readout = caen->TRnReadoutEnabled();
    uint32_t n_channels = caen->getActiveChannels();
    bore.SetTag("active_channels", n_channels);
    bore.SetTag("device_name", "VX1742");
    bore.SetTag("group_mask", m_group_mask); //uint32_t

    //fixme: set tags for time, index and sample correction

    if(sampling_frequency==0) bore.SetTag("sampling_speed", 5000);
    if(sampling_frequency==1) bore.SetTag("sampling_speed", 2500);
    if(sampling_frequency==2) bore.SetTag("sampling_speed", 1000);
    if(sampling_frequency==3) bore.SetTag("sampling_speed", 750);

    uint32_t samples_c = caen->getCustomSize();
    if(samples_c==0) bore.SetTag("samples_in_channel", 1024);
    if(samples_c==1) bore.SetTag("samples_in_channel", 520);
    if(samples_c==2) bore.SetTag("samples_in_channel", 256);
    if(samples_c==3) bore.SetTag("samples_in_channel", 136);

    //fixme - offset for groups other than 0
    for(int ch=0; ch < n_channels; ch++){
      std::string conf_ch = "CH_"+std::to_string(ch);
      std::string ch_name = m_config.Get(conf_ch, conf_ch);
      bore.SetTag(conf_ch, ch_name);
    }

    uint32_t block_no = 0;
    //sent calibration data
    for(uint32_t grp=0; grp<vmec::VX1742_GROUPS; grp++){
      bore.AddBlock(block_no, reinterpret_cast<const char*>(&time_corr[grp]), 1024*sizeof(float));
      block_no++;
    }
    for(uint32_t ch=0; ch<vmec::VX1742_CHANNELS; ch++){
      bore.AddBlock(block_no, reinterpret_cast<const char*>(&cell_corr[ch]), 1024*sizeof(int16_t));
      block_no++;
    }
    for(uint32_t ch=0; ch<vmec::VX1742_CHANNELS; ch++){
      bore.AddBlock(block_no, reinterpret_cast<const char*>(&index_corr[ch]), 1024*sizeof(int8_t));
      block_no++;
    }

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
          uint32_t event_trigger_time_tag = vxEvent.TriggerTimeTag();
          uint32_t n_groups = vxEvent.Groups();
          uint32_t group_mask = vxEvent.GroupMask();
          uint32_t event_size = vxEvent.EventSize();


          uint32_t block_no = 0;
          eudaq::RawDataEvent ev(m_event_type, m_run, event_counter);          

          ev.AddBlock(block_no, static_cast<const uint32_t*>(&event_size), sizeof(event_size));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&n_groups), sizeof(n_groups));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&group_mask), sizeof(group_mask));
          block_no++;
          ev.AddBlock(block_no, static_cast<const uint32_t*>(&event_trigger_time_tag), sizeof(event_trigger_time_tag));
          block_no++;


          //loop over all (activated) groups
          for(uint32_t grp = 0; grp < vmec::VX1742_GROUPS; grp++){
            if(group_mask & (1<<grp)){ 

              uint32_t samples_per_channel = vxEvent.SamplesPerChannel(grp);
              uint32_t start_index_cell = vxEvent.GetStartIndexCell(grp);
              uint32_t event_timestamp = vxEvent.GetEventTimeStamp(grp);
              uint32_t channels = vxEvent.Channels(grp);


              #ifdef DEBUG
                std::cout << "***********************************************************************" << std::endl << std::endl;
                std::cout << "Event number: " << event_counter << std::endl;
                std::cout << "Event size: " << event_size << std::endl;
                std::cout << "Groups enabled: " << n_groups << std::endl;
                std::cout << "Group mask: " << group_mask << std::endl;
                std::cout << "Trigger time tag: " << event_trigger_time_tag << std::endl;
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
              ev.AddBlock(block_no, static_cast<const uint32_t*>(&channels), sizeof(channels));
              block_no++;


              //apply cell and nsamples correction and store it to temporary array
              for(uint32_t ch = 0; ch < channels; ch++){
                uint16_t *payload = new uint16_t[samples_per_channel];
                vxEvent.getChannelData(grp, ch, payload, samples_per_channel);
                for(uint32_t i=0; i<samples_per_channel; i++){
                  wf_storage[ch][i] = (uint16_t)(((int16_t)payload[i]) - cell_corr[grp*channels+ch][(i+start_index_cell)%1024]*cell_offset + ((int16_t)index_corr[grp*channels+ch][i])*index_sampling);
                }
                delete payload;
              }  

              //uint16_t test[1024];
              //for(uint32_t id=0; id<1024; id++)
              //  test[id] = wf_storage[0][id];


              //CEAN spike correction
              if(spike_correction)
                this->CAENPeakCorrection(channels, samples_per_channel);

              //for(uint32_t id=0; id<1024; id++){
              //  if(wf_storage[0][id]-test[id] != 0)
              //    std::cout << "diff: " << wf_storage[0][id]-test[id] << std::endl;

              //}



              //copy waveforms back
              for(uint32_t ch = 0; ch < channels; ch++){
                uint16_t *payload = new uint16_t[samples_per_channel];
                for(uint32_t i=0; i<samples_per_channel; i++){
                  payload[i] = wf_storage[ch][i];
                }
                ev.AddBlock(block_no, reinterpret_cast<const char*>(payload), samples_per_channel*sizeof(uint16_t));
                block_no++;
                delete payload;
              }  


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


//from CAEN Digitizer library:
void VX1742Producer::CAENPeakCorrection(uint32_t channels, uint32_t nsamples){
    uint32_t offset;
    uint32_t i;
    uint32_t j;

    for(j=0; j<channels; j++){
        wf_storage[j][0] = wf_storage[j][1];
    }

    for(i=1; i<nsamples; i++){ //loop over samples
        offset=0;

        for(j=0; j<channels; j++){ //and over all channels
            if (i==1){
                if ((wf_storage[j][2]-wf_storage[j][1])>30){  //if (sample 3 - sample 2)>30                               
                    offset++;
                }

                else {
                    if (((wf_storage[j][3]- wf_storage[j][1])>30)&&((wf_storage[j][3]- wf_storage[j][2])>30)){  //if((s3-s2) > 30 AND (s4-s3) > 30)                             
                        offset++;
                    }
                }
            }
            else{
                if ((i==nsamples-1)&&((wf_storage[j][nsamples-2]- wf_storage[j][nsamples-1])>30)){  
                    offset++;                                                                             
                }
                else{
                    if ((wf_storage[j][i-1]- wf_storage[j][i])>30){ 
                        if ((wf_storage[j][i+1]- wf_storage[j][i])>30)
                            offset++;
                        else {
                            if ((i==nsamples-2)||((wf_storage[j][i+2]-wf_storage[j][i])>30))
                                offset++;
                        }                                     
                    }
                }
            }
        }                                


        if (offset==channels){
            for(j=0; j<channels; j++){
                if (i==1){
                    if ((wf_storage[j][2]-wf_storage[j][1])>30) {
                        wf_storage[j][0]=wf_storage[j][2];
                        wf_storage[j][1]=wf_storage[j][2];
                    }
                    else{
                        wf_storage[j][0]=wf_storage[j][3];
                        wf_storage[j][1]=wf_storage[j][3];
                        wf_storage[j][2]=wf_storage[j][3];
                    }
                }
                else{
                    if (i==nsamples-1){
                        wf_storage[j][nsamples-1]=wf_storage[j][nsamples-2];
                    }
                    else{
                        if ((wf_storage[j][i+1]- wf_storage[j][i])>30)
                            wf_storage[j][i]=((wf_storage[j][i+1]+wf_storage[j][i-1])/2);
                        else {
                            if (i==nsamples-2){
                                wf_storage[j][nsamples-2]=wf_storage[j][nsamples-3];
                                wf_storage[j][nsamples-1]=wf_storage[j][nsamples-3];
                            }
                            else {
                                wf_storage[j][i]=((wf_storage[j][i+2]+wf_storage[j][i-1])/2);
                                wf_storage[j][i+1]=( (wf_storage[j][i+2]+wf_storage[j][i-1])/2);
                            }
                        }
                    }
                }
            }
        }                                
    }
}


//called once for each group
void VX1742Producer::CAENTimeCorrection(uint32_t grp, uint32_t channels, uint32_t nsamples, uint32_t freq, uint32_t st_index){
    float t0, vcorr, Tsamp;
    float Time[1024];
    float wave_tmp[1024]; 
    uint32_t i, j, k;

    switch(freq){
    case 0:
        Tsamp =(float)((1.0/5000.0)*1000.0); //0.2ns
        break;
    case 1:
        Tsamp =(float)((1.0/2500.0)*1000.0); //0.4ns
        break;
    case 2:
        Tsamp =(float)((1.0/1000.0)*1000.0); //1.0ns
        break;
    default:
        Tsamp =(float)((1.0/750.0)*1000.0); //1.33ns
        break;
    }


    t0=time_corr[grp][st_index];
    Time[0]=0.0;

    for(j=1; j< nsamples; j++) {
        t0 = time_corr[grp][(st_index+j)%1024]-t0;
        if(t0 >0){Time[j] =  Time[j-1]+ t0;} 
        else{Time[j] =  Time[j-1]+ t0 + (Tsamp*1024);}
        t0 = time_corr[grp][(st_index+j)%1024];
    }

    for (j=0; j<channels; j++) {
        wf_storage[j][0] = wf_storage[j][1];
        wave_tmp[0] = wf_storage[j][0];
        vcorr = 0.0;
        k=0;
        i=0;

        for(i=1; i<nsamples; i++) {
            while ((k<nsamples-1) && (Time[k]<(i*Tsamp)))  k++;
            vcorr =(((float)(wf_storage[j][k] - wf_storage[j][k-1])/(Time[k]-Time[k-1]))*((i*Tsamp)-Time[k-1]));
            wave_tmp[i]=wf_storage[j][k-1] + vcorr;
            k--;                                
        }

        for(i=1; i<nsamples; i++)
            wf_storage[j][i] = wave_tmp[i];
    }
}
