#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "TFile.h"

#include "TDirectory.h"
#include "TTree.h"
#include "TROOT.h"
#include <Math/MinimizerOptions.h>
#include "TH1F.h"
#include "TF1.h"
#include "TString.h"
#include "TFitResultPtr.h"
#include "TMath.h"

using namespace std;
using namespace eudaq;

template <typename T>
inline T unpack_fh (vector <unsigned char >::iterator &src, T& data){ //unpack from host-byte-order
  data=0;
  for(unsigned int i=0;i<sizeof(T);i++){
    data+=((uint64_t)*src<<(8*i));
    src++;
  }
  return data;
}

template <typename T>
inline T unpack_fn(vector<unsigned char>::iterator &src, T& data){            //unpack from network-byte-order
  data=0;
  for(unsigned int i=0;i<sizeof(T);i++){
    data+=((uint64_t)*src<<(8*(sizeof(T)-1-i)));
    src++;
  }
  return data;
}

template <typename T>
inline T unpack_b(vector<unsigned char>::iterator &src, T& data, unsigned int nb){            //unpack number of bytes n-b-o only
  data=0;
  for(unsigned int i=0;i<nb;i++){
    data+=(uint64_t(*src)<<(8*(nb-1-i)));
    src++;
  }
  return data;
}

typedef pair<vector<unsigned char>::iterator,unsigned int> datablock_t;


namespace eudaq {

  class FileWriterTreeTelescope : public FileWriter {
  public:
    FileWriterTreeTelescope(const std::string &);
    virtual void StartRun(unsigned);
    virtual void Configure();
    virtual void WriteEvent(const DetectorEvent &);
      virtual uint64_t FileBytes() const;
      virtual ~FileWriterTreeTelescope();
      // Add to get maximum number of events: DA
    virtual long GetMaxEventNumber();
      virtual string GetStats(const DetectorEvent &);
      virtual void setTU(bool tu) { hasTU = tu; }
      virtual void WriteEvent2(const DetectorEvent &);
      virtual void StartRun2(unsigned);
      virtual void TempFunction();
      virtual std::vector<float> TempFunctionLevel1();
      virtual std::vector<float> TempFunctionDecOff();
      virtual std::vector<float> TempFunctionAlphas();

  private:
    TFile * m_tfile; // book the pointer to a file (to store the otuput)
    TTree * m_ttree; // book the tree (to store the needed event info)
      TFile * m_tfile2; // book the pointer to a file (to store the otuput)
      TFile * m_tfile3; // book the pointer to a file (to store the otuput)
      TDirectory * m_thdir;
      TDirectory * m_thdir2;
      TDirectory * m_thdir3;
    // Book variables for the Event_to_TTree conversion
    unsigned m_noe;
    short chan;
    int n_pixels;
    // for Configuration file
    long max_event_number;
    bool hasTU;

    // Scalar Branches
    int f_event_number;
    double f_time;
    double old_time;
    uint16_t f_beam_current;

    // Vector Branches
    std::vector<uint16_t> * f_plane;
    std::vector<uint16_t> * f_col;
    std::vector<uint16_t> * f_row;
    std::vector<int16_t> * f_adc;
    std::vector<uint32_t> * f_charge;
    std::vector<int16_t> * f_trig_phase;
      std::vector<float> *Level1s;
      std::vector<float> *decOffsets;
      std::vector<float> *alphas;


      //tu
    std::vector<uint64_t> * v_scaler;
    std::vector<uint64_t> * old_scaler;
    void SetTimeStamp(StandardEvent /*sev*/);
    void SetBeamCurrent(StandardEvent /*sev*/);
    void SetScalers(StandardEvent /*sev*/);

      // Decoding stuff
      int FindFirstBinAbove(TH1F* blah, float value, int binl, int binh);
      int FindLastBinAbove(TH1F* blah, float value, int binl, int binh);

      float GetTimeCorrectedValue(float value, float prevValue, float alpha=0.1);
      TString* GetFitString(int NL=3, int NSp=6, float sigmaSp=2.5);
      TF1* SetFit(TH1F* blah, TH1F* hblack, int NL=3, int NSp=6, int NTotal=6, int rocn=0);
  };

  namespace {
    static RegisterFileWriter<FileWriterTreeTelescope> reg("telescopetree");
  }

  FileWriterTreeTelescope::FileWriterTreeTelescope(const std::string & /*param*/)
    : m_tfile(0), m_ttree(0),m_noe(0),chan(4),n_pixels(90*90+60*60), f_event_number(0), hasTU(false)
  {
    gROOT->ProcessLine("#include <vector>");
    //Initialize for configuration file:
    //how many events will be analyzed, 0 = all events
    max_event_number = 0;

    f_plane  = new std::vector<uint16_t>;
    f_col    = new std::vector<uint16_t>;
    f_row    = new std::vector<uint16_t>;
    f_adc    = new std::vector<int16_t>;
    f_charge = new std::vector<uint32_t>;
    f_trig_phase = new std::vector<int16_t>;
    f_trig_phase->resize(2);

    //tu
    v_scaler = new vector<uint64_t>;
    v_scaler->resize(5);
    old_scaler = new vector<uint64_t>;
    old_scaler->resize(5, 0);

//    f_waveforms = new std::vector< std::vector<float> >;


  }
  // Configure : DA
  void FileWriterTreeTelescope::Configure() {
    if(!this->m_config){
      std::cout<<"Configure: abortion [!this->m_config is True]"<<endl;
      return;
    }
    m_config->SetSection("Converter.telescopetree");
    if(m_config->NSections()==0){
      std::cout<<"Configure: abortion [m_config->NSections()==0 is True]"<<endl;
      return;
    }
    EUDAQ_INFO("Configuring FileWriteTree");

    max_event_number = m_config->Get("max_event_number",0);
    std::cout << "Max events: " << max_event_number << std::endl;
    std::cout<<"End of Configure.";
  }

  void FileWriterTreeTelescope::StartRun(unsigned runnumber) {
    EUDAQ_INFO("Converting the inputfile into a Telescope TTree(felix format) " );
    std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);
    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("tree", "a simple Tree with simple variables");
    m_thdir = m_tfile->mkdir("DecodingHistos");

    // Set Branch Addresses
    m_ttree->Branch("event_number",&f_event_number, "event_number/I");
    m_ttree->Branch("time", &f_time, "time/D");

    // telescope
    m_ttree->Branch("plane", &f_plane);
    m_ttree->Branch("col", &f_col);
    m_ttree->Branch("row", &f_row);
    m_ttree->Branch("adc", &f_adc);
    m_ttree->Branch("charge", &f_charge);
    m_ttree->Branch("trigger_phase", &f_trig_phase);

    // tu
    if (hasTU){
      m_ttree->Branch("beam_current", &f_beam_current, "beam_current/s");
      m_ttree->Branch("rate", &v_scaler);
    }

  }

  string FileWriterTreeTelescope::GetStats(const DetectorEvent & dev) {
    return PluginManager::GetStats(dev);
  }

    void FileWriterTreeTelescope::StartRun2(unsigned runno){
        m_tfile2 = new TFile(TString::Format("decoding_%d.root", runno), "RECREATE");
        std::cout << "\nCreated file \"decoding_" << int(runno) << ".root\"" << std::endl;
        m_thdir2 = m_tfile2->mkdir("DecodingHistos");
        m_thdir2->cd();
        std::cout << "Created directory \"DecodingHistos\"" << std::endl;
    }

    void FileWriterTreeTelescope::WriteEvent2(const DetectorEvent & ev) {
        if (ev.IsBORE()) {
            eudaq::PluginManager::SetConfig(ev, m_config);
            eudaq::PluginManager::Initialize(ev);
            //firstEvent =true;
            cout << "loading the first event...." << endl;
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
    void FileWriterTreeTelescope::TempFunction(){
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

    std::vector<float> FileWriterTreeTelescope::TempFunctionLevel1() {
        return *Level1s;
    }
    std::vector<float> FileWriterTreeTelescope::TempFunctionDecOff() {
        return *decOffsets;
    }
    std::vector<float> FileWriterTreeTelescope::TempFunctionAlphas() {
        return *alphas;
    }

  void FileWriterTreeTelescope::WriteEvent(const DetectorEvent & ev) {
      if (ev.IsBORE()) {
          eudaq::PluginManager::SetConfig(ev, m_config);
          eudaq::PluginManager::Initialize(ev);
          //firstEvent =true;
          cout << "loading the first event..." << endl;
          return;
      } else if (ev.IsEORE()) {
          eudaq::PluginManager::ConvertToStandard(ev);
          cout << "loading the last event..." << endl;
          return;
      }
      // Condition to evaluate only certain number of events defined in configuration file  : DA
      if(max_event_number>0 && f_event_number>max_event_number) {
          return;
      }

      StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
      f_event_number = sev.GetEventNumber();
      m_tfile2 = 0;
      m_tfile3 = 0;

    /** TU STUFF */
    SetTimeStamp(sev);
    SetBeamCurrent(sev);
    SetScalers(sev);

    f_plane->clear();
    f_col->clear();
    f_row->clear();
    f_adc->clear();
    f_charge->clear();
    fill(f_trig_phase->begin(), f_trig_phase->end(), -1);
    uint8_t ind = 0;
    for (uint8_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
      if(plane.Sensor() == "DUT") {
        std::vector<double> cds = plane.GetPixels<double>();
        f_trig_phase->at(0) = int16_t(plane.GetTrigPhase());
        for (uint16_t ipix = 0; ipix < cds.size(); ++ipix) {
          f_plane->push_back(ind); // has to be ind and not iplane, so that the DUT are always first and REF after
          f_col->push_back(uint16_t(plane.GetX(ipix)));
          f_row->push_back(uint16_t(plane.GetY(ipix)));
          f_adc->push_back(int16_t(plane.GetPixel(ipix)));
          f_charge->push_back(42);                        // todo: do charge conversion here!
        }
        ind++;
      }
    }
    for (uint8_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {

      const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
      if(plane.Sensor() != "DUT") {
        std::vector<double> cds = plane.GetPixels<double>();
        f_trig_phase->at(1) = int16_t(plane.GetTrigPhase());
        for (uint16_t ipix = 0; ipix < cds.size(); ++ipix) {
          f_plane->push_back(ind); // has to be ind and not iplane, so that the DUT are always first and REF after
          f_col->push_back(uint16_t(plane.GetX(ipix)));
          f_row->push_back(uint16_t(plane.GetY(ipix)));
          f_adc->push_back(int16_t(plane.GetPixel(ipix)));
          f_charge->push_back(42);                        // todo: do charge conversion here!
        }
        ind++;
      }
    }
    m_ttree->Fill();

  }
  // Get max event number: DA
  long FileWriterTreeTelescope::GetMaxEventNumber(){
    return max_event_number;
  }


  FileWriterTreeTelescope::~FileWriterTreeTelescope() {
      if(m_ttree) {
          std::cout << "Tree has " << m_ttree->GetEntries() << " entries" << std::endl;
      }
      if(m_tfile)
          if(m_tfile->IsOpen())
              m_tfile->cd();
      if(m_ttree)
          m_ttree->Write();
      if(m_tfile2)
          if(m_tfile2->IsOpen())
              m_tfile2->Close();
      if(m_tfile3)
          if(m_tfile3->IsOpen())
              m_tfile3->Close();
  }

  uint64_t FileWriterTreeTelescope::FileBytes() const { return 0; }

}void FileWriterTreeTelescope::SetTimeStamp(StandardEvent sev) {
  if (sev.hasTUEvent()){
    if (sev.GetTUEvent(0).GetValid()) { f_time = sev.GetTimestamp(); }
  }
  else
    f_time = sev.GetTimestamp() / 384066.;
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
         *  first scaler value is the scintillator and then the planes */
        for (uint8_t i(0); i < 5; i++) {
            v_scaler->at(i) = uint64_t(
                    valid ? (tuev.GetScalerValue(i) - old_scaler->at(i)) * 1000 / (f_time - old_time) : UINT32_MAX);
            if (valid)
                old_scaler->at(i) = tuev.GetScalerValue(i);
        }
        if (valid)
            old_time = f_time;
    }
}

 int FileWriterTreeTelescope::FindFirstBinAbove(TH1F* blah, float value, int binl, int binh){
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

 int FileWriterTreeTelescope::FindLastBinAbove(TH1F* blah, float value, int binl, int binh){
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

float FileWriterTreeTelescope::GetTimeCorrectedValue(float value, float prevValue, float alpha){
    return (value - prevValue * alpha) / (1 - alpha);
}

 TString* FileWriterTreeTelescope::GetFitString(int NL, int NSp, float sigmaSp) {
    TString *temps = new TString("0");
    for(int m = 0; m < NL; m++){
        for(int n = 0; n < NSp; n++){
            temps->Append(TString::Format("+([%d]*TMath::Exp(-TMath::Power((x-[0]-%d*[1]-%d*[2]-%f*[%d])/(TMath::Sqrt(2)*[3]),2)))", 4 + NSp * m + n, n, m, sigmaSp, 4 + NSp * (NL + m) + n));
//            temps->Append(TString::Format("+([%d]*TMath::Exp(-TMath::Power((x-[0]-%d*[1]-%d*[2]-%f*[%d])/(TMath::Sqrt(2)*[3]),2)))", 4 + NSp * m + n, n, m, sigmaSp, 4 + NSp * NL + m));
        }
    }
    return temps;
}

TF1* FileWriterTreeTelescope::SetFit(TH1F* blah, TH1F* hblack, int NL, int NSp, int NTotal, int rocn){
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
#endif // ROOT_FOUND
