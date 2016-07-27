/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUHistos>.hh
** 
** Date: May 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#ifndef TUHISTOS_HH_
#define TUHISTOS_HH_

class TH1I;
class SimpleStandardEvent;
class RootMonitor;
class SimpleStandardTUEvent;

#include <cstdint>

class TUHistos{

  protected:
    TH1I* _CoincidenceCount;
	TH1I* _CoincidenceCountNoScint;
	TH1I* _PrescalerCount;
	TH1I* _PrescalerXPulser;
	TH1I* _AcceptedPrescaledEvents;
	TH1I* _AcceptedPulserEvents;
	TH1I* _EventCount;
	TH1I* _AvgBeamCurrent;
	TH1I* _Scaler1;
	TH1I* _Scaler2;
	uint64_t start_time;
	bool called;

    uint64_t old_timestamp;
    uint32_t old_coincidence_count;
    uint32_t old_coincidence_count_no_sin;
    uint32_t old_prescaler_count;
    uint32_t old_prescaler_count_xor_pulser_count;
    uint32_t old_accepted_prescaled_events;
    uint32_t old_accepted_pulser_events;
    uint32_t old_handshake_count;
    uint64_t old_scaler1;
    uint64_t old_scaler2;


  public:
    TUHistos();
    virtual ~TUHistos();
    void Fill(SimpleStandardTUEvent ev, unsigned int event_nr);
    void Write();
    void Reset();
    TH1I*  getCoincidenceCountHisto(){return _CoincidenceCount;}
    TH1I*  getCoincidenceCountNoScintHisto(){return _CoincidenceCountNoScint;}
	TH1I*  getPrescalerCountHisto(){return _PrescalerCount;}
	TH1I*  getPrescalerXPulserHisto(){return _PrescalerXPulser;}
	TH1I*  getAcceptedPrescaledEventsHisto(){return _AcceptedPrescaledEvents;}
	TH1I*  getAcceptedPulserEventsHisto(){return _AcceptedPulserEvents;}
	TH1I*  getEventCountHisto(){return _EventCount;}
	TH1I*  getAvgBeamCurrentHisto(){return _AvgBeamCurrent;}
	TH1I*  getScaler1Histo(){return _Scaler1;}
	TH1I*  getScaler2Histo(){return _Scaler2;}

};


#ifdef __CINT__
#pragma link C++ class TUHistos-;
#endif

#endif
