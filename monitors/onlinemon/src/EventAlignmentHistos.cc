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
#include "TList.h"

using namespace std;


EventAlignmentHistos::EventAlignmentHistos(): hasWaveForm(true), _nOffsets(10), _bin_size(1000), max_event_number(uint32_t(5e7)), _n_analogue_planes(4),
                                              _n_dig_planes(0),  _n_devices(1), foundPulser(false), count(0), _lastNClusters(0), fillColor(821), _PixelIsAligned(nullptr)
{
    _PulserRate = init_profile("p_pu", "Pulser Rate", 200, "Fraction of Pulser Events [%]", fillColor);
    _PulserLegend = new TText(.7, .8, "Mean: ?");
    _PulserRate->GetListOfFunctions()->Add(_PulserLegend);
    for (uint8_t i(7); i < 18; i++)
      _PixelCorrelations.push_back(init_profile(string(TString::Format("p_pc%d", i)), string(TString::Format("Pixel Correlation - REF %d", i)), _bin_size, "Correlation Factor", fillColor));
    _Corr = new TGraph();
    rowAna1 = new vector<vector<uint8_t> >;
    rowDig = new vector<vector<uint8_t> >;
}

EventAlignmentHistos::~EventAlignmentHistos() = default;

void EventAlignmentHistos::Write(){

    for (const auto & d:_PadAlignment)
      for (auto h:d)
        h.second->Write();
    _PulserRate->Write();
    for (auto h:_IsAligned)
      h->Write();
    _PixelIsAligned->Write();
    for (auto icor:_PixelCorrelations)
      icor->Write();
}

void EventAlignmentHistos::FillCorrelationVectors(const SimpleStandardEvent &sev) {

    //TODO: d0 that the first time we get an event!
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
        rowAna1->at(iplane).push_back(uint8_t(pAna1.getCluster(0).getY()));
        rowDig->at(iplane).push_back(uint8_t(pDigs.at(iplane).getCluster(0).getY()));
      }
    }
}

void EventAlignmentHistos::BuildCorrelation() {

  for (uint8_t iplane(0); iplane < _n_dig_planes; iplane++) {
    if (eventNumbers.at(iplane).size() == 500) {
      double mean_evt = mean(eventNumbers.at(iplane));
      double corr = pearsoncoeff(rowAna1->at(iplane), rowDig->at(iplane));
      _PixelCorrelations.at(iplane)->Fill(mean_evt, corr);
      rowDig->at(iplane).clear();
      rowAna1->at(iplane).clear();
      eventNumbers.at(iplane).clear();
      _PixelCorrelations.at(iplane)->GetYaxis()->SetRangeUser(-.2, 1);
      _PixelIsAligned->SetBinContent(_PixelIsAligned->GetXaxis()->FindBin(mean_evt), iplane * 2 + 2, (corr > .4) ? 3 : 5);
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


  uint32_t event_no = sev.getEvent_number();
  ResizeObjects(event_no);

  // Pixel Stuff
  if (sev.getNWaveforms() == 0){
    FillCorrelationVectors(sev);
    BuildCorrelation();
    return;
  }

  SimpleStandardWaveform wf = sev.getWaveform(0);
  for (auto idev(0); idev < _n_devices; idev++){
    _lastNClusters.at(idev).push_back(0);
    if (_lastNClusters.at(idev).size() > _nOffsets * 2 + 1)
      _lastNClusters.at(idev).pop_front();
  }
  for (auto ipl(0); ipl < sev.getNPlanes(); ipl++){
    uint8_t idev = _device_names.at(sev.getPlane(ipl).getName());
    _lastNClusters.at(idev).back() += sev.getPlane(ipl).getNClusters();
  }

  _PulserRate->Fill(event_no, 100 * wf.isPulserEvent());
  foundPulser = wf.isPulserEvent() or foundPulser;
  if (foundPulser)
    count++;
  if (count == _nOffsets + 1 and _PulserRate->GetBinContent(_PulserRate->GetNbinsX() - 1) < 30){
    foundPulser = false;
    count = 0;
    for (int ioff(-_nOffsets); ioff <= _nOffsets; ioff++) {
      if (_lastNClusters.at(0).size() > ioff + _nOffsets) {
        for (auto idev(0); idev < _n_devices; idev++) {
          _PadAlignment.at(idev).at(ioff)->Fill(event_no, _lastNClusters.at(idev).at(uint8_t(ioff + _nOffsets)) ? 100 : .1);
        }
      }
    }

    for (auto idev(0); idev < _n_devices; idev++)
      FillIsAligned(idev);
  }

}

TProfile * EventAlignmentHistos::init_profile(const std::string & name, const std::string & title, uint16_t bin_size, const string & ytit, Color_t fill_color) {

    bin_size = bin_size ? bin_size : _bin_size;
    auto * prof = new TProfile(name.c_str(), title.c_str(), 1, 0, bin_size);
    prof->SetStats(false);
    prof->GetYaxis()->SetRangeUser(-10, 110);
    prof->GetXaxis()->SetTitle("Event Number");
    prof->GetYaxis()->SetTitle(ytit.c_str());
    prof->GetYaxis()->SetTitleOffset(1.3);
    if (fill_color)
      prof->SetFillColor(fill_color);

    return prof;
}

TH2I * EventAlignmentHistos::init_th2i(const std::string & name, const std::string & title){
    auto ybins = uint8_t(_nOffsets * 2 + 1);
    TH2I * histo = new TH2I(name.c_str(), title.c_str(), max_event_number / _bin_size, 0, max_event_number, ybins + 2, 0, ybins + 2);
    histo->SetStats(false);
    histo->GetZaxis()->SetRangeUser(0, 5);
    histo->GetXaxis()->SetRangeUser(0, _bin_size);
    for (int ioff(-_nOffsets); ioff <= _nOffsets; ioff++)
      histo->GetYaxis()->SetBinLabel(ioff + _nOffsets + 2, TString::Format("%d", ioff));
    histo->GetYaxis()->SetLabelSize(0.06);
    histo->GetXaxis()->SetTitle("Event Number");
    return histo;
}
TH2F * EventAlignmentHistos::init_pix_align(uint16_t n_planes){

  int n_cols = n_planes * 2 + 1;
  TH2F * histo = new TH2F("h_pal", "Pixel Event Alignment", max_event_number / _bin_size, 0, max_event_number, n_cols, 0, n_cols);
  histo->SetStats(false);
  histo->GetZaxis()->SetRangeUser(0, 5);
  histo->GetXaxis()->SetRangeUser(0, _bin_size);
  for (int i_pl(0); i_pl < n_planes; i_pl++)
    histo->GetYaxis()->SetBinLabel(2 * i_pl + 2, TString::Format("DUT%d", i_pl));
//  histo->GetYaxis()->SetBinLabel(4, "Sil");
  histo->GetXaxis()->SetTitle("Event Number");
  return histo;
}

void EventAlignmentHistos::FillIsAligned(uint8_t idev) {

    // check if all are aligned (may happen at interruptions)
    uint16_t last_bin = uint16_t(_PadAlignment.at(idev).at(0)->GetNbinsX() - 1);
    uint8_t all = 0;
    for (auto h:_PadAlignment.at(idev))
      if (h.second->GetBinContent(last_bin) < 20)
        all++;
    // don't fill if already set
    if (_IsAligned.at(idev)->GetBinContent(last_bin) > 0 or all > _nOffsets)
        return;
    for (uint8_t i(0); i < _PadAlignment.at(idev).size(); i++){
      _IsAligned.at(idev)->SetBinContent(last_bin, i + 2, _PadAlignment.at(idev).at(i - _nOffsets)->GetBinContent(last_bin) < 30 ? 3 : 5);
    }
}

void EventAlignmentHistos::ResizeObjects(uint32_t ev_no) {

  for (auto idev(0); idev < _n_devices; idev++){
    if (_PadAlignment.at(idev).at(0)->GetXaxis()->GetXmax() < ev_no) {
      uint32_t bins = (ev_no + _bin_size) / _bin_size;
      uint32_t max = bins * _bin_size;
      for (auto h:_PadAlignment.at(idev))
        h.second->SetBins(bins, 0, max);
      _IsAligned.at(idev)->GetXaxis()->SetRangeUser(0, max);
      if (not hasWaveForm){
        _PixelIsAligned->GetXaxis()->SetRangeUser(0, max);
        for (uint8_t iplane(0); iplane < _n_dig_planes; iplane++)
          _PixelCorrelations.at(iplane)->SetBins(bins, 0, max);
      }
    }

    if (_PulserRate->GetXaxis()->GetXmax() < ev_no) {
      vector<float> values;
      for (uint16_t ibin(0); ibin < _PulserRate->GetNbinsX(); ibin++){
        auto x = float(_PulserRate->GetBinContent(ibin));
        if (x < 30)
          values.push_back(x);
      }
      _PulserLegend->SetText(.7, .8, TString::Format("Mean: %.1f", mean(values)));
      auto bin_size = uint16_t(_PulserRate->GetBinWidth(0));
      uint32_t bins = (ev_no + bin_size) / bin_size;
      uint32_t max = bins * bin_size;
      _PulserRate->SetBins(bins, 0, max);
    }
  }
}

void EventAlignmentHistos::Reset(){

    for (const auto & d: _PadAlignment)
        for (auto h:d)
          h.second->Reset();
    for (auto h:_IsAligned)
      h->Reset();
    _PulserRate->Reset();
}

void EventAlignmentHistos::InitVectors() {

  eventNumbers.resize(_n_dig_planes);
  rowDig->resize(_n_dig_planes);
  rowAna1->resize(_n_dig_planes);
}

TH2F* EventAlignmentHistos::getPixelIsAlignedHisto(const SimpleStandardEvent & sev) {

  _PixelIsAligned = init_pix_align(uint16_t(sev.getNPlanes() - _n_analogue_planes));
  return _PixelIsAligned;

}

void EventAlignmentHistos::InitPadAlignment() {

  for (auto idev(0); idev < _n_devices; idev++){
    map<int, TProfile *> temp;
    for (int ioff(-_nOffsets); ioff <= _nOffsets; ioff++) {
      string title = string(TString::Format("Hits @ Pulser DUT-%d", idev));
      temp[ioff] = init_profile(string(TString::Format("p_al%d-%d", ioff, idev)), title, _bin_size, "Fraction of Hits @ Pulser Events [%]", fillColor);
    }
    _PadAlignment.push_back(temp);
    _IsAligned.push_back(init_th2i(string(TString::Format("hal%d", idev)), string(TString::Format("Event Alignment DUT %d", idev))));
  }
}

void EventAlignmentHistos::init(const SimpleStandardEvent & sev) {

  _n_devices = sev.getNDevices();
  _device_names = sev.getDeviceNames();
  _lastNClusters.resize(_n_devices);
  _n_analogue_planes = sev.getNPlanes() < _n_analogue_planes ? sev.getNPlanes() : _n_analogue_planes;
  _n_dig_planes = uint8_t(sev.getNPlanes() - _n_analogue_planes);
  hasWaveForm = _n_analogue_planes == sev.getNPlanes();
  InitVectors();
  InitPadAlignment();
}


