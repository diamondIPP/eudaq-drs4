//
// Created by reichmann on 2021/07/12.
//
#ifndef EUDAQ_FILEWRITER_TEL_HH
#define EUDAQ_FILEWRITER_TEL_HH

// eudaq imports
#include "FileNamer.hh"
#include "FileWriter.hh"
#include "PluginManager.hh"
#include "Logger.hh"
#include "FileSerializer.hh"
#include "WaveformSignalRegion.hh"
#include "WaveformSignalRegions.hh"
#include "include/SimpleStandardEvent.hh"

// ROOT imports
#include "TStopwatch.h"
class TTree;
class TH1F;
class TSpectrum;
class TVirtualFFT;
class TMacro;

namespace eudaq {

  class FileWriterTreeTelescope : public FileWriter {

  public:
    explicit FileWriterTreeTelescope(const std::string & /*params*/);
    ~FileWriterTreeTelescope() override;

    void StartRun(unsigned) override;
    void Configure() override;
    void WriteEvent(const DetectorEvent &) override;
    uint64_t FileBytes() const override { return 0; }

    uint64_t GetMaxEventNumber() override { return max_event_number_; }
    std::string GetStats(const DetectorEvent &dev) override { return PluginManager::GetStats(dev); }
    void SetTU(bool status) override { has_tu_ = status; }

    virtual void InitBORE(const DetectorEvent&);
    virtual void AddWrite(StandardEvent&) { };
    
  protected:
    std::string name_;
    uint32_t run_number_;
    uint64_t max_event_number_;
    bool has_tu_;
    int verbose_;
    uint8_t at_plane_;

    /** ROOT File and Tree */
    TFile * tfile_;  // book the pointer to a file (to store the output)
    TTree * ttree_;  // book the tree (to store the needed event info)

    void SetTimeStamp(StandardEvent);
    void SetBeamCurrent(StandardEvent);
    void SetScalers(StandardEvent);

    void FillTelescopeArrays(const StandardEvent&, bool=false);

    /** SCALAR BRANCHES */
    uint32_t f_event_number;
    double f_time;
    double old_time;

    /** VECTOR BRANCHES */
    uint8_t f_n_hits;
    uint8_t * f_plane;
    uint8_t * f_col;
    uint8_t * f_row;
    int16_t * f_adc;
    float * f_charge;
    uint8_t * f_trig_phase;

    //tu
    uint16_t f_beam_current;
    uint64_t * v_scaler;
    uint64_t * old_scaler;
  };
}

#endif //EUDAQ_FILEWRITER_TEL_HH
