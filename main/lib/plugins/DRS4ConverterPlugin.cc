#include "eudaq/DataConverterPlugin.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

using namespace std;

namespace eudaq {

// The event type for which this converter plugin will be registered
// Modify this to match your actual event type (from the Producer)
static const char* EVENT_TYPE = "DRS4";

// Declare a new class that inherits from DataConverterPlugin
class DRS4ConverterPlugin : public DataConverterPlugin {
private:
	int m_serial_no;
	int m_firmware;
	int m_board_type;
	int m_n_channels;
	unsigned char m_activated_channels;
	std::map<int, std::string> m_channel_names;
	std::string m_dut_name;
	std::map<uint8_t, std::vector<float> > m_tcal;
public:
	// This is called once at the beginning of each run.
	// You may extract information from the BORE and/or configuration
	// and store it in member variables to use during the decoding later.
	virtual void Initialize(const Event & bore, const Configuration & cnf) {
		std::cout<<"Read DRS4 Bore Event"<<std::endl;
#ifndef WIN32  //some linux Stuff //$$change
		(void)cnf; // just to suppress a warning about unused parameter cnf
#endif
		m_serial_no = bore.GetTag("DRS4_serial_no", (int)-1);
		m_firmware = bore.GetTag("DRS4_FW",(int)-1);
		m_board_type = bore.GetTag("DRS4_Board_type", (int)-1);
		m_n_channels = bore.GetTag("DRS4_n_channels", (int)-1);
		m_activated_channels = bore.GetTag("activated_channels",(unsigned char)0);
		m_dut_name = bore.GetTag("device_name","DRS4");
		std::cout<<"  serial_no:  "<<m_serial_no<<std::endl;
		std::cout<<"  firmware:   "<<m_firmware<<std::endl;
		std::cout<<"  board_type: "<<m_board_type<<std::endl;
		std::cout<<"  n_channels: "<<m_n_channels<<":"<<std::endl;
		for (int ch = 0; ch< m_n_channels;ch++){
					std::string tag = "CH_"+std::to_string(ch+1);
					m_channel_names[ch] = bore.GetTag(tag,tag);
					std::cout<<"    "<<tag<<": "<<m_channel_names[ch]<<std::endl;
				}
		// extracting the timing calibration
		const RawDataEvent & in_bore = dynamic_cast<const RawDataEvent &>(bore);
		for (uint8_t id = 0; id < in_bore.NumBlocks();){
			RawDataEvent::data_t data = in_bore.GetBlock(id++);
			char buffer [5];
			std::memcpy(&buffer,&(data[0]), 4);
			buffer[4]='\0';
			int ch = atoi(&buffer[1])-1;
			data = in_bore.GetBlock(id++);
			int n_samples = int(data.size() / sizeof(unsigned short));
			float * raw_tcal_array = (float*) &data[0];
			for (int i = 0; i < n_samples; i++)
				m_tcal[ch].push_back(raw_tcal_array[i]); }
		//todo set range
	} // end Initialize

	 virtual map<uint8_t, vector<float> > GetTimeCalibration(const Event & bore) {
		 return this->m_tcal;
	 } // end GetTimeCalibration

	// Here, the data from the RawDataEvent is extracted into a StandardEvent.
	// The return value indicates whether the conversion was successful.
	// Again, this is just an example, adapted it for the actual data layout.
	virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {

		const auto & in_raw = dynamic_cast<const RawDataEvent &>(ev);
		int n_blocks = in_raw.NumBlocks();
    if (sev.IsEORE() and n_blocks == 0) {
      std::cout << "received empty DRS4 EORE" << std::endl;
      return true; }
		// If the event type is used for different sensors
		// they can be differentiated here
		// Create a StandardPlane representing one sensor plane
		uint8_t id = 0;
		RawDataEvent::data_t data = in_raw.GetBlock(id++);  // Trigger cell
		auto trigger_cell = static_cast<uint16_t>(*((int*) &data[0]));
		data = in_raw.GetBlock(id++); // Get Timestamp
		uint64_t timestamp = *((uint64_t*) &data[0]);
//		sev.SetTimestamp(timestamp);
		float min_waves[m_n_channels];
		float max_waves[m_n_channels];

		for (id = id; id < n_blocks;){  // Get Raw data
			data = in_raw.GetBlock(id++); // Get Header
			char buffer [5];
			std::memcpy(&buffer,&(data[0]), 4);
      buffer[4]='\0';
      int ch = atoi(&buffer[1])-1;
      if (ch < 0) { EUDAQ_THROW("Memory error!"); }

			//Get Waveform
			data = in_raw.GetBlock(id++);
			size_t wave_size = data.size();
			int n_samples = int(wave_size / sizeof(unsigned short));

			unsigned short *raw_wave_array = (unsigned short*) &data[0];
			float wave_array[n_samples];
			//Conversion of raw data to voltage data
			for (int i = 0; i < n_samples; i++)
				wave_array[i] = (raw_wave_array[i] / 65536. + range/1000.0 - 0.5)*1000.;
//			min_waves[ch] = *std::min_element(wave_array,wave_array+n_samples);
//			max_waves[ch] = *std::max_element(wave_array,wave_array+n_samples);
			//conversion of time:
//			 for (j=0,time[chn_index][i]=0 ; j<i ; j++)
//			               time[chn_index][i] += bin_width[chn_index][(j+eh.trigger_cell) % 1024];
			//add Waveform to standard event
			StandardWaveform wf(ch,EVENT_TYPE,m_dut_name+(std::string)"_"+m_channel_names.at(ch));
			wf.SetChannelName(m_channel_names.at(ch));
			wf.SetChannelNumber(ch);
			wf.SetNSamples(n_samples);
			wf.SetWaveform((float*) wave_array);
			wf.SetTimeStamp(timestamp);
			wf.SetTriggerCell(trigger_cell);
			sev.AddWaveform(wf);
		}
		return true;
	}

#if USE_LCIO
	// This is where the conversion to LCIO is done
	virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {
		return 0;
	}
#endif

private:
	unsigned short range;
	// The constructor can be private, only one static instance is created
	// The DataConverterPlugin constructor must be passed the event type
	// in order to register this converter for the corresponding conversions
	// Member variables should also be initialized to default values here.
	DRS4ConverterPlugin(): DataConverterPlugin(EVENT_TYPE),
			m_serial_no(-1),
			m_firmware(-1),
			m_board_type(-1),
			m_n_channels(0),
			m_dut_name(""),
			range(0)
	{
//		std::cout<<"DRS4ConverterPlugin Constructor"<<std::endl;
	}


	// The single instance of this converter plugin
	static DRS4ConverterPlugin m_instance;

}; // class DRS4ConverterPlugin

// Instantiate the converter plugin instance
DRS4ConverterPlugin DRS4ConverterPlugin::m_instance;

} // namespace eudaq
