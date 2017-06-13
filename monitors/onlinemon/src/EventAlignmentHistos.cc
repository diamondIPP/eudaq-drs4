/* ---------------------------------------------------------------------------------
** Event Alignment Histograms for Alignment between WaveForms and Telescope Planes
**
**
** <EventAlignmentHistos>.cc
**
** Date: May 2016
** Author: Michael Reichmann (remichae@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#include "EventAlignmentHistos.hh"
#include "SimpleStandardEvent.hh"
#include "TProfile.h"
#include "TH2I.h"
#include "TGraph.h"

using namespace std;


EventAlignmentHistos::EventAlignmentHistos(): _bin_size(5000), max_event_number(uint32_t(5e7)), _lastNClusters(0), fillColor(821)
{
    _Alignment = init_profile("p_al", "Fraction of Hits at Pulser Events");
    _AlignmentPlus1 = init_profile("p_al1", "Fraction of Hits at Pulser Events + 1");
    _PulserRate = init_profile("p_pu", "Pulser Rate", 50, "Fraction of Pulser Events [%]");
    _3DPixelCorrelation = init_profile("p_pc1", "Pixel Correlation - 3D", _bin_size, "Correlation Factor");
    _3DPixelCorrelation->SetFillColor(fillColor);
    _SilPixelCorrelation = init_profile("p_pc2", "Pixel Correlation - Silicon", _bin_size, "Correlation Factor");
    _SilPixelCorrelation->SetFillColor(fillColor);
    _PulserRate->SetFillColor(fillColor);
    _IsAligned = init_th2i("h_al", "Event Alignment");
    _IsAlignedPlus1 = init_th2i("h_al1", "Event Alignment Plus 1");
    _PixelIsAligned = init_pix_align();
    _Corr = new TGraph();
}

EventAlignmentHistos::~EventAlignmentHistos(){}

void EventAlignmentHistos::Write(){

    _Alignment->Write();
    _AlignmentPlus1->Write();
    _PulserRate->Write();
    _IsAligned->Write();
    _IsAlignedPlus1->Write();
    _PixelIsAligned->Write();
    _3DPixelCorrelation->Write();
    _SilPixelCorrelation->Write();
}

void EventAlignmentHistos::FillCorrelationVectors(const SimpleStandardEvent &sev) {

    int iRef1 = (sev.getPlane(0).getName() == "DUT") ? 4 : 0;
    int iRef2 = (sev.getPlane(0).getName() == "DUT") ? 5 : 1;
    int iDut1 = (sev.getPlane(0).getName() == "DUT") ? 2 : 5;
    int iDut2 = (sev.getPlane(0).getName() == "DUT") ? 1 : 4;
    SimpleStandardPlane pAna1 = sev.getPlane(iDut1);
    SimpleStandardPlane pAna2 = sev.getPlane(iDut2);
    SimpleStandardPlane pDig1 = sev.getPlane(iRef1);
    SimpleStandardPlane pDig2 = sev.getPlane(iRef2);
    if (pAna1.getNClusters() == 1 and pDig1.getNClusters() == 1){
      evntNumbers1.push_back(sev.getEvent_number());
      rowAna1.push_back(uint8_t(pAna1.getCluster(0).getY()));
      rowDig3D.push_back(uint8_t(pDig1.getCluster(0).getY()));
    }
  if (pAna2.getNClusters() == 1 and pDig2.getNClusters() == 1){
    evntNumbers2.push_back(sev.getEvent_number());
    rowAna2.push_back(uint8_t(pAna2.getCluster(0).getY()));
    rowDigSil.push_back(uint8_t(pDig2.getCluster(0).getY()));
  }
}

void EventAlignmentHistos::BuildCorrelation() {

  if (evntNumbers1.size() == 500){
    double mean_evt = mean(evntNumbers1);
    double corr = pearsoncoeff(rowAna1, rowDig3D);
    _3DPixelCorrelation->Fill(mean_evt, corr);
    rowDig3D.clear();
    rowAna1.clear();
    evntNumbers1.clear();
    _3DPixelCorrelation->GetYaxis()->SetRangeUser(-.2, 1);
    _PixelIsAligned->SetBinContent(_PixelIsAligned->GetXaxis()->FindBin(mean_evt), 4, (corr > .4) ? 3 : 5);
  }
  if (evntNumbers2.size() == 500){
    double mean_evt = mean(evntNumbers2);
    double corr = pearsoncoeff(rowAna2, rowDigSil);
    _SilPixelCorrelation->Fill(mean_evt, corr);
    rowDigSil.clear();
    rowAna2.clear();
    evntNumbers2.clear();
    _SilPixelCorrelation->GetYaxis()->SetRangeUser(-.2, 1);
    _PixelIsAligned->SetBinContent(_PixelIsAligned->GetXaxis()->FindBin(mean_evt), 2, (corr > .4) ? 3 : 5);
  }
}

template <typename T>
double EventAlignmentHistos::pearsoncoeff(std::vector<T> X, std::vector<T> Y) {

  for (uint16_t i_ev = 0; i_ev < X.size(); i_ev++)
    _Corr->SetPoint(i_ev, X.at(i_ev), Y.at(i_ev));
  double corr_fac =  _Corr->GetCorrelationFactor();
  _Corr->Clear();
  return corr_fac;
}


void EventAlignmentHistos::Fill(const SimpleStandardEvent & sev){

  uint32_t event_no = sev.getEvent_number();
  ResizeObjects(event_no);

  // Pixel Stuff
  if (!sev.getNWaveforms()){
    FillCorrelationVectors(sev);
    BuildCorrelation();
    return;
  }

  SimpleStandardWaveform wf = sev.getWaveform(0);
  uint16_t n_clusters = 0;
  for (uint8_t i_pl = 0; i_pl < sev.getNPlanes(); i_pl++){
      SimpleStandardPlane pl = sev.getPlane(i_pl);
      n_clusters += pl.getNClusters();
  }
  _PulserRate->Fill(event_no, 100 * wf.isPulserEvent());
  if (wf.isPulserEvent() and _PulserRate->GetBinContent(_PulserRate->GetNbinsX() - 1) < 30){
      _Alignment->Fill(event_no, n_clusters ? 100 : .1);
      _AlignmentPlus1->Fill(event_no, _lastNClusters ? 100 : .1);
      FillIsAligned(getAlignmentHisto(), _IsAligned, getPulserRate());
      FillIsAligned(getAlignmentPlus1Histo(), _IsAlignedPlus1, getPulserRate());
  }
  _lastNClusters = n_clusters;
}

TProfile * EventAlignmentHistos::init_profile(std::string name, std::string title, uint16_t bin_size, string ytit) {

    bin_size = bin_size ? bin_size : _bin_size;
    TProfile * prof = new TProfile(name.c_str(), title.c_str(), 1, 0, bin_size);
    prof->SetStats(false);
    prof->GetYaxis()->SetRangeUser(-10, 100);
    prof->GetXaxis()->SetTitle("Event Number");
    prof->GetYaxis()->SetTitle(ytit.c_str());
    prof->GetYaxis()->SetTitleOffset(1.3);

    return prof;
}

TH2I * EventAlignmentHistos::init_th2i(std::string name, std::string title){
    TH2I * histo = new TH2I(name.c_str(), title.c_str(), max_event_number / _bin_size, 0, max_event_number, 5, 0, 5);
    histo->SetStats(false);
    histo->GetZaxis()->SetRangeUser(0, 5);
    histo->GetXaxis()->SetRangeUser(0, _bin_size);
    histo->GetYaxis()->SetBinLabel(2, "Aligned");
    histo->GetYaxis()->SetBinLabel(4, "#splitline{Not}{Aligned}");
    histo->GetXaxis()->SetTitle("Event Number");
    return histo;
}
TH2F * EventAlignmentHistos::init_pix_align(){
  TH2F * histo = new TH2F("h_pal", "Pixel Event Aignment", max_event_number / _bin_size, 0, max_event_number, 5, 0, 5);
  histo->SetStats(false);
  histo->GetZaxis()->SetRangeUser(0, 5);
  histo->GetXaxis()->SetRangeUser(0, _bin_size);
  histo->GetYaxis()->SetBinLabel(2, "3D");
  histo->GetYaxis()->SetBinLabel(4, "Sil");
  histo->GetXaxis()->SetTitle("Event Number");
  return histo;
}

void EventAlignmentHistos::FillIsAligned(TProfile * prof, TH2I * histo, TProfile * pulser) {
    if (pulser->GetBinContent(pulser->GetNbinsX() - 1) > 30)
        return;
    uint16_t last_bin = uint16_t(prof->GetNbinsX() - 1);
    // only fill if not already set!
    if (histo->GetBinContent(last_bin) > 0)
        return;
//    cout << last_bin << " " << prof->GetBinContent(last_bin)
    if (prof->GetBinContent(last_bin) > 20)
        histo->SetBinContent(last_bin, 4, 5);
    else
        histo->SetBinContent(last_bin, 2, 3);
}

void EventAlignmentHistos::ResizeObjects(uint32_t ev_no) {

    if (_Alignment->GetXaxis()->GetXmax() < ev_no) {
        uint32_t bins = (ev_no + _bin_size) / _bin_size;
        uint32_t max = bins * _bin_size;
        _Alignment->SetBins(bins, 0, max);
        _AlignmentPlus1->SetBins(bins, 0, max);
        _IsAligned->GetXaxis()->SetRangeUser(0, max);
        _IsAlignedPlus1->GetXaxis()->SetRangeUser(0, max);
        _PixelIsAligned->GetXaxis()->SetRangeUser(0, max);
        _3DPixelCorrelation->SetBins(bins, 0, max);
        _SilPixelCorrelation->SetBins(bins, 0, max);
    }

    if (_PulserRate->GetXaxis()->GetXmax() < ev_no) {
        uint16_t bin_size = uint16_t(_PulserRate->GetBinWidth(0));
        uint32_t bins = (ev_no + bin_size) / bin_size;
        uint32_t max = bins * bin_size;
        _PulserRate->SetBins(bins, 0, max);
    }
}

void EventAlignmentHistos::Reset(){

    _Alignment->Reset();
    _AlignmentPlus1->Reset();
    _IsAligned->Reset();
    _IsAlignedPlus1->Reset();
    _PulserRate->Reset();
}

