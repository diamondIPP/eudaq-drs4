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

using namespace std;


EventAlignmentHistos::EventAlignmentHistos(): bin_size(2000), max_event_number(uint32_t(5e7)), _lastNClusters(0)
{
    _Alignment = init_profile("p_al", "Fraction of Hits at Pulser Events");
    _AlignmentPlus1 = init_profile("p_al1", "Fraction of Hits at Pulser Events + 1");
    _IsAligned = init_th2i("h_al", "Event Alignment");
    _IsAlignedPlus1 = init_th2i("h_al1", "Event Alignment Plus 1");
}

EventAlignmentHistos::~EventAlignmentHistos(){}

void EventAlignmentHistos::Write(){

    _Alignment->Write();
    _AlignmentPlus1->Write();
    _IsAligned->Write();
    _IsAlignedPlus1->Write();
}

void EventAlignmentHistos::Fill(const SimpleStandardEvent & sev){

    //we need at least on waveform!
    if (!sev.getNWaveforms())
        return;
    uint32_t event_no = sev.getEvent_number();

    if (_Alignment->GetXaxis()->GetXmax() < event_no) {
        uint32_t bins = (event_no + bin_size) / bin_size;
        uint32_t max = bins * bin_size;
        _Alignment->SetBins(bins, 0, max);
        _AlignmentPlus1->SetBins(bins, 0, max);
        _IsAligned->GetXaxis()->SetRangeUser(0, max);
        _IsAlignedPlus1->GetXaxis()->SetRangeUser(0, max);
    }

    SimpleStandardWaveform wf = sev.getWaveform(0);
    uint16_t n_clusters = 0;
    for (uint8_t i_pl = 0; i_pl < sev.getNPlanes(); i_pl++){
        SimpleStandardPlane pl = sev.getPlane(i_pl);
        n_clusters += pl.getNClusters();
    }
    if (wf.isPulserEvent()){
        _Alignment->Fill(event_no, n_clusters ? 100 : .1);
        _AlignmentPlus1->Fill(event_no, _lastNClusters ? 100 : .1);
        FillIsAligned(getAlignmentHisto(), _IsAligned);
        FillIsAligned(getAlignmentPlus1Histo(), _IsAlignedPlus1);
    }
    _lastNClusters = n_clusters;
}

TProfile * EventAlignmentHistos::init_profile(std::string name, std::string title) {

    TProfile * prof = new TProfile(name.c_str(), title.c_str(), 1, 0, bin_size);
    prof->SetStats(false);
    prof->GetYaxis()->SetRangeUser(-10, 100);
    return prof;
}

TH2I * EventAlignmentHistos::init_th2i(std::string name, std::string title){
    TH2I * histo = new TH2I(name.c_str(), title.c_str(), max_event_number / bin_size, 0, max_event_number, 5, 0, 5);
    histo->SetStats(false);
    histo->GetZaxis()->SetRangeUser(0, 5);
    histo->GetXaxis()->SetRangeUser(0, bin_size);
    histo->GetYaxis()->SetBinLabel(2, "Aligned");
    histo->GetYaxis()->SetBinLabel(4, "#splitline{Not}{Aligned}");
    return histo;
}
void EventAlignmentHistos::FillIsAligned(TProfile * prof, TH2I * histo) {
    uint16_t last_bin = uint16_t(prof->GetNbinsX() - 1);
    if (prof->GetBinContent(last_bin) > 20)
        histo->SetBinContent(last_bin, 4, 5);
    else
        histo->SetBinContent(last_bin, 2, 3);
}

void EventAlignmentHistos::Reset(){

    _Alignment->Reset();
    _AlignmentPlus1->Reset();
    _IsAligned->Reset();
    _IsAlignedPlus1->Reset();
}

