/* ---------------------------------------------------------------------------------
** Event Alignment Histograms for Alignment between WaveForms and Telescope Planes
**
**
** <EventAlignmentHistos>.hh
**
** Date: May 2016
** Author: Michael Reichmann (remichae@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#ifndef EUDAQ_EVENTALIGNMENTHISTOS_HH
#define EUDAQ_EVENTALIGNMENTHISTOS_HH

class TH1;
class TH2I;
class TH2F;
class TProfile;
class SimpleStandardEvent;
class RootMonitor;
class TString;
class TGraph;
class TText;

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <algorithm>
#include "Rtypes.h"

class EventAlignmentHistos {

protected:
    std::vector<std::map<int, TProfile *> > _PadAlignment;
    TH1 *_PulserRate;
    TText * _PulserLegend;
    std::vector<TH2I *>_IsAligned;
    std::vector<TH1 *> _PixelCorrelations;
    std::vector<std::deque<uint8_t> > _lastNClusters;
    TH2F * _PixelIsAligned;
    std::vector<std::vector<size_t> > eventNumbers;
    std::vector<std::vector<uint8_t> > * rowAna1;
    std::vector<std::vector<uint8_t > > * rowDig;
    TGraph * _Corr;

public:
    EventAlignmentHistos();

    virtual ~EventAlignmentHistos();

    void Fill(const SimpleStandardEvent &);

    void Write();

    void Reset();

    TProfile * getAlignmentHisto(uint8_t idev) { return (TProfile *) _PadAlignment.at(idev).at(0); }
    TProfile * getPulserRate() { return (TProfile *) _PulserRate; }
    TProfile * getPixelCorrelation(uint8_t icor) { return (TProfile *)_PixelCorrelations.at(icor); }
    TH2I *getIsAlignedHisto(uint8_t idev) { return _IsAligned.at(idev); }
    TH2F *getPixelIsAlignedHisto(const SimpleStandardEvent &);
    uint8_t getNDigPlanes() { return _n_dig_planes; }
    uint8_t getNAnaPlanes() { return _n_analogue_planes; }
    bool hasWaveForm;
    void init(const SimpleStandardEvent &);

private:
    const uint8_t _nOffsets;
    const uint16_t _bin_size;
    const uint32_t max_event_number;
    uint8_t _n_analogue_planes;
    uint8_t _n_dig_planes;
    uint8_t _n_devices;
    std::map< std::string, uint8_t> _device_names;

    bool foundPulser;
    uint8_t count;

    TProfile *init_profile(const std::string&, const std::string&, uint16_t bin_size = 0, const std::string& ytit = "Fraction of Hits @ Pulser Events [%]", Color_t fill_color = 0);
    TH2I *init_th2i(const std::string&, const std::string&);
    TH2F * init_pix_align(uint16_t);

    void FillIsAligned(uint8_t);

    void FillCorrelationVectors(const SimpleStandardEvent &);
    void BuildCorrelation();

    void ResizeObjects(uint32_t);
    void InitVectors();
    void InitPadAlignment();

    const Color_t fillColor;

    template <typename T>
    T sum(std::vector<T> a) {
      T s = 0;
      for (int i = 0; i < a.size(); i++) {
        s += a[i];
      }
      return s;
    }

    template <typename T>
    double mean(std::vector<T> a) {
      return sum(a) / double(a.size());
    }

    template <typename T>
    double sqsum(std::vector<T> a) {
      double s = 0;
      for (int i = 0; i < a.size(); i++) {
        s += pow(a[i], 2);
      }
      return s;
    }

    template <typename T>
    double stdev(std::vector<T> nums) {
      double N = nums.size();
      return pow(sqsum(nums) / N - pow(sum(nums) / N, 2), 0.5);
    }

    template <typename T>
    double pearsoncoeff(std::vector<T> X, std::vector<T> Y);

    template <typename T>
    std::vector<T> minus(std::vector<T> a, double b) {
      std::vector<T> retvect;
      for (int i = 0; i < a.size(); i++)
        retvect.push_back(a[i] - b);
      return retvect;
    }

    template <typename T>
    std::vector<T> multiply(std::vector<T> a, std::vector<T> b) {
      std::vector<T> retvect;
      for (int i = 0; i < a.size(); i++) {
        retvect.push_back(a[i] * b[i]);
      }
      return retvect;
    }
};



#ifdef __CINT__
#pragma link C++ class EventAlignmentHistos-;
#endif

#endif //EUDAQ_EVENTALIGNMENTHISTOS_HH