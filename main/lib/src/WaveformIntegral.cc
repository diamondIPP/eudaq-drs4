/*
 * WaveformIntegral.cc
 *
 *  Created on: Oct 14, 2015
 *      Author: bachmair
 */

#include <eudaq/WaveformIntegral.hh>

//ClassImp(WaveformIntegral);

WaveformIntegral::WaveformIntegral(int down_range, int up_range, std::string name):calculated(false) {
    this->down_range = down_range;
    this->up_range = up_range;
    if (name=="")
        this->name = TString::Format("Integral_%d-%d",down_range,up_range);
    else
        this->name=name;
    this->SetName(name);
}

void WaveformIntegral::Reset() {
    calculated = false;
    integral_start = -1;
    integral_stop = -1;
    integral = std::numeric_limits<double>::quiet_NaN();
}

void WaveformIntegral::SetIntegral(float integral) {
    calculated = true;
    this->integral = integral;
}

void WaveformIntegral::SetPeakPosition(int peak_position, int n_samples) {
    integral_start = uint16_t(std::max(peak_position - down_range, 0));
    integral_stop = uint16_t(std::min(peak_position + up_range, n_samples));
}

void WaveformIntegral::Print(std::ostream & out, bool bEndl) const{
    out<<"WaveformIntegral: "<<name<<"["<<std::setw(3)<<down_range<<","<<std::setw(3)<<up_range<<"]";
    if (calculated)
        out<<": ["<<std::setw(3)<<integral_start<<","<<std::setw(3)<<integral_stop<<"] "<<integral;
    if (bEndl)
        out<<std::endl;

}
WaveformIntegral::~WaveformIntegral() {
}

