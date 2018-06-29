/*
 * WaveformIntegral.hh
 *
 *  Created on: Oct 14, 2015
 *      Author: bachmair
 */

#ifndef WAVEFORMINTEGRAL_HH_
#define WAVEFORMINTEGRAL_HH_

#include <limits>
#include <vector>
#include <iosfwd>
#include <iostream>
#include <iomanip>
#include "TROOT.h"

class WaveformIntegral:public TObject{
    public:
        WaveformIntegral(int low=-1, int high=-1, std::string name="");
        void SetPeakPosition(int peak_position,int n_samples);
        uint16_t GetIntegralStart() const {return integral_start;}
        uint16_t GetIntegralStop() const {return integral_stop;}
        void SetIntegral(float integral);
        void SetTimeIntegral(float integral) { time_integral = integral; }
        float GetIntegral(){ return !calculated ? std::numeric_limits<float>::quiet_NaN() : integral; }
        float GetTimeIntegral(){ return time_integral; }
        virtual ~WaveformIntegral();
        void Reset();
        void Print() const {Print(std::cout,true);}
        void Print(std::ostream& out, bool bEndl=false) const;
        void SetName(std::string name){this->name=name;}
//        const char * GetName() const { return name.c_str(); }
        std::string GetName() { return name; }
    private:
        bool calculated;
        int down_range;
        int up_range;
        uint16_t integral_start;
        uint16_t integral_stop;
        float integral;
        float time_integral;
        std::string name;
};


inline std::ostream & operator << (std::ostream & os, const WaveformIntegral & c){c.Print(os,false);return os;}

#endif /* WAVEFORMINTEGRAL_HH_ */
