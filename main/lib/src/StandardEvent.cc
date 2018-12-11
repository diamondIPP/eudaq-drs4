#include "eudaq/StandardEvent.hh"
#include <TMath.h>

#include <cmath>
#include "TF1.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TFitResult.h"
#include "TGraphErrors.h"
#include <algorithm>

using namespace std;

namespace eudaq {

  EUDAQ_DEFINE_EVENT(StandardEvent, str2id("_STD"));

/************************************************************************************************/
/************************************* Standard Waveform ****************************************/
/************************************************************************************************/

eudaq::StandardWaveform::StandardWaveform(unsigned id, const std::string &type,
                                          const std::string &sensor) : m_channelnumber(-1) {
  m_id = id;
  m_type = type;
  m_sensor = sensor;
}

eudaq::StandardWaveform::StandardWaveform(Deserializer &ds) : m_samples(0), m_channelnumber(-1) {
  ds.read(m_type);
  ds.read(m_sensor);
  ds.read(m_id);
  ds.read(m_channelnumber);
}

eudaq::StandardWaveform::StandardWaveform() : m_id(0), m_sensor(0), m_type(0), m_channelnumber(-1) {}

void eudaq::StandardWaveform::Serialize(Serializer &ser) const {
  ser.write(m_type);
  ser.write(m_sensor);
  ser.write(m_id);
  ser.write(m_channelnumber);
}

void eudaq::StandardWaveform::SetNSamples(unsigned n_samples) {
  m_n_samples = n_samples;
}

void StandardWaveform::Print(std::ostream &os) const {
  os << m_id << ", " << m_type << ":" << m_sensor << ", [";
  for (size_t i = 0; i < m_samples.size() && i < m_n_samples; i++) {
    os << m_samples[i] << " ";
  }
  os << "]";
}

unsigned StandardWaveform::ID() const {
  return m_id;
}


float StandardWaveform::getIntegral(uint16_t min, uint16_t max, bool _abs) const {
  if (max > this->GetNSamples() - 1) max = uint16_t(this->GetNSamples() - 1);
  if (min < 0) min = 0;
  float integral = 0;
  for (uint16_t i = min; i <= max; i++) {
    if (!_abs)
      integral += m_samples.at(i);
    else
      integral += std::abs(m_samples.at(i));
  }
  return integral / (float) (max - (int) min);
}

float StandardWaveform::getIntegral(uint16_t low_bin, uint16_t high_bin, uint16_t peak_pos, float sspeed) const {

  high_bin = min(high_bin, uint16_t(m_n_samples - 1));
  if (high_bin == peak_pos and low_bin == peak_pos)
    return m_samples.at(peak_pos);
  float max_low_length = (peak_pos - low_bin) / sspeed;
  float max_high_length = (high_bin - peak_pos) / sspeed;
  float integral = 0;  // take the value at the peak pos as start value
  // sum up the times if the bins to the left side of the peak pos until max length is reached
  float low_length(0), high_length(0);
  uint16_t ibin(peak_pos);
  while (low_length <= max_low_length - 0.001) {
    float width = min(getBinWidth(--ibin), max_low_length - low_length);  // take the diff between max und current length if it gets smaller than bin width
    integral += width * (m_samples.at(ibin) + m_samples.at(ibin + 1)) / 2;
    low_length += width;
  }
  ibin = uint16_t(peak_pos - 1);
  while (high_length <= max_high_length - 0.001) {
    float width = min(getBinWidth(++ibin), max_high_length - high_length);  // take the diff between max und current length if it gets smaller than bin width
    integral += width * (m_samples.at(ibin) + m_samples.at(ibin + 1)) / 2;
    high_length += width;
  }
  return integral / (max_high_length + max_low_length);
}

TF1 StandardWaveform::getRFFit(std::vector<float> *tcal) const {

  std::vector<float> t = getCalibratedTimes(tcal);
  TGraph gr = TGraph(unsigned(t.size()), &t[0], &m_samples[0]);
  TF1 fit("rf_fit", "[0] * TMath::Sin((x+[2])*2*pi/[1])+[3]", 0, 1000);
  fit.SetParameters(100, 20, 3, -40);
  fit.SetParLimits(0, 10, 500);
  fit.SetParLimits(2, -35, 35);
  gr.Fit("rf_fit", "q");
  return fit;
}

std::vector<float> StandardWaveform::getCalibratedTimes(std::vector<float> *tcal) const {

  std::vector<float> t = {tcal->at(m_trigger_cell)};
  for (uint16_t i(1); i < m_n_samples; i++)
    t.push_back(tcal->at(unsigned((m_trigger_cell + i) % m_n_samples)) + t.back());
  return t;
}

float StandardWaveform::getPeakFit(uint16_t bin_low, uint16_t bin_high, signed char pol) const {

  uint16_t high_bin = getIndex(bin_low, bin_high, pol);
  float t_high = m_times.at(high_bin);
  vector<float> t = vector<float>(m_times.begin() + high_bin - 50, m_times.begin() + high_bin + 5);
  std::vector<float> v = std::vector<float>(m_samples.begin() + high_bin - 50, m_samples.begin() + high_bin + 5);
  TGraph gr = TGraph(unsigned(t.size()), &t[0], &v[0]);
  TF1 fit("fit", "[0]*TMath::Landau(x, [1], [2]) - [3]", 0, 500);
  fit.SetParameters(pol * 1000, t_high, 5, pol * 5);
  gr.Fit("fit", "q");
  return float(fit.GetParameter(1));
}

TF1 StandardWaveform::getErfFit(uint16_t bin_low, uint16_t bin_high, signed char pol) const {

  TF1 fit("fit", "[0]*TMath::Erf((x-[1])*[2]) + [3]", 0, 500);
  if (getAbsMaxInRange(bin_low, bin_high) > 2000) {
    fit.SetParameters(0, 0, 0, 0);
    return fit;
  }
  uint16_t high_bin = getIndex(bin_low, bin_high, pol);
  float t_high = m_times.at(high_bin);
  TGraph gr = TGraph(unsigned(m_times.size()), &m_times[0], &m_samples[0]);
  fit.SetParameters(100, t_high, pol * .5, pol * 100);
  fit.SetParLimits(0, 10, 500);
  fit.SetParLimits(1, t_high - 20, t_high + 2);
  gr.Fit("fit", "q", "", t_high - 20, t_high + 1);
  return fit;
}

float StandardWaveform::interpolateTime(uint16_t ibin, float value) const {

  /** v = mt + a */
  float m = (m_samples.at(ibin) - m_samples.at(uint16_t(ibin - 1))) / (m_times.at(ibin) - m_times.at(uint16_t(ibin - 1)));
  float a = m_samples.at(ibin) - m * m_times.at(ibin);
	return (value - a) / m;
}

float StandardWaveform::getRiseTime(uint16_t bin_low, uint16_t bin_high, float noise) const {

  uint16_t max_index = getIndex(bin_low, bin_high, m_polarity);
  float max_value = m_samples.at(max_index) - noise;
  float t_start = m_times.at(bin_low);
  float t_stop = m_times.at(uint16_t(max_index - 1));
  bool found_stop(false);
  for (uint16_t i(max_index); i > bin_low; i--) {
    if (fabs(m_samples.at(i) - noise) < std::fabs(max_value) * .8 and not found_stop) {
      t_stop = interpolateTime(i, float(.8 * max_value));
      found_stop = true;
    }
    if (fabs(m_samples.at(i) - noise) < std::fabs(max_value) * .2) {
      t_start = interpolateTime(i, float(.2 * max_value));
      break;
    }
  }
  return t_stop - t_start;
}

float StandardWaveform::getFallTime(uint16_t bin_low, uint16_t bin_high, float noise) const {

  uint16_t max_index = getIndex(bin_low, bin_high, m_polarity);
  float t_start = m_times.at(uint16_t(max_index + 1));
  float t_stop = m_times.at(bin_high);
  float max_value = m_samples.at(max_index) - noise;
  bool found_start(false);
  for (uint16_t i(max_index); i < bin_high; i++) {
    if (fabs(m_samples.at(i) - noise) <= std::fabs(max_value) * .8 and not found_start) {
      t_start = interpolateTime(i, float(.8 * max_value));
      found_start = true;
    }
    if (fabs(m_samples.at(i) - noise) <= std::fabs(max_value) * .2) {
      t_stop = interpolateTime(i, float(.2 * max_value));
      break;
    }
  }
  return t_stop - t_start;
}

float StandardWaveform::getWFStartTime(uint16_t bin_low, uint16_t bin_high, float noise, float max_value) const {

  uint16_t max_index = getIndex(bin_low, bin_high, m_polarity);
  TGraphErrors g;
  uint16_t i_point(0);
  float max = std::fabs(max_value) - noise;
  for (uint16_t i(max_index); i > bin_low; i--)
    if (max * .2 <= fabs(m_samples.at(i) - noise) and fabs(m_samples.at(i) - noise) <= max * .8)
      g.SetPoint(i_point++, m_times.at(i), m_samples.at(i));
  TF1 fit("fit", "pol1", m_times.at(bin_low), m_times.at(bin_high));
  g.Fit(&fit, "q");
  return float(fit.GetX(noise));
}

std::pair<float, float> StandardWaveform::fitMaximum(uint16_t bin_low, uint16_t bin_high) const {

	TGraph gr;
	TF1 fit("landau", "landau + [3]");
	uint16_t max_index = getIndex(bin_low, bin_high, m_polarity);
	for (auto i(uint16_t(max_index - 6)); i <= max_index + 6; i++)
		gr.SetPoint(i - max_index + 6, m_times.at(i), m_samples.at(i));
	fit.SetParameters(m_polarity * 300, m_times.at(max_index), 2, 0);
	gr.Fit(&fit, "q");
	return make_pair(fit.GetParameter(1), fit(fit.GetParameter(1)));
}

float StandardWaveform::getTriggerTime(std::vector<float> * tcal) const {

  float min = calc_mean(std::vector<float>(m_samples.begin() + 5, m_samples.begin() + 15)).first;
  float max = calc_mean(std::vector<float>(m_samples.end() - 15, m_samples.end() - 5)).first;
  float half = (max + min) / 2;
  std::pair<float, float> p1, p2;
  for (uint16_t i(5); i < m_n_samples; i++){
    if (m_samples.at(i) > half){
      p1 = std::make_pair(getCalibratedTimes(tcal).at(uint16_t(i - 1)), m_samples.at(uint16_t(i - 1)));
      p2 = std::make_pair(getCalibratedTimes(tcal).at(i), m_samples.at(i));
      break;
    }
  }
  // take a straight line y = mx + a through the two points and calculate at which time it's a the half point
  float m = (p2.second - p1.second) / (p2.first - p1.first);
  float a = p1.second - m * p1.first;
  return (half - a) / m;
}

std::pair<uint16_t, float> StandardWaveform::getMaxPeak() const {
    auto max = std::max_element(m_samples.begin(), m_samples.end());
    auto min = std::min_element(m_samples.begin(), m_samples.end());
    auto peak = (abs(int(*max)) > abs(int(*min))) ? max : min;
    return std::make_pair(uint16_t(std::distance(m_samples.begin(), peak)), *peak);
}

std::vector<uint16_t> * StandardWaveform::getAllPeaksAbove(uint16_t min, uint16_t max, float threshold) const {

	std::vector<uint16_t> * peak_positions = new std::vector<uint16_t>;
  uint16_t low(0), high;

	for (uint16_t j = uint16_t(min + 1); j <= max - 1; j++){
    float val = std::abs(m_samples.at(j));
    //find point when waveform gets above threshold
    if (val > threshold and std::abs(m_samples.at(uint16_t(j - 1))) < threshold)
      low = j;
    //find point when waveform gets below threshold and find max in between
    if (val > threshold and std::abs(m_samples.at(uint16_t(j + 1))) < threshold){
      high = j;
      // the peak has to have a certain width -> avoid spikes
      if (high > low + 5){
        auto min = std::min_element(m_samples.begin() + low, m_samples.begin() + high);
        auto max = std::max_element(m_samples.begin() + low, m_samples.begin() + high);
        auto peak = (std::abs((*max)) > std::abs(*min)) ? max : min;
        uint16_t pos = uint16_t(std::distance(m_samples.begin(), peak));
        if (not peak_positions->size())
          peak_positions->push_back(pos);
          // value has to be at least in the next bunch
        else if (pos > peak_positions->back() + 35)
          peak_positions->push_back(pos);
      }
    }
  }
	return peak_positions;
}

float StandardWaveform::getMedian(uint32_t min, uint32_t max) const
{
    float median;
    int n = abs(max - min + 1);
    median = (float)TMath::Median(n, &m_samples.at(min));
    return median;
}

/************************************************************************************************/
/*************************************** Standard Plane *****************************************/
/************************************************************************************************/

StandardPlane::StandardPlane() : m_id(0), m_tluevent(0), m_xsize(0), m_ysize(0), m_flags(0), m_pivotpixel(0), m_result_pix(0), m_result_x(0), m_result_y(0) {}

StandardPlane::StandardPlane(unsigned id, const std::string & type, const std::string & sensor)
: m_type(type), m_sensor(sensor), m_id(id), m_tluevent(0), m_xsize(0), m_ysize(0),
  m_flags(0), m_pivotpixel(0), m_result_pix(0), m_result_x(0), m_result_y(0)
{}

StandardPlane::StandardPlane(Deserializer & ds) : m_result_pix(0), m_result_x(0), m_result_y(0) {
	ds.read(m_type);
	ds.read(m_sensor);
	ds.read(m_id);
	ds.read(m_tluevent);
	ds.read(m_xsize);
	ds.read(m_ysize);
	ds.read(m_flags);
	ds.read(m_pivotpixel);
	ds.read(m_pix);
	ds.read(m_x);
	ds.read(m_y);
	ds.read(m_pivot);
	ds.read(m_mat);
}

void StandardPlane::Serialize(Serializer & ser) const {
	ser.write(m_type);
	ser.write(m_sensor);
	ser.write(m_id);
	ser.write(m_tluevent);
	ser.write(m_xsize);
	ser.write(m_ysize);
	ser.write(m_flags);
	ser.write(m_pivotpixel);
	ser.write(m_pix);
	ser.write(m_x);
	ser.write(m_y);
	ser.write(m_pivot);
	ser.write(m_mat);
}

// StandardPlane::StandardPlane(size_t pixels, size_t frames)
//   : m_pix(frames, std::vector<pixel_t>(pixels)), m_x(pixels), m_y(pixels), m_pivot(pixels)
// {
//   //
// }

void StandardPlane::Print(std::ostream & os) const {
	os << m_id << ", " << m_type << ":" << m_sensor << ", " << m_xsize << "x" << m_ysize << "x" << m_pix.size()
    				  << " (" << (m_pix.size() ? m_pix[0].size() : 0) << "), tlu=" << m_tluevent << ", pivot=" << m_pivotpixel;
}

void StandardPlane::SetSizeRaw(unsigned w, unsigned h, unsigned frames, int flags) {
	m_flags = flags;
	SetSizeZS(w, h, w*h, frames, flags);
	m_flags &= ~FLAG_ZS;
}

void StandardPlane::SetSizeZS(unsigned w, unsigned h, unsigned npix, unsigned frames, int flags) {
	m_flags = flags | FLAG_ZS;
	//std::cout << "DBG flags " << hexdec(m_flags) << std::endl;
	m_xsize = w;
	m_ysize = h;
	m_pix.resize(frames);
	m_x.resize(GetFlags(FLAG_DIFFCOORDS) ? frames : 1);
	m_y.resize(GetFlags(FLAG_DIFFCOORDS) ? frames : 1);
	m_pivot.resize(GetFlags(FLAG_WITHPIVOT) ? (GetFlags(FLAG_DIFFCOORDS) ? frames : 1) : 0);
	for (size_t i = 0; i < frames; ++i) {
		m_pix[i].resize(npix);
	}
	for (size_t i = 0; i < m_x.size(); ++i) {
		m_x[i].resize(npix);
		m_y[i].resize(npix);
		if (m_pivot.size()) m_pivot[i].resize(npix);
	}
}

void StandardPlane::PushPixelHelper(unsigned x, unsigned y, double p, bool pivot, unsigned frame) {
	if (frame > m_x.size()) EUDAQ_THROW("Bad frame number " + to_string(frame) + " in PushPixel");
	m_x[frame].push_back(x);
	m_y[frame].push_back(y);
	m_pix[frame].push_back(p);
	if (m_pivot.size()) m_pivot[frame].push_back(pivot);
	//std::cout << "DBG: " << frame << ", " << x << ", " << y << ", " << p << ";" << m_pix[0].size() << ", " << m_pivot.size() << std::endl;
}

void StandardPlane::SetPixelHelper(unsigned index, unsigned x, unsigned y, double pix, bool pivot, unsigned frame) {
	if (frame >= m_pix.size()) EUDAQ_THROW("Bad frame number " + to_string(frame) + " in SetPixel");
	if (frame < m_x.size()) m_x.at(frame).at(index) = x;
	if (frame < m_y.size()) m_y.at(frame).at(index) = y;
	if (frame < m_pivot.size()) m_pivot.at(frame).at(index) = pivot;
	m_pix.at(frame).at(index) = pix;
}

void StandardPlane::SetFlags(StandardPlane::FLAGS flags) {
	m_flags |= flags;
}

double StandardPlane::GetPixel(unsigned index, unsigned frame) const {
	return m_pix.at(frame).at(index);
}
double StandardPlane::GetPixel(unsigned index) const {
	SetupResult();
	return m_result_pix->at(index);
}
double StandardPlane::GetX(unsigned index, unsigned frame) const {
	if (!GetFlags(FLAG_DIFFCOORDS)) frame = 0;
	return m_x.at(frame).at(index);
}
double StandardPlane::GetX(unsigned index) const {
	SetupResult();
	return m_result_x->at(index);
}
double StandardPlane::GetY(unsigned index, unsigned frame) const {
	if (!GetFlags(FLAG_DIFFCOORDS)) frame = 0;
	return m_y.at(frame).at(index);
}
double StandardPlane::GetY(unsigned index) const {
	SetupResult();
	return m_result_y->at(index);
}
bool StandardPlane::GetPivot(unsigned index, unsigned frame) const {
	if (!GetFlags(FLAG_DIFFCOORDS)) frame = 0;
	return m_pivot.at(frame).at(index);
}

void StandardPlane::SetPivot(unsigned index, unsigned frame , bool PivotFlag) {
	m_pivot.at(frame).at(index) = PivotFlag;
}

const std::vector<StandardPlane::coord_t> & StandardPlane::XVector(unsigned frame) const {
	return GetFrame(m_x, frame);
}

const std::vector<StandardPlane::coord_t> & StandardPlane::XVector() const {
	SetupResult();
	return *m_result_x;
}

const std::vector<StandardPlane::coord_t> & StandardPlane::YVector(unsigned frame) const {
	return GetFrame(m_y, frame);
}

const std::vector<StandardPlane::coord_t> & StandardPlane::YVector() const {
	SetupResult();
	return *m_result_y;
}

const std::vector<StandardPlane::pixel_t> & StandardPlane::PixVector(unsigned frame) const {
	return GetFrame(m_pix, frame);
}

const std::vector<StandardPlane::pixel_t> & StandardPlane::PixVector() const {
	SetupResult();
	return *m_result_pix;
}

void StandardPlane::SetXSize(unsigned x) {
	m_xsize = x;
}

void StandardPlane::SetYSize(unsigned y) {
	m_ysize = y;
}

void StandardPlane::SetTLUEvent(unsigned ev) {
	m_tluevent = ev;
}

void StandardPlane::SetPivotPixel(unsigned p) {
	m_pivotpixel = p;
}

unsigned StandardPlane::ID() const {
	return m_id;
}

const std::string & StandardPlane::Type() const {
	return m_type;
}

const std::string & StandardPlane::Sensor() const {
	return m_sensor;
}

unsigned StandardPlane::XSize() const {
	return m_xsize;
}

unsigned StandardPlane::YSize() const {
	return m_ysize;
}

unsigned StandardPlane::NumFrames() const {
	return m_pix.size();
}

unsigned StandardPlane::TotalPixels() const {
	return m_xsize * m_ysize;
}

unsigned StandardPlane::HitPixels(unsigned frame) const {
	return GetFrame(m_pix, frame).size();
}

unsigned StandardPlane::HitPixels() const {
	SetupResult();
	return m_result_pix->size();
}

unsigned StandardPlane::TLUEvent() const {
	return m_tluevent;
}

unsigned StandardPlane::PivotPixel() const {
	return m_pivotpixel;
}

int StandardPlane::GetFlags(int f) const {
	return m_flags & f;
}

bool StandardPlane::NeedsCDS() const {
	return GetFlags(FLAG_NEEDCDS) != 0;
}

int StandardPlane::Polarity() const {
	return GetFlags(FLAG_NEGATIVE) ? -1 : 1;
}

const std::vector<StandardPlane::pixel_t> & StandardPlane::GetFrame(const std::vector<std::vector<pixel_t> > & v, unsigned f) const {
	return v.at(f);
}

//   template <typename T>
//     std::vector<T> StandardPlane::GetPixels() const {
//       SetupResult();
//       std::vector<T> result(m_result_pix->size());
//       for (size_t i = 0; i < result.size(); ++i) {
//         result[i] = static_cast<T>((*m_result_pix)[i] * Polarity());
//       }
//       return result;
//     }

void StandardPlane::SetupResult() const {
	if (m_result_pix) return;
	m_result_x = &m_x[0];
	m_result_y = &m_y[0];
	if (GetFlags(FLAG_ACCUMULATE)) {
		m_temp_pix.resize(0);
		m_temp_x.resize(0);
		m_temp_y.resize(0);
		for (size_t f = 0; f < m_pix.size(); ++f) {
			for (size_t p = 0; p < m_pix[f].size(); ++p) {
				m_temp_x.push_back(GetX(p, f));
				m_temp_y.push_back(GetY(p, f));
				m_temp_pix.push_back(GetPixel(p, f));
			}
		}
		m_result_x = &m_temp_x;
		m_result_y = &m_temp_y;
		m_result_pix = &m_temp_pix;
	} else if (m_pix.size() == 1 && !GetFlags(FLAG_NEEDCDS)) {
		m_result_pix = &m_pix[0];
	} else if (m_pix.size() == 2) {
		if (GetFlags(FLAG_NEEDCDS)) {
			m_temp_pix.resize(m_pix[0].size());
			for (size_t i = 0; i < m_temp_pix.size(); ++i) {
				m_temp_pix[i] = m_pix[1][i] - m_pix[0][i];
			}
			m_result_pix = &m_temp_pix;
		} else {
			if (m_x.size() == 1) {
				m_temp_pix.resize(m_pix[0].size());
				for (size_t i = 0; i < m_temp_pix.size(); ++i) {
					m_temp_pix[i] = m_pix[1 - m_pivot[0][i]][i];
				}
			} else {
				m_temp_x.resize(0);
				m_temp_y.resize(0);
				m_temp_pix.resize(0);
				const bool inverse = false;
				size_t i;
				for (i = 0; i < m_pix[1-inverse].size(); ++i) {
					if (m_pivot[1][i]) break;
					m_temp_x.push_back(m_x[1-inverse][i]);
					m_temp_y.push_back(m_y[1-inverse][i]);
					m_temp_pix.push_back(m_pix[1-inverse][i]);
				}
				for (i = 0; i < m_pix[0+inverse].size(); ++i) {
					if (m_pivot[0+inverse][i]) break;
				}
				for (/**/; i < m_pix[0+inverse].size(); ++i) {
					m_temp_x.push_back(m_x[0+inverse][i]);
					m_temp_y.push_back(m_y[0+inverse][i]);
					m_temp_pix.push_back(m_pix[0+inverse][i]);
				}
				m_result_x = &m_temp_x;
				m_result_y = &m_temp_y;
			}
			m_result_pix = &m_temp_pix;
		}
	} else if (m_pix.size() == 3 && GetFlags(FLAG_NEEDCDS)) {
		m_temp_pix.resize(m_pix[0].size());
		for (size_t i = 0; i < m_temp_pix.size(); ++i) {
			m_temp_pix[i] = m_pix[0][i] * (m_pivot[0][i]-1)
        				  + m_pix[1][i] * (2*m_pivot[0][i]-1)
        				  + m_pix[2][i] * (m_pivot[0][i]);
		}
		m_result_pix = &m_temp_pix;
	} else {
		EUDAQ_THROW("Unrecognised pixel format (" + to_string(m_pix.size())
				+ " frames, CDS=" + (GetFlags(FLAG_NEEDCDS) ? "Needed" : "Done") + ")");
	}

}

template std::vector<short> StandardPlane::GetPixels<>() const;
template std::vector<int> StandardPlane::GetPixels<>() const;
template std::vector<double> StandardPlane::GetPixels<>() const;


/************************************************************************************************/
/************************************** Standard TUEvent ****************************************/
/************************************************************************************************/


StandardTUEvent::StandardTUEvent(const std::string & type){
	m_type = type;
}


StandardTUEvent::StandardTUEvent():m_type(0){}



StandardTUEvent::StandardTUEvent(Deserializer & ds){
	ds.read(m_type);
}


void eudaq::StandardTUEvent::Serialize(Serializer& ser) const {
	ser.write(m_type);
}


void StandardTUEvent::Print(std::ostream & os) const {
	os << m_type << ": ";
	os << "m_timestamp: " << m_timestamp;
	os << "coincidence_count: " << coincidence_count;
	os << "coincidence_count_no_sin: " << coincidence_count_no_sin;
    os << "prescaler_count: " << prescaler_count;                
    os << "handshake_count: " << handshake_count;
	os << "cal_beam_current: " << cal_beam_current;
}



/************************************************************************************************/
/*************************************** Standard Event *****************************************/
/************************************************************************************************/

StandardEvent::StandardEvent(unsigned run, unsigned evnum, uint64_t timestamp):Event(run, evnum, timestamp){}

StandardEvent::StandardEvent(const Event &e):Event(e){
}

StandardEvent::StandardEvent(Deserializer & ds):Event(ds){
	ds.read(m_planes);
	ds.read(m_waveforms);
}

void StandardEvent::Serialize(Serializer & ser) const {
	Event::Serialize(ser);
	ser.write(m_planes);
	ser.write(m_waveforms);
}

void StandardEvent::SetTimestamp(uint64_t val) {
	m_timestamp = val;
}

void StandardEvent::Print(std::ostream & os) const {
	Event::Print(os);
	os << ", " << m_planes.size() << " planes, " << m_waveforms.size() << " waveforms:\n";
	for (size_t i = 0; i < m_planes.size(); ++i) {
		os << "  Plane"<<i<<": " << m_planes[i] << " \n";
	}
	for (size_t i = 0; i < m_waveforms.size(); ++i) {
		os << "  Waveform"<<i<<": " << m_waveforms[i] << " \n";
	}
}

size_t StandardEvent::NumPlanes() const {
	return m_planes.size();
}

StandardPlane & StandardEvent::GetPlane(size_t i) {
	return m_planes[i];
}

const StandardPlane & StandardEvent::GetPlane(size_t i) const {
	return m_planes[i];
}

StandardPlane & StandardEvent::AddPlane(const StandardPlane & plane) {
	m_planes.push_back(plane);
	return m_planes.back();
}


//begin added CD
size_t StandardEvent::NumTUEvents() const{
	return m_tuevent.size();
}

StandardTUEvent & StandardEvent::GetTUEvent(size_t i){
	return m_tuevent[i];
}

const StandardTUEvent& StandardEvent::GetTUEvent(size_t i) const {
	return m_tuevent[i];
}

StandardTUEvent & StandardEvent::AddTUEvent(const StandardTUEvent & tuev){
	m_tuevent.push_back(tuev);
	return m_tuevent.back();
}

//end added CD

bool StandardEvent::hasTUEvent() {
	return m_tuevent.size() !=0;
}

StandardWaveform & StandardEvent::GetWaveform(size_t i) {//ok
	return m_waveforms[i];
}

const StandardWaveform & StandardEvent::GetWaveform(size_t i) const {
	return m_waveforms[i];
}

StandardWaveform & StandardEvent::AddWaveform(const StandardWaveform & waveform) {//ok
	m_waveforms.push_back(waveform);
	return m_waveforms.back();
}

} // end namespace eudaq

