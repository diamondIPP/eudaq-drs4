#ifdef ROOT_FOUND

#include "eudaq/FileWriteTreeDRS4.hh"

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeDRS4> reg("drs4tree"); }

/** =====================================================================
    --------------------------CONSTRUCTOR--------------------------------
    =====================================================================*/
FileWriterTreeDRS4::FileWriterTreeDRS4(const std::string & /*param*/)
: m_tfile(0), m_ttree(0), m_noe(0), chan(4), n_pixels(90*90+60*60), histo(0), spec(0), fft_own(0), runnumber(0) {

    gROOT->ProcessLine("#include <vector>");
//    gROOT->ProcessLine(".L ~/lib/root_loader.c+"); // what is that for? MR
//    gROOT->ProcessLine(".L loader.C+");
    gROOT->ProcessLine( "gErrorIgnoreLevel = 5001;");
    gROOT->ProcessLine("#include <pair>");
    gROOT->ProcessLine("#include <map>");
    gInterpreter->GenerateDictionary("map<string,Float_t>;vector<map<string,Float_t> >,vector<vector<Float_t> >","vector;string;map");
    //Polarities of signals, default is positive signals
    polarities.resize(4, 1);
    pulser_polarities.resize(4, 1);

    //how many events will be analyzed, 0 = all events
    max_event_number = 0;
    macro = new TMacro();
    macro->SetNameTitle("region_information", "Region Information");

    f_nwfs = 0;
    f_event_number = -1;
    f_pulser_events = 0;
    f_signal_events = 0;
    f_time = -1;
    f_pulser = -1;
    v_forc_pos = new vector<uint16_t>;
    v_forc_time = new vector<float>;

    // integrals
    regions = new std::map<int,WaveformSignalRegions* >;
    IntegralNames = new std::vector<std::string>;
    IntegralValues = new std::vector<float>;
    TimeIntegralValues = new vector<float>;
    IntegralPeaks = new std::vector<Int_t>;
    IntegralPeakTime = new std::vector<float>;
    IntegralLength  = new std::vector<float>;

    // general waveform information
    v_polarities = new vector<int16_t>;
    v_pulser_polarities = new vector<int16_t>;
    v_is_saturated = new vector<bool>;
    v_median = new vector<float>;
    v_average = new vector<float>;

    // waveforms
    for (uint8_t i = 0; i < 4; i++) f_wf[i] = new vector<float>;

    // spectrum vectors
    peaks_x.resize(4, new std::vector<float>);
    peaks_y.resize(4, new std::vector<float>);
    peaks_no.resize(4, new std::vector<int>);
    npeaks = new std::vector<float>;

    // fft analysis
    fft_modes.resize(4, new std::vector<float>);
    fft_values.resize(4, new std::vector<float>);
    fft_mean = new std::vector<float>;
    fft_mean_freq = new std::vector<float>;
    fft_max = new std::vector<float>;
    fft_max_freq = new std::vector<float>;
    fft_min = new std::vector<float>;
    fft_min_freq = new std::vector<float>;

    // telescope
    f_plane  = new std::vector<uint16_t>;
    f_col    = new std::vector<uint16_t>;
    f_row    = new std::vector<uint16_t>;
    f_adc    = new std::vector<int16_t>;
    f_charge = new std::vector<uint32_t>;

    // average waveforms of channels
    avgWF_0 = new TH1F("avgWF_0","avgWF_0", 1024, 0, 1024);
    avgWF_0_pul = new TH1F("avgWF_0_pul","avgWF_0_pul", 1024, 0, 1024);
    avgWF_0_sig = new TH1F("avgWF_0_sig","avgWF_0_sig", 1024, 0, 1024);
    avgWF_1 = new TH1F("avgWF_1","avgWF_1", 1024, 0, 1024);
    avgWF_2 = new TH1F("avgWF_2","avgWF_2", 1024, 0, 1024);
    avgWF_3 = new TH1F("avgWF_3","avgWF_3", 1024, 0, 1024);
    avgWF_3_pul = new TH1F("avgWF_3_pul","avgWF_3_pul", 1024, 0, 1024);
    avgWF_3_sig = new TH1F("avgWF_3_sig","avgWF_3_sig", 1024, 0, 1024);

    spec = new TSpectrum(20, 3);
    fft_own = 0;
    if(!fft_own){
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
void FileWriterTreeDRS4::Configure(){

    // do some assertions
    if (!this->m_config){
        EUDAQ_WARN("Configuration class instance m_config does not exist!");
        return;
    }
    m_config->SetSection("Converter.drs4tree");
    if (m_config->NSections()==0){
        EUDAQ_WARN("Config file has no sections!");
        return;
    }
    cout << endl;
    EUDAQ_INFO("Configuring FileWriterTreeDRS4");

    max_event_number = m_config->Get("max_event_number", 0);

    // spectrum and fft
    spectrum_sigma = m_config->Get("spectrum_sigma", 10);
    spectrum_threshold = m_config->Get("spectrum_threshold", 10);
    spectrum_deconIterations = m_config->Get("spectrum_deconIterations", 10);
    spectrum_averageWindow = m_config->Get("spectrum_averageWindow", 5);
    spec->SetDeconIterations(spectrum_deconIterations);
    spec->SetAverageWindow(spectrum_averageWindow);
    spectrum_markov = m_config->Get("spectrum_markov", true);
    spectrum_background_removal= m_config->Get("spectrum_background_removal", true);
    spectrum_waveforms = m_config->Get("spectrum_waveforms", uint16_t(0));
    fft_waveforms = m_config->Get("fft_waveforms", uint16_t(0));

    // channels
    pulser_threshold = m_config->Get("pulser_drs4_threshold", 80);
    pulser_channel = uint8_t(m_config->Get("pulser_channel", 1));
    trigger_channel = uint8_t(m_config->Get("trigger_channel", 2));

    // default ranges
    ranges["pulserDRS4"] = new pair<float, float>(m_config->Get("pulser_range_drs4", make_pair(800, 1000)));
    ranges["PeakIntegral1"] = new pair<float, float>(m_config->Get("PeakIntegral1_range", make_pair(0, 1)));
    ranges["PeakIntegral2"] = new pair<float, float>(m_config->Get("PeakIntegral2_range", make_pair(8, 12)));
    // additional ranges from the config file
    for (auto i_key: m_config->GetKeys()){
        size_t found = i_key.find("_range");
        if (found == string::npos) continue;
        if (i_key.at(0) == '#') continue;
        string name = i_key.substr(0, found);
        if (ranges.count(name)==0)
            ranges[name] = new pair<float, float>(m_config->Get(i_key, make_pair(0, 0)));
    }

    // saved waveforms
    save_waveforms = m_config->Get("save_waveforms", uint16_t(9));

    // polarities
    polarities = m_config->Get("polarities", polarities);
    pulser_polarities = m_config->Get("pulser_polarities", pulser_polarities);
    for (auto i: polarities)  v_polarities->push_back(uint16_t(i * 1));
    for (auto i: pulser_polarities) v_pulser_polarities->push_back(uint16_t(i * 1));

    // regions todo: add default range
    uint16_t active_regions = m_config->Get("active_regions", uint16_t(0));
    macro->AddLine(TString::Format("active_regions: %d", active_regions));
    for (uint8_t i = 0; i < 4; i++)
        if (UseWaveForm(active_regions, i)) (*regions)[i] = new WaveformSignalRegions(i, polarities.at(i), pulser_polarities.at(i));
    macro->AddLine("");
    macro->AddLine("Signal Windows");
    stringstream ss_ped, ss_sig, ss_pul;
    ss_ped << "  Pedestal:\n"; ss_sig << "  Signal:\n"; ss_pul << "  Pulser:\n";
    for (auto i_key: m_config->GetKeys()){
        size_t found = i_key.find("_region");
        if (found == string::npos) continue;
        if (i_key.find("active_regions") != string::npos) continue;
        pair<int, int> region_def = m_config->Get(i_key, make_pair(0, 0));
        string name = i_key.substr(0, found);
        string reg = (split(name, "_").size() > 1) ? split(name, "_").at(1) : "";
        if (name.find("pedestal_") != string::npos) ss_ped << "    region_" << reg << ":" << string(5 - reg.size(), ' ') << to_string(region_def) << "\n";
        if (name.find("signal_") != string::npos) ss_sig << "    region_" << reg << ":" << string(5 - reg.size(), ' ') << to_string(region_def) << "\n";
        if (name.find("pulser_") != string::npos) ss_pul << "    region_" << reg << ":" << string(5 - reg.size(), ' ') << to_string(region_def) << "\n";
        TString key = name + ":" + string(20 - name.size(), ' ') + TString::Format("%4d - %4d", region_def.first, region_def.second);
        macro->AddLine(key);
        WaveformSignalRegion region = WaveformSignalRegion(region_def.first, region_def.second, name);
        // add all PeakIntegral ranges for the region
        for (auto i_rg: ranges){
            if (i_rg.first.find("PeakIntegral") != std::string::npos){
                WaveformIntegral integralDef = WaveformIntegral(int(i_rg.second->first), int(i_rg.second->second), i_rg.first);
                region.AddIntegral(integralDef);
            }
        }
        for (uint8_t i = 0; i< 4;i++)
            if ((active_regions & 1<<i) == 1<<i)
                regions->at(i)->AddRegion(region);
    }
    macro->AddLine("");
    macro->AddLine("Signal Definitions:");
    for (auto i: ranges){
        if (i.first.find("PeakIntegral") != std::string::npos){
                TString key = "* " + i.first + ":";
                key += string(abs(20 - key.Length()), ' ') + TString::Format("%4d - %4d", int(i.second->first), int(i.second->second));
                macro->AddLine(key);
        }
    }

    // output
    cout << "CHANNEL AND PULSER SETTINGS:" << endl;
    cout << append_spaces(24, "  pulser channel:") << pulser_channel << endl;
    cout << append_spaces(24, "  trigger channel:") << trigger_channel << endl;
    cout << append_spaces(24, "  pulser_int threshold:") << pulser_threshold << endl;
    cout << append_spaces(24, "  save waveforms:") << save_waveforms << ":  " << GetBitMask(save_waveforms) << endl;
    cout << append_spaces(24, "  fft waveforms:") << fft_waveforms << ":  " << GetBitMask(fft_waveforms) << endl;
    cout << append_spaces(24, "  spectrum waveforms:") << spectrum_waveforms << ":  " << GetBitMask(spectrum_waveforms) << endl;
    cout << append_spaces(24, "  active regions:") << active_regions << ":  " << GetBitMask(active_regions) << endl;
    cout << append_spaces(27, "  polarities:") << GetPolarities(polarities) << endl;
    cout << append_spaces(27, "  pulser polarites:") << GetPolarities(pulser_polarities) << endl;

    cout<<"RANGES: " << ranges.size() << endl;
    for (auto& it: ranges) cout << append_spaces(25, "  range_" + it.first + ":") << to_string(*(it.second)) << endl;

    cout << "SIGNAL WINDOWS (REGIONS):" << endl;
    cout << "  " << trim(ss_ped.str(), " ,") << "  " << trim(ss_sig.str(), " ,") << "  " << trim(ss_pul.str(), " ,") << flush;

    cout << "\nMAXIMUM NUMBER OF EVENTS: " << (max_event_number ? to_string(max_event_number) : "ALL") << endl;
    EUDAQ_INFO("End of Configure!");
    cout << endl;
    macro->Write();
} // end Configure()

/** =====================================================================
    --------------------------START RUN----------------------------------
    =====================================================================*/
void FileWriterTreeDRS4::StartRun(unsigned runnumber) {
    this->runnumber = runnumber;
    EUDAQ_INFO("Converting the input file into a DRS4 TTree " );
    string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the output file: " + foutput);

    c1 = new TCanvas();
    c1->Draw();

    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");

    // Set Branch Addresses
    m_ttree->Branch("event_number", &f_event_number, "event_number/I");
    m_ttree->Branch("time",& f_time, "time/F");
    m_ttree->Branch("pulser",& f_pulser, "pulser/I");
    m_ttree->Branch("nwfs", &f_nwfs, "n_waveforms/I");
    m_ttree->Branch("forc_pos", &v_forc_pos);
    m_ttree->Branch("forc_time", &v_forc_time);

    // drs4
    m_ttree->Branch("trigger_cell", &f_trigger_cell, "trigger_cell/s");

    // waveforms
    for (uint8_t i_wf = 0; i_wf < 4; i_wf++)
        if ((save_waveforms & 1 << i_wf) == 1 << i_wf)
            m_ttree->Branch(TString::Format("wf%i", i_wf), &f_wf.at(i_wf));

    // integrals
    m_ttree->Branch("IntegralNames",&IntegralNames);
    m_ttree->Branch("IntegralValues",&IntegralValues);
    m_ttree->Branch("TimeIntegralValues",&TimeIntegralValues);
    m_ttree->Branch("IntegralPeaks",&IntegralPeaks);
    m_ttree->Branch("IntegralPeakTime",&IntegralPeakTime);
    m_ttree->Branch("IntegralLength",&IntegralLength);

    // DUT
    m_ttree->Branch("polarities", &v_polarities);
    m_ttree->Branch("pulser_polarities", &v_pulser_polarities);
    m_ttree->Branch("is_saturated", &v_is_saturated);
    m_ttree->Branch("median", &v_median);
    m_ttree->Branch("average", &v_average);

    // fft stuff and spectrum
    if (fft_waveforms) {
        print_banner("This is not executed!");
        m_ttree->Branch("fft_mean", &fft_mean);
        m_ttree->Branch("fft_mean_freq", &fft_mean_freq);
        m_ttree->Branch("fft_max", &fft_max);
        m_ttree->Branch("fft_max_freq", &fft_max_freq);
        m_ttree->Branch("fft_min", &fft_min);
        m_ttree->Branch("fft_min_freq", &fft_min_freq);
    }
    if (spectrum_waveforms) m_ttree->Branch("npeaks", &npeaks);
    for (uint8_t i_wf = 0; i_wf < 4; i_wf++){
        if (UseWaveForm(spectrum_waveforms, i_wf)){
            m_ttree->Branch(TString::Format("peaks%d_x", i_wf), &peaks_x.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_y", i_wf), &peaks_y.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_no", i_wf), &peaks_no.at(i_wf));
        }
        if (UseWaveForm(fft_waveforms, i_wf)) {
            m_ttree->Branch(TString::Format("fft_modes%d", i_wf), &fft_modes.at(i_wf));
            m_ttree->Branch(TString::Format("fft_values%d", i_wf), &fft_values.at(i_wf));
        }
    }

    // telescope
    m_ttree->Branch("plane", &f_plane);
    m_ttree->Branch("col", &f_col);
    m_ttree->Branch("row", &f_row);
    m_ttree->Branch("adc", &f_adc);
    m_ttree->Branch("charge", &f_charge);
    verbose = 0;
    
    EUDAQ_INFO("Done with creating Branches!");
}

/** =====================================================================
    -------------------------WRITE EVENT---------------------------------
    =====================================================================*/
void FileWriterTreeDRS4::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
        eudaq::PluginManager::Initialize(ev);
        tcal = PluginManager::GetTimeCalibration(ev);
        FillFullTime();
        cout << "loading the first event...." << endl;
        return;
    }
    else if (ev.IsEORE()) {
        cout << "loading the last event...." << endl;
        return;
    }
    if (max_event_number > 0 && f_event_number > max_event_number) return;

    w_total.Start(false);
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);

    f_event_number = sev.GetEventNumber();
    f_time = sev.GetTimestamp() / float(384066.);
    // --------------------------------------------------------------------
    // ---------- get the number of waveforms -----------------------------
    // --------------------------------------------------------------------
    unsigned int nwfs = (unsigned int) sev.NumWaveforms();
    f_nwfs = nwfs;

    if(f_event_number <= 10 && verbose > 0){
        cout << "event number " << f_event_number << endl;
        cout << "number of waveforms " << nwfs << endl;
        if(nwfs ==0){
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
    if (verbose > 3) cout << "number of waveforms " << nwfs << endl;

    // --------------------------------------------------------------------
    // ---------- get and save all info for all waveforms -----------------
    // --------------------------------------------------------------------


    //use different order of wfs in order to 'know' if its a pulser event or not.
    vector<uint8_t > wf_order = {2,1,0,3};
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

        this->FillSpectrumData(iwf);
        if (verbose > 3) cout<<"DoSpectrumFitting "<<iwf<<endl;
        this->DoSpectrumFitting(iwf);
        if (verbose > 3) cout << "DoFFT " << iwf << endl;
        this->DoFFTAnalysis(iwf);

        // calculate the signal and so on
        // float sig = CalculatePeak(data, 1075, 1150);
        if (verbose > 3) cout << "get Values1.0 " << iwf << endl;
        FillRegionIntegrals(iwf, &waveform);

        if (verbose > 3) cout << "get Values1.1 " << iwf << endl;
        FillTotalRange(iwf, &waveform);

        // drs4 info
        if (iwf == wf_order.at(0)) f_trigger_cell = waveform.GetTriggerCell(); // same for every waveform

        // determine FORC timing: trigger WF august: 2, may: 1
        if (verbose > 3) cout << "get trigger wf " << iwf << endl;
        if (iwf == trigger_channel) ExtractForcTiming(data);

        // determine pulser events: pulser WF august: 1, may: 2
        if (verbose > 3) cout<<"get pulser wf "<<iwf<<endl;
        if (iwf == pulser_channel){
            f_pulser = this->IsPulserEvent(&waveform);
            if (f_pulser) f_pulser_events++;
            else f_signal_events++;
        }

        // fill waveform vectors
        if (verbose > 3) cout << "fill wf " << iwf << endl;
        UpdateWaveforms(iwf);

        data->clear();
    } // end iwf waveform loop

    FillRegionVectors();

    // --------------------------------------------------------------------
    // ---------- save all info for the telescope -------------------------
    // --------------------------------------------------------------------
    for (uint8_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {
        const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
        std::vector<double> cds = plane.GetPixels<double>();

        for (uint16_t ipix = 0; ipix < cds.size(); ++ipix) {
            f_plane->push_back(iplane);
            f_col->push_back(uint16_t(plane.GetX(ipix)));
            f_row->push_back(uint16_t(plane.GetY(ipix)));
            f_adc->push_back(int16_t(plane.GetPixel(ipix)));
            f_charge->push_back(42);						// todo: do charge conversion here!
        }
    }
    m_ttree->Fill();
    if (f_event_number + 1 % 1000 == 0) cout << "of run " << runnumber << flush;
//        <<" "<<std::setw(7)<<f_event_number<<"\tSpectrum: "<<w_spectrum.RealTime()/w_spectrum.Counter()<<"\t" <<"LinearFitting: "
//        <<w_linear_fitting.RealTime()/w_linear_fitting.Counter()<<"\t"<< w_spectrum.Counter()<<"/"<<w_linear_fitting.Counter()<<"\t"<<flush;
    w_total.Stop();
} // end WriteEvent()

/** =====================================================================
    -------------------------DECONSTRUCTOR-------------------------------
    =====================================================================*/
FileWriterTreeDRS4::~FileWriterTreeDRS4() {
    macro->AddLine("\nSensor Names:");
    macro->AddLine(("  " + to_string(sensor_name)).c_str());
    stringstream ss;
    ss << "Summary of RUN " << runnumber << "\n";
    long entries = m_ttree->GetEntries();
    ss << "Tree has " << entries << " entries\n";
    double t = w_total.RealTime();
    ss << "\nTotal time: " <<  setw(2) << setfill('0') << int(t / 60) << ":" << setw(2) << setfill('0') << int(t - int(t / 60) * 60);
    if (entries > 1000) ss << "\nTime/1000 events: " << int(t / entries * 1000 * 1000) << " ms";
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
    if (macro) macro->Write();
    if (m_tfile->IsOpen()) m_tfile->Close();
}

float FileWriterTreeDRS4::Calculate(std::vector<float> * data, int min, int max, bool _abs) {
    float integral = 0;
    uint16_t i;
    for (i = uint16_t(min) ; i <= (max + 1) && i < data->size() ; i++){
        if(!_abs) integral += data->at(i);
        else integral += abs(data->at(i));
    }
    return integral / float(i - min);
}

/*float FileWriterTreeDRS4::CalculatePeak(std::vector<float> * data, int min, int max) {
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

std::pair<int, float> FileWriterTreeDRS4::FindMaxAndValue(std::vector<float> * data, int min, int max) {
    float maxVal = -999;
    int imax = min;
    for (int i = min; i <= int(max+1) && i < data->size() ;i++){
        if (abs(data->at(i)) > maxVal){ maxVal = abs(data->at(i)); imax = i; }
    }
    maxVal = data->at(imax);
    std::pair<int, float> res = make_pair(imax, maxVal);
    return res;
}*/

float FileWriterTreeDRS4::avgWF(float old_avg, float new_value, int n) {
    float avg = old_avg;
    avg -= old_avg/n;
    avg += new_value/n;
    return avg;
}

uint64_t FileWriterTreeDRS4::FileBytes() const { return 0; }

inline void FileWriterTreeDRS4::ClearVectors(){

    v_is_saturated->clear();
    v_median->clear();
    v_average->clear();
    v_forc_pos->clear();
    v_forc_time->clear();

    for (auto v_wf:f_wf) v_wf.second->clear();

    f_plane->clear();
    f_col->clear();
    f_row->clear();
    f_adc->clear();
    f_charge->clear();

    for (auto peak: peaks_x) peak->clear();
    for (auto peak: peaks_y) peak->clear();
    for (auto peak: peaks_no) peak->clear();
} // end ClearVectors()

inline void FileWriterTreeDRS4::ResizeVectors(size_t n_channels) {
    for (auto r: *regions) r.second->Reset();

    v_is_saturated->resize(n_channels);
    v_median->resize(n_channels);
    v_average->resize(n_channels);

    npeaks->resize(n_channels, 0);
    for (auto p: peaks_x) p->clear();
    for (auto p: peaks_y) p->clear();
    for (auto p: peaks_no) p->clear();

    fft_mean->resize(n_channels, 0);
    fft_min->resize(n_channels, 0);
    fft_max->resize(n_channels, 0);

    fft_mean_freq->resize(n_channels, 0);
    fft_min_freq->resize(n_channels, 0);
    fft_max_freq->resize(n_channels, 0);
    for (auto p: fft_modes) p->clear();
    for (auto p: fft_values) p->clear();
} // end ResizeVectors()

void FileWriterTreeDRS4::DoFFTAnalysis(uint8_t iwf){
    if (!UseWaveForm(fft_waveforms, iwf)) return;
    fft_mean->at(iwf) = 0;
    fft_max->at(iwf) = 0;
    fft_min->at(iwf) = 1e9;
    fft_mean_freq->at(iwf) = -1;
    fft_max_freq->at(iwf) = -1;
    fft_min_freq->at(iwf) = -1;
    w_fft.Start(false);
    uint32_t n = uint32_t(data->size());
    float sample_rate = 2e6;
    if(fft_own->GetN()[0] != n+1){
        n_samples = n+1;
        cout << "RECreating a new VirtualFFT with " << n_samples << " Samples, before " << fft_own->GetN() << endl;
        delete fft_own;
        delete in;
        delete re_full;
        delete im_full;
        fft_own = 0;
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

void FileWriterTreeDRS4::DoSpectrumFitting(uint8_t iwf){ // todo revise if going to use
    bool do_spectrum = (spectrum_waveforms & 1 << iwf) == 1 << iwf;
    if (!do_spectrum) return;
    npeaks->at(iwf) = 0;

    w_spectrum.Start(false);
    v_yy.resize(v_y.size());
    int peaks = spec->SearchHighRes(&v1[0], &(v_yy[0]), int(v1.size()), spectrum_sigma, spectrum_threshold,
            spectrum_background_removal, spectrum_deconIterations,spectrum_markov, spectrum_averageWindow);
    npeaks->at(iwf) = peaks;
    for(UInt_t i=0; i< peaks; i++){
        float xval = spec->GetPositionX()[i];
        uint32_t bin = uint32_t(xval + .5);
        uint32_t min_bin = bin - 5 >= 0 ? bin-5 : 0;
        uint32_t max_bin = bin + 5 < v_y.size() ? bin + 5 : uint32_t(v_y.size()) - 1;
        float max = *std::max_element(&v1.at(min_bin),&v1.at(max_bin));
        peaks_x.at(iwf)->push_back(xval);
        peaks_y.at(iwf)->push_back(max);
        peaks_no.at(iwf)->push_back(i);
    }
    w_spectrum.Stop();
} // end DoSpectrumFitting()

void FileWriterTreeDRS4::FillSpectrumData(uint8_t iwf){
    bool b_spectrum = UseWaveForm(spectrum_waveforms, iwf);
    bool b_fft = UseWaveForm(fft_waveforms, iwf);
    if(b_spectrum || b_fft){
        v_y.resize(data->size());
        v1.resize(data->size());
        int pol = polarities.at(iwf);
        for (unsigned i = 0; i < data->size(); i++){
            v_y.at(i) = pol * data->at(i);
            v1.at(i) = pol * data->at(i);
        }
    }
} // end FillSpectrumData()

void FileWriterTreeDRS4::FillRegionIntegrals(uint8_t iwf, const StandardWaveform *wf){
    if (regions->count(iwf) == 0) return;
    WaveformSignalRegions * this_regions = (*regions)[iwf];
    UInt_t nRegions = UInt_t(this_regions->GetNRegions());
    for (uint16_t i=0; i < nRegions; i++){
        if(verbose) cout << "REGION LOOP" << endl;
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
            if (p==0){
                std::cout<<"Invalid Integral Pointer. Continue."<<std::endl;
                continue;
            }
            if (verbose) cout << "GET INTEGRAL " << k << " " << peak_pos << " " << endl;
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
                time_integral = wf->getIntegral(p->GetIntegralStart(), p->GetIntegralStop(), peak_pos, f_trigger_cell, &tcal.at(iwf));
            }
            p->SetIntegral(integral);
            p->SetTimeIntegral(time_integral);
        }
    }
} // end FillRegionIntegrals()

void FileWriterTreeDRS4::FillRegionVectors(){
    IntegralNames->clear();
    IntegralValues->clear();
    TimeIntegralValues->clear();
    IntegralPeaks->clear();
    IntegralPeakTime->clear();
    IntegralLength->clear();

    for (uint8_t iwf = 0; iwf < 4; iwf++){
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
                IntegralPeaks->push_back(peak_pos);
                IntegralPeakTime->push_back(getTriggerTime(iwf, f_trigger_cell, peak_pos));
                uint16_t bin_low = p->GetIntegralStart();
                uint16_t bin_up = p->GetIntegralStop();
                IntegralLength->push_back(getTimeDifference(iwf, f_trigger_cell, bin_low, bin_up));
            }
        }
    }
}

void FileWriterTreeDRS4::FillTotalRange(uint8_t iwf, const StandardWaveform *wf){

    signed char pol = polarities.at(iwf);
    v_is_saturated->at(iwf) = wf->getAbsMaxInRange(0, 1023) > 498; // indicator if saturation is reached in sampling region (1-1024)
    v_median->at(iwf) = pol * wf->getMedian(0, 1023); // Median over whole sampling region
    v_average->at(iwf) = pol * wf->getIntegral(0, 1023);
}

void FileWriterTreeDRS4::UpdateWaveforms(uint8_t iwf){

    for (uint16_t j = 0; j < data->size(); j++) {
        if (UseWaveForm(save_waveforms, iwf))
            f_wf.at(uint8_t(iwf))->push_back(data->at(j));
        if     (iwf == 0) {
            if (f_pulser)
                avgWF_0_pul->SetBinContent(j+1, avgWF(float(avgWF_0_pul->GetBinContent(j+1)),data->at(j),f_pulser_events));
            else
                avgWF_0_sig->SetBinContent(j+1, avgWF(float(avgWF_0_sig->GetBinContent(j+1)),data->at(j),f_signal_events));
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

inline int FileWriterTreeDRS4::IsPulserEvent(const StandardWaveform *wf){
    float pulser_int = wf->getIntegral(uint16_t(ranges["pulserDRS4"]->first), uint16_t(ranges["pulserDRS4"]->second), true);
    return pulser_int > pulser_threshold;
} //end IsPulserEvent

inline void FileWriterTreeDRS4::ExtractForcTiming(vector<float> * data) {
    bool found_timing = false;
    for (uint16_t j=1; j<data->size(); j++){
        if( abs(data->at(j)) > 200 && abs(data->at(uint16_t(j - 1))) < 200) {
            v_forc_pos->push_back(j);
            v_forc_time->push_back(getTriggerTime(trigger_channel, f_trigger_cell, j));
            found_timing = true;
        }
    }
    if (!found_timing) {
        v_forc_pos->push_back(0);
        v_forc_time->push_back(-999);
    }
} //end ExtractForcTiming()

void FileWriterTreeDRS4::FillFullTime(){
    uint16_t n_waveform_samples = uint16_t(tcal.at(0).size() / 2);  // tcal vec is two times to big atm, todo: fix that!
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

inline float FileWriterTreeDRS4::getTriggerTime(const uint8_t & ch, const uint16_t & tcell, const uint16_t & bin) {
    return full_time.at(ch).at(bin + tcell) - full_time.at(ch).at(tcell);
}

float FileWriterTreeDRS4::getTimeDifference(uint8_t ch, uint16_t tcell, uint16_t bin_low, uint16_t bin_up) {
    return full_time.at(ch).at(bin_up + tcell) - full_time.at(ch).at(uint16_t(bin_low + tcell));
}

string FileWriterTreeDRS4::GetBitMask(uint16_t bitmask){
    stringstream ss;
    for (uint8_t i = 0; i < 4; i++) {
        string bit = to_string(UseWaveForm(bitmask, i));
        ss << string(3 - bit.size(), ' ') << bit;
    }
    return trim(ss.str(), " ");
}

string FileWriterTreeDRS4::GetPolarities(vector<signed char> pol) {
    stringstream ss;
    for (auto i_pol:pol) ss << string(3 - to_string(i_pol).size(), ' ') << to_string(i_pol);
    return trim(ss.str(), " ");
}

#endif // ROOT_FOUND
