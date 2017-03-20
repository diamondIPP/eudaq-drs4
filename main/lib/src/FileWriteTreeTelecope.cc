#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"

using namespace std;

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
  private:
    TFile * m_tfile; // book the pointer to a file (to store the otuput)
    TTree * m_ttree; // book the tree (to store the needed event info)
    // Book variables for the Event_to_TTree conversion
    unsigned m_noe;
    short chan;
    int n_pixels;
    // for Configuration file
    long max_event_number;

    // Scalar Branches
    int f_event_number;
    double f_time;

    // Vector Branches
    std::vector<uint16_t> * f_plane;
    std::vector<uint16_t> * f_col;
    std::vector<uint16_t> * f_row;
    std::vector<int16_t> * f_adc;
    std::vector<uint32_t> * f_charge;
    std::vector<int16_t> * f_trig_phase;
//    std::vector< std::vector<float>> * f_waveforms;

  };

  namespace {
    static RegisterFileWriter<FileWriterTreeTelescope> reg("telescopetree");
  }

  FileWriterTreeTelescope::FileWriterTreeTelescope(const std::string & /*param*/)
    : m_tfile(0), m_ttree(0),m_noe(0),chan(4),n_pixels(90*90+60*60)
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

  }

  string FileWriterTreeTelescope::GetStats(const DetectorEvent & dev) {
    return PluginManager::GetStats(dev);
  }

  void FileWriterTreeTelescope::WriteEvent(const DetectorEvent & ev) {

    if (ev.IsBORE()) {
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
    f_event_number = sev.GetEventNumber();

    // set time stamp
    if (sev.hasTUEvent()){
      if (sev.GetTUEvent(0).GetValid())
        f_time = sev.GetTimestamp();
    }
    else
      f_time = sev.GetTimestamp() / 384066.;

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
    m_ttree->Write();
    if(m_tfile->IsOpen()) m_tfile->Close();
  }

  uint64_t FileWriterTreeTelescope::FileBytes() const { return 0; }

}
#endif // ROOT_FOUND
