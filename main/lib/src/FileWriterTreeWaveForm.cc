#ifdef ROOT_FOUND

#include "eudaq/FileWriterTreeWaveForm.hh"

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeWaveForm> reg("waveformtree"); }

/** =====================================================================
    --------------------------CONSTRUCTOR--------------------------------
    =====================================================================*/
FileWriterTreeWaveForm::FileWriterTreeWaveForm(const std::string & /*param*/)
: m_tfile(0), m_ttree(0), m_noe(0), chan(4), histo(0), runnumber(0) {

    gROOT->ProcessLine("#include <vector>");
    gROOT->ProcessLine( "gErrorIgnoreLevel = 5001;");
    gROOT->ProcessLine("#include <pair>");
    gROOT->ProcessLine("#include <map>");
    gInterpreter->GenerateDictionary("map<string,Float_t>;vector<map<string,Float_t> >,vector<vector<Float_t> >","vector;string;map");
    
    //how many events will be analyzed, 0 = all events
    max_event_number = 0;
    macro = new TMacro();
    macro->SetNameTitle("region_information", "Region Information");

    f_nwfs = 0;
    f_event_number = -1;
    f_time = -1.;

    // integrals
    regions = new std::map<int,WaveformSignalRegions* >;
    IntegralNames = new std::vector<std::string>;
    IntegralValues = new std::vector<float>;
    TimeIntegralValues = new vector<float>;
    IntegralPeaks = new std::vector<Int_t>;
    IntegralPeakTime = new std::vector<float>;
    IntegralLength  = new std::vector<float>;

    for (uint8_t i = 0; i < 4; i++) f_wf[i] = new vector<float>;

    // general waveform information
    v_polarities = new vector<int16_t>;
    v_peak_positions = new vector<uint16_t>;
    v_peak_values = new vector<float>;
    v_peak_timings = new vector<float>;
    v_pedestals = new vector<float>;
    v_peak_positions->resize(4, 0);
    v_pedestals->resize(4, 0);
    v_peak_values->resize(4, 0);
    v_peak_timings->resize(4, 0);
} // end Constructor

/** =====================================================================
    --------------------------CONFIGURE----------------------------------
    =====================================================================*/
void FileWriterTreeWaveForm::Configure(){

    // do some assertions
    if (!this->m_config){
        EUDAQ_WARN("Configuration class instance m_config does not exist!");
        return;
    }
    m_config->SetSection("Converter.waveformtree");
    if (m_config->NSections()==0){
        EUDAQ_WARN("Config file has no sections!");
        return;
    }
    cout << endl;
    EUDAQ_INFO("Configuring FileWriterTreeWaveForm");

    max_event_number = m_config->Get("max_event_number", 0);

    // default ranges
    ranges["pedestal"] = new pair<float, float>(m_config->Get("pedestal_range", make_pair(0, 100)));

    // saved waveforms
    save_waveforms = m_config->Get("save_waveforms", uint16_t(9));

    // regions todo: add default range
    active_regions = m_config->Get("active_regions", uint16_t(0));
    macro->AddLine(TString::Format("active_regions: %d", active_regions));
    macro->AddLine("");


    // output
    cout << "CHANNEL AND PULSER SETTINGS:" << endl;
    cout << append_spaces(24, "  save waveforms:") << save_waveforms << ":  " << GetBitMask(save_waveforms) << endl;
    cout << append_spaces(24, "  active regions:") << active_regions << ":  " << GetBitMask(active_regions) << endl;

    cout<<"RANGES: " << ranges.size() << endl;
    for (auto& it: ranges) cout << append_spaces(25, "  range_" + it.first + ":") << to_string(*(it.second)) << endl;

    cout << "\nMAXIMUM NUMBER OF EVENTS: " << (max_event_number ? to_string(max_event_number) : "ALL") << endl;
    EUDAQ_INFO("End of Configure!");
    cout << endl;
    macro->Write();
} // end Configure()

/** =====================================================================
    --------------------------START RUN----------------------------------
    =====================================================================*/
void FileWriterTreeWaveForm::StartRun(unsigned runnumber) {
    this->runnumber = runnumber;
    EUDAQ_INFO("Converting the input file into a WaveForm TTree " );
    string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the output file: " + foutput);

    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");

    // Set Branch Addresses
    m_ttree->Branch("event_number", &f_event_number, "event_number/I");
    m_ttree->Branch("time",& f_time, "time/D");
    m_ttree->Branch("nwfs", &f_nwfs, "n_waveforms/I");

    // drs4
    m_ttree->Branch("trigger_cell", &f_trigger_cell, "trigger_cell/s");

    // waveforms
    for (uint8_t i_wf = 0; i_wf < 4; i_wf++)
        if ((save_waveforms & 1 << i_wf) == 1 << i_wf)
            m_ttree->Branch(TString::Format("wf%i", i_wf), &f_wf.at(i_wf));

    // integrals
//    m_ttree->Branch("IntegralNames",&IntegralNames);
//    m_ttree->Branch("IntegralValues",&IntegralValues);
//    m_ttree->Branch("TimeIntegralValues",&TimeIntegralValues);
//    m_ttree->Branch("IntegralPeaks",&IntegralPeaks);
//    m_ttree->Branch("IntegralPeakTime",&IntegralPeakTime);
//    m_ttree->Branch("IntegralLength",&IntegralLength);

    // DUT
    m_ttree->Branch("peak_positions", &v_peak_positions);
    m_ttree->Branch("peak_values", &v_peak_values);
    m_ttree->Branch("peak_timings", &v_peak_timings);
    m_ttree->Branch("pedestals", &v_pedestals);

    EUDAQ_INFO("Done with creating Branches!");
}

/** =====================================================================
    -------------------------WRITE EVENT---------------------------------
    =====================================================================*/
void FileWriterTreeWaveForm::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
        PluginManager::SetConfig(ev, m_config);
        eudaq::PluginManager::Initialize(ev);
        tcal = PluginManager::GetTimeCalibration(ev);
        FillFullTime();
        stringstream ss;
        ss << "tcal [";
        for (auto i:tcal.at(0)) ss << i << ", ";
        ss << "\b\b]";
        macro->AddLine(ss.str().c_str());
        cout << "loading the first event...." << endl;
        //todo: add time stamp for the very first tu event
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
    // set time stamp
    SetTimeStamp(sev);
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

        // drs4 info
        if (iwf == wf_order.at(0)) f_trigger_cell = waveform.GetTriggerCell(); // same for every waveform

//        // calculate the signal and so on
//        // float sig = CalculatePeak(data, 1075, 1150);
//        if (verbose > 3) cout << "get Values1.0 " << iwf << endl;
//        FillRegionIntegrals(iwf, &waveform);
//
//        if (verbose > 3) cout << "get Values1.1 " << iwf << endl;
        FillPeaks(iwf, &waveform);

        // fill waveform vectors
        if (verbose > 3) cout << "fill wf " << iwf << endl;
        UpdateWaveforms(iwf);

        data->clear();
    } // end iwf waveform loop

//    FillRegionVectors();

    m_ttree->Fill();
    if (f_event_number + 1 % 1000 == 0) cout << "of run " << runnumber << flush;
//        <<" "<<std::setw(7)<<f_event_number<<"\tSpectrum: "<<w_spectrum.RealTime()/w_spectrum.Counter()<<"\t" <<"LinearFitting: "
//        <<w_linear_fitting.RealTime()/w_linear_fitting.Counter()<<"\t"<< w_spectrum.Counter()<<"/"<<w_linear_fitting.Counter()<<"\t"<<flush;
    w_total.Stop();
} // end WriteEvent()

/** =====================================================================
    -------------------------DECONSTRUCTOR-------------------------------
    =====================================================================*/
FileWriterTreeWaveForm::~FileWriterTreeWaveForm() {
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
    if (macro) macro->Write();
    if (m_tfile->IsOpen()) m_tfile->Close();
}

float FileWriterTreeWaveForm::Calculate(std::vector<float> * data, int min, int max, bool _abs) {
    float integral = 0;
    uint16_t i;
    for (i = uint16_t(min) ; i <= (max + 1) && i < data->size() ; i++){
        if(!_abs) integral += data->at(i);
        else integral += abs(data->at(i));
    }
    return integral / float(i - min);
}

/*float FileWriterTreeWaveForm::CalculatePeak(std::vector<float> * data, int min, int max) {
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

std::pair<int, float> FileWriterTreeWaveForm::FindMaxAndValue(std::vector<float> * data, int min, int max) {
    float maxVal = -999;
    int imax = min;
    for (int i = min; i <= int(max+1) && i < data->size() ;i++){
        if (abs(data->at(i)) > maxVal){ maxVal = abs(data->at(i)); imax = i; }
    }
    maxVal = data->at(imax);
    std::pair<int, float> res = make_pair(imax, maxVal);
    return res;
}*/

float FileWriterTreeWaveForm::avgWF(float old_avg, float new_value, int n) {
    float avg = old_avg;
    avg -= old_avg/n;
    avg += new_value/n;
    return avg;
}

uint64_t FileWriterTreeWaveForm::FileBytes() const { return 0; }

inline void FileWriterTreeWaveForm::ClearVectors(){

    for (auto v_wf:f_wf) v_wf.second->clear();
} // end ClearVectors()

inline void FileWriterTreeWaveForm::ResizeVectors(size_t n_channels) {
    for (auto r: *regions) r.second->Reset();

} // end ResizeVectors()


void FileWriterTreeWaveForm::FillRegionIntegrals(uint8_t iwf, const StandardWaveform *wf){
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
                time_integral = wf->getIntegral(p->GetIntegralStart(), p->GetIntegralStop(), peak_pos, f_trigger_cell, &tcal.at(iwf), 2.0);
            }
            p->SetIntegral(integral);
            p->SetTimeIntegral(time_integral);
        }
    }
} // end FillRegionIntegrals()

void FileWriterTreeWaveForm::FillRegionVectors(){
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
                IntegralPeakTime->push_back(getTriggerTime(iwf, peak_pos));
                uint16_t bin_low = p->GetIntegralStart();
                uint16_t bin_up = p->GetIntegralStop();
                IntegralLength->push_back(getTimeDifference(iwf, bin_low, bin_up));
            }
        }
    }
}

void FileWriterTreeWaveForm::FillPeaks(uint8_t iwf, const StandardWaveform *wf){

    if (UseWaveForm(active_regions, iwf)){
        pair<uint16_t, float> peak = wf->getMaxPeak();
        v_peak_positions->at(iwf) = peak.first;
        v_peak_values->at(iwf) = peak.second;
        v_peak_timings->at(iwf) = getTriggerTime(iwf, peak.first);
        v_pedestals->at(iwf) = wf->getIntegral(uint16_t(ranges["pedestal"]->first), uint16_t(ranges["pedestal"]->second), false);
    }
}

void FileWriterTreeWaveForm::UpdateWaveforms(uint8_t iwf){

    for (uint16_t j = 0; j < data->size(); j++) {
        if (UseWaveForm(save_waveforms, iwf))
            f_wf.at(uint8_t(iwf))->push_back(data->at(j));
    } // data loop
} // end UpdateWaveforms()


void FileWriterTreeWaveForm::FillFullTime(){
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

inline float FileWriterTreeWaveForm::getTriggerTime(const uint8_t & ch, const uint16_t & bin) {
    return full_time.at(ch).at(bin + f_trigger_cell) - full_time.at(ch).at(f_trigger_cell);
}

float FileWriterTreeWaveForm::getTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up) {
    return full_time.at(ch).at(bin_up + f_trigger_cell) - full_time.at(ch).at(uint16_t(bin_low + f_trigger_cell));
}

string FileWriterTreeWaveForm::GetBitMask(uint16_t bitmask){
    stringstream ss;
    for (uint8_t i = 0; i < 4; i++) {
        string bit = to_string(UseWaveForm(bitmask, i));
        ss << string(3 - bit.size(), ' ') << bit;
    }
    return trim(ss.str(), " ");
}

string FileWriterTreeWaveForm::GetPolarities(vector<signed char> pol) {
    stringstream ss;
    for (auto i_pol:pol) ss << string(3 - to_string(i_pol).size(), ' ') << to_string(i_pol);
    return trim(ss.str(), " ");
}

void FileWriterTreeWaveForm::SetTimeStamp(StandardEvent sev) {
    if (sev.hasTUEvent()){
        if (sev.GetTUEvent(0).GetValid())
            f_time = sev.GetTimestamp();
    }
    else
        f_time = sev.GetTimestamp() / 384066.;
}

#endif // ROOT_FOUND
