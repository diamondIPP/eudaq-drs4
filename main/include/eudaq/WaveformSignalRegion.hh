/*
 * WaveformSignalRegion.h
 *
 *  Created on: Oct 13, 2015
 *      Author: bachmair
 */

#ifndef WAVEFORMSIGNALREGION_H_
#define WAVEFORMSIGNALREGION_H_

#include "WaveformIntegral.hh"
#include <limits>
#include <vector>
#include <map>

class WaveformSignalRegion {
    public:
        WaveformSignalRegion(int low=-1, int high=-1, std::string name="");
        virtual ~WaveformSignalRegion();
        void SetPeakPostion(int16_t peak_position);
        void SetPeakTime(float t_peak) { peak_time = t_peak; }
        size_t GetNIntegrals(){return integrals.size();}
        std::vector<WaveformIntegral *> GetIntegrals();
        WaveformIntegral* GetIntegralPointer(uint16_t i);
        WaveformIntegral* GetIntegralPointer(std::string s);
        void ResetIntegrals();
        void Print() const {Print(std::cout);}
        void Print(std::ostream& out) const;
        void SetPolarity(signed char pol){polarity=pol;}
        void SetPulserPolarity(signed char pol){pulserPolarity=pol;}
        void AddIntegral(WaveformIntegral integralDef);
        uint16_t GetLowBoarder() {return low_border;}
        uint16_t GetHighBoarder() {return high_border;}
        uint16_t GetPeakPosition(){return peak_pos;}
        float GetPeakTime() { return peak_time; }
        void SetName(std::string name) { this->name = name; }
        const char *GetName() { return name.c_str(); }
        float operator [](int i){return integrals[integral_names[i]].GetIntegral();}
        float operator [](std::string s){return integrals[s].GetIntegral();}
        std::pair<int, int> GetRegion() { return std::make_pair(low_border, high_border) ;}
    private:
        std::string name;
        signed char polarity;
        signed char pulserPolarity;
        uint16_t low_border;
        uint16_t high_border;
        uint16_t peak_pos;
        float peak_time;
        std::map<std::string,WaveformIntegral> integrals;
        std::vector<std::string> integral_names;
//        ClassDef(WaveformSignalRegion,1);
};

inline std::ostream & operator << (std::ostream & os, const WaveformSignalRegion & c){c.Print(os);return os;}
#endif /* WAVEFORMSIGNALREGION_H_ */
