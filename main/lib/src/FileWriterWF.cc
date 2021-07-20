#ifdef ROOT_FOUND

#include "eudaq/FileWriterWF.hh"
#include "TROOT.h"
#include "TTree.h"
#include "TVirtualFFT.h"
#include "TFile.h"
#include "TH1F.h"
#include "TSystem.h"
#include "TInterpreter.h"
#include "TMacro.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TSpectrum.h"
#include "TMath.h"
#include "TString.h"
#include <Math/MinimizerOptions.h>

#include <utility>

#define MAX_SIZE 255

using namespace std;
using namespace eudaq;

/** --------------------------CONSTRUCTOR-------------------------------- */
FileWriterWF::FileWriterWF(const std::string & /*params*/, string name, int n_channels)
: name_(move(name)), producer_name_("PROD"), run_number_(0), max_event_number_(0), has_tu_(false), verbose_(0), tfile_(nullptr), ttree_(nullptr), sampling_speed_(2),
  bunch_width_(19.751), n_channels_(n_channels), n_active_channels_(0), rise_time_(5), n_samples_(1024), tfft_(nullptr), re_full_(nullptr), im_full_(nullptr), in_(nullptr) {

  gROOT->ProcessLine("gErrorIgnoreLevel = 5001;");
  gROOT->ProcessLine("#include <vector>");
  gROOT->ProcessLine("#include <map>");
  gInterpreter->GenerateDictionary("vector<vector<float> >;vector<vector<uint16> >", "vector");

  polarities_.resize(n_channels_, 1);
  pulser_polarities_.resize(n_channels_, 1);
  spectrum_polarities.resize(n_channels_, 1);

  macro_ = new TMacro("region_information", "Region Information");

  f_nwfs = 0;
  f_event_number = -1;
  f_pulser_events = 0;
  f_signal_events = 0;
  f_time = -1.;
  old_time = 0;
  f_pulser = false;
  f_beam_current = UINT16_MAX;
  v_forc_pos = new vector<uint16_t>;
  v_forc_time = new vector<float>;

  // integrals
  regions = new map<uint8_t, WaveformSignalRegions* >;
  IntegralValues = new std::vector<float>;
  TimeIntegralValues = new vector<float>;
  IntegralPeaks = new std::vector<Int_t>;
  IntegralPeakTime = new std::vector<float>;
  IntegralLength  = new std::vector<float>;
  v_cft = new float[n_channels_];
  f_bucket = new bool[n_channels_];
  f_ped_bucket = new bool[n_channels_];
  f_b2_int = new float[n_channels_];

  // general waveform information
  v_is_saturated = new vector<bool>;
  v_median = new vector<float>;
  v_average = new vector<float>;
  v_max_peak_position = new vector<uint16_t>(n_channels_, 0);
  v_max_peak_time = new vector<float>(n_channels_, 0);
  v_peak_positions = new vector<vector<uint16_t> >(n_channels_);
  v_peak_times = new vector<vector<float> >(n_channels_);
  v_npeaks = new vector<uint8_t>(n_channels_, 0);
  v_rise_time = new vector<float>;
  v_fall_time = new vector<float>;
  v_wf_start = new vector<float>;
  v_fit_peak_time = new vector<float>;
  v_fit_peak_value = new vector<float>;
  v_peaking_time = new vector<float>;

  // waveforms
  for (auto i = 0; i < n_channels_; i++) f_wf[i] = new float[n_samples_];

  // spectrum vectors
  noise_.resize(n_channels_);
  int_noise_.resize(n_channels_);
  int_noise_vectors.resize(n_channels_);
  noise_vectors.resize(n_channels_);
  decon.resize(1024, 0);
  peaks_x.resize(4, new std::vector<uint16_t>);
  peaks_x_time.resize(4, new std::vector<float>);
  peaks_y.resize(4, new std::vector<float>);

  // fft analysis
  fft_modes.resize(n_channels_, new std::vector<float>);
  fft_values.resize(n_channels_, new std::vector<float>);
  fft_mean = new std::vector<float>;
  fft_mean_freq = new std::vector<float>;
  fft_max = new std::vector<float>;
  fft_max_freq = new std::vector<float>;
  fft_min = new std::vector<float>;
  fft_min_freq = new std::vector<float>;

  // telescope
  InitTelescopeArrays();
  f_trigger_cell = 0;

  //tu
  v_scaler = new vector<uint64_t>;
  v_scaler->resize(n_channels_ + 1);
  old_scaler = new vector<uint64_t>;
  old_scaler->resize(n_channels_ + 1, 0);

  spec_ = new TSpectrum(25);
} // end Constructor

/** --------------------------CONFIGURE---------------------------------- */
void FileWriterWF::Configure(){

  // do some assertions
  if (m_config == nullptr) { return EUDAQ_WARN("Configuration class instance m_config does not exist!"); }

  m_config->SetSection(Form("Converter.%s", name_.c_str()));
  if (m_config->NSections() == 0) { return EUDAQ_WARN("Config file has no sections!"); }
  EUDAQ_INFO(Form("Configuring FileWriter %s\n", name_.c_str()));

  max_event_number_ = m_config->Get("max_event_number", 0);
  rise_time_ = m_config->Get("rise_time", float(5));

  // spectrum and fft
  peak_noise_pos = m_config->Get("peak_noise_pos", m_config->Get("pedestal_ab_region", make_pair(20, 20)).first);
  spec_sigma = m_config->Get("spectrum_sigma", float(5));
  spec_decon_iter = m_config->Get("spectrum_deconIterations", 3);
  spec_aver_win = m_config->Get("spectrum_averageWindow", 5);
  spec_markov = m_config->Get("spectrum_markov", true);
  spec_rm_bg = m_config->Get("spectrum_background_removal", true);
  spectrum_waveforms = m_config->Get("spectrum_waveforms", uint16_t(0));
  fft_waveforms = m_config->Get("fft_waveforms", uint16_t(0));

  //peak finding
  peak_finding_roi = m_config->Get("peak_finding_roi", make_pair(0.0, 0.0));

  // channels
  pulser_threshold = m_config->Get("pulser_channel_threshold", 80);
  pulser_channel = uint8_t(m_config->Get("pulser_channel", 1));
  trigger_channel = uint8_t(m_config->Get("trigger_channel", 2));

  // saved waveforms
  save_waveforms_ = m_config->Get("save_waveforms", uint16_t(9));

  // polarities
  polarities_ = m_config->Get("polarities", polarities_);
  pulser_polarities_ = m_config->Get("pulser_polarities", pulser_polarities_);
  spectrum_polarities = m_config->Get("spectrum_polarities", spectrum_polarities);

  // regions todo: add default range
  active_regions_ = m_config->Get("active_regions", uint16_t(0));
  macro_->AddLine("[General]");
  macro_->AddLine((append_spaces(21, "max event number = ") + to_string(max_event_number_)).c_str());
  macro_->AddLine((append_spaces(21, "pulser channel = ") + to_string(int(pulser_channel))).c_str());
  macro_->AddLine((append_spaces(21, "trigger channel = ") + to_string(int(trigger_channel))).c_str());
  macro_->AddLine((append_spaces(21, "pulser threshold = ") + to_string(pulser_threshold)).c_str());
  macro_->AddLine((append_spaces(21, "active regions = ") + to_string(GetBitMask(active_regions_))).c_str());
  macro_->AddLine((append_spaces(21, "save waveforms = ") + GetBitMask(save_waveforms_)).c_str());
  macro_->AddLine((append_spaces(21, "fft waveforms = ") + GetBitMask(fft_waveforms)).c_str());
  macro_->AddLine((append_spaces(21, "spectrum waveforms = ") + GetBitMask(spectrum_waveforms)).c_str());
  macro_->AddLine((append_spaces(20, "polarities = ") + GetPolarityStr(polarities_)).c_str());
  macro_->AddLine((append_spaces(20, "pulser polarities = ") + GetPolarityStr(pulser_polarities_)).c_str());
  macro_->AddLine((append_spaces(20, "spectrum_polarities = ") + GetPolarityStr(spectrum_polarities)).c_str());

  for (auto i = 0; i < n_channels_; i++) { if (UseWaveForm(active_regions_, i)) n_active_channels_++; }
  for (auto i = 0; i < n_channels_; i++) {
    if (UseWaveForm(active_regions_, i)) (*regions)[i] = new WaveformSignalRegions(i, polarities_.at(i), pulser_polarities_.at(i));
  }
  // read the peak integral ranges from the config file
  pulser_region = m_config->Get("pulser_channel_region", make_pair(800, 1000));
  ReadIntegralRanges();
  for (const auto & ch: ranges) {
    macro_->AddLine(TString::Format("\n[Integral Ranges %d]", ch.first));
    for (const auto & range: ch.second) {
      macro_->AddLine((append_spaces(21, range.first + " =") + to_string(*range.second)).c_str());
    }
  }
  // read the region where we want to find the peak from the config file
  ReadIntegralRegions();
  for (auto ch: *regions) {
    macro_->AddLine(TString::Format("\n[Integral Regions %d]", ch.first));
    for (auto region: ch.second->GetRegions()) {
      macro_->AddLine((append_spaces(21, to_string(region->GetName()) + " =") + to_string(region->GetRegion())).c_str());
    }
  }

  macro_->Print();
  // add the list of all the integral names corresponding to the entries in the integral vectors
  macro_->AddLine("\n[Integral Names]");
  vector<TString> names;
  for (auto ch: *regions)
    for (auto region: ch.second->GetRegions())
      for (auto integral: region->GetIntegrals())
        names.push_back(TString::Format("\"ch%d_%s_%s\"", int(ch.first), region->GetName(), integral->GetName().c_str()));
  macro_->AddLine(("Names = [" + to_string(names) + "]").c_str());

  cout << "\nMAXIMUM NUMBER OF EVENTS: " << (max_event_number_ ? to_string(max_event_number_) : "ALL") << endl;
  EUDAQ_INFO("End of Configure!");
  cout << endl;
  macro_->Write();
} // end Configure()

/** --------------------------START RUN---------------------------------- */
void FileWriterWF::StartRun(unsigned run_number) {
  run_number_ = run_number;
  EUDAQ_INFO(Form("Converting the input file into a %s TTree", name_.c_str()));
  string f_output = FileNamer(m_filepattern).Set('X', ".root").Set('R', run_number);
  EUDAQ_INFO("Preparing the output file: " + f_output);

  tfile_ = new TFile(f_output.c_str(), "RECREATE");
  ttree_ = new TTree("tree", "EUDAQ TTree with telescope and waveform branches");

  // Set Branch Addresses
  ttree_->Branch("event_number", &f_event_number, "event_number/I");
  ttree_->Branch("time", & f_time, "time/D");
  ttree_->Branch("pulser", & f_pulser, "pulser/O");
  ttree_->Branch("nwfs", &f_nwfs, "n_waveforms/I");
  ttree_->Branch("beam_current", &f_beam_current, "beam_current/s");
  if (has_tu_) { ttree_->Branch("rate", &v_scaler); }
  ttree_->Branch("forc_pos", &v_forc_pos);
  ttree_->Branch("forc_time", &v_forc_time);

  // drs4
  ttree_->Branch("trigger_cell", &f_trigger_cell, "trigger_cell/s");

  // waveforms
  for (uint8_t i_wf = 0; i_wf < 4; i_wf++) {
    if (UseWaveForm(save_waveforms_, i_wf)) {
      ttree_->Branch(Form("wf%i", i_wf), f_wf.at(i_wf), Form("wf%i[%d]/F", i_wf, n_samples_));
    }
  }

  // integrals
  ttree_->Branch("IntegralValues", &IntegralValues);
  ttree_->Branch("TimeIntegralValues", &TimeIntegralValues);
  ttree_->Branch("IntegralPeaks", &IntegralPeaks);
  ttree_->Branch("IntegralPeakTime", &IntegralPeakTime);
  ttree_->Branch("IntegralLength", &IntegralLength);
  ttree_->Branch("cft", v_cft, Form("cft[%d]/F", n_active_channels_));
  ttree_->Branch("bucket", f_bucket, Form("bucket[%d]/O", n_active_channels_));
  ttree_->Branch("ped_bucket", f_ped_bucket, Form("ped_bucket[%d]/O", n_active_channels_));
  ttree_->Branch("b2_integral", f_b2_int, Form("b2_integral[%d]/F", n_active_channels_));

  // DUT
  ttree_->Branch("is_saturated", &v_is_saturated);
  ttree_->Branch("median", &v_median);
  ttree_->Branch("average", &v_average);

  if (active_regions_) {
    ttree_->Branch(TString::Format("max_peak_position"), &v_max_peak_position);
    ttree_->Branch(TString::Format("max_peak_time"), &v_max_peak_time);
    ttree_->Branch("peak_positions", &v_peak_positions);
    ttree_->Branch("peak_times", &v_peak_times);
    ttree_->Branch("n_peaks", &v_npeaks);
    ttree_->Branch("fall_time", &v_fall_time);
    ttree_->Branch("rise_time", &v_rise_time);
    ttree_->Branch("wf_start", &v_wf_start);
    ttree_->Branch("fit_peak_time", &v_fit_peak_time);
    ttree_->Branch("fit_peak_value", &v_fit_peak_value);
    ttree_->Branch("peaking_time", &v_peaking_time);
  }
  // fft stuff and spectrum
  if (fft_waveforms) {
    ttree_->Branch("fft_mean", &fft_mean);
    ttree_->Branch("fft_mean_freq", &fft_mean_freq);
    ttree_->Branch("fft_max", &fft_max);
    ttree_->Branch("fft_max_freq", &fft_max_freq);
    ttree_->Branch("fft_min", &fft_min);
    ttree_->Branch("fft_min_freq", &fft_min_freq);
  }
  for (auto i_wf = 0; i_wf < n_channels_; i_wf++){
    if (UseWaveForm(spectrum_waveforms, i_wf)){
      ttree_->Branch(TString::Format("peaks%d_x", i_wf), &peaks_x.at(i_wf));
      ttree_->Branch(TString::Format("peaks%d_x_time", i_wf), &peaks_x_time.at(i_wf));
      ttree_->Branch(TString::Format("peaks%d_y", i_wf), &peaks_y.at(i_wf));
      ttree_->Branch(TString::Format("n_peaks%d_total", i_wf), &n_peaks_total);
      ttree_->Branch(TString::Format("n_peaks%d_before_roi", i_wf), &n_peaks_before_roi);
      ttree_->Branch(TString::Format("n_peaks%d_after_roi", i_wf), &n_peaks_after_roi);
    }
    if (UseWaveForm(fft_waveforms, i_wf)) {
      ttree_->Branch(TString::Format("fft_modes%d", i_wf), &fft_modes.at(i_wf));
      ttree_->Branch(TString::Format("fft_values%d", i_wf), &fft_values.at(i_wf));
    }
  }
  // telescope
  SetTelescopeBranches();

  EUDAQ_INFO("Done with creating Branches!");
} // end StartRun()

/** -------------------------WRITE EVENT--------------------------------- */
void FileWriterWF::WriteEvent(const DetectorEvent & ev) {
  if (ev.IsBORE()) {
    PluginManager::SetConfig(ev, m_config);
    PluginManager::Initialize(ev);
    sampling_speed_ = LoadSamplingFrequency(ev);
    tcal_ = PluginManager::GetTimeCalibration(ev);
    FillFullTime();
    macro_->AddLine("\n[Time Calibration]");
    macro_->AddLine(("tcal = [" + to_string(tcal_.at(0)) + "]").c_str());
    cout << "loading the first event...." << endl;
    //todo: add time stamp for the very first tu event
    return;
  } else if (ev.IsEORE()) {
    eudaq::PluginManager::ConvertToStandard(ev);
    cout << "loading the last event...." << endl;
    return;
  }
  f_event_number = ev.GetEventNumber();
  if (max_event_number_ > 0 && f_event_number > max_event_number_) return;

  w_total.Start(false);
  StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);

  if (sev.NumPlanes() == 0) return;

  /** TU STUFF */
  SetTimeStamp(sev);
  SetBeamCurrent(sev);
  SetScalers(sev);

  f_nwfs = sev.NumWaveforms();
  if (f_event_number <= 10 and verbose_ > 0 and not f_nwfs) { return EUDAQ_WARN("NO WAVEFORMS IN THIS EVENT!!!"); }

  ClearVectors();

  if (verbose_ > 3) cout << "event number " << f_event_number << endl;
  if (verbose_ > 3) cout << "number of waveforms " << f_nwfs << endl;

  /** Acquire the waveform data
   * use different order of wfs in order to 'know' if its a pulser event or not. */
  vector<uint8_t> wf_order = {pulser_channel};
  for (auto iwf = 0; iwf < f_nwfs; iwf++) { if (iwf != pulser_channel) wf_order.push_back(iwf); }

  for (auto iwf : wf_order) {
    sev.GetWaveform(iwf).SetPolarities(polarities_.at(iwf), pulser_polarities_.at(iwf));
    sev.GetWaveform(iwf).SetTimes(&tcal_.at(0));
  }

  ResizeVectors(sev.GetNWaveforms());
  FillRegionIntegrals(sev);
  FillBucket(sev);

  for (auto iwf : wf_order) {
    if (verbose_ > 3) cout << "Channel Nr: " << int(iwf) << endl;

    const eudaq::StandardWaveform & waveform = sev.GetWaveform(iwf);

    if (f_event_number == 0) { // save the sensor names
      sensor_names_.resize(sev.GetNWaveforms(), "");
      sensor_names_.at(iwf) = waveform.GetChannelName();
    }
    n_samples_ = waveform.GetNSamples();
    if (verbose_ > 3) cout << "number of samples in my wf " << n_samples_ << endl;

    data_ = waveform.GetData();  // load the waveforms into the vector
    calc_noise(iwf);
    FillSpectrumData(iwf);
    DoSpectrumFitting(iwf);
    DoFFTAnalysis(iwf);

    if (iwf == wf_order.at(0)) { f_trigger_cell = waveform.GetTriggerCell(); } // same for every waveform
    FillTotalRange(iwf, &waveform);

    if (verbose_ > 3) cout << "get pulser wf " << iwf << endl;
    if (iwf == pulser_channel){
      f_pulser = IsPulserEvent(&waveform);
      if (f_pulser) f_pulser_events++;
      else f_signal_events++;
    }

    UpdateWaveforms(iwf); // fill waveform vectors
    AddInfo(iwf, &waveform);
    data_->clear();
  } // end iwf waveform loop

  FillRegionVectors();
  FillTelescopeData(sev);

  ttree_->Fill();
  w_total.Stop();
} // end WriteEvent()

/** -------------------------DECONSTRUCTOR------------------------------- */
FileWriterWF::~FileWriterWF() {

  if (macro_ and ttree_) {
    macro_->AddLine("\n[Sensor Names]");
    vector<string> names;
    for (const auto &name: sensor_names_)
        names.push_back("\"" + name + "\"");
    macro_->AddLine(("Names = [" + to_string(names) + "]").c_str());
    stringstream ss;
    ss << "Summary of RUN " << run_number_ << "\n";
    uint32_t entries = ttree_->GetEntries();
    ss << "Tree has " << entries << " entries\n";
    double t = w_total.RealTime();
    ss << "\nTotal time: " << setw(2) << setfill('0') << int(t / 60) << ":" << setw(2) << setfill('0') << int(t - int(t / 60) * 60);
    if (entries > 1000) ss << "\nTime/1000 events: " << int(t / float(entries) * 1000 * 1000) << " ms";
    if (spectrum_waveforms) ss << "\nTSpectrum: " << w_spectrum.RealTime() << " seconds";
    print_banner(ss.str(), '*');
  }

  if(tfile_) { if(tfile_->IsOpen()) tfile_->cd(); }
  if(ttree_) { ttree_->Write(); }
  if (macro_) { macro_->Write(); }
  tfile_->Close();
  delete tfile_;
}

uint64_t FileWriterWF::FileBytes() const { return 0; }

inline void FileWriterWF::ClearVectors(){

  if (verbose_ > 3) { cout << "ClearVectors" << endl; }
  v_is_saturated->clear();
  v_median->clear();
  v_average->clear();
  v_forc_pos->clear();
  v_forc_time->clear();

  for (auto vec: *v_peak_positions) vec.clear();
  for (auto vec: *v_peak_times) vec.clear();

  if (spectrum_waveforms) {
    for (auto peak: peaks_x) peak->clear();
    for (auto peak: peaks_x_time) peak->clear();
    for (auto peak: peaks_y) peak->clear();
  }
} // end ClearVectors()

inline void FileWriterWF::ResizeVectors(size_t n_channels) {

  for (auto r: *regions) { r.second->Reset(); }
  v_is_saturated->resize(n_channels);
  v_median->resize(n_channels);
  v_average->resize(n_channels);
  v_max_peak_time->resize(n_channels, 0);
  v_max_peak_position->resize(n_channels, 0);
  v_rise_time->resize(n_channels);
  v_fall_time->resize(n_channels);
  v_wf_start->resize(n_channels);
  v_fit_peak_time->resize(n_channels);
  v_fit_peak_value->resize(n_channels);
  v_peaking_time->resize(n_channels);

  if (fft_waveforms) {
    fft_mean->resize(n_channels, 0);
    fft_min->resize(n_channels, 0);
    fft_max->resize(n_channels, 0);
    fft_mean_freq->resize(n_channels, 0);
    fft_min_freq->resize(n_channels, 0);
    fft_max_freq->resize(n_channels, 0);
    for (auto p: fft_modes) p->clear();
    for (auto p: fft_values) p->clear();
  }
} // end ResizeVectors()

void FileWriterWF::InitFFT() {
  if (!tfft_){
    Int_t n_samples = n_samples_ + 1;
    tfft_ = TVirtualFFT::FFT(1, &n_samples, "R2C");
    re_full_ = new Double_t[n_samples_];
    im_full_ = new Double_t[n_samples_];
    in_ = new Double_t[n_samples_];
  }
}

void FileWriterWF::DoFFTAnalysis(uint8_t iwf){
  if (!UseWaveForm(fft_waveforms, iwf)) return;
  if (verbose_ > 3) cout << "DoFFT " << iwf << endl;
  InitFFT();
  fft_mean->at(iwf) = 0;
  fft_max->at(iwf) = 0;
  fft_min->at(iwf) = 1e9;
  fft_mean_freq->at(iwf) = -1;
  fft_max_freq->at(iwf) = -1;
  fft_min_freq->at(iwf) = -1;
  w_fft.Start(false);
  float sample_rate = 2e6;
  for (uint32_t j = 0; j < n_samples_; ++j) in_[j] = data_->at(j);
  tfft_->SetPoints(in_);
  tfft_->Transform();
  tfft_->GetPointsComplex(re_full_, im_full_);
  float finalVal = 0;
  float max = -1;
  float min = 1e10;
  float freq;
  float mean_freq = 0;
  float min_freq = -1;
  float max_freq  = -1;
  float value;
  for (auto j = 0; j < (n_samples_/2+1); ++j) {
    freq = j * sample_rate / float(n_samples_);
    value = float(TMath::Sqrt(re_full_[j] * re_full_[j] + im_full_[j] * im_full_[j]));
    if (value>max){
      max = value;
      max_freq = freq;
    }
    if (value<min) {
      min = value;
      min_freq = freq;
    }
    finalVal+= value;
    mean_freq += freq * value;
    fft_values.at(iwf)->push_back(value);
    if (j < 10 || j == n_samples_/2)
      fft_modes.at(iwf)->push_back(value);
  }
  mean_freq /= finalVal;
  finalVal/= ((float(n_samples_) / 2) + 1);
  fft_mean->at(iwf) = finalVal;
  fft_max->at(iwf) = max;
  fft_min->at(iwf) = min;
  fft_mean_freq->at(iwf) = mean_freq;
  fft_max_freq->at(iwf) = max_freq;
  fft_min_freq->at(iwf) = min_freq;
  w_fft.Stop();
  if (verbose_ > 0 && f_event_number < 1000) {
    cout << run_number_ << " " << std::setw(3) << f_event_number << " " << iwf << " " << finalVal << " " << max << " " << min << endl; }
} // end DoFFTAnalysis()

inline void FileWriterWF::DoSpectrumFitting(uint8_t iwf){
  if (!UseWaveForm(spectrum_waveforms, iwf)) return;
  if (verbose_ > 3) cout << "DoSpectrumFitting " << iwf << endl;
  w_spectrum.Start(false);

  double max = *max_element(data_pos.begin(), data_pos.end());
  double threshold = 4 * noise_.at(iwf).second + noise_.at(iwf).first;
  if (max <= threshold) return; //return if the max element is lower than 4 sigma of the noise

  threshold = threshold / max * 100; // TSpectrum threshold is in per cent to max peak
  size_t size = data_pos.size();

  n_peaks_total = 0;
  n_peaks_before_roi = 0;
  n_peaks_after_roi = 0;

  int peaks = spec_->SearchHighRes(&data_pos[0], &decon[0], size, spec_sigma, threshold, spec_rm_bg, spec_decon_iter, spec_markov, spec_aver_win);
  n_peaks_total = peaks; //this goes in the root file
  for(auto i=0; i < peaks; i++){
    auto bin = uint16_t(round(spec_->GetPositionX()[i] + .5));
    uint16_t min_bin = bin - 5 >= 0 ? uint16_t(bin - 5) : uint16_t(0);
    uint16_t max_bin = bin + 5 < size ? uint16_t(bin + 5) : uint16_t(size - 1);
    max = *std::max_element(&data_pos.at(min_bin), &data_pos.at(max_bin));
    peaks_x.at(iwf)->push_back(bin);
    float peaktime = GetTriggerTime(iwf, bin);

    if (peaktime < peak_finding_roi.first){ n_peaks_before_roi++;}
    if (peaktime >= peak_finding_roi.second){ n_peaks_after_roi++;}

    peaks_x_time.at(iwf)->push_back(peaktime);
    peaks_y.at(iwf)->push_back(max);
  }
    w_spectrum.Stop();
} // end DoSpectrumFitting()

void FileWriterWF::FillSpectrumData(uint8_t iwf){

  bool b_spectrum = UseWaveForm(spectrum_waveforms, iwf);
  bool b_fft = UseWaveForm(fft_waveforms, iwf);
  if(b_spectrum || b_fft) {
    data_pos.resize(data_->size());
    for (auto i = 0; i < data_->size(); i++)
      data_pos.at(i) = float(spectrum_polarities.at(iwf)) * data_->at(i);
  }
} // end FillSpectrumData()

void FileWriterWF::calc_noise(uint8_t iwf) {
  float value = data_->at(peak_noise_pos);
  // filter out peaks at the pedestal
  if (abs(value) < 6 * noise_.at(iwf).second + abs(noise_.at(iwf).first) or noise_vectors.at(iwf).size() < 10)
    noise_vectors.at(iwf).push_back(value);
  if (noise_vectors.at(iwf).size() >= 1000)
    noise_vectors.at(iwf).pop_front();
  noise_.at(iwf) = calc_mean(noise_vectors.at(iwf));
}

void FileWriterWF::FillBucket(const StandardEvent & sev) {
  /** fills the integral noise for the bucket cut calculations */
  auto bw = uint16_t(round(sampling_speed_ * bunch_width_));
  uint8_t j(0);
  for (auto ch: *regions){
    uint8_t i_ch = ch.first;
    const StandardWaveform * wf = &sev.GetWaveform(i_ch);
    WaveformSignalRegion * r = ch.second->GetRegion("signal_b");
    WaveformIntegral * i = r->GetIntegralPointer("PeakIntegral2");

    /** noise */
    float noise = wf->getIntegral(r, i, sampling_speed_, -bw);  // find highest value and integrate as for signal
    if (abs(noise) < 4 * int_noise_.at(i_ch).second + abs(int_noise_.at(i_ch).first) or int_noise_vectors.at(i_ch).size() < 10) {
      int_noise_vectors.at(i_ch).push_back(noise); }
    if (int_noise_vectors.at(i_ch).size() >= 1000) {
      int_noise_vectors.at(i_ch).pop_front(); }
    int_noise_.at(i_ch) = calc_mean(int_noise_vectors.at(i_ch));

    /** bucket */
    float pol = polarities_.at(i_ch);
    f_b2_int[j] = pol * wf->getIntegral(r, i, sampling_speed_, bw);
    float thresh = GetNoiseThreshold(i_ch, 3);
    f_bucket[j] = f_b2_int[j] > thresh and pol * i->GetTimeIntegral() < thresh; // sig < thresh and bucket 2 > thresh
    float intm1 = r->GetLowBoarder() - 2 * bw < 0 ? 0 : pol * wf->getIntegral(r, i, sampling_speed_, -2 * bw);
    f_ped_bucket[j++] = intm1 > GetNoiseThreshold(i_ch, 4);
  }
}

void FileWriterWF::FillRegionIntegrals(const StandardEvent & sev){

  uint8_t i = 0;
  for (auto channel: *regions) {
    const StandardWaveform * wf = &sev.GetWaveform(channel.first);
    channel.second->GetRegion(0)->SetPeakPostion(5);
    for (auto region: channel.second->GetRegions()){
      signed char polarity = (string(region->GetName()).find("pulser") != string::npos) ? channel.second->GetPulserPolarity() : channel.second->GetPolarity();
      uint16_t peak_pos = wf->FindPeakIndex(region->GetLowBoarder(), region->GetHighBoarder(), sampling_speed_, polarity);
      region->SetPeakPostion(peak_pos);
      if (string(region->GetName()) == "signal_b"){  // TODO change this naming or define a definite signal region
        v_cft[i++] = wf->getCFT(region->GetLowBoarder(), region->GetHighBoarder(), rise_time_ * 2); }
      for (auto integral: region->GetIntegrals()){
        std::string name = integral->GetName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        integral->SetPeakPosition(peak_pos, wf->GetNSamples());
        integral->SetTimeIntegral(wf->getIntegral(integral->GetDownRange(), integral->GetUpRange(), peak_pos, sampling_speed_));
        if (name.find("peaktopeak") != string::npos) {
          integral->SetIntegral(wf->getPeakToPeak(integral->GetIntegralStart(), integral->GetIntegralStop()));
        } else if (name.find("median") != string::npos) {
          integral->SetIntegral(wf->getMedian(integral->GetIntegralStart(), integral->GetIntegralStop()));
        } else
          integral->SetIntegral(wf->getAverage(integral->GetDownRange(), integral->GetUpRange()));
      }
    }
  }
} // end FillRegionIntegrals()

void FileWriterWF::FillRegionVectors(){
  IntegralValues->clear();
  TimeIntegralValues->clear();
  IntegralPeaks->clear();
  IntegralPeakTime->clear();
  IntegralLength->clear();
  for (auto ch: *regions){
    for (auto region: ch.second->GetRegions()) {
      uint16_t peak_pos = region->GetPeakPosition();
      for (auto integral: region->GetIntegrals()) {
        IntegralValues->push_back(integral->GetIntegral());
        TimeIntegralValues->push_back(integral->GetTimeIntegral());
        IntegralPeaks->emplace_back(peak_pos);
        IntegralPeakTime->push_back(GetTriggerTime(ch.first, peak_pos));
        IntegralLength->push_back(GetTimeDifference(ch.first, integral->GetIntegralStart(), integral->GetIntegralStop()));
      }
    }
  }
}

void FileWriterWF::FillTotalRange(uint8_t iwf, const StandardWaveform *wf){

  if (verbose_ > 3) cout << "Getting total range data " << iwf << endl;
  signed char pol = polarities_.at(iwf);
  v_is_saturated->at(iwf) = wf->getAbsMaxInRange(0, 1023) > 498; // indicator if saturation is reached in sampling region (1-1024)
  v_median->at(iwf) = float(pol) * wf->getMedian(0, 1023); // Median over whole sampling region
  v_average->at(iwf) = float(pol) * wf->getAverage(0, 1023);
  if (UseWaveForm(active_regions_, iwf)){
    WaveformSignalRegion * reg = regions->at(iwf)->GetRegion("signal_b");
    v_rise_time->at(iwf) = wf->getRiseTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_.at(iwf).first);
    v_fall_time->at(iwf) = wf->getFallTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_.at(iwf).first);
    auto fit_peak = wf->fitMaximum(reg->GetLowBoarder(), reg->GetHighBoarder());
    v_fit_peak_time->at(iwf) = fit_peak.first;
    v_fit_peak_value->at(iwf) = fit_peak.second;
    v_wf_start->at(iwf) = wf->getWFStartTime(reg->GetLowBoarder(), reg->GetHighBoarder(), noise_.at(iwf).first, fit_peak.second);
    v_peaking_time->at(iwf) = v_fit_peak_time->at(iwf) - v_wf_start->at(iwf);
    pair<uint16_t, float> peak = wf->getMaxPeak();
    v_max_peak_position->at(iwf) = peak.first;
    v_max_peak_time->at(iwf) = GetTriggerTime(iwf, peak.first);
    float threshold = float(polarities_.at(iwf)) * 4 * noise_.at(iwf).second + noise_.at(iwf).first;
    vector<uint16_t> * peak_positions = wf->getAllPeaksAbove(0, 1023, threshold);
    v_peak_positions->at(iwf) = *peak_positions;
    v_npeaks->at(iwf) = uint8_t(v_peak_positions->at(iwf).size());
    vector<float> peak_timings;
    for (auto i_pos:*peak_positions) peak_timings.push_back(GetTriggerTime(iwf, i_pos));
    v_peak_times->at(iwf) = peak_timings;
  }
}

void FileWriterWF::UpdateWaveforms(uint8_t iwf){
  if (verbose_ > 3) { cout << "fill wf " << iwf << endl; }
  if (UseWaveForm(save_waveforms_, iwf)) { copy(data_->begin(), data_->end(), f_wf.at(iwf)); }
} // end UpdateWaveforms()

inline bool FileWriterWF::IsPulserEvent(const StandardWaveform *wf) const {
  float pulser_int = wf->getAverage(pulser_region.first, pulser_region.second, true);
  return pulser_int > float(pulser_threshold);
} //end IsPulserEvent

void FileWriterWF::FillFullTime(){
  for (auto i_ch: tcal_) {
    i_ch.second.resize(n_samples_);
    float sum = 0;
    full_time_[i_ch.first] = vector<float>();
    full_time_.at(i_ch.first).push_back(sum);
    for (auto j = 0; j < i_ch.second.size() * 2 - 1; j++) {
      sum += i_ch.second.at(uint16_t(j % n_samples_));
      full_time_.at(i_ch.first).push_back(sum);
    }
  }
}

inline float FileWriterWF::GetTriggerTime(const uint8_t & ch, const uint16_t & bin) {
    return full_time_.at(ch).at(bin + f_trigger_cell) - full_time_.at(ch).at(f_trigger_cell);
}

float FileWriterWF::GetTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up) {
    return full_time_.at(ch).at(bin_up + f_trigger_cell) - full_time_.at(ch).at(uint16_t(bin_low + f_trigger_cell));
}

string FileWriterWF::GetBitMask(uint16_t bitmask) const {
  stringstream ss;
  for (auto i = 0; i < n_channels_; i++) {
      string bit = to_string(UseWaveForm(bitmask, i));
      ss << string(3 - bit.size(), ' ') << bit;
  }
  return trim(ss.str(), " ");
}

string FileWriterWF::GetPolarityStr(const vector<signed char> &pol) {
    stringstream ss;
    for (auto i_pol:pol) ss << string(3 - to_string(i_pol).size(), ' ') << to_string(i_pol);
    return trim(ss.str(), " ");
}

void FileWriterWF::SetTimeStamp(StandardEvent sev) {
  if (sev.hasTUEvent()){
    if (sev.GetTUEvent(0).GetValid())
      f_time = sev.GetTimestamp();
  } else
    f_time = sev.GetTimestamp() / 384066.;
}

void FileWriterWF::SetBeamCurrent(StandardEvent sev) {

  if (sev.hasTUEvent()){
    StandardTUEvent tuev = sev.GetTUEvent(0);
    f_beam_current = uint16_t(tuev.GetValid() ? tuev.GetBeamCurrent() : UINT16_MAX);
  }
}

void FileWriterWF::SetScalers(StandardEvent sev) {

  if (sev.hasTUEvent()) {
    StandardTUEvent tuev = sev.GetTUEvent(0);
    bool valid = tuev.GetValid();
    /** scaler continuously count upwards: subtract old scaler value and divide by time interval to get rate
      * first scaler value is the scintillator and then the planes */
    for (uint8_t i(0); i < 5; i++) {
      v_scaler->at(i) = uint64_t(valid ? double(tuev.GetScalerValue(i) - old_scaler->at(i)) * 1000 / (f_time - old_time) : UINT32_MAX);
      if (valid) { old_scaler->at(i) = tuev.GetScalerValue(i); }
    }
    if (valid) { old_time = f_time; }
  }
}

void FileWriterWF::ReadIntegralRanges() {

  for (auto i_ch(0); i_ch < n_channels_; i_ch++) {
    if (not UseWaveForm(active_regions_, i_ch)) continue;
    ranges[i_ch]["PeakIntegral1"] = new pair<float, float>(m_config->Get("PeakIntegral1_range", make_pair(8, 12))); // Default range
  }
  // additional ranges from the config file
  for (auto i_key: m_config->GetKeys()) {
    if (i_key.find("pulser") != string::npos) continue; // skip if it's the pulser range for the drs4
    size_t found = i_key.find("_range");
    if (found == string::npos) continue;
    if (i_key.at(0) == '#') continue;
    string name = i_key.substr(0, found);
    vector<uint16_t> tmp = {0, 0, 0, 0, 0, 0, 0, 0};
    vector<uint16_t> range_def = from_string(m_config->Get(i_key, "bla"), tmp);
    uint8_t i(0);
    for (auto i_ch(0); i_ch < n_channels_; i_ch++) {
      if (not UseWaveForm(active_regions_, i_ch)) continue;  // skip if the channel has no signal waveform
      if (ranges.at(i_ch).count(name) > 0) continue;        // skip if the range already exists
      ranges.at(i_ch)[name] = new pair<float, float>(make_pair(range_def.at(i), range_def.at(i + 1)));
      if (range_def.size() > i + 2) i += 2;
    }
  }
}

void FileWriterWF::ReadIntegralRegions() {

  for (const auto &i_key: m_config->GetKeys()){
    if (i_key.find("active_regions") != string::npos) continue;
    if (i_key.find("pulser_channel") != string::npos) continue;
    size_t found = i_key.find("_region");
    if (found == string::npos) continue;
    string name = i_key.substr(0, found);
    vector<uint16_t> tmp = {0, 0, 0, 0, 0, 0, 0, 0};
    vector<uint16_t> region_def = from_string(m_config->Get(i_key, "bla"), tmp);
    uint8_t i(0);
    for (const auto & ch: ranges) {
      WaveformSignalRegion region = WaveformSignalRegion(region_def.at(i), region_def.at(i + 1), name);
      if (region_def.size() > i + 2) i += 2;
      // add all PeakIntegral ranges for the region
      for (const auto & range: ch.second){
        if (range.first.find("PeakIntegral") == string::npos) continue;  // only consider the integral ranges around the peak
        WaveformIntegral integralDef = WaveformIntegral(int(range.second->first), int(range.second->second), range.first);
        region.AddIntegral(integralDef);
      }
      regions->at(ch.first)->AddRegion(region);
    }
  }
}

float FileWriterWF::GetNoiseThreshold(uint8_t i_wf, float n_sigma) {
  /** :returns:  threshold based on the current noise */
  return float(polarities_.at(i_wf)) * int_noise_.at(i_wf).first + n_sigma * int_noise_.at(i_wf).second;
}

void FileWriterWF::SetTelescopeBranches() {

  ttree_->Branch("n_hits_tot", &f_n_hits, "n_hits_tot/b");
  ttree_->Branch("plane", f_plane, "plane[n_hits_tot]/b");
  ttree_->Branch("col", f_col, "col[n_hits_tot]/b");
  ttree_->Branch("row", f_row, "row[n_hits_tot]/b");
  ttree_->Branch("adc", f_adc, "adc[n_hits_tot]/S");
  ttree_->Branch("charge", f_charge, "charge[n_hits_tot]/F");
  ttree_->Branch("trigger_phase", &f_trig_phase, "trigger_phase/b");
}

void FileWriterWF::FillTelescopeData(const StandardEvent & sev) {

  f_trig_phase = sev.GetPlane(0).GetTrigPhase();  // there is only one trigger phase for the whole telescope (one DTB)
  f_n_hits = 0;
  for (auto iplane(0); iplane < sev.NumPlanes(); ++iplane) {
    const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
    std::vector<double> cds = plane.GetPixels<double>();
    for (auto ipix(0); ipix < cds.size(); ++ipix) {
      f_plane[f_n_hits] = iplane;
      f_col[f_n_hits] = uint8_t(plane.GetX(ipix));
      f_row[f_n_hits] = uint8_t(plane.GetY(ipix));
      f_adc[f_n_hits] = int16_t(plane.GetPixel(ipix));
      f_charge[f_n_hits++] = 42;						// todo: do charge conversion here!
    }
  }
}

void FileWriterWF::InitTelescopeArrays() {

  f_n_hits = 0;
  f_plane  = new uint8_t[MAX_SIZE];
  f_col    = new uint8_t[MAX_SIZE];
  f_row    = new uint8_t[MAX_SIZE];
  f_adc    = new int16_t [MAX_SIZE];
  f_charge = new float[MAX_SIZE];
}

#endif // ROOT_FOUND
