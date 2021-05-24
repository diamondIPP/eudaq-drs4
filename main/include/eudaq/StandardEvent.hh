#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
#include "WaveformSignalRegion.hh"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
class TF1;

namespace eudaq {

class DLLEXPORT StandardWaveform : public Serializable{
public:
	StandardWaveform(unsigned id, const std::string & type, const std::string & sensor = "");
	explicit StandardWaveform(Deserializer &);
	StandardWaveform();
	void Serialize(Serializer &) const;
	void SetNSamples(unsigned n_samples);
	uint16_t GetNSamples() const {return m_n_samples;}
	template <typename T>
	void SetWaveform(T (*data)) {//todo: FIx issue with template
		m_samples.clear();
		for (size_t i = 0; i < m_n_samples;i++)
			m_samples.push_back(data[i]);
	} //todo: FIx issue with template
//	void SetWaveform(float* data);
	std::vector<float>* GetData() const{return &m_samples;};
	void SetTriggerCell(uint16_t trigger_cell) {m_trigger_cell=trigger_cell;}
	uint16_t GetTriggerCell() const{return m_trigger_cell;}
	void SetPolarities(signed char polarity, signed char pulser_polarity) { m_polarity = polarity; m_pulser_polarity = pulser_polarity; }
	void SetTimes(std::vector<float> * tcal) { m_times = getCalibratedTimes(tcal); m_t = getTVec();}
	unsigned ID() const;
	void Print(std::ostream &) const;
	std::string GetType() const {return m_type;}
	std::string GetSensor() const {return m_sensor;}
	std::string GetChannelName() const {return m_channelname;};
	void SetChannelName(std::string channelname){m_channelname = channelname;}
	int GetChannelNumber() const {return m_channelnumber;};
	void SetChannelNumber(int channelnumber){m_channelnumber = channelnumber;}
	void SetTimeStamp(uint64_t timestamp){m_timestamp=timestamp;}
	uint64_t GetTimeStamp() const {return m_timestamp;}
//	std::string GetName() const {return m_sensor+(std::string)"_"+m_type+(std::string)to_string(m_id);}
    float getMinInRange(int min, int max) const{
        return (*std::min_element(&m_samples.at(min), &m_samples.at(max)));
    };
    float getMaxInRange(int min, int max) const{
        return (*std::max_element(&m_samples.at(min), &m_samples.at(max)));
    };
    float getAbsMaxInRange(int min,int max) const{
        return abs(m_samples.at(getIndexAbsMax(min,max)));
    }
	uint16_t getIndexMin(int min, int max) const{
        float* min_el = std::min_element(&m_samples.at(min), &m_samples.at(max));
        return uint16_t(std::distance(&m_samples.at(0),min_el));//-m_samples.begin();
    };
    int getIndexAbsMax(int min,int max) const{
        float mi = getIndexMin(min,max);
        float ma = getIndexMax(min,max);
        return abs(m_samples.at(mi))>abs(m_samples.at(ma))?mi:ma;
    }
	uint16_t getIndexMax(int min, int max) const{
        float* max_el = std::max_element(&m_samples.at(min), &m_samples.at(max));
        return uint16_t(std::distance(&m_samples.at(0),max_el));//-m_samples.begin();
    };

	uint16_t getIndex(uint16_t min, uint16_t max, signed char pol) const {
		return (pol * 1 > 0) ? getIndexMax(min, max) : getIndexMin(min, max);
	}

	float getCFT(uint16_t min, uint16_t max, uint8_t delay=4, float factor=.5) const;  // return the constant fraction time of the signal in [min, max]

    std::pair<int,float> getAbsMaxAndValue(int min, int max) const{
        int index = getIndexAbsMax(min,max);
        return std::make_pair(index,m_samples.at(index));
    }
    float getMedian(uint32_t min, uint32_t max) const;

		std::vector<uint16_t> * getAllPeaksAbove(uint16_t min, uint16_t max, float threshold) const;
		std::pair<uint16_t, float> getMaxPeak() const;
    float getSpreadInRange(int min, int max) const{return (getMaxInRange(min,max)-getMinInRange(min,max));};
    float getPeakToPeak(int min, int max) const{return getSpreadInRange(min,max);}
    float getAverage(uint16_t min, uint16_t max, bool _abs=false) const;
    float getIntegral(uint16_t, uint16_t, uint16_t, float) const;
    float getIntegral(uint16_t peak_pos, std::pair<uint16_t, uint16_t> range, float sspeed) const;
    float getIntegral(WaveformSignalRegion*, WaveformIntegral*, float, int16_t off=0) const;
		std::vector<float> getCalibratedTimes(std::vector<float>*) const;
		std::vector<float> getTVec() const;
    float getTriggerTime(std::vector<float>*) const;
    float getPeakFit(uint16_t, uint16_t, signed char) const;
    TF1 getRFFit(std::vector<float>*) const;
    TF1 getErfFit(uint16_t, uint16_t, signed char) const;
    float getRiseTime(uint16_t bin_low, uint16_t bin_high, float noise) const;
    float getFallTime(uint16_t bin_low, uint16_t bin_high, float noise) const;
    float getWFStartTime(uint16_t bin_low, uint16_t bin_high, float noise, float max_value) const;
    std::pair<float, float> fitMaximum(uint16_t bin_low, uint16_t bin_high) const;
    float interpolateTime(uint16_t ibin, float value) const;
    float getBinWidth(uint16_t ibin) const { return m_times.at(ibin) - m_times.at(uint16_t(ibin - 1)); }
    uint16_t FindPeakIndex(uint16_t , uint16_t, float, signed char) const;

private:
	uint64_t m_timestamp;
	uint16_t m_n_samples;
	int m_channelnumber;
	mutable std::vector<float> m_samples;
	unsigned m_id;
	std::string m_type, m_sensor, m_channelname;
	uint16_t m_trigger_cell;
	signed char m_polarity;
	signed char m_pulser_polarity;
	std::vector<float> m_times;
	std::vector<float> m_t;

};

class DLLEXPORT StandardPlane : public Serializable {
public:
	enum FLAGS { FLAG_ZS = 0x1, // Data are zero suppressed
		FLAG_NEEDCDS = 0x2, // CDS needs to be calculated (data is 2 or 3 raw frames)
		FLAG_NEGATIVE = 0x4, // Signal polarity is negative
		FLAG_ACCUMULATE = 0x8, // Multiple frames should be accumulated for output

		FLAG_WITHPIVOT = 0x10000, // Include before/after pivot boolean per pixel
		FLAG_WITHSUBMAT = 0x20000, // Include Submatrix ID per pixel
		FLAG_DIFFCOORDS = 0x40000 // Each frame can have different coordinates (in ZS mode)
	};
	typedef double pixel_t;
	typedef double coord_t;
	StandardPlane(unsigned id, const std::string & type,
			const std::string & sensor = "");
	StandardPlane(Deserializer &);
	StandardPlane();
	void Serialize(Serializer &) const;
	void SetSizeRaw(unsigned w, unsigned h,
			unsigned frames = 1, int flags = 0);
	void SetSizeZS(unsigned w, unsigned h, unsigned npix,
			unsigned frames = 1, int flags = 0);

	template <typename T>
	void SetPixel(unsigned index, unsigned x, unsigned y,
			T pix, bool pivot = false, unsigned frame = 0) {
		SetPixelHelper(index, x, y, (double)pix, pivot, frame);
	}
	template <typename T>
	void SetPixel(unsigned index, unsigned x, unsigned y,
			T pix, unsigned frame) {
		SetPixelHelper(index, x, y, (double)pix, false, frame);
	}
	template <typename T>
	void PushPixel(unsigned x, unsigned y, T pix,
			bool pivot = false, unsigned frame = 0) {
		PushPixelHelper(x, y, (double)pix, pivot, frame);
	}
	template <typename T>
	void PushPixel(unsigned x, unsigned y, T pix, unsigned frame) {
		PushPixelHelper(x, y, (double)pix, false, frame);
	}

	void SetPixelHelper(unsigned index, unsigned x, unsigned y,
			double pix, bool pivot, unsigned frame);
	void PushPixelHelper(unsigned x, unsigned y,
			double pix, bool pivot, unsigned frame);
	double GetPixel(unsigned index, unsigned frame) const;
	double GetPixel(unsigned index) const;
	double GetX(unsigned index, unsigned frame) const;
	double GetX(unsigned index) const;
	double GetY(unsigned index, unsigned frame) const;
	double GetY(unsigned index) const;
	bool GetPivot(unsigned index, unsigned frame = 0) const;
	void SetPivot(unsigned index, unsigned frame , bool PivotFlag);
	// defined for short, int, double
	template <typename T>
	std::vector<T> GetPixels() const{
		SetupResult();
		std::vector<T> result(m_result_pix->size());
		for (size_t i = 0; i < result.size(); ++i) {
			result[i] = static_cast<T>((*m_result_pix)[i] * Polarity());
		}
		return result;
	}
	const std::vector<coord_t> & XVector(unsigned frame) const;
	const std::vector<coord_t> & XVector() const;
	const std::vector<coord_t> & YVector(unsigned frame) const;
	const std::vector<coord_t> & YVector() const;
	const std::vector<pixel_t> & PixVector(unsigned frame) const;
	const std::vector<pixel_t> & PixVector() const;

	void SetXSize(unsigned x);
	void SetYSize(unsigned y);
	void SetTLUEvent(unsigned ev);
	void SetPivotPixel(unsigned p);
	void SetFlags(FLAGS flags);

	unsigned ID() const;
	const std::string & Type() const;
	const std::string & Sensor() const;
	unsigned XSize() const;
	unsigned YSize() const;
	unsigned NumFrames() const;
	unsigned TotalPixels() const;
	unsigned HitPixels(unsigned frame) const;
	unsigned HitPixels() const;
	unsigned TLUEvent() const;
	unsigned PivotPixel() const;

	int GetFlags(int f) const;
	bool NeedsCDS() const;
	int  Polarity() const;

	void Print(std::ostream &) const;
	//Trigger Time Tag  - useful for CMS Pixel Detectors using TriggerCount() from header there
	void SetTrigCount(unsigned trigger_count) {m_trigger_count = trigger_count;};
	unsigned GetTrigCount() const {return m_trigger_count;}
	void SetTrigPhase(unsigned trigger_phase) {m_trigger_phase = trigger_phase;};
	unsigned GetTrigPhase() const {return m_trigger_phase;}
private:
	const std::vector<pixel_t> & GetFrame(const std::vector<std::vector<pixel_t> > & v, unsigned f) const;
	void SetupResult() const;
	std::string m_type, m_sensor;
	unsigned m_id, m_tluevent;
	unsigned m_xsize, m_ysize;
	unsigned m_flags, m_pivotpixel;
	unsigned m_trigger_count;
	unsigned m_trigger_phase;
	std::vector<std::vector<pixel_t> > m_pix;
	std::vector<std::vector<coord_t> > m_x, m_y;
	std::vector<std::vector<bool> > m_pivot;
	std::vector<unsigned> m_mat;

	mutable const std::vector<pixel_t> * m_result_pix;
	mutable const std::vector<coord_t> * m_result_x, * m_result_y;

	mutable std::vector<pixel_t> m_temp_pix;
	mutable std::vector<coord_t> m_temp_x, m_temp_y;
};


//added April 2016, cdorfer
class DLLEXPORT StandardTUEvent : public Serializable{
public:
	StandardTUEvent(const std::string & type);
	StandardTUEvent(Deserializer &);
	StandardTUEvent();
	void Serialize(Serializer &) const;

	void SetTimeStamp(uint64_t timestamp){ m_timestamp=timestamp; }
	uint64_t GetTimeStamp() const { return m_timestamp;}

	void SetCoincCount(uint32_t cc){coincidence_count=cc;}
	uint32_t GetCoincCount() const {return coincidence_count;}

    void SetCoincCountNoSin(uint32_t cc){coincidence_count_no_sin=cc;}
	uint32_t GetCoincCountNoSin() const {return coincidence_count_no_sin;}

    void SetPrescalerCount(uint32_t cc){prescaler_count=cc;}
	uint32_t GetPrescalerCount() const {return prescaler_count;}

	void SetPrescalerCountXorPulserCount(uint32_t cc){prescaler_count_xor_pulser_count=cc;}
	uint32_t GetPrescalerCountXorPulserCount() const {return prescaler_count_xor_pulser_count;}

	void SetAcceptedPrescaledEvents(uint32_t cc){accepted_prescaled_events=cc;}
	uint32_t GetAcceptedPrescaledEvents() const {return accepted_prescaled_events;}

	void SetAcceptedPulserCount(uint32_t cc){accepted_pulser_events=cc;}
	uint32_t GetAcceptedPulserCount() const {return accepted_pulser_events;}

	void SetHandshakeCount(uint32_t cc){handshake_count=cc;}
	uint32_t GetHandshakeCount() const {return handshake_count;}

	void SetBeamCurrent(uint32_t cc){cal_beam_current=cc;}
	uint32_t GetBeamCurrent() const {return cal_beam_current;}

	void SetScalerValue(int idx, uint64_t val){scaler_values[idx] = val;}
	uint64_t GetScalerValue(int idx) const {return scaler_values[idx];}

	void SetValid(bool in){m_valid=in;}
	bool GetValid() const {return m_valid;}

	std::string GetType() const {
		return m_type;}

	void Print(std::ostream &) const;

private:
	bool m_valid;
	std::string m_type;
	uint64_t m_timestamp;
	uint32_t coincidence_count;
	uint32_t coincidence_count_no_sin;
    uint32_t prescaler_count; 
    uint32_t prescaler_count_xor_pulser_count;
    uint32_t accepted_prescaled_events;
    uint32_t accepted_pulser_events;
    uint32_t handshake_count;
	uint32_t cal_beam_current;
	uint64_t scaler_values[10];
};



class DLLEXPORT StandardEvent : public Event {
	EUDAQ_DECLARE_EVENT(StandardEvent);
public:
	StandardEvent(unsigned run = 0, unsigned evnum = 0,
			uint64_t timestamp = NOTIMESTAMP);
	StandardEvent(const Event &);
	StandardEvent(Deserializer &);
	void SetTimestamp(uint64_t);

	//also implemented as vector, even though there will most likely only be one event per event ;)
	StandardTUEvent & AddTUEvent(const StandardTUEvent &);
	size_t NumTUEvents() const;
	const StandardTUEvent & GetTUEvent(size_t i) const;
	StandardTUEvent & GetTUEvent(size_t i);


	StandardPlane & AddPlane(const StandardPlane &);
	size_t NumPlanes() const;
	const StandardPlane & GetPlane(size_t i) const;
	StandardPlane & GetPlane(size_t i);
	virtual void Serialize(Serializer &) const;
	virtual void Print(std::ostream &) const;

	StandardWaveform & AddWaveform(const StandardWaveform &);
	uint16_t NumWaveforms() const { return uint16_t(m_waveforms.size()); }
	uint16_t GetNWaveforms() const {return NumWaveforms();}
	const StandardWaveform & GetWaveform(size_t i) const;
	StandardWaveform & GetWaveform(size_t i);
	bool hasTUEvent();


private:
	std::vector<StandardPlane> m_planes;
	std::vector<StandardWaveform> m_waveforms;
	std::vector<StandardTUEvent> m_tuevent;
};


inline std::ostream & operator << (std::ostream & os, const StandardTUEvent & tuev) {
	tuev.Print(os);
	return os;
}

inline std::ostream & operator << (std::ostream & os,
		const StandardWaveform & wf) {
	wf.Print(os);
	return os;
}
inline std::ostream & operator << (std::ostream & os,
		const StandardPlane & pl) {
	pl.Print(os);
	return os;
}
inline std::ostream & operator << (std::ostream & os,
		const StandardEvent & ev) {
	ev.Print(os);
	return os;
}

} // namespace eudaq

#endif // EUDAQ_INCLUDED_StandardEvent
