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

  _CoincidenceCount = new TH1I("Concidence Rate", "Coincidence Rate [Hz]; Run Time [s]",500, 0, 500);
  _CoincidenceCountNoScint = new TH1I("Coincidence Rate No Scintillator", "Coincidence Rate No Scintillator [Hz]; Run Time [s]",500, 0, 500);
  _PrescalerCount = new TH1I("Prescaler Rate", "Prescaler Rate [Hz]; Run Time [s]",500, 0, 500);
  _PrescalerXPulser = new TH1I("Prescaler Xor Pulser Rate", "Prescaler Xor Pulser Rate [Hz]; Run Time [s]",500, 0, 500);
  _AcceptedPrescaledEvents = new TH1I("Accepted Prescaled Event Rate", "Accepted Prescaled Rate [Hz]; Run Time [s]",500, 0, 500);
  _AcceptedPulserEvents = new TH1I("Accepted Pulser Event Rate", "Accepted Pulser Event Rate [Hz]; Run Time [s]",500, 0, 500);
  _EventCount = new TH1I("Event Rate", "Event Rate [Hz]; Run Time [s]",500, 0, 500);
  _AvgBeamCurrent = new TH1I("Average Beam Current", "Average Beam Current [mA]; Run Time [s]",500, 0, 500);
  _Scaler1 = new TH1I("Rate Plane Scaler 1", "Rate Plane Scaler 1 [Hz]; Run Time [s]",500, 0, 500);
  _Scaler2 = new TH1I("Rate Plane Scaler 2", "Rate Plane Scaler 2 [Hz]; Run Time [s]",500, 0, 500);


 	_CoincidenceCount->SetBit(TH1::kCanRebin);
	_CoincidenceCountNoScint->SetBit(TH1::kCanRebin);
	_PrescalerCount->SetBit(TH1::kCanRebin);
	_PrescalerXPulser->SetBit(TH1::kCanRebin);
	_AcceptedPrescaledEvents->SetBit(TH1::kCanRebin);
	_AcceptedPulserEvents->SetBit(TH1::kCanRebin);
	_EventCount->SetBit(TH1::kCanRebin);
	_AvgBeamCurrent->SetBit(TH1::kCanRebin);
	_Scaler1->SetBit(TH1::kCanRebin);
	_Scaler2->SetBit(TH1::kCanRebin);

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
      uint64_t scaler1 = ev.GetScalerValue(0);
      uint64_t scaler2 = ev.GetScalerValue(1);

      uint32_t x_axis = (uint32_t) (new_timestamp - start_time)/500;
      uint32_t t_diff = (uint32_t) (new_timestamp - old_timestamp);

      //std::cout << "Prescaler difference: " << (coincidence_count - old_coincidence_count) << std::endl;
      //std::cout << "Old timestamp: " << old_timestamp << ", new timestamp: " << new_timestamp << std::endl;
      //std::cout << "X-Achse: " << x_axis << ", zeit differenz: " << t_diff << std::endl;

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

      old_timestamp = ev.GetTimeStamp();
      old_coincidence_count = ev.GetCoincCount();
      old_coincidence_count_no_sin = ev.GetCoincCountNoSin();
      old_prescaler_count = ev.GetPrescalerCount();
      old_prescaler_count_xor_pulser_count = ev.GetPrescalerCountXorPulserCount();
      old_accepted_prescaled_events = ev.GetAcceptedPrescaledEvents();
      old_accepted_pulser_events = ev.GetAcceptedPulserCount();
      old_handshake_count = ev.GetHandshakeCount();
      old_scaler1 = ev.GetScalerValue(0);
      old_scaler2 = ev.GetScalerValue(1);

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

