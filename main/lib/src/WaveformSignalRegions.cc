/*
 * WaveformSignalRegions.cpp
 *
 *  Created on: Oct 14, 2015
 *      Author: bachmair
 */

#include <eudaq/WaveformSignalRegions.hh>

WaveformSignalRegions::WaveformSignalRegions(int channel, signed char pol, signed char pul_pol) {
    this->channel=channel;
    polarity = pol;
    pulserPolarity = pul_pol;
}


void WaveformSignalRegions::AddRegion(WaveformSignalRegion region) {
//    std::cout<<"AddRegion: "<<region<<std::endl;
    region.SetPolarity(polarity);
    region.SetPulserPolarity(pulserPolarity);
    this->regions.push_back(region);
    names.emplace_back(region.GetName());
}

void WaveformSignalRegions::Print(std::ostream& out) const {
    for (const auto &reg: this->regions)
        std::cout << "\t" << reg << "\n";
}

void WaveformSignalRegions::Reset(){
    for (auto i: this->regions)
            i.ResetIntegrals();
}


WaveformSignalRegion* WaveformSignalRegions::GetRegion(UInt_t i) {
    if (i<regions.size())
        return &(regions[i]);
    return nullptr;
}

WaveformSignalRegion* WaveformSignalRegions::GetRegion(std::string name) {

    for (uint16_t i(0); i < names.size(); i++)
        if (names.at(i) == name)
            return &regions.at(i);
    return nullptr;
}
