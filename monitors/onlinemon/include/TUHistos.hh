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
class TString;

#include <cstdint>
#include <vector>

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
  std::vector<TH1I*> _Scaler;
	uint64_t start_time;
	bool called;
  uint8_t n_scaler;

	uint64_t old_timestamp;
	uint32_t old_coincidence_count;
	uint32_t old_coincidence_count_no_sin;
	uint32_t old_prescaler_count;
	uint32_t old_prescaler_count_xor_pulser_count;
	uint32_t old_accepted_prescaled_events;
	uint32_t old_accepted_pulser_events;
	uint32_t old_handshake_count;
  std::vector<uint64_t> old_scaler;
	uint32_t old_xaxis;


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
	TH1I*  getScalerHisto(int i_scaler){return _Scaler.at(i_scaler);}
  uint8_t getNScaler() { return n_scaler; }

private:
		TH1I * create_th1i(TString, TString, int nbins=60000, int xmin=0, int xmax=60000, int markerstyle=34, int markersize=1, int markercolor=4);

};


#ifdef __CINT__
#pragma link C++ class TUHistos-;
#endif

#endif
