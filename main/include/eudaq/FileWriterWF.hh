//
// Created by reichmann on 2021/07/12.
//
#ifndef EUDAQ_FILEWRITERWF_HH
#define EUDAQ_FILEWRITERWF_HH

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

  class FileWriterWF : public FileWriter {

  public:
    explicit FileWriterWF(const std::string &, std::string="", int=4);
    ~FileWriterWF() override;

    void StartRun(unsigned) override;
    void Configure() override;
    void WriteEvent(const DetectorEvent &) override;
    uint64_t FileBytes() const override;

    long GetMaxEventNumber() override { return max_event_number_; }
    std::string GetStats(const DetectorEvent &dev) override { return PluginManager::GetStats(dev); }
    void SetTU(bool status) override { has_tu_ = status; }
    void SetTelescopeBranches();
    void FillTelescopeData(const StandardEvent&);
    void InitTelescopeArrays();

  protected:
    std::string name_{};
    std::string producer_name_;
    uint32_t run_number_;
    uint64_t max_event_number_;
    uint16_t save_waveforms_{};
    uint16_t active_regions_{};
    bool has_tu_;
    int verbose_;

    /** ROOT File and Tree */
    TFile * tfile_;  // book the pointer to a file (to store the output)
    TTree * ttree_;  // book the tree (to store the needed event info)
    TMacro * macro_;  // macro to add text information to the file

    /** Waveform info */
    float sampling_speed_;
    float bunch_width_;
    std::vector<float> * data_{};
    std::vector<std::string> sensor_names_{};
    uint16_t n_channels_;
    uint16_t n_active_channels_;
    std::vector<signed char> polarities_{};
    std::vector<signed char> pulser_polarities_{};
    std::vector<signed char> spectrum_polarities{};
    float rise_time_;
    std::map<uint8_t, std::vector<float> > tcal_;  // timing calibration of the DRS4 chip
    std::map<uint8_t, std::vector<float> > full_time_;
    uint16_t n_samples_;

    void ClearVectors();
    void ResizeVectors(size_t n_channels);
    virtual bool IsPulserEvent(const StandardWaveform * wf) const;
    void FillRegionIntegrals(const StandardEvent& sev);
    void FillRegionVectors();
    void FillTotalRange(uint8_t iwf, const StandardWaveform *wf);
    void UpdateWaveforms(uint8_t iwf);
    virtual void AddInfo(uint8_t iwf, const StandardWaveform*){};
    void FillSpectrumData(uint8_t iwf);
    void DoSpectrumFitting(uint8_t iwf);
    void InitFFT();
    void DoFFTAnalysis(uint8_t iwf);
    static bool UseWaveForm(uint16_t bitmask, uint8_t iwf) { return ((bitmask & 1 << iwf) == 1 << iwf); }
    std::string GetBitMask(uint16_t bitmask) const;
    static std::string GetPolarityStr(const std::vector<signed char>& pol);
    void SetTimeStamp(StandardEvent);
    void SetBeamCurrent(StandardEvent);
    void SetScalers(StandardEvent);
    void ReadIntegralRanges();
    void ReadIntegralRegions();
    float GetNoiseThreshold(uint8_t, float=2);

    // clocks for checking execution time
    TStopwatch w_spectrum;
    TStopwatch w_fft;
    TStopwatch w_total;

    void FillFullTime();
    inline float GetTriggerTime(const uint8_t &ch, const uint16_t &bin);
    float GetTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up);

    /** SCALAR BRANCHES */
    int f_nwfs;
    uint32_t f_event_number;
    int f_pulser_events;
    int f_signal_events;
    double f_time;
    double old_time;
    uint16_t f_beam_current;

    //drs4
    uint16_t f_trigger_cell;

    bool f_pulser;
    std::vector<uint16_t> *v_forc_pos;
    std::vector<float> *v_forc_time;

    // spectrum parameters
    float spec_sigma;
    int spec_decon_iter;
    int spec_aver_win;
    bool spec_markov;
    bool spec_rm_bg;

    uint16_t spectrum_waveforms;
    uint16_t fft_waveforms;
    std::pair<uint16_t, uint16_t> pulser_region;
    int pulser_threshold;
    uint8_t pulser_channel;
    uint8_t trigger_channel;

    /** VECTOR BRANCHES */
    // integrals
    std::map<uint8_t, std::map<std::string, std::pair<float, float> *> > ranges;
    std::map<uint8_t, WaveformSignalRegions *> * regions;
    std::vector<float> *IntegralValues;
    std::vector<float> *TimeIntegralValues;
    std::vector<Int_t> *IntegralPeaks;
    std::vector<float> *IntegralPeakTime;
    std::vector<float> *IntegralLength;
    float * v_cft;  // constant fraction time
    bool * f_bucket;
    bool * f_ped_bucket;
    float * f_b2_int;

    // general waveform information
    std::vector<bool> *v_is_saturated;
    std::vector<float> *v_median;
    std::vector<float> *v_average;
    std::vector<uint16_t> * v_max_peak_position;
    std::vector<float> * v_max_peak_time;
    std::vector<std::vector<uint16_t> > * v_peak_positions;
    std::vector<std::vector<float> > * v_peak_times;
    std::vector<uint8_t> * v_npeaks;
    std::vector<float> * v_rise_time;
    std::vector<float> * v_fall_time;
    std::vector<float> * v_wf_start;
    std::vector<float> * v_fit_peak_time;
    std::vector<float> * v_fit_peak_value;
    std::vector<float> * v_peaking_time;

    // waveforms
    std::map<uint8_t, float*> f_wf;

    // telescope
    uint8_t f_n_hits;
    uint8_t * f_plane;
    uint8_t * f_col;
    uint8_t * f_row;
    int16_t * f_adc;
    float * f_charge;
    uint8_t f_trig_phase;

    // peak finding and FFT
    TSpectrum * spec_;
    TVirtualFFT * tfft_;
    Double_t * re_full_;
    Double_t * im_full_;
    Double_t * in_;

    // spectrum
    unsigned peak_noise_pos;
    std::vector<std::pair<float, float> > noise_;
    std::vector<std::deque<float> > noise_vectors;
    std::vector<std::pair<float, float> > int_noise_;
    std::vector<std::deque<float> > int_noise_vectors;
    void calc_noise(uint8_t);
    void FillBucket(const StandardEvent& sev);
    std::vector<double> data_pos;
    std::vector<double> decon;
    std::vector<std::vector<uint16_t> *> peaks_x;
    std::vector<std::vector<float> *> peaks_x_time;
    std::vector<std::vector<float> *> peaks_y;
    // fft
    std::vector<std::vector<float> *> fft_values;
    std::vector<float> *fft_mean;
    std::vector<float> *fft_mean_freq;
    std::vector<float> *fft_max;
    std::vector<float> *fft_max_freq;
    std::vector<float> *fft_min;
    std::vector<float> *fft_min_freq;
    std::vector<std::vector<float> *> fft_modes;

    //tu
    std::vector<uint64_t> * v_scaler;
    std::vector<uint64_t> * old_scaler;

    //peak finding
    std::pair<float, float> peak_finding_roi;
    int n_peaks_total;
    int n_peaks_after_roi;
    int n_peaks_before_roi;
  };
}

#endif //EUDAQ_FILE_WRITER_WF_HH
