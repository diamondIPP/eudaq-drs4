//
// Created by reichmann on 07.04.16.
//
#ifndef EUDAQ_FILEWRITETREEWF_HH
#define EUDAQ_FILEWRITETREEWF_HH

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
#include "TVirtualFFT.h"
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

    class FileWriterTreeWaveForm : public FileWriter {
    public:
        FileWriterTreeWaveForm(const std::string &);
        virtual void StartRun(unsigned);
        virtual void Configure();
        virtual void WriteEvent(const DetectorEvent &);
        virtual uint64_t FileBytes() const;
        float Calculate(std::vector<float> *data, int min, int max, bool _abs = false);

//        float CalculatePeak(std::vector<float> * data, int min, int max);
//        std::pair<int, float> FindMaxAndValue(std::vector<float> * data, int min, int max);

        float avgWF(float, float, int);
        virtual ~FileWriterTreeWaveForm();
        virtual uint64_t GetMaxEventNumber() { return max_event_number; }

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
        void FillPeaks(uint8_t iwf, const StandardWaveform *wf);
        void UpdateWaveforms(uint8_t iwf);
        void FillSpectrumData(uint8_t iwf);
        void DoSpectrumFitting(uint8_t iwf);
        void DoFFTAnalysis(uint8_t iwf);
        bool UseWaveForm(uint16_t bitmask, uint8_t iwf) { return ((bitmask & 1 << iwf) == 1 << iwf); }
        std::string GetBitMask(uint16_t bitmask);
        std::string GetPolarities(std::vector<signed char> pol);
        void SetTimeStamp(StandardEvent);

        TStopwatch w_total;
        TFile *m_tfile; // book the pointer to a file (to store the output)
        TTree *m_ttree; // book the tree (to store the needed event info)
        int verbose;
        std::vector<float> * data;
        std::vector<std::string> sensor_name;
        // Book variables for the Event_to_TTree conversion
        unsigned m_noe;
        short chan;
        std::map<std::string, std::pair<float, float> *> ranges;


        std::vector<int16_t> *v_polarities;

        // drs4 timing calibration
        std::map<uint8_t, std::vector<float> > tcal;
        std::map<uint8_t, std::vector<float> > full_time;

        void FillFullTime();
        inline float getTriggerTime(const uint8_t &ch, const uint16_t &bin);
        float getTimeDifference(uint8_t ch, uint16_t bin_low, uint16_t bin_up);

        /** SCALAR BRANCHES */
        int f_nwfs;
        int f_event_number;
        double f_time;
        uint16_t f_trigger_cell;

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
        std::vector<uint16_t> * v_peak_positions;
        std::vector<float> * v_peak_timings;
        std::vector<float> * v_peak_values;
        std::vector<float> * v_pedestals;
        std::vector<float> * v_peak_integrals;

        // waveforms
        std::map<uint8_t, std::vector<float> *> f_wf;

        // average waveforms of channels
        TMacro *macro;

    };
}

#endif //EUDAQ_FILEWRITETREEWF_HH
