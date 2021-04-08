#ifdef ROOT_FOUND

#include "eudaq/FileWriterTreeCAEN.hh"

// eudaq imports
#include "eudaq/FileNamer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "include/SimpleStandardEvent.hh"

// ROOT imports
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TSystem.h"
#include "TInterpreter.h"
#include "TMacro.h"
#include "TF1.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TSpectrum.h"
#include "TPolyMarker.h"

#define MAX_SIZE 255

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeCAEN> reg("caentree"); }

/** =====================================================================
    --------------------------CONSTRUCTOR--------------------------------
    =====================================================================*/
FileWriterTreeCAEN::FileWriterTreeCAEN(const std::string & /*param*/)
: bunch_width_(19.76), sampling_speed_(2.5), m_tfile(nullptr), m_ttree(nullptr), m_noe(0), n_channels(9), n_pixels(90*90+60*60),
  histo(nullptr), spec(nullptr), fft_own(nullptr), runnumber(0) {

    gROOT->ProcessLine("gErrorIgnoreLevel = 5001;");
    gROOT->ProcessLine("#include <vector>");
    gROOT->ProcessLine("#include <map>");
    gInterpreter->GenerateDictionary("vector<vector<float> >;vector<vector<uint16> >", "vector");

    //Polarities of signals, default is positive signals
    polarities.resize(9, 1);
    pulser_polarities.resize(9, 1);
    spectrum_polarities.resize(9, 1);
    dia_channels = new vector<uint16_t>;

    //how many events will be analyzed, 0 = all events
    max_event_number = 0;
    macro = new TMacro();
    macro->SetNameTitle("region_information", "Region Information");

    f_nwfs = 0;
    f_event_number = -1;
    f_pulser_events = 0;
    f_signal_events = 0;
    f_time = -1.;
    f_pulser = false;
    f_rf_period = 0;
    f_rf_phase = 0;
    f_rf_chi2 = UINT16_MAX;
    f_beam_current = UINT16_MAX;
    v_forc_pos = new vector<uint16_t>;
    v_forc_time = new vector<float>;

    f_trigger_cell = 0;
    f_trigger_time = 0;

    // integrals
    regions = new std::map<int,WaveformSignalRegions* >;
    IntegralNames = new std::vector<std::string>;
    IntegralValues = new std::vector<float>;
    TimeIntegralValues = new vector<float>;
    IntegralPeaks = new std::vector<Int_t>;
    IntegralPeakTime = new std::vector<float>;
    IntegralLength  = new std::vector<float>;
    f_bucket = new bool[n_channels];
    f_ped_bucket = new bool[n_channels];

    // general waveform information
    v_is_saturated = new vector<bool>;
    v_median = new vector<float>;
    v_average = new vector<float>;
    v_max_peak_position = new vector<uint16_t>;
    v_signal_peak_time = new vector<float>;
    v_rise_time = new vector<float>;
    v_fall_time = new vector<float>;
    v_max_peak_time = new vector<float>;
    v_max_peak_position->resize(9, 0);
    v_peak_positions = new vector<vector<uint16_t> >;
    v_peak_times = new vector<vector<float> >;
    v_peak_positions->resize(9);
    v_peak_times->resize(9);
    v_npeaks = new vector<uint8_t>;
    v_npeaks->resize(9, 0);
    v_max_peak_time->resize(9, 0);

  // waveforms
    for (uint8_t i = 0; i < 9; i++) f_wf[i] = new vector<float>;
    f_isDa = new vector<bool>;
    wf_thr = {125, 10, 40, 300, 200, 300, 200, 200, 200};

    // spectrum vectors
    noise_ = new vector<pair<float, float> >;
    noise_->resize(9);
    int_noise_.resize(n_channels);
    int_noise_vectors.resize(n_channels);
    for (uint8_t i = 0; i < 9; i++) noise_vectors[i] = new deque<float>;
    decon.resize(1024, 0);
    peaks_x.resize(9, new std::vector<uint16_t>);
    peaks_x_time.resize(9, new std::vector<float>);
    peaks_y.resize(9, new std::vector<float>);

    // fft analysis
    fft_modes.resize(9, new std::vector<float>);
    fft_values.resize(9, new std::vector<float>);
    fft_mean = new std::vector<float>;
    fft_mean_freq = new std::vector<float>;
    fft_max = new std::vector<float>;
    fft_max_freq = new std::vector<float>;
    fft_min = new std::vector<float>;
    fft_min_freq = new std::vector<float>;

    // telescope
    InitTelescopeArrays();
    f_trig_phase = 0;

    // average waveforms of channels
    avgWF_0 = new TH1F("avgWF_0","avgWF_0", 1024, 0, 1024);
    avgWF_0_pul = new TH1F("avgWF_0_pul","avgWF_0_pul", 1024, 0, 1024);
    avgWF_0_sig = new TH1F("avgWF_0_sig","avgWF_0_sig", 1024, 0, 1024);
    avgWF_1 = new TH1F("avgWF_1","avgWF_1", 1024, 0, 1024);
    avgWF_2 = new TH1F("avgWF_2","avgWF_2", 1024, 0, 1024);
    avgWF_3 = new TH1F("avgWF_3","avgWF_3", 1024, 0, 1024);
    avgWF_3_pul = new TH1F("avgWF_3_pul","avgWF_3_pul", 1024, 0, 1024);
    avgWF_3_sig = new TH1F("avgWF_3_sig","avgWF_3_sig", 1024, 0, 1024);

    spec = new TSpectrum(25);
    fft_own = nullptr;
    if(fft_own == nullptr){
        int n = 1024;
        n_samples = n + 1;
        cout << "Creating a new VirtualFFT with " << n_samples << " Samples" << endl;
        re_full = new Double_t[n];
        im_full = new Double_t[n];
        in = new Double_t[n];
        fft_own = TVirtualFFT::FFT(1, &n_samples, "R2C");
    }
} // end Constructor

/** =====================================================================
    --------------------------CONFIGURE----------------------------------
    =====================================================================*/
void FileWriterTreeCAEN::Configure(){

    // do some assertions
    if (this->m_config == nullptr){
        EUDAQ_WARN("Configuration class instance m_config does not exist!");
        return;
    }
    m_config->SetSection("Converter.caentree");
    if (m_config->NSections() == 0){
        EUDAQ_WARN("Config file has no sections!");
        return;
    }
    cout << endl;
    EUDAQ_INFO("Configuring FileWriterTreeCAEN");

    max_event_number = m_config->Get("max_event_number", 0);

    // spectrum and fft
    peak_noise_pos = m_config->Get("peak_noise_pos", unsigned(0));
    spec_sigma = m_config->Get("spectrum_sigma", 5);
    spec_decon_iter = m_config->Get("spectrum_deconIterations", 3);
    spec_aver_win = m_config->Get("spectrum_averageWindow", 5);
    spec_markov = m_config->Get("spectrum_markov", true);
    spec_rm_bg = m_config->Get("spectrum_background_removal", true);
    spectrum_waveforms = m_config->Get("spectrum_waveforms", uint16_t(0));
    fft_waveforms = m_config->Get("fft_waveforms", uint16_t(0));
    fit_rf = m_config->Get("fit_rf", true);

    // channels
    pulser_threshold = m_config->Get("pulser_channel_threshold", 80);
    pulser_channel = uint8_t(m_config->Get("pulser_channel", 1));
    trigger_channel = uint8_t(m_config->Get("trigger_channel", 99));
    rf_channel = uint8_t(m_config->Get("rf_channel", 99));
    scint_channel = uint8_t(m_config->Get("scint_channel", 99));

    // saved waveforms
    save_waveforms = m_config->Get("save_waveforms", uint16_t(9));

    // polarities
    polarities = m_config->Get("polarities", polarities);
    pulser_polarities = m_config->Get("pulser_polarities", pulser_polarities);
    spectrum_polarities = m_config->Get("spectrum_polarities", spectrum_polarities);

    // regions todo: add default range
    active_regions = m_config->Get("active_regions", uint16_t(0));
    macro->AddLine("[General]");
    macro->AddLine((append_spaces(21, "max event number = ") + to_string(max_event_number)).c_str());
    macro->AddLine((append_spaces(21, "pulser channel = ") + to_string(int(pulser_channel))).c_str());
    macro->AddLine((append_spaces(21, "rf channel = ") + to_string(int(rf_channel))).c_str());
    macro->AddLine((append_spaces(21, "fit rf = ") + to_string(int(fit_rf))).c_str());
    macro->AddLine((append_spaces(21, "scint channel = ") + to_string(int(scint_channel))).c_str());
    macro->AddLine((append_spaces(21, "trigger channel = ") + to_string(int(trigger_channel))).c_str());
    macro->AddLine((append_spaces(21, "pulser threshold = ") + to_string(pulser_threshold)).c_str());
    macro->AddLine((append_spaces(21, "active regions = ") + to_string(GetBitMask(active_regions))).c_str());
    macro->AddLine((append_spaces(21, "save waveforms = ") + GetBitMask(save_waveforms)).c_str());
    macro->AddLine((append_spaces(21, "fft waveforms = ") + GetBitMask(fft_waveforms)).c_str());
    macro->AddLine((append_spaces(21, "spectrum waveforms = ") + GetBitMask(spectrum_waveforms)).c_str());
    macro->AddLine((append_spaces(20, "polarities = ") + GetPolarities(polarities)).c_str());
    macro->AddLine((append_spaces(20, "pulser polarities = ") + GetPolarities(pulser_polarities)).c_str());
    macro->AddLine((append_spaces(20, "spectrum_polarities = ") + GetPolarities(spectrum_polarities)).c_str());

    for (uint8_t i = 0; i < n_channels; i++) if (UseWaveForm(active_regions, i)) n_active_channels++;
    for (uint8_t i(0); i < n_channels; i++)
        if (UseWaveForm(active_regions, i))
            dia_channels->emplace_back(i);
    for (uint8_t i = 0; i < n_channels; i++)
        if (UseWaveForm(active_regions, i)) (*regions)[i] = new WaveformSignalRegions(i, polarities.at(i), pulser_polarities.at(i));

    // read the peak integral ranges from the config file
    pulser_region = m_config->Get("pulser_channel_region", make_pair(800, 1000));
    ReadIntegralRanges();
    for (auto ch: ranges) {
        macro->AddLine(TString::Format("\n[Integral Ranges %d]", ch.first));
        for (auto range: ch.second)
            macro->AddLine((append_spaces(21, range.first + " =") + to_string(*range.second)).c_str());
    }

    // read the region where we want to find the peak from the config file
    ReadIntegralRegions();
    for (auto ch: *regions) {
        macro->AddLine(TString::Format("\n[Integral Regions %d]", ch.first));
        for (auto region: ch.second->GetRegions())
            macro->AddLine((append_spaces(21, to_string(region->GetName()) + " =") + to_string(region->GetRegion())).c_str());
    }

    macro->Print();
    // add the list of all the integral names corresponding to the entries in the integral vectors
    macro->AddLine("\n[Integral Names]");
    vector<TString> names;
    for (auto ch: *regions)
      for (auto region: ch.second->GetRegions())
        for (auto integral: region->GetIntegrals())
          names.push_back(TString::Format("\"ch%d_%s_%s\"", int(ch.first), region->GetName(), integral->GetName().c_str()));
    macro->AddLine(("Names = [" + to_string(names) + "]").c_str());

    cout << "\nMAXIMUM NUMBER OF EVENTS: " << (max_event_number ? to_string(max_event_number) : "ALL") << endl;
    EUDAQ_INFO("End of Configure!");

    cout << endl;
    macro->Write();
} // end Configure()

/** =====================================================================
    --------------------------START RUN----------------------------------
    =====================================================================*/
void FileWriterTreeCAEN::StartRun(unsigned runnumber) {
    this->runnumber = runnumber;
    EUDAQ_INFO("Converting the input file into a CAEN TTree " );
    string foutput = FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber);
    EUDAQ_INFO("Preparing the output file: " + foutput);

    c1 = new TCanvas();
    c1->Draw();

    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");

    // Set Branch Addresses
    m_ttree->Branch("event_number", &f_event_number, "event_number/I");
    m_ttree->Branch("time",& f_time, "time/D");
    m_ttree->Branch("pulser",& f_pulser, "pulser/O");
    m_ttree->Branch("nwfs", &f_nwfs, "n_waveforms/I");
    m_ttree->Branch("beam_current", &f_beam_current, "beam_current/s");
    m_ttree->Branch("forc_pos", &v_forc_pos);
    m_ttree->Branch("forc_time", &v_forc_time);

    //beam rf
    if (rf_channel < 32 ){
      m_ttree->Branch("rf_period", &f_rf_period, "rf_period/F");
      m_ttree->Branch("rf_phase",& f_rf_phase, "rf_phase/F");
      m_ttree->Branch("rf_chi2",& f_rf_chi2, "rf_chi2/F");
    }

    // drs4
    m_ttree->Branch("trigger_cell", &f_trigger_cell, "trigger_cell/s");
    if (trigger_channel > 0)
      m_ttree->Branch("trigger_time", &f_trigger_time, "trigger_time/F");

    // waveforms
    for (uint8_t i_wf = 0; i_wf < 9; i_wf++)
        if ((save_waveforms & 1 << i_wf) == 1 << i_wf)
            m_ttree->Branch(TString::Format("wf%i", i_wf), &f_wf.at(i_wf));
    m_ttree->Branch("wf_isDA", &f_isDa);

    // integrals
    m_ttree->Branch("IntegralNames",&IntegralNames);
    m_ttree->Branch("IntegralValues",&IntegralValues);
    m_ttree->Branch("TimeIntegralValues",&TimeIntegralValues);
    m_ttree->Branch("IntegralPeaks",&IntegralPeaks);
    m_ttree->Branch("IntegralPeakTime",&IntegralPeakTime);
    m_ttree->Branch("IntegralLength",&IntegralLength);
    m_ttree->Branch("bucket", f_bucket, TString::Format("bucket[%d]/O", n_active_channels));
    m_ttree->Branch("ped_bucket", f_ped_bucket, TString::Format("ped_bucket[%d]/O", n_active_channels));

    // DUT
    m_ttree->Branch("is_saturated", &v_is_saturated);
    m_ttree->Branch("median", &v_median);
    m_ttree->Branch("average", &v_average);

    if (active_regions > 0){
      m_ttree->Branch("rise_time", &v_rise_time);
      m_ttree->Branch("fall_time", &v_fall_time);
      m_ttree->Branch("signal_peak_time", &v_signal_peak_time);
      m_ttree->Branch("max_peak_position", &v_max_peak_position);
      m_ttree->Branch("max_peak_time", &v_max_peak_time);
      m_ttree->Branch("peak_positions", &v_peak_positions);
      m_ttree->Branch("peak_times", &v_peak_times);
      m_ttree->Branch("n_peaks", &v_npeaks);
    }

    // fft stuff and spectrum
    if (fft_waveforms > 0) {
        m_ttree->Branch("fft_mean", &fft_mean);
        m_ttree->Branch("fft_mean_freq", &fft_mean_freq);
        m_ttree->Branch("fft_max", &fft_max);
        m_ttree->Branch("fft_max_freq", &fft_max_freq);
        m_ttree->Branch("fft_min", &fft_min);
        m_ttree->Branch("fft_min_freq", &fft_min_freq);
    }
    for (uint8_t i_wf = 0; i_wf < 9; i_wf++){
        if (UseWaveForm(spectrum_waveforms, i_wf)){
            m_ttree->Branch(TString::Format("peaks%d_x", i_wf), &peaks_x.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_x_time", i_wf), &peaks_x_time.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_y", i_wf), &peaks_y.at(i_wf));
        }
        if (UseWaveForm(fft_waveforms, i_wf)) {
            m_ttree->Branch(TString::Format("fft_modes%d", i_wf), &fft_modes.at(i_wf));
            m_ttree->Branch(TString::Format("fft_values%d", i_wf), &fft_values.at(i_wf));
        }
    }

    // telescope
    SetTelescopeBranches();
    m_ttree->Branch("trigger_phase", &f_trig_phase, "trigger_phase/b");
    verbose = 0;
    
    EUDAQ_INFO("Done with creating Branches!");
}

/** =====================================================================
    -------------------------WRITE EVENT---------------------------------
    =====================================================================*/
void FileWriterTreeCAEN::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
        PluginManager::SetConfig(ev, m_config);
        eudaq::PluginManager::Initialize(ev);
        Configuration conf(ev.GetTag("CONFIG"));
        sampling_speed_ = conf.Get("Producer.CAEN","sampling_frequency", 5);
        tcal = PluginManager::GetTimeCalibration(ev);
        FillFullTime();
        macro->AddLine("\n[Time Calibration]");
        macro->AddLine(("tcal = [" + to_string(tcal.at(0)) + "]").c_str());
        cout << "loading the first event...." << endl;
        //todo: add time stamp for the very first tu event
        return;
    }
    if (ev.IsEORE()) {
        cout << "loading the last event...." << endl;
        return;
    }
    if (max_event_number > 0 && f_event_number > max_event_number) return;

    w_total.Start(false);
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);

    f_event_number = sev.GetEventNumber();
    // set time stamp
    /** TU STUFF */
    SetTimeStamp(sev);
    SetBeamCurrent(sev);
    // --------------------------------------------------------------------
    // ---------- get the number of waveforms -----------------------------
    // --------------------------------------------------------------------
    f_nwfs = sev.GetNWaveforms();

    if (f_event_number <= 10 && verbose > 0){
        cout << "event number " << f_event_number << endl;
        cout << "number of waveforms " << f_nwfs << endl;
        if (f_nwfs == 0){
            cout << "----------------------------------------" << endl;
            cout << "WARNING!!! NO WAVEFORMS IN THIS EVENT!!!" << endl;
            cout << "----------------------------------------" << endl;
        }
    }
    if (verbose > 3) cout << "ClearVectors" << endl;
    ClearVectors();

    // --------------------------------------------------------------------
    // ---------- verbosity level and some printouts ----------------------
    // --------------------------------------------------------------------

    if (verbose > 3) cout << "event number " << f_event_number << endl;
    if (verbose > 3) cout << "number of waveforms " << f_nwfs << endl;

    // --------------------------------------------------------------------
    // ---------- get and save all info for all waveforms -----------------
    // --------------------------------------------------------------------

    //use different order of wfs in order to 'know' if its a pulser event or not.
    vector<uint8_t> wf_order = {pulser_channel};
    uint8_t n_wfs = sev.GetNWaveforms() < 10 ? uint8_t(sev.GetNWaveforms()) : uint8_t(9);
    for (uint8_t iwf = 0; iwf < n_wfs; iwf++)
        if (iwf != pulser_channel)
            wf_order.push_back(iwf);
    for (auto iwf : wf_order) {
        sev.GetWaveform(iwf).SetPolarities(polarities.at(iwf), pulser_polarities.at(iwf));
        sev.GetWaveform(iwf).SetTimes(&tcal.at(0));
    }
    ResizeVectors(sev.GetNWaveforms());
    for (auto iwf:wf_order){
        const eudaq::StandardWaveform & waveform = sev.GetWaveform(iwf);
        // save the sensor names
        if (f_event_number == 0) {
            sensor_name.resize(sev.GetNWaveforms(), "");
            sensor_name.at(iwf) = waveform.GetChannelName();
        }

        int n_samples = waveform.GetNSamples();
        if (verbose > 3) cout << "number of samples in my wf " << n_samples << std::endl;
        // load the waveforms into the vector
        data = waveform.GetData();
        calc_noise(iwf);

        this->FillSpectrumData(iwf);
        if (verbose > 3) cout<<"DoSpectrumFitting "<<iwf<<endl;
        this->DoSpectrumFitting(iwf);
        if (verbose > 3) cout << "DoFFT " << iwf << endl;
        this->DoFFTAnalysis(iwf);

        // drs4 info
        if (iwf == wf_order.at(0)) f_trigger_cell = waveform.GetTriggerCell(); // same for every waveform

        // calculate the signal and so on
        // float sig = CalculatePeak(data, 1075, 1150);
        if (verbose > 3) cout << "get Values1.0 " << iwf << endl;
        FillRegionIntegrals(iwf, &waveform);
        FillBucket(sev);

      if (verbose > 3) cout << "get Values1.1 " << iwf << endl;
        FillTotalRange(iwf, &waveform);

        // determine FORC timing: trigger WF august: 2, may: 1
        if (verbose > 3) cout << "get trigger wf " << iwf << endl;
//        if (iwf == trigger_channel) ExtractForcTiming(data);

        // determine pulser events: pulser WF august: 1, may: 2
        if (verbose > 3) cout<<"get pulser wf "<<iwf<<endl;
        if (iwf == pulser_channel){
            f_pulser = this->IsPulserEvent(&waveform);
            if (f_pulser) f_pulser_events++;
            else f_signal_events++;
        }
        //rf
        if (iwf == rf_channel and fit_rf){
          auto fit = waveform.getRFFit(&tcal.at(0));
          f_rf_period = float(fit.GetParameter(1));
          f_rf_phase = GetRFPhase(float(fit.GetParameter(2)), f_rf_period);
          f_rf_chi2 = float(fit.GetChisquare() / fit.GetNDF());
        }
        if (iwf == trigger_channel)
          f_trigger_time = waveform.getTriggerTime(&tcal.at(0));

        // fill waveform vectors
        if (verbose > 3) cout << "fill wf " << iwf << endl;
        UpdateWaveforms(iwf);

        f_isDa->at(iwf) = *max_element(&data->at(20), &data->at(1023)) > wf_thr.at(iwf);

        data->clear();
    } // end iwf waveform loop

    FillRegionVectors();

    // --------------------------------------------------------------------
    // ---------- save all info for the telescope -------------------------
    // --------------------------------------------------------------------
    FillTelescopeArrays(sev);
    m_ttree->Fill();
    if (f_event_number + 1 % 1000 == 0) cout << "of run " << runnumber << flush;
    w_total.Stop();
} // end WriteEvent()

/** =====================================================================
    -------------------------DECONSTRUCTOR-------------------------------
    =====================================================================*/
FileWriterTreeCAEN::~FileWriterTreeCAEN() {
    macro->AddLine("\n[Sensor Names]");
    vector<string> names;
    for (const auto &name: sensor_name)
        names.push_back("\"" + name + "\"");
    macro->AddLine(("Names = [" + to_string(names) + "]").c_str());
    stringstream ss;
    ss << "Summary of RUN " << runnumber << "\n";
    long entries = m_ttree->GetEntries();
    ss << "Tree has " << entries << " entries\n";
    double t = w_total.RealTime();
    ss << "\nTotal time: " <<  setw(2) << setfill('0') << int(t / 60) << ":" << setw(2) << setfill('0') << int(t - int(t / 60) * 60);
    if (entries > 1000) ss << "\nTime/1000 events: " << int(t / entries * 1000 * 1000) << " ms";
    if (spectrum_waveforms > 0) ss << "\nTSpectrum: " << w_spectrum.RealTime() << " seconds";
    print_banner(ss.str(), '*');
    m_ttree->Write();
    avgWF_0->Write();
    avgWF_0_pul->Write();
    avgWF_0_sig->Write();
    avgWF_1->Write();
    avgWF_2->Write();
    avgWF_3->Write();
    avgWF_3_sig->Write();
    avgWF_3_pul->Write();
    if (macro != nullptr) macro->Write();
    if (m_tfile->IsOpen()) m_tfile->Close();
}

float FileWriterTreeCAEN::Calculate(std::vector<float> * data, int min, int max, bool _abs) {
    float integral = 0;
    uint16_t i;
    for (i = uint16_t(min) ; i <= (max + 1) && i < data->size() ; i++){
        if(!_abs) integral += data->at(i);
        else integral += abs(data->at(i));
    }
    return integral / float(i - min);
}

/*float FileWriterTreeCAEN::CalculatePeak(std::vector<float> * data, int min, int max) {
    int mid = (int)( (max + min)/2 );
    int n = 1;
    while( (data->at(mid-1)/data->at(mid) -1)*(data->at(mid+1)/data->at(mid) -1) < 0 ){
        // jump up and down from the center position to find the max or min
        mid += pow(-1, n)*n;
        n+=1;
    }
    // extremal value is now at mid
    float integral = Calculate(data, mid-3, mid+6);
    return integral;
}

std::pair<int, float> FileWriterTreeCAEN::FindMaxAndValue(std::vector<float> * data, int min, int max) {
    float maxVal = -999;
    int imax = min;
    for (int i = min; i <= int(max+1) && i < data->size() ;i++){
        if (abs(data->at(i)) > maxVal){ maxVal = abs(data->at(i)); imax = i; }
    }
    maxVal = data->at(imax);
    std::pair<int, float> res = make_pair(imax, maxVal);
    return res;
}*/

float FileWriterTreeCAEN::avgWF(float old_avg, float new_value, int n) {
    float avg = old_avg;
    avg -= old_avg/n;
    avg += new_value/n;
    return avg;
}

uint64_t FileWriterTreeCAEN::FileBytes() const { return 0; }

inline void FileWriterTreeCAEN::ClearVectors(){

    v_is_saturated->clear();
    v_median->clear();
    v_average->clear();
    v_forc_pos->clear();
    v_forc_time->clear();

    for (auto v_wf:f_wf) v_wf.second->clear();
    f_isDa->clear();

    for (auto vec: *v_peak_positions) vec.clear();
    for (auto vec: *v_peak_times) vec.clear();
    for (auto peak: peaks_x) peak->clear();
    for (auto peak: peaks_x_time) peak->clear();
    for (auto peak: peaks_y) peak->clear();
} // end ClearVectors()

inline void FileWriterTreeCAEN::ResizeVectors(size_t n_channels) {
    for (auto r: *regions) r.second->Reset();

    v_is_saturated->resize(n_channels);
    v_median->resize(n_channels);
    v_average->resize(n_channels);
    v_signal_peak_time->resize(n_channels);
    v_rise_time->resize(n_channels);
    v_fall_time->resize(n_channels);
    v_max_peak_time->resize(n_channels, 0);
    v_max_peak_position->resize(n_channels, 0);

    f_isDa->resize(n_channels);

    fft_mean->resize(n_channels, 0);
    fft_min->resize(n_channels, 0);
    fft_max->resize(n_channels, 0);

    fft_mean_freq->resize(n_channels, 0);
    fft_min_freq->resize(n_channels, 0);
    fft_max_freq->resize(n_channels, 0);
    for (auto p: fft_modes) p->clear();
    for (auto p: fft_values) p->clear();
} // end ResizeVectors()

void FileWriterTreeCAEN::DoFFTAnalysis(uint8_t iwf){
    if (!UseWaveForm(fft_waveforms, iwf)) return;
    fft_mean->at(iwf) = 0;
    fft_max->at(iwf) = 0;
    fft_min->at(iwf) = 1e9;
    fft_mean_freq->at(iwf) = -1;
    fft_max_freq->at(iwf) = -1;
    fft_min_freq->at(iwf) = -1;
    w_fft.Start(false);
    auto n = uint32_t(data->size());
    float sample_rate = 2e6;
    if(fft_own->GetN()[0] != n+1){
        n_samples = n+1;
        cout << "Recreating a new VirtualFFT with " << n_samples << " Samples, before " << fft_own->GetN() << endl;
        delete fft_own;
        delete in;
        delete re_full;
        delete im_full;
        fft_own = nullptr;
        in = new Double_t[n];
        re_full = new Double_t[n];
        im_full = new Double_t[n];
        fft_own = TVirtualFFT::FFT(1, &n_samples, "R2C");
    }
    for (uint32_t j = 0; j < n; ++j) in[j] = data->at(j);
    fft_own->SetPoints(in);
    fft_own->Transform();
    fft_own->GetPointsComplex(re_full,im_full);
    float finalVal = 0;
    float max = -1;
    float min = 1e10;
    float freq;
    float mean_freq = 0;
    float min_freq = -1;
    float max_freq  = -1;
    float value;
    for (int j = 0; j < (n/2+1); ++j) {
        freq = j * sample_rate/n;
        value = float(TMath::Sqrt(re_full[j]*re_full[j] + im_full[j]*im_full[j]));
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
        if (j < 10 || j == n/2)
            fft_modes.at(iwf)->push_back(value);
    }
    mean_freq /= finalVal;
    finalVal/= ((n/2) + 1);
    fft_mean->at(iwf) = finalVal;
    fft_max->at(iwf) = max;
    fft_min->at(iwf) = min;
    fft_mean_freq->at(iwf) = mean_freq;
    fft_max_freq->at(iwf) = max_freq;
    fft_min_freq->at(iwf) = min_freq;
    w_fft.Stop();
    if (verbose>0 && f_event_number < 1000)
        cout<<runnumber<<" "<<std::setw(3)<<f_event_number<<" "<<iwf<<" "<<finalVal<<" "<<max<<" "<<min<<endl;
} // end DoFFTAnalysis()

inline void FileWriterTreeCAEN::DoSpectrumFitting(uint8_t iwf){

    if (!UseWaveForm(spectrum_waveforms, iwf)) return;

    w_spectrum.Start(false);
    float max = *max_element(data_pos.begin(), data_pos.end());
    //return if the max element is lower than 4 sigma of the noise
    float threshold = 4 * noise_->at(iwf).second + noise_->at(iwf).first;
    if (max <= threshold) return;
    // tspec threshold is in per cent to max peak
    threshold = threshold / max * 100;
    auto size = uint16_t(data_pos.size());
    int peaks = spec->SearchHighRes(&data_pos[0], &decon[0], size, spec_sigma, threshold, spec_rm_bg, spec_decon_iter, spec_markov, spec_aver_win);
    for(uint8_t i=0; i < peaks; i++){
        cout << f_event_number << " " << iwf << " " << spec->GetPositionX()[i] << endl;
        auto bin = uint16_t(spec->GetPositionX()[i] + .4);
        uint16_t min_bin = bin - 5 >= 0 ? uint16_t(bin - 5) : uint16_t(0);
        uint16_t max_bin = bin + 5 < size ? uint16_t(bin + 5) : uint16_t(size - 1);
        max = *std::max_element(&data_pos.at(min_bin), &data_pos.at(max_bin));
        peaks_x.at(iwf)->push_back(bin);
        peaks_x_time.at(iwf)->push_back(getTriggerTime(iwf, bin));
        peaks_y.at(iwf)->push_back(max);
    }
    w_spectrum.Stop();
} // end DoSpectrumFitting()

void FileWriterTreeCAEN::FillSpectrumData(uint8_t iwf){
    bool b_spectrum = UseWaveForm(spectrum_waveforms, iwf);
    bool b_fft = UseWaveForm(fft_waveforms, iwf);
    if(b_spectrum || b_fft){
        data_pos.resize(data->size());
        for (uint16_t i = 0; i < data->size(); i++)
            data_pos.at(i) = polarities.at(iwf) * data->at(i);
    }
} // end FillSpectrumData()

void FileWriterTreeCAEN::calc_noise(uint8_t iwf) {
  float value = data->at(peak_noise_pos);
  // filter out peaks at the pedestal
  if (std::abs(value) < 6 * noise_->at(iwf).second + std::abs(noise_->at(iwf).first) or noise_vectors.at(iwf)->size() < 10)
    noise_vectors.at(iwf)->push_back(value);
  if (noise_vectors.at(iwf)->size() >= 1000)
    noise_vectors.at(iwf)->pop_front();
  noise_->at(iwf) = calc_mean(*noise_vectors.at(iwf));
}

void FileWriterTreeCAEN::FillBucket(const StandardEvent & sev) {
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
    float int2 = abs(wf->getIntegral(r, i, sampling_speed_, bw));
    float thresh = GetNoiseThreshold(i_ch, 3);
    f_bucket[j] = int2 > thresh and abs(wf->getIntegral(r, i, sampling_speed_)) < thresh; // sig < thresh and bucket 2 > thresh
    float intm1 = r->GetLowBoarder() - 2 * bw < 0 ? 0 : abs(wf->getIntegral(r, i, sampling_speed_, -2 * bw));
    f_ped_bucket[j++] = intm1 > GetNoiseThreshold(i_ch, 4);
  }
}

void FileWriterTreeCAEN::FillRegionIntegrals(uint8_t iwf, const StandardWaveform *wf){
    if (regions->count(iwf) == 0) return;
    WaveformSignalRegions * this_regions = (*regions)[iwf];
    uint16_t nRegions = this_regions->GetNRegions();
    for (uint16_t i=0; i < nRegions; i++){
        if (verbose > 0) cout << "REGION LOOP" << endl;
        WaveformSignalRegion * region = this_regions->GetRegion(i);
        signed char polarity;
        polarity = (string(region->GetName()).find("pulser") != string::npos) ? this_regions->GetPulserPolarity() : this_regions->GetPolarity();
        uint16_t low_border = region->GetLowBoarder();
        uint16_t high_border = region->GetHighBoarder();
        uint16_t peak_pos = wf->getIndex(low_border, high_border, polarity);
        region->SetPeakPostion(peak_pos);
        size_t nIntegrals = region->GetNIntegrals();
        for (UInt_t k = 0; k < nIntegrals;k++){
            WaveformIntegral* p = region->GetIntegralPointer(k);
            if (p == nullptr){
                std::cout<<"Invalid Integral Pointer. Continue."<<std::endl;
                continue;
            }
            if (verbose > 0) cout << "GET INTEGRAL " << k << " " << peak_pos << " " << endl;
            std::string name = p->GetName();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            p->SetPeakPosition(peak_pos,wf->GetNSamples());
            float integral, time_integral(0);
            if (name.find("peaktopeak")!=std::string::npos){
                integral = wf->getPeakToPeak(low_border,high_border);
            }
            else if (name.find("median")!=name.npos){
                integral = wf->getMedian(low_border,high_border);
            }
            else if (name.find("full")!=name.npos){
                integral = wf->getIntegral(low_border,high_border);
            }
            else {
                integral = wf->getIntegral(p->GetIntegralStart(), p->GetIntegralStop());
                time_integral = wf->getIntegral(p->GetIntegralStart(), p->GetIntegralStop(), peak_pos, 2.5);
            }
            p->SetIntegral(integral);
            p->SetTimeIntegral(time_integral);
        }
    }
} // end FillRegionIntegrals()

void FileWriterTreeCAEN::FillRegionVectors(){
    IntegralNames->clear();
    IntegralValues->clear();
    TimeIntegralValues->clear();
    IntegralPeaks->clear();
    IntegralPeakTime->clear();
    IntegralLength->clear();

    for (uint8_t iwf = 0; iwf < 9; iwf++){
        if (regions->count(iwf) == 0) continue;
        WaveformSignalRegions *this_regions = (*regions)[iwf];
        for (UInt_t i = 0; i < this_regions->GetNRegions(); i++){
            string name = "ch"+to_string(int(iwf))+"_";
            WaveformSignalRegion* region = this_regions->GetRegion(i);
            name += region->GetName();
            name += "_";
            uint16_t peak_pos = region->GetPeakPosition();

            for (UInt_t k = 0; k<region->GetNIntegrals();k++){
                WaveformIntegral* p = region->GetIntegralPointer(k);
                float integral = p->GetIntegral();
                string final_name = name;
                final_name += p->GetName();
                IntegralNames->push_back(final_name);
                IntegralValues->push_back(integral);
                TimeIntegralValues->push_back(p->GetTimeIntegral());
                IntegralPeaks->emplace_back(peak_pos);
                IntegralPeakTime->push_back(getTriggerTime(iwf, peak_pos));
                uint16_t bin_low = p->GetIntegralStart();
                uint16_t bin_up = p->GetIntegralStop();
                IntegralLength->push_back(getTimeDifference(iwf, bin_low, bin_up));
            }
        }
    }
}

void FileWriterTreeCAEN::FillTotalRange(uint8_t iwf, const StandardWaveform *wf){

    signed char pol = polarities.at(iwf);
    v_is_saturated->at(iwf) = wf->getAbsMaxInRange(0, 1023) > 488; // indicator if saturation is reached in sampling region (1-1024)
    v_median->at(iwf) = pol * wf->getMedian(0, 1023); // Median over whole sampling region
    v_average->at(iwf) = pol * wf->getIntegral(0, 1023);
    if (UseWaveForm(active_regions, iwf)){

        WaveformSignalRegion * reg = regions->at(iwf)->GetRegion("signal_b");
        v_signal_peak_time->at(iwf) = wf->getPeakFit(reg->GetLowBoarder(), reg->GetHighBoarder(), regions->at(iwf)->GetPolarity());
        v_rise_time->at(iwf) = wf->getRiseTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_->at(iwf).first);
        v_fall_time->at(iwf) = wf->getFallTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_->at(iwf).first);
        pair<uint16_t, float> peak = wf->getMaxPeak();
        v_max_peak_position->at(iwf) = peak.first;
        v_max_peak_time->at(iwf) = getTriggerTime(iwf, peak.first);
        float threshold = polarities.at(iwf) * 4 * noise_->at(iwf).second + noise_->at(iwf).first;
        vector<uint16_t> * peak_positions = wf->getAllPeaksAbove(0, 1023, threshold);
        v_peak_positions->at(iwf) = *peak_positions;
        v_npeaks->at(iwf) = uint8_t(v_peak_positions->at(iwf).size());
        vector<float> peak_timings;
        for (auto i_pos:*peak_positions) peak_timings.push_back(getTriggerTime(iwf, i_pos));
        v_peak_times->at(iwf) = peak_timings;
    }

  if (scint_channel == iwf) {
      WaveformSignalRegion * reg = regions->at(dia_channels->at(0))->GetRegion("signal_b");
      v_rise_time->at(iwf) = wf->getRiseTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_->at(iwf).first);
      v_fall_time->at(iwf) = wf->getFallTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise_->at(iwf).first);
//    cout << f_event_number << " " << int(iwf) << " " << v_rise_time->at(iwf) << " " << v_t_thresh->at(iwf) << " " << (-10 * noise->at(iwf).second + noise->at(iwf).first)<<endl;
  }
}

void FileWriterTreeCAEN::UpdateWaveforms(uint8_t iwf){

    for (uint16_t j = 0; j < data->size(); j++) {
        if (UseWaveForm(save_waveforms, iwf))
            f_wf.at(uint8_t(iwf))->emplace_back(data->at(j));
        if     (iwf == 0) {
            if (f_pulser)
                avgWF_0_pul->SetBinContent(j+1, avgWF(float(avgWF_0_pul->GetBinContent(j+1)),data->at(j),f_pulser_events));
            else
                avgWF_0_sig->SetBinContent(j+1, avgWF(float(avgWF_0_sig->GetBinContent(j+1)),data->at(j),f_signal_events));
            avgWF_0->SetBinContent(j+1, avgWF(float(avgWF_0->GetBinContent(j+1)),data->at(j),f_event_number+1));
            avgWF_0->SetBinContent(j+1, avgWF(float(avgWF_0->GetBinContent(j+1)),data->at(j),f_event_number+1));
        }
        else if(iwf == 1) {
            avgWF_1->SetBinContent(j+1, avgWF(float(avgWF_1->GetBinContent(j+1)),data->at(j),f_event_number+1));
        }
        else if(iwf == 2) {
            avgWF_2->SetBinContent(j+1, avgWF(float(avgWF_2->GetBinContent(j+1)),data->at(j),f_event_number+1));
        }
        else if(iwf == 3) {
            if (f_pulser)
                avgWF_3_pul->SetBinContent(j+1, avgWF(float(avgWF_3_pul->GetBinContent(j+1)),data->at(j),f_pulser_events));
            else
                avgWF_3_sig->SetBinContent(j+1, avgWF(float(avgWF_3_sig->GetBinContent(j+1)),data->at(j),f_signal_events));
            avgWF_3->SetBinContent(j+1, avgWF(float(avgWF_3->GetBinContent(j+1)),data->at(j),f_event_number+1));
        }
    } // data loop
} // end UpdateWaveforms()

inline bool FileWriterTreeCAEN::IsPulserEvent(const StandardWaveform *wf){
    float pulser_int = wf->getIntegral(pulser_region.first, pulser_region.second, true);
    float baseline_int = wf->getIntegral(5, uint16_t(pulser_region.first - pulser_region.second + 5), true);
    return abs(pulser_int - baseline_int) > pulser_threshold;
} //end IsPulserEvent

void FileWriterTreeCAEN::FillFullTime(){
    uint16_t n_waveform_samples = 1024;
    for (auto i_ch:tcal){
        i_ch.second.resize(n_waveform_samples);
        float sum = 0;
        full_time[i_ch.first] = vector<float>();
        full_time.at(i_ch.first).push_back(sum);
        for (uint16_t j = 0; j < i_ch.second.size() * 2 - 1; j++){
            sum += i_ch.second.at(uint16_t(j % 1024));
            full_time.at(i_ch.first).push_back(sum);
        }
    }
}

inline float FileWriterTreeCAEN::getTriggerTime(const uint8_t & ch, const uint16_t & bin) {
    //todo add group number
    return full_time.at(0).at(bin + f_trigger_cell) - full_time.at(0).at(f_trigger_cell);
}

float FileWriterTreeCAEN::getTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up) {
    return full_time.at(ch).at(bin_up + f_trigger_cell) - full_time.at(ch).at(uint16_t(bin_low + f_trigger_cell));
}

string FileWriterTreeCAEN::GetBitMask(uint16_t bitmask){
    stringstream ss;
    for (uint8_t i = 0; i < 9; i++) {
        string bit = to_string(UseWaveForm(bitmask, i));
        ss << string(3 - bit.size(), ' ') << bit;
    }
    return trim(ss.str(), " ");
}

string FileWriterTreeCAEN::GetPolarities(vector<signed char> pol) {
    stringstream ss;
    for (auto i_pol:pol) ss << string(3 - to_string(i_pol).size(), ' ') << to_string(i_pol);
    return trim(ss.str(), " ");
}

void FileWriterTreeCAEN::SetTimeStamp(StandardEvent sev) {
    if (sev.hasTUEvent()){
        if (sev.GetTUEvent(0).GetValid())
            f_time = sev.GetTimestamp();
    }
    else
        f_time = sev.GetTimestamp() / 384066.;
}

void FileWriterTreeCAEN::SetBeamCurrent(StandardEvent sev) {

  if (sev.hasTUEvent()){
    StandardTUEvent tuev = sev.GetTUEvent(0);
    f_beam_current = uint16_t(tuev.GetValid() ? tuev.GetBeamCurrent() : UINT16_MAX);
  }
}

void FileWriterTreeCAEN::ReadIntegralRanges() {

    for (uint8_t i_ch(0); i_ch < n_channels; i_ch++) {
        if (not UseWaveForm(active_regions, i_ch)) continue;
        // Default ranges
        ranges[i_ch]["PeakIntegral1"] = new pair<float, float>(m_config->Get("PeakIntegral1_range", make_pair(8, 12)));
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
        for (uint8_t i_ch(0); i_ch < n_channels; i_ch++) {
            if (not UseWaveForm(active_regions, i_ch)) continue;  // skip if the channel has no signal waveform
            if (ranges.at(i_ch).count(name) > 0) continue;        // skip if the range already exists
            ranges.at(i_ch)[name] = new pair<float, float>(make_pair(range_def.at(i), range_def.at(i + 1)));
            if (range_def.size() > i + 2) i += 2;
        }
    }
}

void FileWriterTreeCAEN::ReadIntegralRegions() {

    for (const auto &i_key: m_config->GetKeys()){
        if (i_key.find("active_regions") != string::npos) continue;
        if (i_key.find("pulser_channel") != string::npos) continue;
        size_t found = i_key.find("_region");
        if (found == string::npos) continue;
        string name = i_key.substr(0, found);
        vector<uint16_t> tmp = {0, 0, 0, 0, 0, 0, 0, 0};
        vector<uint16_t> region_def = from_string(m_config->Get(i_key, "bla"), tmp);
        uint8_t i(0);
        for (auto ch: ranges){
            WaveformSignalRegion region = WaveformSignalRegion(region_def.at(i), region_def.at(i + 1), name);
            if (region_def.size() > i + 2) i += 2;
            // add all PeakIntegral ranges for the region
            for (auto range: ch.second){
                if (range.first.find("PeakIntegral") == string::npos) continue;  // only consider the integral ranges around the peak
                WaveformIntegral integralDef = WaveformIntegral(int(range.second->first), int(range.second->second), range.first);
                region.AddIntegral(integralDef);
            }
            regions->at(ch.first)->AddRegion(region);
        }
    }
}

float FileWriterTreeCAEN::GetNoiseThreshold(uint8_t i_wf, float n_sigma) {
  /** :returns:  threshold based on the current noise */
  return float(polarities.at(i_wf)) * int_noise_.at(i_wf).first + n_sigma * int_noise_.at(i_wf).second;
}

float FileWriterTreeCAEN::GetRFPhase(float phase, float period) {

  // shift phase always in [-pi,pi]
  float fac = int(phase / period - .5);
  return phase - fac * period;
}

void FileWriterTreeCAEN::SetTelescopeBranches() {

  m_ttree->Branch("n_hits_tot", &f_n_hits, "n_hits_tot/b");
  m_ttree->Branch("plane", f_plane, "plane[n_hits_tot]/b");
  m_ttree->Branch("col", f_col, "col[n_hits_tot]/b");
  m_ttree->Branch("row", f_row, "row[n_hits_tot]/b");
  m_ttree->Branch("adc", f_adc, "adc[n_hits_tot]/S");
  m_ttree->Branch("charge", f_charge, "charge[n_hits_tot]/F");
}

void FileWriterTreeCAEN::FillTelescopeArrays(const StandardEvent & sev) {

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

void FileWriterTreeCAEN::InitTelescopeArrays() {

  f_n_hits = 0;
  f_plane  = new uint8_t[MAX_SIZE];
  f_col    = new uint8_t[MAX_SIZE];
  f_row    = new uint8_t[MAX_SIZE];
  f_adc    = new int16_t [MAX_SIZE];
  f_charge = new float[MAX_SIZE];
}
#endif // ROOT_FOUND
