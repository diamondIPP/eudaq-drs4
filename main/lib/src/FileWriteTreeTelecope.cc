#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include <Math/MinimizerOptions.h>
#include "TH1F.h"
#include "TF1.h"
#include "TString.h"
#include "TFitResultPtr.h"
#include "eudaq/FileWriterTelescope.hh"

using namespace std;
using namespace eudaq;


namespace { static RegisterFileWriter<FileWriterTreeTelescope> reg("telescopetree"); }

FileWriterTreeTelescope::FileWriterTreeTelescope(const std::string & /*param*/)
: name_("telescopetree"), run_number_(0), max_event_number_(0), has_tu_(false), verbose_(0), at_plane_(0), tfile_(nullptr), ttree_(nullptr),
  f_event_number(0), f_time(-1), old_time(0), f_n_hits(0), f_beam_current(UINT16_MAX) {

  gROOT->ProcessLine("gErrorIgnoreLevel = 5001;");
  gROOT->ProcessLine("#include <vector>");
}

void FileWriterTreeTelescope::Configure() {

  if (m_config == nullptr) { return EUDAQ_WARN("Configuration class instance m_config does not exist!"); }

  m_config->SetSection(Form("Converter.%s", name_.c_str()));
  if (m_config->NSections() == 0) { return EUDAQ_WARN("Config file has no sections!"); }
  EUDAQ_INFO(Form("Configuring FileWriter %s\n", name_.c_str()));

  max_event_number_ = m_config->Get("max_event_number", 0);
  cout << "\nMAXIMUM NUMBER OF EVENTS: " << (max_event_number_ ? to_string(max_event_number_) : "ALL") << endl;
}

void FileWriterTreeTelescope::StartRun(unsigned run_number) {

  run_number_ = run_number;
  EUDAQ_INFO(Form("Converting the input file into a %s TTree", name_.c_str()));
  string f_output = FileNamer(m_filepattern).Set('X', ".root").Set('R', run_number);
  EUDAQ_INFO("Preparing the output file: " + f_output);

  tfile_ = new TFile(f_output.c_str(), "RECREATE");
  ttree_ = new TTree("tree", "EUDAQ TTree with telescope and waveform branches");

  // Set Branch Addresses
  ttree_->Branch("event_number", &f_event_number, "event_number/I");
  ttree_->Branch("time", &f_time, "time/D");
  if (has_tu_) {
    ttree_->Branch("beam_current", &f_beam_current, "beam_current/s");
    ttree_->Branch("rate", v_scaler, "rate[5]/l");
  }

  ttree_->Branch("n_hits_tot", &f_n_hits);
  ttree_->Branch("plane", f_plane, "plane[n_hits_tot]/b");
  ttree_->Branch("col", f_col, "col[n_hits_tot]/b");
  ttree_->Branch("row", f_row, "row[n_hits_tot]/b");
  ttree_->Branch("adc", f_adc, "adc[n_hits_tot]/S");
  ttree_->Branch("charge", f_charge, "charge[n_hits_tot]/F");
  ttree_->Branch("trigger_phase", f_trig_phase, "trigger_phase[2]/b");
  EUDAQ_INFO("Done with creating telescope branches!");
}

void FileWriterTreeTelescope::InitBORE(const DetectorEvent & dev) {

  cout << "loading the first event ..." << endl;
  PluginManager::SetConfig(dev, m_config);
  PluginManager::Initialize(dev);
  //todo: add time stamp for the very first tu event
}

void FileWriterTreeTelescope::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
      InitBORE(ev);
      return;
    } else if (ev.IsEORE()) {
        eudaq::PluginManager::ConvertToStandard(ev);
        cout << "loading the last event..." << endl;
        return;
    }
  f_event_number = ev.GetEventNumber();
  if (verbose_ > 3) { cout << "Event number: " << f_event_number << endl; }
  if (max_event_number_ > 0 && f_event_number > max_event_number_) return;

  StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);

  /** TU STUFF */
  SetTimeStamp(sev);
  SetBeamCurrent(sev);
  SetScalers(sev);

  f_n_hits = 0;
  at_plane_ = 0;
  FillTelescopeArrays(sev, false);  // first fill the telescope planes and then the DUT
  FillTelescopeArrays(sev, true);  // nothing happens in case there are no dut planes

  AddWrite(sev);
  ttree_->Fill();
}

FileWriterTreeTelescope::~FileWriterTreeTelescope() {

  if (tfile_ != nullptr) {
    if (tfile_->IsOpen()) { tfile_->cd(); }
    if (ttree_) { ttree_->Write(); }
    tfile_->Close();
    delete tfile_;
  }
}

void FileWriterTreeTelescope::SetTimeStamp(StandardEvent sev) {

  f_time = sev.hasTUEvent() ? (sev.GetTUEvent(0).GetValid() ? double(sev.GetTimestamp()) : f_time) : double(sev.GetTimestamp()) / 384066.;
}

void FileWriterTreeTelescope::SetBeamCurrent(StandardEvent sev) {

  if (sev.hasTUEvent()){
    StandardTUEvent tuev = sev.GetTUEvent(0);
    f_beam_current = uint16_t(tuev.GetValid() ? tuev.GetBeamCurrent() : UINT16_MAX);
  }
}

void FileWriterTreeTelescope::SetScalers(StandardEvent sev) {

  if (sev.hasTUEvent()) {
    StandardTUEvent tuev = sev.GetTUEvent(0);
    bool valid = tuev.GetValid();
    /** scaler continuously count upwards: subtract old scaler value and divide by time interval to get rate
      * first scaler value is the scintillator and then the planes */
    for (uint8_t i(0); i < 5; i++) {
      v_scaler[i] = uint64_t(valid ? double(tuev.GetScalerValue(i) - old_scaler[i]) * 1000 / (f_time - old_time) : UINT32_MAX);
      if (valid) { old_scaler[i] = tuev.GetScalerValue(i); }
    }
    if (valid) { old_time = f_time; }
  }
}

void FileWriterTreeTelescope::FillTelescopeArrays(const StandardEvent & sev, bool is_dut) {

  for (auto iplane(0); iplane < sev.NumPlanes(); ++iplane) {
    const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
    bool is_telescope = plane.Sensor() == "DUT";  // the telescope planes are called DUT here...
    if (is_telescope == is_dut) { continue; }
    std::vector<double> cds = plane.GetPixels<double>();
    f_trig_phase[is_dut ? 1 : 0] = uint8_t(plane.GetTrigPhase());
    for (auto ipix(0); ipix < cds.size(); ++ipix) {
      f_plane[f_n_hits] = at_plane_;
      f_col[f_n_hits] = uint8_t(plane.GetX(ipix));
      f_row[f_n_hits] = uint8_t(plane.GetY(ipix));
      f_adc[f_n_hits] = int16_t(plane.GetPixel(ipix));
      f_charge[f_n_hits++] = 42;						// todo: do charge conversion here!
    }
    at_plane_++;
  }
}

#endif // ROOT_FOUND
