#ifdef ROOT_FOUND

#include "eudaq/FileWriterTreeCAEN.hh"
#include "TMacro.h"
#include "TTree.h"
#include "TF1.h"

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeCAEN> reg("caentree"); }

/** --------------------------CONSTRUCTOR-------------------------------- */
FileWriterTreeCAEN::FileWriterTreeCAEN(const std::string & params):
  FileWriterWF(params, "caentree", 9), f_trigger_time(0), f_rf_period(0), f_rf_phase(0), f_rf_chi2(UINT16_MAX), fit_rf(false) {
  producer_name_ = "VX1742";
  sampling_speed_ = 2.5;
  dia_channels_ = new vector<uint16_t>;
}// end Constructor


/** --------------------------CONFIGURE---------------------------------- */
void FileWriterTreeCAEN::Configure() {
  FileWriterWF::Configure();

  fit_rf = m_config->Get("fit_rf", false);

  // channels
  rf_channel_ = uint8_t(m_config->Get("rf_channel", 99));
  scint_channel_ = uint8_t(m_config->Get("scint_channel", 99));

  macro_->AddLine("[CAEN]");
  macro_->AddLine((append_spaces(21, "rf channel = ") + to_string(int(rf_channel_))).c_str());
  macro_->AddLine((append_spaces(21, "fit rf = ") + to_string(int(fit_rf))).c_str());
  macro_->AddLine((append_spaces(21, "scint channel = ") + to_string(int(scint_channel_))).c_str());

  for (auto i(0); i < n_channels_; i++) { if (UseWaveForm(active_regions_, i)) dia_channels_->emplace_back(i); }

} // end Configure()

/** --------------------------START RUN---------------------------------- */
void FileWriterTreeCAEN::StartRun(unsigned runnumber) {

    FileWriterWF::StartRun(runnumber);
    //beam rf
    if (rf_channel_ < 32 ){
      ttree_->Branch("rf_period", &f_rf_period, "rf_period/F");
      ttree_->Branch("rf_phase",& f_rf_phase, "rf_phase/F");
      ttree_->Branch("rf_chi2",& f_rf_chi2, "rf_chi2/F");
    }
    // digitiser
    if (trigger_channel > 0) {
      ttree_->Branch("trigger_time", &f_trigger_time, "trigger_time/F"); }
}

void FileWriterTreeCAEN::AddInfo(uint8_t iwf, const StandardWaveform * wf) {
  if (iwf == rf_channel_ and fit_rf){
    auto fit = wf->getRFFit(&tcal_.at(0));
    f_rf_period = float(fit.GetParameter(1));
    f_rf_phase = GetRFPhase(float(fit.GetParameter(2)), f_rf_period);
    f_rf_chi2 = float(fit.GetChisquare() / fit.GetNDF());
  }
  if (iwf == trigger_channel)
    f_trigger_time = wf->getTriggerTime(&tcal_.at(0));
}

float FileWriterTreeCAEN::GetRFPhase(float phase, float period) {
  // shift phase always in [-pi,pi]
  double fac = round(phase / period - .5) ;
  return float(phase - fac * period);
}

inline bool FileWriterTreeCAEN::IsPulserEvent(const StandardWaveform *wf) const{
  float pulser_int = wf->getAverage(pulser_region.first, pulser_region.second, true);
  float baseline_int = wf->getAverage(5, uint16_t(pulser_region.second - pulser_region.first + 5), true);
  return abs(pulser_int - baseline_int) > float(pulser_threshold);
}

float FileWriterTreeCAEN::LoadSamplingFrequency(const DetectorEvent & ev) {

  Configuration conf(ev.GetTag("CONFIG"));
  auto f = float(ev.GetRawSubEvent(producer_name_).GetTag("sampling_speed", 5000)) / 1000;
  cout << "Sampling Frequency: " << f << " GHz" << endl;
  return f;
}
//end IsPulserEvent

#endif // ROOT_FOUND
