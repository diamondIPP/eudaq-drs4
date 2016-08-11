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
#include "SimpleStandardTUEvent.hh"

#include <iostream>
#include <TH1I.h>
#include <TH1F.h>
#include <TFile.h>
#include <cstdint>


TUHistos::TUHistos(){

  _CoincidenceCount = new TH1I("Concidence Rate", "Coincidence Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _CoincidenceCount->SetMarkerStyle(34);
  _CoincidenceCount->SetMarkerSize(1);
  _CoincidenceCount->SetMarkerColor(4);
  _CoincidenceCountNoScint = new TH1I("Coincidence Rate No Scintillator", "Coincidence Rate No Scintillator [Hz]; Run Time [s]",60000, 0, 60000);
  _CoincidenceCountNoScint->SetMarkerStyle(34);
  _CoincidenceCountNoScint->SetMarkerSize(1);
  _CoincidenceCountNoScint->SetMarkerColor(4);  
  _PrescalerCount = new TH1I("Prescaler Rate", "Prescaler Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _PrescalerCount->SetMarkerStyle(34);
  _PrescalerCount->SetMarkerSize(1);
  _PrescalerCount->SetMarkerColor(4);
  _PrescalerXPulser = new TH1I("Prescaler Xor Pulser Rate", "Prescaler Xor Pulser Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _PrescalerXPulser->SetMarkerStyle(34);
  _PrescalerXPulser->SetMarkerSize(1);
  _PrescalerXPulser->SetMarkerColor(4);
  _AcceptedPrescaledEvents = new TH1I("Accepted Prescaled Event Rate", "Accepted Prescaled Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _AcceptedPrescaledEvents->SetMarkerStyle(34);
  _AcceptedPrescaledEvents->SetMarkerSize(1);
  _AcceptedPrescaledEvents->SetMarkerColor(4);
  _AcceptedPulserEvents = new TH1I("Accepted Pulser Event Rate", "Accepted Pulser Event Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _AcceptedPulserEvents->SetMarkerStyle(34);
  _AcceptedPulserEvents->SetMarkerSize(1);
  _AcceptedPulserEvents->SetMarkerColor(4);
  _EventCount = new TH1I("Event Rate", "Event Rate [Hz]; Run Time [s]",60000, 0, 60000);
  _EventCount->SetMarkerStyle(34);
  _EventCount->SetMarkerSize(1);
  _EventCount->SetMarkerColor(4);
  _AvgBeamCurrent = new TH1I("Average Beam Current", "Average Beam Current [mA]; Run Time [s]",60000, 0, 60000);
  _AvgBeamCurrent->SetMarkerStyle(34);
  _AvgBeamCurrent->SetMarkerSize(1);
  _AvgBeamCurrent->SetMarkerColor(2);
  _Scaler1 = new TH1I("Rate Plane Scaler 1", "Rate Plane Scaler 1 [Hz]; Run Time [s]",60000, 0, 60000);
  _Scaler1->SetMarkerStyle(34);
  _Scaler1->SetMarkerSize(1);
  _Scaler1->SetMarkerColor(4);
  _Scaler2 = new TH1I("Rate Plane Scaler 2", "Rate Plane Scaler 2 [Hz]; Run Time [s]",60000, 0, 60000);
  _Scaler2->SetMarkerStyle(34);
  _Scaler2->SetMarkerSize(1);
  _Scaler2->SetMarkerColor(4);


  called = false;
  old_timestamp = 0;
  old_coincidence_count = 0;
  old_coincidence_count_no_sin = 0;
  old_prescaler_count = 0;
  old_prescaler_count_xor_pulser_count = 0;
  old_accepted_prescaled_events = 0;
  old_accepted_pulser_events = 0;
  old_handshake_count = 0;
  old_scaler1 = 0;
  old_scaler2 = 0;
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
  _Scaler1->Write();
  _Scaler2->Write();

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
      uint64_t scaler1 = ev.GetScalerValue(1);
      uint64_t scaler2 = ev.GetScalerValue(2);

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
      _Scaler1->GetXaxis()->SetRangeUser(0, x_axis+10);
      _Scaler2->GetXaxis()->SetRangeUser(0, x_axis+10);


      _CoincidenceCount->Fill(x_axis, 1000*(coincidence_count - old_coincidence_count)/t_diff);
      _CoincidenceCountNoScint->Fill(x_axis, 1000*(coincidence_count_no_sin - old_coincidence_count_no_sin)/t_diff);
      _PrescalerCount->Fill(x_axis, 1000*(prescaler_count - old_prescaler_count)/t_diff);
      _PrescalerXPulser->Fill(x_axis, 1000*(prescaler_count_xor_pulser_count - old_prescaler_count_xor_pulser_count)/t_diff);
      _AcceptedPrescaledEvents->Fill(x_axis, 1000*(accepted_prescaled_events - old_accepted_prescaled_events)/t_diff);
      _AcceptedPulserEvents->Fill(x_axis, 1000*(accepted_pulser_events - old_accepted_pulser_events)/t_diff);
      _EventCount->Fill(x_axis, 1000*(handshake_count - old_handshake_count)/t_diff);
      _AvgBeamCurrent->Fill(x_axis, cal_beam_current);
      _Scaler1->Fill(x_axis, 1000*(scaler1 - old_scaler1)/t_diff);
      _Scaler2->Fill(x_axis, 1000*(scaler2 - old_scaler2)/t_diff);
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
      old_scaler1 = ev.GetScalerValue(1);
      old_scaler2 = ev.GetScalerValue(2);
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
  _Scaler1->Reset();
  _Scaler2->Reset();
}

