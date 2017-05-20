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

#include <cstdint>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

class EventAlignmentHistos {

protected:
    TH1 *_Alignment;
    TH1 *_AlignmentPlus1;
    TH1 *_PulserRate;
    TH2I *_IsAligned;
    TH2I *_IsAlignedPlus1;
    TGraph * _3DPixelCorrelation;
    TGraph * _SilPixelCorrelation;
    uint16_t _lastNClusters;
    std::vector<size_t> evntNumbers1;
    std::vector<size_t> evntNumbers2;
    std::vector<uint8_t> rowAna1;
    std::vector<uint8_t> rowAna2;
    std::vector<uint8_t> rowDig3D;
    std::vector<uint8_t> rowDigSil;
    TGraph * _Corr;

public:
    EventAlignmentHistos();

    virtual ~EventAlignmentHistos();

    void Fill(const SimpleStandardEvent &);

    void Write();

    void Reset();

    TProfile *getAlignmentHisto() { return (TProfile *) _Alignment; }
    TProfile *getAlignmentPlus1Histo() { return (TProfile *) _AlignmentPlus1; }
    TProfile *getPulserRate() { return (TProfile *) _PulserRate; }
    TGraph * get3DPixelCorrelation() { return _3DPixelCorrelation; }
    TGraph * getSilPixelCorrelation() { return _SilPixelCorrelation; }
    TH2I *getIsAlignedHisto() { return _IsAligned; }
    TH2I *getIsAlignedPlus1Histo() { return _IsAlignedPlus1; }
    bool hasWaveForm;

private:
    const uint16_t _bin_size;
    const uint32_t max_event_number;

    TProfile *init_profile(std::string, std::string, uint16_t bin_size = 0, std::string ytit = "Fraction of Hits @ Pulser Events [%]");

    TGraph * init_tgraph(std::string, std::string, std::string);

    TH2I *init_th2i(std::string, std::string);

    void FillIsAligned(TProfile *, TH2I *, TProfile *);

    void FillCorrelationVectors(const SimpleStandardEvent &);
    void BuildCorrelation();

    void ResizeObjects(uint32_t);

    uint16_t fillColor;

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
//    {
//      return sum(multiply(minus(X, mean(X)), minus(Y, mean(Y)))) / (X.size() * stdev(X) * stdev(Y));
//    }

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