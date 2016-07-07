/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework - Event Structure
** 
** <VX1742ConverterPlugin>.cc
** 
** Date: April 2016
** Fixme: replace c-style casts with static_cast<type>
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include <string.h>
#include <cstdint>


namespace eudaq{

//event type for which the converter will be used
static const char* EVENT_TYPE = "VX1742";


class VX1742ConverterPlugin:public DataConverterPlugin {

public:
  virtual void Initialize(const Event & bore, const Configuration & cnf) {
  	std::cout << "Read VX1742 BORE Event" << std::endl;
  	timestamp = bore.GetTag("timestamp", 0);
  	serialno = bore.GetTag("serial_number", " ");
  	firmware = bore.GetTag("firmware_version", " ");
  	active_channels = bore.GetTag("active_channels", 0);
  	sampling_speed = bore.GetTag("sampling_speed", 0);
  	samples_in_channel = bore.GetTag("samples_in_channel", 0);
  	device_name = bore.GetTag("device_name", " ");
  	group_mask = bore.GetTag("group_mask", 0);
  	std::cout<<"Device: " << device_name << std::endl;
	  std::cout<<"Firmware:   " << firmware << std::endl;

	for (int ch = 0; ch < active_channels; ch++){
		std::string tag = "CH_"+std::to_string(ch);
		channel_names[ch] = bore.GetTag(tag, tag);
		std::cout << "CH" << ch << " Name: " << channel_names[ch] << std::endl;
	}

    
    RawDataEvent::data_t data;
    uint32_t block_no = 0;
    data = bore.GetBlock(block_no);

    float *tcorr = (float(*)[1024])(&data[0]);
    for(uint32_t grp=0; grp<4; grp++){
      for(uint32_t idx=0; idx<1024; idx++)
        time_corr[grp][idx] = tcorr[grp][idx];
    }




}



  virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const{
	const RawDataEvent &in_raw = dynamic_cast<const RawDataEvent &>(ev);
	int nblocks = in_raw.NumBlocks();

	//get data:
	uint32_t id = 0;
	RawDataEvent::data_t data = in_raw.GetBlock(id++);
	uint32_t event_size = *((uint32_t*) &data[0]);
	//uint32_t event_size = static_cast<uint32_t>(*((uint32_t*) &data[0]));

	data = in_raw.GetBlock(id++);
	uint32_t n_groups = *((uint32_t*) &data[0]);

	data = in_raw.GetBlock(id++);
	uint32_t group_mask = *((uint32_t*) &data[0]);

	data = in_raw.GetBlock(id++);
	uint32_t trigger_time_tag = *((uint32_t*) &data[0]);


	//loop over all groups
    for(uint32_t grp = 0; grp < 4; grp++){
    	if(group_mask & (1<<grp)){ 
    	  //std::cout << "Reading data from group: " << grp << std::endl;

        data = in_raw.GetBlock(id++);
       	uint32_t samples_per_channel = *((uint32_t*) &data[0]);

       	data = in_raw.GetBlock(id++);
       	uint32_t start_index_cell = *((uint32_t*) &data[0]);
       	  

       	data = in_raw.GetBlock(id++);
       	uint32_t event_timestamp = *((uint32_t*) &data[0]);

        #ifdef DEBUG
          std::cout << "***********************************************************************" << std::endl << std::endl;
          std::cout << "Event: " << ev.GetEventNumber() << std::endl;
          std::cout << "Event size: " << event_size << std::endl;
          std::cout << "Groups enabled: " << n_groups << std::endl;
          std::cout << "Group mask: " << group_mask << std::endl;
          std::cout << "Trigger time tag: " << trigger_time_tag << std::endl;
          std::cout << "Samples per channel: " << samples_per_channel << std::endl;
          std::cout << "Start index cell : " << start_index_cell << std::endl;
          std::cout << "Event trigger time tag: " << event_timestamp << std::endl;
          std::cout << "***********************************************************************" << std::endl << std::endl; 
        #endif

    	  for(u_int ch = 0; ch < active_channels; ch++){
    		data = in_raw.GetBlock(id);
    		//uint16_t wave_array[samples_per_channel];
        float fwave_array[samples_per_channel];
	  		uint16_t *raw_wave_array = (uint16_t*)(&data[0]);
			  for (int i = 0; i < samples_per_channel; i++){
	  		  //wave_array[i] = (uint16_t)(raw_wave_array[i]);
          fwave_array[i] = 1000*(raw_wave_array[i]/4096.0); //convert to mV
	   	  	}

	  		StandardWaveform wf(ch, EVENT_TYPE, (std::string)("VX1742 - " + channel_names.at(ch)));
	  		wf.SetChannelName(channel_names.at(ch));
	  		wf.SetChannelNumber(grp*8+ch);
	  		wf.SetNSamples(samples_per_channel);
	  		//wf.SetWaveform((uint16_t*) wave_array);
        wf.SetWaveform((float*) fwave_array);
	  		wf.SetTimeStamp(event_timestamp);
	  		wf.SetTriggerCell(start_index_cell);
	  		sev.AddWaveform(wf);
	  		id++;
		  }//end ch loop
		}//end if group mask
	}//end group loop

	return true;
}




private:
  VX1742ConverterPlugin():DataConverterPlugin(EVENT_TYPE){}

  uint64_t timestamp;
  std::string serialno;
  std::string firmware;
  uint32_t active_channels;
  uint32_t sampling_speed, samples_in_channel;
  std::string device_name;
  uint32_t group_mask;
  std::map<int, std::string> channel_names;
  float time_corr[4][1024]; //4 groups a 8 channels
  uint16_t cell_corr[32][1024]; //1 for each channel
  uint8_t index_corr[32][1024];
  static VX1742ConverterPlugin m_instance;

}; // class VX1742ConverterPlugin

VX1742ConverterPlugin VX1742ConverterPlugin::m_instance;
} // namespace eudaq
