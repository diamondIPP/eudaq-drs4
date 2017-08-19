/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUHistos>.cc
** 
** Date: May 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#include "TUHistos.hh"
#include "SimpleStandardEvent.hh"

#include <TH1I.h>
#include <TFile.h>


TUHistos::TUHistos(): n_scaler(4){

  _CoincidenceCount = create_th1i("Concidence Rate", "Coincidence Rate [Hz]; Run Time [s]");
  _CoincidenceCountNoScint = create_th1i("Coincidence Rate No Scintillator", "Coincidence Rate No Scintillator [Hz]; Run Time [s]");
  _PrescalerCount = create_th1i("Prescaler Rate", "Prescaler Rate [Hz]; Run Time [s]");
  _PrescalerXPulser = create_th1i("Prescaler Xor Pulser Rate", "Prescaler Xor Pulser Rate [Hz]; Run Time [s]");
  _AcceptedPrescaledEvents = create_th1i("Accepted Prescaled Event Rate", "Accepted Prescaled Rate [Hz]; Run Time [s]");
  _AcceptedPulserEvents = create_th1i("Accepted Pulser Event Rate", "Accepted Pulser Event Rate [Hz]; Run Time [s]");
  _EventCount = create_th1i("Event Rate", "Event Rate [Hz]; Run Time [s]");
  _AvgBeamCurrent = create_th1i("Average Beam Current", "Average Beam Current [uA]; Run Time [s]");
  for (unsigned i(0); i < n_scaler; i++)
    _Scaler.push_back(create_th1i(TString::Format("Rate Plane Scaler %d", i + 1), TString::Format("Rate Plane Scaler %d [Hz]; Run Time [s]", i + 1)));


  called = false;
  old_timestamp = 0;
  old_coincidence_count = 0;
  old_coincidence_count_no_sin = 0;
  old_prescaler_count = 0;
  old_prescaler_count_xor_pulser_count = 0;
  old_accepted_prescaled_events = 0;
  old_accepted_pulser_events = 0;
  old_handshake_count = 0;
  old_scaler.resize(n_scaler, 0);
  old_xaxis =0;


  if((_CoincidenceCount==NULL)){
    std::cout<< "TUHistos:: Error allocating Histograms" <<std::endl;
    exit(-1);
  }

}

TUHistos::~TUHistos(){}



void TUHistos::Write(){
  _CoincidenceCount->Write();
  _CoincidenceCountNoScint->Write();
  _PrescalerCount->Write();
  _PrescalerXPulser->Write();
  _AcceptedPrescaledEvents->Write();
  _AcceptedPulserEvents->Write();
  _EventCount->Write();
  _AvgBeamCurrent->Write();
  for (unsigned i(0); i < n_scaler; i++)
    _Scaler.at(i)->Write();

}



void TUHistos::Fill(SimpleStandardTUEvent ev, unsigned int event_nr){
  bool valid = ev.GetValid();

  //set time stamp of beginning event or some of the first events (not 100% precise!)
  if(!called && valid){
    start_time = ev.GetTimeStamp();
    called = true;
  }

	

  if(valid){

    if(old_timestamp > 0){ //it was already set
      uint64_t new_timestamp = ev.GetTimeStamp();
      uint32_t coincidence_count = ev.GetCoincCount();
      uint32_t coincidence_count_no_sin = ev.GetCoincCountNoSin();
      uint32_t prescaler_count = ev.GetPrescalerCount();
      uint32_t prescaler_count_xor_pulser_count = ev.GetPrescalerCountXorPulserCount();
      uint32_t accepted_prescaled_events = ev.GetAcceptedPrescaledEvents();
      uint32_t accepted_pulser_events = ev.GetAcceptedPulserCount();
      uint32_t handshake_count = ev.GetHandshakeCount();
      uint32_t cal_beam_current = ev.GetBeamCurrent();

      std::vector<uint32_t> scaler;
      for (unsigned i(0); i < n_scaler; i++)
        scaler.push_back(unsigned(ev.GetScalerValue(i)));

      uint32_t readout_interval = 500;
      uint32_t t_diff = (uint32_t) (new_timestamp - old_timestamp);
      uint32_t x_axis = (uint32_t) (new_timestamp - start_time)/1000;

      


      if(x_axis > old_xaxis){
      
      //this is not the ideal solution but it works
      _CoincidenceCount->GetXaxis()->SetRangeUser(0, x_axis+10);
      _CoincidenceCountNoScint->GetXaxis()->SetRangeUser(0, x_axis+10);
      _PrescalerCount->GetXaxis()->SetRangeUser(0, x_axis+10);
      _PrescalerXPulser->GetXaxis()->SetRangeUser(0, x_axis+10);
      _AcceptedPrescaledEvents->GetXaxis()->SetRangeUser(0, x_axis+10);
      _AcceptedPulserEvents->GetXaxis()->SetRangeUser(0, x_axis+10);
      _EventCount->GetXaxis()->SetRangeUser(0, x_axis+10);
      _AvgBeamCurrent->GetXaxis()->SetRangeUser(0, x_axis+10);
      for (unsigned i(0); i < n_scaler; i++)
        _Scaler.at(i)->GetXaxis()->SetRangeUser(0, x_axis+10);


      _CoincidenceCount->Fill(x_axis, 1000*(coincidence_count - old_coincidence_count)/t_diff);
      _CoincidenceCountNoScint->Fill(x_axis, 1000*(coincidence_count_no_sin - old_coincidence_count_no_sin)/t_diff);
      _PrescalerCount->Fill(x_axis, 1000*(prescaler_count - old_prescaler_count)/t_diff);
      _PrescalerXPulser->Fill(x_axis, 1000*(prescaler_count_xor_pulser_count - old_prescaler_count_xor_pulser_count)/t_diff);
      _AcceptedPrescaledEvents->Fill(x_axis, 1000*(accepted_prescaled_events - old_accepted_prescaled_events)/t_diff);
      _AcceptedPulserEvents->Fill(x_axis, 1000*(accepted_pulser_events - old_accepted_pulser_events)/t_diff);
      _EventCount->Fill(x_axis, 1000*(handshake_count - old_handshake_count)/t_diff);
      _AvgBeamCurrent->Fill(x_axis, cal_beam_current);
      for (unsigned i(0); i < n_scaler; i++)
        _Scaler.at(i)->Fill(x_axis, 1000 * (scaler.at(i) - old_scaler.at(i))/t_diff);
      }
      old_xaxis = x_axis;
    }

      old_timestamp = ev.GetTimeStamp();
      old_coincidence_count = ev.GetCoincCount();
      old_coincidence_count_no_sin = ev.GetCoincCountNoSin();
      old_prescaler_count = ev.GetPrescalerCount();
      old_prescaler_count_xor_pulser_count = ev.GetPrescalerCountXorPulserCount();
      old_accepted_prescaled_events = ev.GetAcceptedPrescaledEvents();
      old_accepted_pulser_events = ev.GetAcceptedPulserCount();
      old_handshake_count = ev.GetHandshakeCount();
      for (unsigned i(0); i < n_scaler; i++)
        old_scaler.at(i) = ev.GetScalerValue(i);
  }

}


void TUHistos::Reset(){
  _CoincidenceCount->Reset();
  _CoincidenceCountNoScint->Reset();
  _PrescalerCount->Reset();
  _PrescalerXPulser->Reset();
  _AcceptedPrescaledEvents->Reset();
  _AcceptedPulserEvents->Reset();
  _EventCount->Reset();
  _AvgBeamCurrent->Reset();
  for (unsigned i(0); i < n_scaler; i++)
    _Scaler.at(i)->Reset();
}

TH1I * TUHistos::create_th1i(TString name, TString title, int nbins, int xmin, int xmax, int markerstyle, int markersize, int markercolor) {
  
  TH1I * h = new TH1I(name, title, nbins, xmin, xmax);
  h->SetMarkerStyle(markerstyle);
  h->SetMarkerSize(markersize);
  h->SetMarkerColor(markercolor);
  return h
;
}

