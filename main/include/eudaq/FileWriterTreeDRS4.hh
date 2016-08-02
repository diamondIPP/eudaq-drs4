//
// Created by reichmann on 07.04.16.
//
#ifndef EUDAQ_FILEWRITETREEDRS4_HH
#define EUDAQ_FILEWRITETREEDRS4_HH

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
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TSystem.h"
#include "TInterpreter.h"
#include "TMacro.h"
#include "TF1.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TSpectrum.h"
#include "TPolyMarker.h"

namespace eudaq {

    class FileWriterTreeDRS4 : public FileWriter {
    public:
        FileWriterTreeDRS4(const std::string &);
        virtual void StartRun(unsigned);
        virtual void Configure();
        virtual void WriteEvent(const DetectorEvent &);
        virtual uint64_t FileBytes() const;
        float Calculate(std::vector<float> *data, int min, int max, bool _abs = false);

//        float CalculatePeak(std::vector<float> * data, int min, int max);
//        std::pair<int, float> FindMaxAndValue(std::vector<float> * data, int min, int max);

        float avgWF(float, float, int);
        virtual ~FileWriterTreeDRS4();
        virtual long GetMaxEventNumber() { return max_event_number; }

    private:
        unsigned runnumber;
        TH1F *histo;
        long max_event_number;
        uint16_t save_waveforms;
        uint16_t active_regions;
        void ClearVectors();
        void ResizeVectors(size_t n_channels);
        int IsPulserEvent(const StandardWaveform *wf);
        void ExtractForcTiming(std::vector<float> *);
        void FillRegionIntegrals(uint8_t iwf, const StandardWaveform *wf);
        void FillRegionVectors();
        void FillTotalRange(uint8_t iwf, const StandardWaveform *wf);
        void UpdateWaveforms(uint8_t iwf);
        void FillSpectrumData(uint8_t iwf);
        void DoSpectrumFitting(uint8_t iwf);
        void DoFFTAnalysis(uint8_t iwf);
        bool UseWaveForm(uint16_t bitmask, uint8_t iwf) { return ((bitmask & 1 << iwf) == 1 << iwf); }
        std::string GetBitMask(uint16_t bitmask);
        std::string GetPolarities(std::vector<signed char> pol);

        // clocks for checking execution time
        TStopwatch w_spectrum;
        TStopwatch w_fft;
        TStopwatch w_total;
        TFile *m_tfile; // book the pointer to a file (to store the output)
        TTree *m_ttree; // book the tree (to store the needed event info)
        int verbose;
        std::vector<float> * data;
        std::vector<std::string> sensor_name;
        // Book variables for the Event_to_TTree conversion
        unsigned m_noe;
        short chan;
        int n_pixels;
        std::map<std::string, std::pair<float, float> *> ranges;
        std::vector<signed char> polarities;
        std::vector<signed char> pulser_polarities;

        std::vector<int16_t> *v_polarities;
        std::vector<int16_t> *v_pulser_polarities;

        // drs4 timing calibration
        std::map<uint8_t, std::vector<float> > tcal;
        std::map<uint8_t, std::vector<float> > full_time;

        void FillFullTime();
        inline float getTriggerTime(const uint8_t &ch, const uint16_t &bin);
        float getTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up);

        /** SCALAR BRANCHES */
        int f_nwfs;
        int f_event_number;
        int f_pulser_events;
        int f_signal_events;
        double f_time;

        //drs4
        uint16_t f_trigger_cell;

        int f_pulser;
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
        int pulser_threshold;
        uint8_t pulser_channel;
        uint8_t trigger_channel;

        /** VECTOR BRANCHES */
        // integrals
        std::map<int, WaveformSignalRegions *> *regions;
        std::vector<std::string> *IntegralNames;
        std::vector<float> *IntegralValues;
        std::vector<float> *TimeIntegralValues;
        std::vector<Int_t> *IntegralPeaks;
        std::vector<float> *IntegralPeakTime;
        std::vector<float> *IntegralLength;

        // general waveform information
        std::vector<bool> *v_is_saturated;
        std::vector<float> *v_median;
        std::vector<float> *v_average;
        std::vector<std::vector<uint16_t> *> v_peak_positions;
        std::vector<std::vector<float> *> v_peak_timings;

        // waveforms
        std::map<uint8_t, std::vector<float> *> f_wf;

        // telescope
        std::vector<uint16_t> *f_plane;
        std::vector<uint16_t> *f_col;
        std::vector<uint16_t> *f_row;
        std::vector<int16_t> *f_adc;
        std::vector<uint32_t> *f_charge;

        // average waveforms of channels
        TH1F *avgWF_0;
        TH1F *avgWF_0_pul;
        TH1F *avgWF_0_sig;
        TH1F *avgWF_1;
        TH1F *avgWF_2;
        TH1F *avgWF_3;
        TH1F *avgWF_3_pul;
        TH1F *avgWF_3_sig;
        TSpectrum *spec;
        TVirtualFFT *fft_own;
        Int_t n_samples;
        Double_t *re_full;
        Double_t *im_full;
        Double_t *in;
        TMacro *macro;

        // spectrum
        std::vector<float> data_pos;
        std::vector<float> decon;
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
        TCanvas *c1;
        // wf check
        std::vector<bool> * f_isDa;
        std::vector<uint16_t> wf_thr;

    };
}

#endif //EUDAQ_FILEWRITETREEDRS4_HH
