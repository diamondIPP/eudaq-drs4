#ifdef ROOT_FOUND

#include "eudaq/FileWriterTreeDRS4.hh"

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeDRS4> reg("drs4tree"); }


/** --------------------------CONSTRUCTOR-------------------------------- */
FileWriterTreeDRS4::FileWriterTreeDRS4(const std::string & params): FileWriterWF(params, "drs4tree", 4) {
  producer_name_ = "DRS4";
} // end Constructor

void FileWriterTreeDRS4::AddInfo(uint8_t iwf, const StandardWaveform * wf) {
  if (iwf == trigger_channel) { ExtractForcTiming(); }
}

inline void FileWriterTreeDRS4::ExtractForcTiming() {

  if (verbose_ > 3) cout << "get trigger timing" << endl;
  bool found_timing = false;
  for (auto j(1); j < data_->size(); j++){
    if( abs(data_->at(j)) > 200 && abs(data_->at(uint16_t(j - 1))) < 200) {
      v_forc_pos->push_back(j);
      v_forc_time->push_back(GetTriggerTime(trigger_channel, j));
      found_timing = true;
    }
  }
  if (!found_timing) {
    v_forc_pos->push_back(0);
    v_forc_time->push_back(-999);
  }
}

float FileWriterTreeDRS4::LoadSamplingFrequency(const DetectorEvent & ev) {

  Configuration conf(ev.GetTag("CONFIG"));
  float f = conf.Get(Form("Producer.%s", producer_name_.c_str()), "sampling_frequency", 5);
  cout << "Sampling Frequency: " << f << " GHz" << endl;
  return f;
}

#endif // ROOT_FOUND
