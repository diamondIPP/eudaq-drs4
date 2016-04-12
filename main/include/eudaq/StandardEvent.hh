#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
//#include "eudaq/StandardWaveform.hh"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
//#include <TString.h>

namespace eudaq {

class DLLEXPORT StandardWaveform : public Serializable{
public:
	StandardWaveform(unsigned id, const std::string & type,
			const std::string & sensor = "");
	StandardWaveform(Deserializer &);
	StandardWaveform();
	void Serialize(Serializer &) const;
	void SetNSamples(unsigned n_samples);
	unsigned GetNSamples() const {return m_n_samples;}
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
    int getIndexMin(int min, int max) const{
        float* min_el = std::min_element(&m_samples.at(min), &m_samples.at(max));
        return std::distance(&m_samples.at(0),min_el);//-m_samples.begin();
    };
    int getIndexAbsMax(int min,int max) const{
        float mi = getIndexMin(min,max);
        float ma = getIndexMax(min,max);
        return abs(m_samples.at(mi))>abs(m_samples.at(ma))?mi:ma;
    }
    int getIndexMax(int min, int max) const{
        float* max_el = std::max_element(&m_samples.at(min), &m_samples.at(max));
        return std::distance(&m_samples.at(0),max_el);//-m_samples.begin();
    };

    std::pair<int,float> getAbsMaxAndValue(int min, int max) const{
        int index = getIndexAbsMax(min,max);
        return std::make_pair(index,m_samples.at(index));
    }
    float getMedian(int min, int max) const;

    float getSpreadInRange(int min, int max) const{return (getMaxInRange(min,max)-getMinInRange(min,max));};
    float getPeakToPeak(int min, int max) const{return getSpreadInRange(min,max);}
    float getIntegral(int min, int max,bool _abs=false) const;

private:
	uint64_t m_timestamp;
	int m_n_samples;
	int m_channelnumber;
	mutable std::vector<float> m_samples;
	unsigned m_id;
	std::string m_type, m_sensor, m_channelname;
	uint16_t m_trigger_cell;

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

	void SetTimeStamp(uint64_t timestamp){m_timestamp=timestamp;}
	uint64_t GetTimeStamp() const {return m_timestamp;}

	void SetCoincCount(unsigned int cc){coincidence_count=cc;}
	unsigned int GetCoincCount() const {return coincidence_count;}

    void SetCoincCountNoSin(unsigned int cc){coincidence_count_no_sin=cc;}
	unsigned int GetCoincCountNoSin() const {return coincidence_count_no_sin;}

    void SetPrescalerCount(unsigned int cc){prescaler_count=cc;}
	unsigned int GetPrescalerCount() const {return prescaler_count;}

	void SetHandshakeCount(unsigned int cc){handshake_count=cc;}
	unsigned int GetHandshakeCount() const {return handshake_count;}

	void SetBeamCurrent(unsigned int cc){cal_beam_current=cc;}
	unsigned int GetBeamCurrent() const {return cal_beam_current;}

	void SetValid(bool in){m_valid=in;}
	bool GetValid(){return m_valid;}

	std::string GetType() const {return m_type;}
	void Print(std::ostream &) const;

private:
	bool m_valid;
	std::string m_type;
	uint64_t m_timestamp;
	unsigned int coincidence_count;
	unsigned int coincidence_count_no_sin;
    unsigned int prescaler_count;                
    unsigned int handshake_count;
	unsigned int cal_beam_current;
};



class DLLEXPORT StandardEvent : public Event {
	EUDAQ_DECLARE_EVENT(StandardEvent);
public:
	StandardEvent(unsigned run = 0, unsigned evnum = 0,
			uint64_t timestamp = NOTIMESTAMP);
	StandardEvent(const Event &);
	StandardEvent(Deserializer &);
	void SetTimestamp(uint64_t);


	StandardTUEvent & AddTUEvent(const StandardTUEvent &);
	const StandardTUEvent & GetTUEvent(size_t i) const;
	StandardTUEvent & GetTUEvent(size_t i);


	StandardPlane & AddPlane(const StandardPlane &);
	size_t NumPlanes() const;
	const StandardPlane & GetPlane(size_t i) const;
	StandardPlane & GetPlane(size_t i);
	virtual void Serialize(Serializer &) const;
	virtual void Print(std::ostream &) const;

	StandardWaveform & AddWaveform(const StandardWaveform &);
	size_t NumWaveforms() const;
	size_t GetNWaveforms() const {return NumWaveforms();}
	const StandardWaveform & GetWaveform(size_t i) const;
	StandardWaveform & GetWaveform(size_t i);

private:
	std::vector<StandardPlane> m_planes;
	std::vector<StandardWaveform> m_waveforms;
	std::vector<StandardTUEvent> m_tuevents;
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
