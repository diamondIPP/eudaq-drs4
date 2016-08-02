/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework - Event Structure
** 
** <VX1742ConverterPlugin>.cc
** 
** Date: April 2016
** Fixme: replace c-style casts with static_cast<type> and understand the problem.
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include <string.h>
#include <cstdint>

using namespace std;

namespace eudaq{

//event type for which the converter will be used
static const char* EVENT_TYPE = "VX1742";


class VX1742ConverterPlugin:public DataConverterPlugin {

public:
  virtual void Initialize(const Event & bore, const Configuration & cnf) {
	m_serialno = bore.GetTag("VX1742_serial_no", uint32_t(-1));
	m_firmware = bore.GetTag("VX1742_firmware_v", -1);
	m_timestamp = bore.GetTag("VX1742_timestamp", uint64_t(-1));
	m_range = 0.5; //TODO
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
    	  std::cout << "Reading data from group: " << grp << std::endl;

          data = in_raw.GetBlock(id++);
       	  uint32_t samples_per_channel = *((uint32_t*) &data[0]);

       	  data = in_raw.GetBlock(id++);
       	  uint32_t start_index_cell = *((uint32_t*) &data[0]);
       	  

       	  data = in_raw.GetBlock(id++);
       	  uint32_t event_timestamp = *((uint32_t*) &data[0]);


       	  std::cout << "***********************************************************************" << std::endl << std::endl;
       	  std::cout << "Event size: " << event_size << std::endl;
          std::cout << "Groups enabled: " << n_groups << std::endl;
          std::cout << "Group mask: " << group_mask << std::endl;
          std::cout << "Trigger time tag: " << trigger_time_tag << std::endl;
          std::cout << "Samples per channel: " << samples_per_channel << std::endl;
          std::cout << "Start index cell : " << start_index_cell << std::endl;
          std::cout << "Event trigger time tag: " << event_timestamp << std::endl;
          std::cout << "***********************************************************************" << std::endl << std::endl; 


    	  for(u_int ch = 0; ch < 8; ch++){
    		data = in_raw.GetBlock(id);
    		uint16_t wave_array[samples_per_channel];
	  		uint16_t *raw_wave_array = (uint16_t*)(&data[0]);
			for (int i = 0; i < samples_per_channel; i++){
	  		  wave_array[i] = (uint16_t)(raw_wave_array[i]);
	   	  	}
	
	  		StandardWaveform wf(ch, EVENT_TYPE, " VX1742 CH" + std::to_string(grp*8+ch));
	  		wf.SetChannelName("CH" + std::to_string(grp*8+ch));
	  		wf.SetChannelNumber(grp*8+ch);
	  		wf.SetNSamples(samples_per_channel);
	  		wf.SetWaveform((uint16_t*) wave_array);
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
  VX1742ConverterPlugin():DataConverterPlugin(EVENT_TYPE), m_serialno(-1), m_firmware(-1), m_range(0){

}

  uint64_t m_timestamp;
  uint32_t m_serialno;
  float m_firmware, m_range;
  static VX1742ConverterPlugin m_instance;
	std::map<uint8_t, std::vector<float> > m_tcal;

}; // class VX1742ConverterPlugin

VX1742ConverterPlugin VX1742ConverterPlugin::m_instance;
} // namespace eudaq
