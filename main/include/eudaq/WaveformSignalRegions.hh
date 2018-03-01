/*
 * WaveformSignalRegions.hh
 *
 *  Created on: Oct 14, 2015
 *      Author: bachmair
 */

#ifndef WAVEFORMSIGNALREGIONS_HH_
#define WAVEFORMSIGNALREGIONS_HH_


#include "StandardEvent.hh"
#include "WaveformIntegral.hh"
#include <limits>
#include <vector>
#include "TROOT.h"
#include "WaveformSignalRegion.hh"

class WaveformSignalRegions:public TObject{
    public:
        WaveformSignalRegions(int channel=-1, signed char pol=0, signed char pul_pol=0);
        virtual ~WaveformSignalRegions(){};
        void AddRegion(WaveformSignalRegion region);
        void Clear(){channel=-1;regions.clear();polarity=0;}
        void Print() const{Print(std::cout);}
        void Print(std::ostream& out) const;
//        void CalculateIntegrals(const StandardWaveform* wf);
        void Reset();
        uint16_t GetNRegions() { return uint16_t(regions.size()); }
        signed char GetPolarity() { return polarity; }
        signed char GetPulserPolarity() { return pulserPolarity; }
        WaveformSignalRegion* GetRegion(UInt_t i);
        WaveformSignalRegion* GetRegion(std::string name);
    private:
        std::vector<WaveformSignalRegion> regions;
        std::vector<std::string> names;
        int channel;
        signed char polarity;
        signed char pulserPolarity;
//        ClassDef(WaveformSignalRegions,1);
};
inline std::ostream & operator << (std::ostream& os, const WaveformSignalRegions& regions) { regions.Print(os); return os;}

#endif /* WAVEFORMSIGNALREGIONS_HH_ */
