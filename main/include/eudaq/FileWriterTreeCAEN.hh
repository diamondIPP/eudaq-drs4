//
// Created by reichmann on 07.04.16.
//
#ifndef EUDAQ_FILEWRITETREECAEN_HH
#define EUDAQ_FILEWRITETREECAEN_HH

#include "eudaq/FileWriterWF.hh"

namespace eudaq {

  class FileWriterTreeCAEN : public FileWriterWF {
  public:
    explicit FileWriterTreeCAEN(const std::string &);
    void Configure() override;
    void StartRun(unsigned) override;
    void AddInfo(uint8_t, const StandardWaveform*) override;
    static float GetRFPhase(float, float);
    bool IsPulserEvent(const StandardWaveform *) const override;
    float LoadSamplingFrequency(const DetectorEvent&) override;

  private:
    std::vector<uint16_t> * dia_channels_;

    // digitiser
    float f_trigger_time;

    // rf data
    float f_rf_phase;
    float f_rf_period;
    float f_rf_chi2;
    bool fit_rf;

    // channels
    uint8_t rf_channel_{};
    uint8_t scint_channel_{};
  };
}

#endif //EUDAQ_FILEWRITETRECAEN_HH
