#include "eudaq/Monitor.hh"
#include "eudaq/Logger.hh"
#include "eudaq/PluginManager.hh"

// #define EUDAQ_MAX_EVENTS_PER_IDLE 1000
#define EUDAQ_MAX_EVENTS_PER_IDLE  10000

#include "TStopwatch.h"

namespace eudaq {

  Monitor::Monitor(const std::string & name, const std::string & runcontrol, const unsigned lim,
      const unsigned skip_, const unsigned int skip_evts, const std::string & datafile, const std::string config) :
    CommandReceiver("Monitor", name, runcontrol, false),
    m_run(0),
    m_callstart(false),
    m_reader(0),
    limit(lim),
    skip(100-skip_),
    skip_events_with_counter(skip_evts),
    start_event(0)
  {
    if (datafile != "") {
      // set offline
      m_reader = std::shared_ptr<FileReader>(new FileReader(datafile));
      PluginManager::Initialize(m_reader->GetDetectorEvent(), config); // process BORE
      //m_callstart = true;
      std::cout << "DEBUG: Reading file " << datafile << " -> " << m_reader->Filename() << std::endl;
      //OnStartRun(m_run);
    }
    StartThread();
  }

  bool Monitor::ProcessEvent() {

    if (!m_reader.get()) return false;
    if (!m_reader->NextEvent()) return false;

    unsigned evt_number = m_reader->GetDetectorEvent().GetEventNumber();
    //  std::cout<< "at event number; " << evt_number << std::endl;
    if(limit > 0 && evt_number > limit)
      return true;


    if (evt_number % 1000 == 0) {
      std::cout << "\rProcessEvent " << m_reader->GetDetectorEvent().GetEventNumber()
        << (m_reader->GetDetectorEvent().IsBORE() ? "B" : m_reader->GetDetectorEvent().IsEORE() ? "E" : "")
        << std::flush;
    }

    if(skip > 0 && (evt_number % 100 >= skip))  //-s functionality
      return true;

    else if (skip_events_with_counter > 0)      //-sc functionality, you cant have both
    {
      if(++counter_for_skipping < skip_events_with_counter && evt_number > 0)
        return true;
      else
        counter_for_skipping = 0;
    }
    if (evt_number < start_event){
        return true;
    }

    try {
      const DetectorEvent & dev = m_reader->GetDetectorEvent();
      if (dev.IsBORE()) m_lastbore =std::shared_ptr<DetectorEvent>(new DetectorEvent(dev));
      OnEvent(PluginManager::ConvertToStandard(dev));
    } catch (const InterruptedException &) {
      return false;
    }
    return true;
  }

  void Monitor::OnIdle() {
    //std::cout << "..." << std::endl;
    if (m_callstart) {
      m_callstart = false;
      OnStartRun(m_run);
    }
    bool processed = false;
    for (int i = 0; i < EUDAQ_MAX_EVENTS_PER_IDLE; ++i) {
      if (ProcessEvent()) {
        processed = true;
      } else {
        //if (offline) OnTerminate();
        break;
      }
    }
    if (!processed) mSleep(1);
  }

  void Monitor::OnStartRun(unsigned param) {
    std::cout << "run " << param << std::endl;
    m_run = param;
    m_reader = std::shared_ptr<FileReader>(new FileReader(to_string(m_run)));
    PluginManager::Initialize(m_reader->GetDetectorEvent()); // process BORE
    EUDAQ_INFO("Starting run " + to_string(m_run));
  }

  void Monitor::OnStopRun() {
    m_reader->Interrupt();
  }

}
