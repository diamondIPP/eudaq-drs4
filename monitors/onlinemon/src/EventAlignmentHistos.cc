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
#include "TText.h"

using namespace std;


EventAlignmentHistos::EventAlignmentHistos(): _nOffsets(5), _bin_size(1000), max_event_number(uint32_t(5e7)), _n_analogue_planes(4), _n_dig_planes(0), foundPulser(false),
                                              count(0), _lastNClusters(0), fillColor(821)
{
    for (int ioff(-_nOffsets); ioff <= _nOffsets; ioff++)
      _Alignment[ioff] = init_profile(string(TString::Format("p_al%d", ioff)), "Fraction of Hits at Pulser Events", _bin_size, "Fraction of Hits @ Pulser Events [%]", fillColor);
    _PulserRate = init_profile("p_pu", "Pulser Rate", 200, "Fraction of Pulser Events [%]", fillColor);
    _PulserLegend = new TText(.7, .8, "Mean: ?");
    _PulserRate->GetListOfFunctions()->Add(_PulserLegend);
    for (uint8_t i(7); i < 10; i++)
      _PixelCorrelations.push_back(init_profile(string(TString::Format("p_pc%d", i)), string(TString::Format("Pixel Correlation - REF %d", i)), _bin_size, "Correlation Factor", fillColor));
    _IsAligned = init_th2i("h_al", "Event Alignment");
    _PixelIsAligned = init_pix_align();
    _Corr = new TGraph();
}

EventAlignmentHistos::~EventAlignmentHistos(){}

void EventAlignmentHistos::Write(){

    for (auto h:_Alignment)
      h.second->Write();
    _PulserRate->Write();
    _IsAligned->Write();
    _PixelIsAligned->Write();
    for (auto icor:_PixelCorrelations)
      icor->Write();
}

void EventAlignmentHistos::FillCorrelationVectors(const SimpleStandardEvent &sev) {

    //TODO: dp that the first time we get an event!
    vector<int> ana_planes;
    vector<SimpleStandardPlane> pDigs;
    for (uint8_t iplane(0); iplane < _n_analogue_planes; iplane++)
      ana_planes.push_back((sev.getPlane(0).getName() == "DUT") ? iplane : _n_analogue_planes + iplane);
    for (uint8_t iplane(0); iplane < sev.getNPlanes() - _n_analogue_planes; iplane++)
      pDigs.push_back(sev.getPlane((sev.getPlane(0).getName() == "DUT") ? _n_analogue_planes + iplane : iplane));
    // choose planes closest to digital planes
    SimpleStandardPlane pAna1 = sev.getPlane(ana_planes.at(2));
    SimpleStandardPlane pAna2 = sev.getPlane(ana_planes.at(1));

    for (uint8_t iplane(0); iplane < pDigs.size(); iplane++){
      if (pAna1.getNClusters() == 1 and pDigs.at(iplane).getNClusters() == 1){
        eventNumbers.at(iplane).push_back(sev.getEvent_number());
        rowAna1.push_back(uint8_t(pAna1.getCluster(0).getY()));
        rowDig.at(iplane).push_back(uint8_t(pDigs.at(iplane).getCluster(0).getY()));
      }
    }
}

void EventAlignmentHistos::BuildCorrelation() {

  for (uint8_t iplane(0); iplane < _n_dig_planes; iplane++) {
    if (eventNumbers.at(iplane).size() == 500) {
      double mean_evt = mean(eventNumbers.at(iplane));
      double corr = pearsoncoeff(rowAna1, rowDig.at(iplane));
      _PixelCorrelations.at(iplane)->Fill(mean_evt, corr);
      rowDig.at(iplane).clear();
      rowAna1.clear();
      eventNumbers.at(iplane).clear();
      _PixelCorrelations.at(iplane)->GetYaxis()->SetRangeUser(-.2, 1);
      _PixelIsAligned->SetBinContent(_PixelIsAligned->GetXaxis()->FindBin(mean_evt), 4, (corr > .4) ? 3 : 5);
    }
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


  if (not _n_dig_planes){
    _n_dig_planes = uint8_t(sev.getNPlanes() - _n_analogue_planes);
    InitVectors();
  }
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
  _lastNClusters.push_back(n_clusters);
  if (_lastNClusters.size() > _nOffsets * 2 + 1)
    _lastNClusters.pop_front();

  _PulserRate->Fill(event_no, 100 * wf.isPulserEvent());
  foundPulser = wf.isPulserEvent() or foundPulser;
  if (foundPulser)
    count++;
  if (count == _nOffsets + 1 and _PulserRate->GetBinContent(_PulserRate->GetNbinsX() - 1) < 30){
    foundPulser = false;
    count = 0;
    for (int ioff(-_nOffsets); ioff <= _nOffsets; ioff++)
      if (_lastNClusters.size() > ioff + _nOffsets)
        _Alignment.at(ioff)->Fill(event_no, _lastNClusters.at(uint8_t(ioff + _nOffsets)) ? 100 : .1);

    FillIsAligned();
  }

}

TProfile * EventAlignmentHistos::init_profile(std::string name, std::string title, uint16_t bin_size, string ytit, Color_t fill_color) {

    bin_size = bin_size ? bin_size : _bin_size;
    TProfile * prof = new TProfile(name.c_str(), title.c_str(), 1, 0, bin_size);
    prof->SetStats(false);
    prof->GetYaxis()->SetRangeUser(-10, 110);
    prof->GetXaxis()->SetTitle("Event Number");
    prof->GetYaxis()->SetTitle(ytit.c_str());
    prof->GetYaxis()->SetTitleOffset(1.3);
    if (fill_color)
      prof->SetFillColor(fill_color);

    return prof;
}

TH2I * EventAlignmentHistos::init_th2i(std::string name, std::string title){
    uint8_t ybins = uint8_t(_nOffsets * 2 + 1);
    TH2I * histo = new TH2I(name.c_str(), title.c_str(), max_event_number / _bin_size, 0, max_event_number, ybins + 2, 0, ybins + 2);
    histo->SetStats(false);
    histo->GetZaxis()->SetRangeUser(0, 5);
    histo->GetXaxis()->SetRangeUser(0, _bin_size);
    for (auto h:_Alignment)
      histo->GetYaxis()->SetBinLabel(h.first + _nOffsets + 2, TString::Format("%d", h.first));
    histo->GetYaxis()->SetLabelSize(0.06);
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

void EventAlignmentHistos::FillIsAligned() {

    // check if all are aligned (may happen at interruptions)
    uint16_t last_bin = uint16_t(_Alignment.at(0)->GetNbinsX() - 1);
    uint8_t all = 0;
    for (auto h:_Alignment)
      if (h.second->GetBinContent(last_bin) < 20)
        all++;
    // don't fill if already set
    if (_IsAligned->GetBinContent(last_bin) > 0 or all > _nOffsets)
        return;
    for (uint8_t i(0); i < _Alignment.size(); i++){
      _IsAligned->SetBinContent(last_bin, i + 2, _Alignment.at(i - _nOffsets)->GetBinContent(last_bin) < 30 ? 3 : 5);
    }
}

void EventAlignmentHistos::ResizeObjects(uint32_t ev_no) {

    if (_Alignment.at(0)->GetXaxis()->GetXmax() < ev_no) {
        uint32_t bins = (ev_no + _bin_size) / _bin_size;
        uint32_t max = bins * _bin_size;
        for (auto h:_Alignment)
          h.second->SetBins(bins, 0, max);
        _IsAligned->GetXaxis()->SetRangeUser(0, max);
        _PixelIsAligned->GetXaxis()->SetRangeUser(0, max);
        for (uint8_t iplane(0); iplane < _n_dig_planes; iplane++)
          _PixelCorrelations.at(iplane)->SetBins(bins, 0, max);
    }

    if (_PulserRate->GetXaxis()->GetXmax() < ev_no) {
        vector<float> values;
        for (uint16_t ibin(0); ibin < _PulserRate->GetNbinsX(); ibin++){
          float x = float(_PulserRate->GetBinContent(ibin));
          if (x < 30)
            values.push_back(x);
        }
        _PulserLegend->SetText(.7, .8, TString::Format("Mean: %.1f", mean(values)));
        uint16_t bin_size = uint16_t(_PulserRate->GetBinWidth(0));
        uint32_t bins = (ev_no + bin_size) / bin_size;
        uint32_t max = bins * bin_size;
        _PulserRate->SetBins(bins, 0, max);
    }
}

void EventAlignmentHistos::Reset(){

    for (auto h:_Alignment)
      h.second->Reset();
    _IsAligned->Reset();
    _PulserRate->Reset();
}

void EventAlignmentHistos::InitVectors() {

  eventNumbers.resize(_n_dig_planes);
  rowDig.resize(_n_dig_planes);
}

