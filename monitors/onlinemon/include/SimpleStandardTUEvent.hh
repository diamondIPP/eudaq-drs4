/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <SimpleStandardTUEvent>.hh
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#ifndef SimpleStandardTUEvent_HH
#define SimpleStandardTUEvent_HH

class OnlineMonConfiguration;

#include <string>


class SimpleStandardTUEvent{
protected:
	std::string name;

public:
	SimpleStandardTUEvent(const std::string &name, OnlineMonConfiguration* mymon);
	SimpleStandardTUEvent(const std::string &name);
	SimpleStandardTUEvent();
	virtual ~SimpleStandardTUEvent() {};


	void SetTimeStamp(unsigned long timestamp){m_timestamp=timestamp;}
	unsigned long GetTimeStamp() const {return m_timestamp;}

	void SetCoincCount(unsigned int cc){coincidence_count=cc;}
	unsigned int GetCoincCount() const {return coincidence_count;}

    void SetCoincCountNoSin(unsigned int cc){coincidence_count_no_sin=cc;}
	unsigned int GetCoincCountNoSin() const {return coincidence_count_no_sin;}

    void SetPrescalerCount(unsigned int cc){prescaler_count=cc;}
	unsigned int GetPrescalerCount() const {return prescaler_count;}

	void SetPrescalerCountXorPulserCount(unsigned int cc){prescaler_count_xor_pulser_count=cc;}
	unsigned int GetPrescalerCountXorPulserCount() const {return prescaler_count_xor_pulser_count;}

	void SetAcceptedPrescaledEvents(unsigned int cc){accepted_prescaled_events=cc;}
	unsigned int GetAcceptedPrescaledEvents() const {return accepted_prescaled_events;}

	void SetAcceptedPulserCount(unsigned int cc){accepted_pulser_events=cc;}
	unsigned int GetAcceptedPulserCount() const {return accepted_pulser_events;}

	void SetHandshakeCount(unsigned int cc){handshake_count=cc;}
	unsigned int GetHandshakeCount() const {return handshake_count;}

	void SetBeamCurrent(unsigned int cc){cal_beam_current=cc;}
	unsigned int GetBeamCurrent() const {return cal_beam_current;}

	void SetScalerValue(int idx, unsigned long val){scaler_values[idx] = val;}
	unsigned long GetScalerValue(int idx){return scaler_values[idx];}

	void SetValid(bool in){m_valid=in;}
	bool GetValid(){return m_valid;}


private:
	OnlineMonConfiguration* mon;
	bool m_valid;
	std::string m_type;
	unsigned long m_timestamp;
	unsigned int coincidence_count;
	unsigned int coincidence_count_no_sin;
    unsigned int prescaler_count; 
    unsigned int prescaler_count_xor_pulser_count;
    unsigned int accepted_prescaled_events;
    unsigned int accepted_pulser_events;
    unsigned int handshake_count;
	unsigned int cal_beam_current;
	unsigned long scaler_values[10];



};

#endif