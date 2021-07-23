/*
 * WaveformIntegral.cc
 *
 *  Created on: Oct 14, 2015
 *      Author: bachmair
 */

#include <eudaq/WaveformIntegral.hh>
#include "TString.h"
#include "TObject.h"

//ClassImp(WaveformIntegral);

WaveformIntegral::WaveformIntegral(int down_range, int up_range, const std::string & name):calculated(false) {
    this->down_range = down_range;
    this->up_range = up_range;
    if (name.empty())
        this->name_ = TString::Format("Integral_%d-%d", down_range, up_range);
    else
        this->name_=name;
    this->SetName(name);
}

void WaveformIntegral::Reset() {
    calculated = false;
    integral_start = -1;
    integral_stop = -1;
  integral_ = std::numeric_limits<double>::quiet_NaN();
}

void WaveformIntegral::SetIntegral(float integral) {
    calculated = true;
    this->integral_ = integral;
}

void WaveformIntegral::SetPeakPosition(int peak_position, int n_samples) {
    integral_start = uint16_t(std::max(peak_position - down_range, 0));
    integral_stop = uint16_t(std::min(peak_position + up_range, n_samples));
}

void WaveformIntegral::Print(std::ostream & out, bool bEndl) const{
    out << "WaveformIntegral: " << name_ << "[" << std::setw(3) << down_range << "," << std::setw(3) << up_range << "]";
    if (calculated)
        out << ": [" << std::setw(3) << integral_start << "," << std::setw(3) << integral_stop << "] " << integral_;
    if (bEndl)
        out<<std::endl;

}

