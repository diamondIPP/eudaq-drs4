#ifdef ROOT_FOUND

#include "eudaq/FileWriterTreeDRS4.hh"

using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<FileWriterTreeDRS4> reg("drs4tree"); }


/** =====================================================================
    --------------------------CONSTRUCTOR--------------------------------
    =====================================================================*/
FileWriterTreeDRS4::FileWriterTreeDRS4(const std::string & /*param*/)
: m_tfile(0), m_ttree(0), m_noe(0), n_channels(4), n_pixels(90*90+60*60), histo(0), spec(0), fft_own(0), runnumber(0), hasTU(false) {

    gROOT->ProcessLine("gErrorIgnoreLevel = 5001;");
    gROOT->ProcessLine("#include <vector>");
    gROOT->ProcessLine("#include <pair>");
    gROOT->ProcessLine("#include <map>");
//    gInterpreter->GenerateDictionary("map<string,Float_t>;vector<map<string,Float_t> >", "vector;string;map");
    gInterpreter->GenerateDictionary("vector<vector<Float_t> >;vector<vector<UShort_t> >");
    //Polarities of signals, default is positive signals
    polarities.resize(4, 1);
    pulser_polarities.resize(4, 1);
    spectrum_polarities.resize(4, 1);

    //how many events will be analyzed, 0 = all events
    max_event_number = 0;
    macro = new TMacro();
    macro->SetNameTitle("region_information", "Region Information");

    f_nwfs = 0;
    f_event_number = -1;
    f_pulser_events = 0;
    f_signal_events = 0;
    f_time = -1.;
    old_time = 0;
    f_pulser = -1;
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

    // general waveform information
    v_is_saturated = new vector<bool>;
    v_median = new vector<float>;
    v_average = new vector<float>;
    v_max_peak_position = new vector<uint16_t>;
    v_max_peak_time = new vector<float>;
    v_max_peak_position->resize(4, 0);
    v_peak_positions = new vector<vector<uint16_t> >;
    v_peak_times = new vector<vector<float> >;
    v_peak_positions->resize(4);
    v_peak_times->resize(4);
    v_npeaks = new vector<uint8_t>;
    v_npeaks->resize(4, 0);
    v_max_peak_time->resize(4, 0);
    v_rise_time = new vector<float>;
    v_fall_time = new vector<float>;
    v_wf_start = new vector<float>;
    v_fit_peak_time = new vector<float>;
    v_fit_peak_value = new vector<float>;
    v_peaking_time = new vector<float>;

    // waveforms
    for (uint8_t i = 0; i < 4; i++) f_wf[i] = new vector<float>;
    f_isDa = new vector<bool>;
    wf_thr = {125, 10, 40, 300};

    // spectrum vectors
    noise = new vector<pair<float, float> >;
    noise->resize(4);
    for (uint8_t i = 0; i < 4; i++) noise_vectors[i] = new deque<float>;
    decon.resize(1024, 0);
    peaks_x.resize(4, new std::vector<uint16_t>);
    peaks_x_time.resize(4, new std::vector<float>);
    peaks_y.resize(4, new std::vector<float>);

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

    //tu
    v_scaler = new vector<uint64_t>;
    v_scaler->resize(5);
    old_scaler = new vector<uint64_t>;
    old_scaler->resize(5, 0);

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
    if (this->m_config == nullptr){
        EUDAQ_WARN("Configuration class instance m_config does not exist!");
        return;
    }
    m_config->SetSection("Converter.drs4tree");
    if (m_config->NSections() == 0){
        EUDAQ_WARN("Config file has no sections!");
        return;
    }
    cout << endl;
    EUDAQ_INFO("Configuring FileWriterTreeDRS4");

    max_event_number = m_config->Get("max_event_number", 0);

    // spectrum and fft
    peak_noise_pos = m_config->Get("peak_noise_pos", m_config->Get("pedestal_ab_region", make_pair(20, 20)).first);
    spec_sigma = m_config->Get("spectrum_sigma", 5);
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
void FileWriterTreeDRS4::StartRun(unsigned runnumber) {
    this->runnumber = runnumber;
    EUDAQ_INFO("Converting the input file into a DRS4 TTree " );
    string f_output = FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber);
    EUDAQ_INFO("Preparing the output file: " + f_output);

    c1 = new TCanvas();
    c1->Draw();

    m_tfile = new TFile(f_output.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");
    m_thdir = m_tfile->mkdir("DecodingHistos");

    // Set Branch Addresses
    m_ttree->Branch("event_number", &f_event_number, "event_number/I");
    m_ttree->Branch("time",& f_time, "time/D");
    m_ttree->Branch("pulser",& f_pulser, "pulser/I");
    m_ttree->Branch("nwfs", &f_nwfs, "n_waveforms/I");
    m_ttree->Branch("beam_current", &f_beam_current, "beam_current/s");
    if (hasTU) {
//        m_ttree->Branch("beam_current", &f_beam_current, "beam_current/s");
        m_ttree->Branch("rate", &v_scaler);
    }
    m_ttree->Branch("forc_pos", &v_forc_pos);
    m_ttree->Branch("forc_time", &v_forc_time);

    // drs4
    m_ttree->Branch("trigger_cell", &f_trigger_cell, "trigger_cell/s");

    // waveforms
    for (uint8_t i_wf = 0; i_wf < 4; i_wf++)
        if ((save_waveforms & 1 << i_wf) == 1 << i_wf)
            m_ttree->Branch(TString::Format("wf%i", i_wf), &f_wf.at(i_wf));
    m_ttree->Branch("wf_isDA", &f_isDa);

    // integrals
    m_ttree->Branch("IntegralValues",&IntegralValues);
    m_ttree->Branch("TimeIntegralValues",&TimeIntegralValues);
    m_ttree->Branch("IntegralPeaks",&IntegralPeaks);
    m_ttree->Branch("IntegralPeakTime",&IntegralPeakTime);
    m_ttree->Branch("IntegralLength",&IntegralLength);

    // DUT
    m_ttree->Branch("is_saturated", &v_is_saturated);
    m_ttree->Branch("median", &v_median);
    m_ttree->Branch("average", &v_average);

    if (active_regions){
      m_ttree->Branch(TString::Format("max_peak_position"), &v_max_peak_position);
      m_ttree->Branch(TString::Format("max_peak_time"), &v_max_peak_time);
      m_ttree->Branch("peak_positions", &v_peak_positions);
      m_ttree->Branch("peak_times", &v_peak_times);
      m_ttree->Branch("n_peaks", &v_npeaks);
      m_ttree->Branch("fall_time", &v_fall_time);
      m_ttree->Branch("rise_time", &v_rise_time);
      m_ttree->Branch("wf_start", &v_wf_start);
      m_ttree->Branch("fit_peak_time", &v_fit_peak_time);
      m_ttree->Branch("fit_peak_value", &v_fit_peak_value);
      m_ttree->Branch("peaking_time", &v_peaking_time);
    }

    // fft stuff and spectrum
    if (fft_waveforms) {
        m_ttree->Branch("fft_mean", &fft_mean);
        m_ttree->Branch("fft_mean_freq", &fft_mean_freq);
        m_ttree->Branch("fft_max", &fft_max);
        m_ttree->Branch("fft_max_freq", &fft_max_freq);
        m_ttree->Branch("fft_min", &fft_min);
        m_ttree->Branch("fft_min_freq", &fft_min_freq);
    }
    for (uint8_t i_wf = 0; i_wf < 4; i_wf++){
        if (UseWaveForm(spectrum_waveforms, i_wf)){
            m_ttree->Branch(TString::Format("peaks%d_x", i_wf), &peaks_x.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_x_time", i_wf), &peaks_x_time.at(i_wf));
            m_ttree->Branch(TString::Format("peaks%d_y", i_wf), &peaks_y.at(i_wf));
            m_ttree->Branch(TString::Format("n_peaks%d_total", i_wf), &n_peaks_total);
            m_ttree->Branch(TString::Format("n_peaks%d_before_roi", i_wf), &n_peaks_before_roi);
            m_ttree->Branch(TString::Format("n_peaks%d_after_roi", i_wf), &n_peaks_after_roi);
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
    verbose = 1;
    
    EUDAQ_INFO("Done with creating Branches!");
}

/**
 * Start Run for decoding settings
 * */

void FileWriterTreeDRS4::StartRun2(unsigned runno){
    m_tfile2 = new TFile(TString::Format("decoding_%d.root", runno), "RECREATE");
    std::cout << "\nCreated file \"decoding_" << int(runno) << ".root\"" << std::endl;
    m_thdir2 = m_tfile2->mkdir("DecodingHistos");
    m_thdir2->cd();
    std::cout << "Created directory \"DecodingHistos\"" << std::endl;
}

/**
 * Write event for finding decoding settings
 * */
void FileWriterTreeDRS4::WriteEvent2(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
        eudaq::PluginManager::SetConfig(ev, m_config);
        eudaq::PluginManager::Initialize(ev);
        //firstEvent =true;
        cout << "loading the first event..." << endl;
        return;
    } else if (ev.IsEORE()) {
        eudaq::PluginManager::ConvertToStandard(ev);
        cout << "loaded the last event." << endl;
        return;
    }
    // Condition to evaluate only certain number of events defined in configuration file  : DA
    if(max_event_number>0 && f_event_number>max_event_number)
        return;

    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
}

/**
 * Function to determine the decoding settings from the histograms in DecodingHistos
 * */
void FileWriterTreeDRS4::TempFunction(){
    m_tfile2->cd();
    TString bla = m_tfile2->GetName();
    std::cout << "FileName is: " << bla << std::endl;
    if(m_tfile2->IsOpen()) m_tfile2->Close();
    m_tfile3 = new TFile(bla, "UPDATE");
    m_thdir3 = (TDirectory*)m_tfile3->Get("DecodingHistos");
    Level1s = new std::vector<float>;
    decOffsets = new std::vector<float>;
    alphas = new std::vector<float>;
    std::vector<float> *blacks = new std::vector<float>;
    alphas->resize(0);
    Level1s->resize(0);
    decOffsets->resize(0);
    blacks->resize(0);
    for(size_t roci = 0; roci < 16; roci++) {
        if (m_thdir3->Get(TString::Format("cr_%d", int(roci)))) {
            TH1F *blah = (TH1F *) m_thdir3->Get(TString::Format("cr_%d", int(roci)));
            TH1F *hblack = (TH1F *) m_thdir3->Get(TString::Format("black_%d", int(roci)));
            TH1F *hublack = (TH1F *) m_thdir3->Get(TString::Format("uBlack_%d", int(roci)));
            TF1 *blaf1 = SetFit(blah, hblack, 3, 6, 6, int(roci));
            ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(100000);
            ROOT::Math::MinimizerOptions::SetDefaultTolerance(0.000001);
            ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Migrad"); // 2.2 seconds good fitting parameters
//                ROOT::Math::MinimizerOptions::SetDefaultMaxIterations(10000000);
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiFit", "GSLLM"); // 0.6 sec large chi2
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Scan"); //
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Simplex"); //
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Fumili2"); //
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","BFGS2"); // Too long more than 10 mins.
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","CONJFR"); // Too long more than 10 mins.
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","CONJPR"); // Too long more than 10 mins.
//                ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLSimAn","SimAn"); // 142 sec
            blah->Fit(TString::Format("blaf1_%d", int(roci)).Data(), "0Q");
            blah->Fit(TString::Format("blaf1_%d", int(roci)).Data(), "0Q");
            auto black(float(hblack->GetMean())), uBlack(float(hublack->GetMean()));
            auto delta(float(blaf1->GetParameter(1))), l1Spacing(float(blaf1->GetParameter(2) + blaf1->GetParameter(1)));
            // check if the first fitted peak is the extra one
            float p0 = blaf1->GetParameter(5) > blaf1->GetParameter(4) * 5? float(blaf1->GetParameter(0) + delta) : float(blaf1->GetParameter(0));
//                auto Level1(float(l1Spacing + p0 + 4 * delta));
            auto Level1(float(l1Spacing + p0));
            auto alpha(float(delta / l1Spacing));
            auto uBlackCorrected(float(GetTimeCorrectedValue(uBlack, 0, alpha)));
            auto blackCorrected(float(GetTimeCorrectedValue(black, uBlackCorrected, alpha)));
            alphas->push_back(alpha);
            blacks->push_back(blackCorrected);
            Level1s->push_back(Level1);
            decOffsets->push_back(Level1 - blackCorrected);
            m_thdir3->cd();
            blaf1->Write();
            m_tfile3->cd();
        }
    }
    m_tfile3->Close();
}

std::vector<float> FileWriterTreeDRS4::TempFunctionLevel1() {
    return *Level1s;
}
std::vector<float> FileWriterTreeDRS4::TempFunctionDecOff() {
    return *decOffsets;
}
std::vector<float> FileWriterTreeDRS4::TempFunctionAlphas() {
    return *alphas;
}

int FileWriterTreeDRS4::FindFirstBinAbove(TH1F* blah, float value, int binl, int binh){
    if(blah->GetBinContent(binl) > value){
        if(binl != 1)
            return FindFirstBinAbove(blah, value, binl - 1, binh);
        else
            return binl;
    } else if(blah->GetBinContent(binl) <= value && blah->GetBinContent(binh) <= value)
        return FindFirstBinAbove(blah, value, binl, binh + 1);
    else {
        for (int bini = binl; bini <= binh; bini++) {
            if (blah->GetBinContent(bini) > value)
                return bini;
        }
        return binh;
    }
}

int FileWriterTreeDRS4::FindLastBinAbove(TH1F* blah, float value, int binl, int binh){
    if(blah->GetBinContent(binh) > value){
        if(binh != blah->GetNbinsX())
            return FindLastBinAbove(blah, value, binl, binh + 1);
        else
            return binh;
    }
    else {
        for (int bini = binl; bini <= binh; bini++) {
            if (blah->GetBinContent(binh + binl - bini) > value)
                return binh + binl - bini;
        }
        return binl;
    }
}

float FileWriterTreeDRS4::GetTimeCorrectedValue(float value, float prevValue, float alpha){
    return (value - prevValue * alpha) / (1 - alpha);
}

TString* FileWriterTreeDRS4::GetFitString(int NL, int NSp, float sigmaSp) {
    TString *temps = new TString("0");
    for(int m = 0; m < NL; m++){
        for(int n = 0; n < NSp; n++){
            temps->Append(TString::Format("+([%d]*TMath::Exp(-TMath::Power((x-[0]-%d*[1]-%d*[2]-%f*[%d])/(TMath::Sqrt(2)*[3]),2)))", 4 + NSp * m + n, n, m, sigmaSp, 4 + NSp * (NL + m) + n));
        }
    }
    return temps;
}

TF1* FileWriterTreeDRS4::SetFit(TH1F* blah, TH1F* hblack, int NL, int NSp, int NTotal, int rocn){
    /**
     * This method calculates the decoding offsets from a histogram.
     * @param blah: is the histogram with the encoded levels. Tipically it is CR because it has homogeneous peaks as it is used to decide left pixel or right pixel
     * @param hblack: is the black histogram used to estimate the real width of the levels
     * @param NL: is the number of Levels to fit. 3 is used as default
     * @param NSp: is the number of peaks on each Level. Noticeable when the timing was not set up correctly. For CR it should be 6.
     * @param NTotal: is the total number of Levels in the histogram. For CR it should be 6 (default)
     * @return A pointer to the fitted function.
     * */
    int NSp2 = NSp + 1; // assume there is an extra peak to help the fitter. The extra peak will have a lower scaling constant much smaller than the other ones.
    float sigmaSp = float(hblack->GetRMS());
    TString *blast = GetFitString(NL, NSp2, float(sigmaSp * 2));
    int entries = int(blah->GetEntries());
    float maxh = float(blah->GetMaximum());
    float fracth = 20;
    int binl(int(blah->FindFirstBinAbove(0))), binh(int(blah->FindLastBinAbove(0)));
//    float xmin(float(blah->GetBinLowEdge(binl))), xmax(float(blah->GetBinLowEdge(binh + 1)));
    // Find levels limits
//    int binl0(FindFirstBinAbove(blah, maxh / fracth, binl, binh)), binh0(FindLastBinAbove(blah, maxh / fracth, binl, binh));
    int binl0(blah->FindFirstBinAbove(maxh / fracth)), binh0(blah->FindLastBinAbove(maxh / fracth));
    float xmin0(float(blah->GetBinLowEdge(binl0))), xmax0(float(blah->GetBinLowEdge(binh0 + 1)));
    // Find an estimate for l1; the spacing between clusters
    float widthp(float(2 * sigmaSp * TMath::Sqrt(2 * TMath::Log(fracth)))); // From FWPercent(p) = 2 * sigma * sqrt(2 * Ln(1/p)) for p = 1/fracth
    float l1(float((xmax0 - xmin0) / (NTotal - 1.) - 2 * widthp));
    // Find First cluster limits
    int binlN(0), binhN(0), binhP(FindLastBinAbove(blah, maxh / fracth, binl0 + 1, int(blah->FindBin(xmin0 + l1))));
    float xminN(0), xmaxN(0), xmaxP(float(blah->GetBinLowEdge(binhP + 1)));
    float xmax10(xmaxP);
    // Find cluster NL limits
    for(size_t it = 2; it <= NL; it++){
        binlN = FindFirstBinAbove(blah, maxh / fracth, binhP + 1, int(blah->FindBin(xmaxP + l1)));
        xminN = float(blah->GetBinLowEdge(binlN));
        binhN = FindLastBinAbove(blah, maxh / fracth, binlN + 1, int(blah->FindBin(xminN + l1)));
        xmaxN = float(blah->GetBinLowEdge(binhN + 1));
        binhP = binhN;
        xmaxP = xmaxN;
    }
    // Guess the offset on each cluster due to bad timing setting, a more accurate spacing between clusters, and the first position of the first cluster
    float delta0((xmax10 - xmin0 - widthp) / float(NSp2 - 1));
    float l1Guess((xminN - xmin0) / float(NL - 1));
    float p0(xmin0 + widthp / 2);
    // Create Fitting function
    TF1* tempf = new TF1(TString::Format("blaf1_%d",rocn).Data(), blast->Data(), xmin0, xmaxN);
    tempf->SetNpx(1000);
    tempf->SetParLimits(0, xmin0 - 50, xmin0 + delta0 / 4 + widthp);
    tempf->SetParLimits(1, 0, 1.5 * delta0);
    tempf->SetParLimits(2, l1Guess / 2, 1.5 * l1Guess);
    tempf->SetParLimits(3, 0.1, 3 * sigmaSp);
    tempf->SetParameter(0, p0);
    tempf->SetParameter(1, delta0);
    tempf->SetParameter(2, l1Guess);
    tempf->SetParameter(3, sigmaSp);
    for(int m = 0; m < NL; m++){
        for(int n = 0; n < NSp2; n++){
            tempf->SetParLimits(4 + NSp2 * m + n, 0, entries);
            tempf->SetParameter(4 + NSp2 * m + n, 0.8 * maxh);
            tempf->SetParLimits(4 + NSp2 * (NL + m) + n, -1, 1);
            tempf->SetParameter(4 + NSp2 * (NL + m) + n, 0.01);
        }
//        tempf->SetParLimits(4 + NSp2 * NL + m, -1, 1);
//        tempf->SetParameter(4 + NSp2 * NL + m, 0.01);
    }
    return tempf;
}


/** =====================================================================
    -------------------------WRITE EVENT---------------------------------
    =====================================================================*/
void FileWriterTreeDRS4::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
        PluginManager::SetConfig(ev, m_config);
        eudaq::PluginManager::Initialize(ev);
        tcal = PluginManager::GetTimeCalibration(ev);
        FillFullTime();
        macro->AddLine("\n[Time Calibration]");
        macro->AddLine(("tcal = [" + to_string(tcal.at(0)) + "]").c_str());
        cout << "loading the first event...." << endl;
        //todo: add time stamp for the very first tu event
        return;
    }
    else if (ev.IsEORE()) {
        eudaq::PluginManager::ConvertToStandard(ev);
        cout << "loading the last event...." << endl;
        return;
    }
    if (max_event_number > 0 && f_event_number > max_event_number) return;

    w_total.Start(false);
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);

    f_event_number = sev.GetEventNumber();

    /** TU STUFF */
    SetTimeStamp(sev);
    SetBeamCurrent(sev);
    SetScalers(sev);
    // --------------------------------------------------------------------
    // ---------- get the number of waveforms -----------------------------
    // --------------------------------------------------------------------
    auto nwfs = (unsigned int) sev.NumWaveforms();
    f_nwfs = nwfs;

    if(f_event_number <= 10 && verbose > 0){
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
    vector<uint8_t> wf_order = {2, 1, 0, 3};
    for (auto iwf : wf_order) {
      sev.GetWaveform(iwf).SetPolarities(polarities.at(iwf), pulser_polarities.at(iwf));
      sev.GetWaveform(iwf).SetTimes(&tcal.at(0));
    }
    ResizeVectors(sev.GetNWaveforms());
    FillRegionIntegrals(sev);

    for (auto iwf : wf_order){
        if (verbose > 3) cout<<"Channel Nr: "<< int(iwf) <<endl;

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
        if (verbose > 3) cout<<"DoSpectrumFitting "<< iwf <<endl;
        this->DoSpectrumFitting(iwf);
        if (verbose > 3) cout << "DoFFT " << iwf << endl;
        this->DoFFTAnalysis(iwf);

        // drs4 info
        if (iwf == wf_order.at(0)) f_trigger_cell = waveform.GetTriggerCell(); // same for every waveform

        // calculate the signal and so on

        if (verbose > 3) cout << "get Values1.1 " << iwf << endl;
        FillTotalRange(iwf, &waveform);

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

        f_isDa->at(iwf) = *max_element(&data->at(20), &data->at(1023)) > wf_thr.at(iwf);

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
            f_plane->emplace_back(iplane);
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
    if (macro && m_ttree) {
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
        ss << "\nTotal time: " << setw(2) << setfill('0') << int(t / 60) << ":" << setw(2) << setfill('0')
           << int(t - int(t / 60) * 60);
        if (entries > 1000) ss << "\nTime/1000 events: " << int(t / entries * 1000 * 1000) << " ms";
        if (spectrum_waveforms) ss << "\nTSpectrum: " << w_spectrum.RealTime() << " seconds";
        print_banner(ss.str(), '*');
    }
    if(m_tfile2)
        if (m_tfile2->IsOpen())
            m_tfile2->Close();
    if(m_tfile3)
        if (m_tfile3->IsOpen())
            m_tfile3->Close();
    if(m_tfile)
        if(m_tfile->IsOpen())
            m_tfile->cd();
    if(m_ttree)
        m_ttree->Write();
    if(avgWF_0) {
        avgWF_0->Write();
        avgWF_0_pul->Write();
        avgWF_0_sig->Write();
        avgWF_1->Write();
        avgWF_2->Write();
        avgWF_3->Write();
        avgWF_3_sig->Write();
        avgWF_3_pul->Write();
    }
    if (macro) macro->Write();
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
    f_isDa->clear();

    f_plane->clear();
    f_col->clear();
    f_row->clear();
    f_adc->clear();
    f_charge->clear();

    for (auto vec: *v_peak_positions) vec.clear();
    for (auto vec: *v_peak_times) vec.clear();
    for (auto peak: peaks_x) peak->clear();
    for (auto peak: peaks_x_time) peak->clear();
    for (auto peak: peaks_y) peak->clear();
} // end ClearVectors()

inline void FileWriterTreeDRS4::ResizeVectors(size_t n_channels) {
    for (auto r: *regions) r.second->Reset();

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
        cout << "Recreating a new VirtualFFT with " << n_samples << " Samples, before " << fft_own->GetN() << endl;
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

inline void FileWriterTreeDRS4::DoSpectrumFitting(uint8_t iwf){
    if (!UseWaveForm(spectrum_waveforms, iwf)) return;

    w_spectrum.Start(false);

    float max = *max_element(data_pos.begin(), data_pos.end());
    //return if the max element is lower than 4 sigma of the noise
    float threshold = 4 * noise->at(iwf).second + noise->at(iwf).first;

    if (max <= threshold) return;
    // tspec threshold is in per cent to max peak

    threshold = threshold / max * 100;
    uint16_t size = uint16_t(data_pos.size());

    n_peaks_total = 0;
    n_peaks_before_roi = 0;
    n_peaks_after_roi = 0;

    int peaks = spec->SearchHighRes(&data_pos[0], &decon[0], size, spec_sigma, threshold, spec_rm_bg, spec_decon_iter, spec_markov, spec_aver_win);
    n_peaks_total = peaks; //this goes in the root file
    for(uint8_t i=0; i < peaks; i++){
        uint16_t bin = uint16_t(spec->GetPositionX()[i] + .5);
        uint16_t min_bin = bin - 5 >= 0 ? uint16_t(bin - 5) : uint16_t(0);
        uint16_t max_bin = bin + 5 < size ? uint16_t(bin + 5) : uint16_t(size - 1);
        max = *std::max_element(&data_pos.at(min_bin), &data_pos.at(max_bin));
        peaks_x.at(iwf)->push_back(bin);
        float peaktime = getTriggerTime(iwf, bin);

        if (peaktime < peak_finding_roi.first){
            n_peaks_before_roi++;}

        if (peaktime >= peak_finding_roi.second){
            n_peaks_after_roi++;}

        peaks_x_time.at(iwf)->push_back(peaktime);
        peaks_y.at(iwf)->push_back(max);

    }
    w_spectrum.Stop();
} // end DoSpectrumFitting()


void FileWriterTreeDRS4::FillSpectrumData(uint8_t iwf){
    bool b_spectrum = UseWaveForm(spectrum_waveforms, iwf);
    bool b_fft = UseWaveForm(fft_waveforms, iwf);
    if(b_spectrum || b_fft){
        data_pos.resize(data->size());
        for (uint16_t i = 0; i < data->size(); i++)
            data_pos.at(i) = spectrum_polarities.at(iwf) * data->at(i);
    }
} // end FillSpectrumData()

void FileWriterTreeDRS4::calc_noise(uint8_t iwf) {
  float value = data->at(peak_noise_pos);
  // filter out peaks at the pedestal
  if (std::abs(value) < 6 * noise->at(iwf).second + std::abs(noise->at(iwf).first) or noise_vectors.at(iwf)->size() < 10)
    noise_vectors.at(iwf)->push_back(value);
  if (noise_vectors.at(iwf)->size() >= 1000)
    noise_vectors.at(iwf)->pop_front();
  noise->at(iwf) = calc_mean(*noise_vectors.at(iwf));
}

void FileWriterTreeDRS4::FillRegionIntegrals(const StandardEvent sev){

    for (auto channel: *regions){
      const StandardWaveform * wf = &sev.GetWaveform(channel.first);
      channel.second->GetRegion(0)->SetPeakPostion(5);
      for (auto region: channel.second->GetRegions()){
        signed char polarity = (string(region->GetName()).find("pulser") != string::npos) ? channel.second->GetPulserPolarity() : channel.second->GetPolarity();
        uint16_t peak_pos = wf->getIndex(region->GetLowBoarder(), region->GetHighBoarder(), polarity);
        region->SetPeakPostion(peak_pos);
        for (auto integral: region->GetIntegrals()){
          std::string name = integral->GetName();
          std::transform(name.begin(), name.end(), name.begin(), ::tolower);
          integral->SetPeakPosition(peak_pos, wf->GetNSamples());
          integral->SetTimeIntegral(wf->getIntegral(integral->GetIntegralStart(), integral->GetIntegralStop(), peak_pos, 2.0));
          if (name.find("peaktopeak")!=std::string::npos){
            integral->SetIntegral(wf->getPeakToPeak(integral->GetIntegralStart(), integral->GetIntegralStop()));
          } else if (name.find("median")!=name.npos){
            integral->SetIntegral(wf->getMedian(integral->GetIntegralStart(), integral->GetIntegralStop()));
          } else
            integral->SetIntegral(wf->getIntegral(integral->GetIntegralStart(), integral->GetIntegralStop()));
        }
      }
    }
} // end FillRegionIntegrals()

void FileWriterTreeDRS4::FillRegionVectors(){
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
          IntegralPeakTime->push_back(getTriggerTime(ch.first, peak_pos));
          IntegralLength->push_back(getTimeDifference(ch.first, integral->GetIntegralStart(), integral->GetIntegralStop()));
        }
      }
    }
}

void FileWriterTreeDRS4::FillTotalRange(uint8_t iwf, const StandardWaveform *wf){

    signed char pol = polarities.at(iwf);
    v_is_saturated->at(iwf) = wf->getAbsMaxInRange(0, 1023) > 498; // indicator if saturation is reached in sampling region (1-1024)
    v_median->at(iwf) = pol * wf->getMedian(0, 1023); // Median over whole sampling region
    v_average->at(iwf) = pol * wf->getIntegral(0, 1023);
    if (UseWaveForm(active_regions, iwf)){

        WaveformSignalRegion * reg = regions->at(iwf)->GetRegion("signal_b");
        v_rise_time->at(iwf) = wf->getRiseTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise->at(iwf).first);
        v_fall_time->at(iwf) = wf->getFallTime(reg->GetLowBoarder(), uint16_t(reg->GetHighBoarder() + 10), noise->at(iwf).first);
        auto fit_peak = wf->fitMaximum(reg->GetLowBoarder(), reg->GetHighBoarder());
        v_fit_peak_time->at(iwf) = fit_peak.first;
        v_fit_peak_value->at(iwf) = fit_peak.second;
        v_wf_start->at(iwf) = wf->getWFStartTime(reg->GetLowBoarder(), reg->GetHighBoarder(), noise->at(iwf).first, fit_peak.second);
        v_peaking_time->at(iwf) = v_fit_peak_time->at(iwf) - v_wf_start->at(iwf);
        pair<uint16_t, float> peak = wf->getMaxPeak();
        v_max_peak_position->at(iwf) = peak.first;
        v_max_peak_time->at(iwf) = getTriggerTime(iwf, peak.first);
        float threshold = polarities.at(iwf) * 4 * noise->at(iwf).second + noise->at(iwf).first;
        vector<uint16_t> * peak_positions = wf->getAllPeaksAbove(0, 1023, threshold);
        v_peak_positions->at(iwf) = *peak_positions;
        v_npeaks->at(iwf) = uint8_t(v_peak_positions->at(iwf).size());
        vector<float> peak_timings;
        for (auto i_pos:*peak_positions) peak_timings.push_back(getTriggerTime(iwf, i_pos));
        v_peak_times->at(iwf) = peak_timings;
    }
}

void FileWriterTreeDRS4::UpdateWaveforms(uint8_t iwf){

    for (uint16_t j = 0; j < data->size(); j++) {
        if (UseWaveForm(save_waveforms, iwf))
            f_wf.at(uint8_t(iwf))->emplace_back(data->at(j));
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
    float pulser_int = wf->getIntegral(pulser_region.first, pulser_region.second, true);
    return pulser_int > pulser_threshold;
} //end IsPulserEvent

inline void FileWriterTreeDRS4::ExtractForcTiming(vector<float> * data) {
    bool found_timing = false;
    for (uint16_t j=1; j < data->size(); j++){
        if( abs(data->at(j)) > 200 && abs(data->at(uint16_t(j - 1))) < 200) {
            v_forc_pos->push_back(j);
            v_forc_time->push_back(getTriggerTime(trigger_channel, j));
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

inline float FileWriterTreeDRS4::getTriggerTime(const uint8_t & ch, const uint16_t & bin) {
    return full_time.at(ch).at(bin + f_trigger_cell) - full_time.at(ch).at(f_trigger_cell);
}

float FileWriterTreeDRS4::getTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up) {
    return full_time.at(ch).at(bin_up + f_trigger_cell) - full_time.at(ch).at(uint16_t(bin_low + f_trigger_cell));
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

void FileWriterTreeDRS4::SetTimeStamp(StandardEvent sev) {
    if (sev.hasTUEvent()){
        if (sev.GetTUEvent(0).GetValid())
            f_time = sev.GetTimestamp();
    }
    else
        f_time = sev.GetTimestamp() / 384066.;
}

void FileWriterTreeDRS4::SetBeamCurrent(StandardEvent sev) {

    if (sev.hasTUEvent()){
        StandardTUEvent tuev = sev.GetTUEvent(0);
        f_beam_current = uint16_t(tuev.GetValid() ? tuev.GetBeamCurrent() : UINT16_MAX);
    }
}

void FileWriterTreeDRS4::SetScalers(StandardEvent sev) {

    if (sev.hasTUEvent()) {
        StandardTUEvent tuev = sev.GetTUEvent(0);
        bool valid = tuev.GetValid();
        /** scaler continuously count upwards: subtract old scaler value and divide by time interval to get rate
         *  first scaler value is the scintillator and then the planes */
        for (uint8_t i(0); i < 5; i++) {
                v_scaler->at(i) = uint64_t(valid ? (tuev.GetScalerValue(i) - old_scaler->at(i)) * 1000 / (f_time - old_time) : UINT32_MAX);
                if (valid)
                    old_scaler->at(i) = tuev.GetScalerValue(i);
            }
        if (valid)
            old_time = f_time;
        }
}

void FileWriterTreeDRS4::ReadIntegralRanges() {

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

void FileWriterTreeDRS4::ReadIntegralRegions() {

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

#endif // ROOT_FOUND
