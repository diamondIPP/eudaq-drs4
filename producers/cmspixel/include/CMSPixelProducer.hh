#ifndef CMSPIXELPRODUCER_HH
#define CMSPIXELPRODUCER_HH

// EUDAQ includes:
#include "eudaq/Producer.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Configuration.hh"

// pxarCore includes:
#include "api.h"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>
#include <mutex>

// CMSPixelProducer
class masking;
class CMSPixelProducer : public eudaq::Producer {

public:
  CMSPixelProducer(const std::string & name, const std::string & runcontrol, const std::string & verbosity);
  void OnConfigure(const eudaq::Configuration & config) override;
  void OnStartRun(unsigned runnumber) override;
  void OnStopRun() override;
  void OnTerminate() override;
  void ReadoutLoop();

private:
  // Helper function to read DACs from file which is provided via eudaq config:
  std::vector<std::pair<std::string,uint8_t> > GetConfDACs(int16_t i2c = -1, bool tbm = false);
  static std::vector<int32_t> &split(const std::string &s, char delim, std::vector<int32_t> &elems);
  static std::vector<int32_t> split(const std::string &s, char delim);

  std::vector<pxar::pixelConfig> GetConfMaskBits();
  std::vector<pxar::pixelConfig> GetConfTrimming(std::vector<pxar::pixelConfig> maskbits, int16_t i2c = -1);
  std::vector<int8_t> GetI2Cs();
  std::vector<std::pair<std::string, uint8_t> > GetTestBoardDelays();

  std::string prepareFilename(const std::string & name, const std::string & n);
  std::vector<masking> GetConfMask();
  std::string readHash(std::string hexMask, char i2c);
  void ReadPxarConfig();

  std::map<std::string, std::string> m_pxar_config;
  unsigned m_run, m_ev, m_ev_filled, m_ev_runningavg_filled;
  unsigned m_tlu_waiting_time;
  unsigned m_roc_resetperiod;
  unsigned m_nplanes;
  std::string m_verbosity, m_foutName, m_roctype, m_tbmtype, m_pcbtype, m_usbId, m_producerName, m_detector, m_event_type, m_alldacs;
  bool m_terminated, m_running, triggering;
  bool m_trimmingFromConf, m_trigger_is_pg;
  bool m_maskingFromConf;
  std::string m_last_mask_filename;
  eudaq::Configuration m_config;
  std::map<int8_t, int8_t> m_i2c_map;

  // Add one mutex to protect calls to pxarCore:
  std::mutex m_mutex;
  pxar::pxarCore *m_api;

  int m_pattern_delay;
  std::ofstream m_fout;
  eudaq::Timer *m_reset_timer;
};

class masking {

private:
    std::string _identifier;
    uint8_t _rocID, _col, _row;
public:
    masking(std::string identifier, uint8_t rocID, uint8_t col, uint8_t row) :
    _identifier(identifier), _rocID(rocID), _col(col), _row(row) {}
    std::string id() {return _identifier;}
    uint8_t col() {return _col;}
    uint8_t row() {return _row;}
    uint8_t roc() {return _rocID;}
};
#endif /*CMSPIXELPRODUCER_HH*/
