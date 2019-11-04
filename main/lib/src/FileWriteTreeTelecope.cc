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
#include "TH1F.h"
#include "TF1.h"
#include "TFitResultPtr.h"

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
      virtual std::vector<float> TempFunctionL1Off();
      virtual std::vector<float> TempFunctionDecOff();

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
      std::vector<float> *offsetsL1;
      std::vector<float> *decOffsets;


      //tu
    std::vector<uint64_t> * v_scaler;
    std::vector<uint64_t> * old_scaler;
    void SetTimeStamp(StandardEvent /*sev*/);
    void SetBeamCurrent(StandardEvent /*sev*/);
    void SetScalers(StandardEvent /*sev*/);

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
            cout << "loading the last event...." << endl;
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
        std::cout << "FileName is: "<< bla << std::endl;
        if(m_tfile2->IsOpen()) m_tfile2->Close();
        m_tfile3 = new TFile(bla, "READ");
        m_thdir3 = (TDirectory*)m_tfile3->Get("DecodingHistos");
        offsetsL1 = new std::vector<float>;
        decOffsets = new std::vector<float>;
        for(size_t roci = 0; roci < 16; roci++){
            std::vector<float> *valleys = new std::vector<float>;
            valleys->resize(0);
            if(m_thdir3->Get(TString::Format("decode%d", int(roci)))){
                TH1F *blah_decode = (TH1F*)m_thdir3->Get(TString::Format("decode%d", int(roci)));
                TH1F *blah_black = (TH1F*)m_thdir3->Get(TString::Format("black_%d", int(roci)));
                TH1F *blah_ub = (TH1F*)m_thdir3->Get(TString::Format("uBlack_%d", int(roci)));
                TH1F *blah_decode2 = (TH1F*)blah_decode->Clone(TString::Format("decode%dV2", int(roci)));
                blah_decode2->SetTitle(TString::Format("decode%dV2", int(roci)));
                blah_decode2->Add(blah_ub, -1);
                blah_decode2->Add(blah_black, -1);
                auto tempUB = float(blah_ub->GetMean());
                auto tempB = float(blah_black->GetMean());
                auto tempL1 = (0 - tempUB) / 4.;
                float offL1 = 0;
                auto p1 = -tempL1 / 2. + offL1;
                auto *blaf1 = new TF1("blaf1", "[0]+TMath::Power((x-[1])/[2],2)", -1000, 2000);
                blaf1->SetParameter(0, 1000);
                blaf1->SetParameter(2, 1000);
                blaf1->SetParLimits(0, -1000, blah_decode2->GetEntries());
                blaf1->SetParLimits(2, 0, 1000);
                blaf1->SetParameter(1, p1);
                blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                p1 = blaf1->GetParameter(1);
                blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                valleys->push_back(float(blaf1->GetParameter(1)));

                p1 = tempL1 / 2. + offL1;
                blaf1->SetParameter(1, p1);
                blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                p1 = blaf1->GetParameter(1);
                blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                valleys->push_back(float(blaf1->GetParameter(1)));

                for(unsigned int it = 1; it < 3; it++){
                    tempL1 = (valleys->at(it) - valleys->at(0)) / it;
                    offL1 = float(valleys->at(0) + tempL1 / 2.);
                    p1 = tempL1 * (it + 0.5) + offL1;
                    blaf1->SetParameter(1, p1);
                    blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                    blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                    blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                    p1 = blaf1->GetParameter(1);
                    blaf1->SetParLimits(1, p1 - tempL1 / 3., p1 + tempL1 / 3.);
                    blah_decode2->GetXaxis()->SetRangeUser(p1 - tempL1/3., p1 + tempL1/3.);
                    blah_decode2->Fit("blaf1", "IMQ", "", p1 - tempL1/3., p1 + tempL1/3.);
                    valleys->push_back(float(blaf1->GetParameter(1)));
                }
                std::cout << "valleys roc " << int(roci) << ": ";
                for(size_t it = 0; it < valleys->size(); it++)
                    std::cout << float(valleys->at(it)) << ", ";
                std::cout << std::endl;

                offsetsL1->push_back(offL1);
                decOffsets->push_back((0 - (tempB - offL1)));
            } else {
//                std::cout << "it DOES NOT have decode" << int(roci) << std::endl;
            }
        }
        m_tfile3->Close();
    }
    std::vector<float> FileWriterTreeTelescope::TempFunctionL1Off() {
        return *offsetsL1;
    }
    std::vector<float> FileWriterTreeTelescope::TempFunctionDecOff() {
        return *decOffsets;
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

      std::cout<<"write event"<< std::endl;
      StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
      std::cout<<"write event..."<< std::endl;
      f_event_number = sev.GetEventNumber();

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
    std::cout<<"Tree has " << m_ttree->GetEntries() << " entries" << std::endl;
    m_tfile->cd();
    m_ttree->Write();
    if(m_tfile->IsOpen()) m_tfile->Close();
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
      v_scaler->at(i) = uint64_t(valid ? (tuev.GetScalerValue(i) - old_scaler->at(i)) * 1000 / (f_time - old_time) : UINT32_MAX);
      if (valid)
        old_scaler->at(i) = tuev.GetScalerValue(i);
    }
    if (valid)
      old_time = f_time;
  }
}
#endif // ROOT_FOUND
