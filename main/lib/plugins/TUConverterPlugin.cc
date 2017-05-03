#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include <string>
#include <cstdint>

namespace eudaq{

    static const char* EVENT_TYPE = "TU";

    class TUConverterPlugin:public DataConverterPlugin{
        public:
            TUConverterPlugin():DataConverterPlugin(EVENT_TYPE){}
            virtual unsigned GetTriggerID(const eudaq::Event & ev) const{return ev.GetEventNumber();}
            
            virtual void Initialize(const Event & bore, const Configuration & cnf){
                //std::cout << bore.GetTag("FirmwareID") << std::endl;

            }

            virtual bool GetStandardSubEvent(eudaq::StandardEvent &sev, const eudaq::Event &ev) const{

                // Check if we have BORE or EORE:
                if (ev.IsBORE() || ev.IsEORE()) { return true; }

                const RawDataEvent & in_raw = dynamic_cast<const RawDataEvent &>(ev);
                int valid = std::stoi(in_raw.GetTag("valid"));
                int nblocks = in_raw.NumBlocks();

                StandardTUEvent tuev(EVENT_TYPE);
                tuev.SetValid(bool(valid));

                if(valid){
                    unsigned int id = 0;
                    RawDataEvent::data_t data = in_raw.GetBlock(id);

                    uint64_t time_stamp = *((uint64_t*)&data[0]);
                    id++;
                  
                    data = in_raw.GetBlock(id);
                    uint32_t coincidence_count = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t coincidence_count_no_sin = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t prescaler_count = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t prescaler_count_xor_pulser_count = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t accepted_prescaled_events = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t accepted_pulser_events = *((uint32_t*)&data[0]);
                    id++;

                    data = in_raw.GetBlock(id);
                    uint32_t handshake_count = *((uint32_t*)&data[0]);
                    id++;


                    data = in_raw.GetBlock(id);
                    uint32_t cal_beam_current = *((float*)&data[0]);
                    id++;


                    #ifdef DEBUG
                        std::cout << "************************************************************************************" << std::endl;
                        std::cout << "nblocks: " << nblocks << std::endl;
                        std::cout << "time stamp: " << time_stamp << std::endl;
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

                    //add data to the StandardEvent:
                    sev.SetTimestamp(time_stamp);


                    //get individual scaler values
                    data = in_raw.GetBlock(id);
                    int data_size = data.size(); 
                    int n_samples = data_size/sizeof(uint64_t);
                    for(int idx=0; idx<n_samples; idx++){
                        uint64_t *sc_val = (uint64_t*)(&data[0]);
                        tuev.SetScalerValue(idx, sc_val[idx]);
                     }

                     tuev.SetTimeStamp(time_stamp);
                     tuev.SetCoincCount(coincidence_count);
                     tuev.SetPrescalerCountXorPulserCount(prescaler_count_xor_pulser_count);
                     tuev.SetAcceptedPrescaledEvents(accepted_prescaled_events);
                     tuev.SetCoincCountNoSin(coincidence_count_no_sin);
                     tuev.SetAcceptedPulserCount(accepted_pulser_events);
                     tuev.SetPrescalerCount(prescaler_count);
                     tuev.SetHandshakeCount(handshake_count);
                     tuev.SetBeamCurrent(cal_beam_current);
                }
                sev.AddTUEvent(tuev);
                return true;
            }

        private:
            static TUConverterPlugin const m_instance;

        };

    TUConverterPlugin const TUConverterPlugin::m_instance;

}//namespace eudaq



