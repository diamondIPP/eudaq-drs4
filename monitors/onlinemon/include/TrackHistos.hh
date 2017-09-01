//
// Created by mreichmann on 01.09.17.
//

#ifndef EUDAQ_TRACKHISTOS_HH
#define EUDAQ_TRACKHISTOS_HH


#include "SimpleStandardEvent.hh"
#include <TFile.h>


using namespace std;

class RootMonitor;
class TH1F;

class TrackHistos {

protected:
    TH1F * _trackAngleX;
    TH1F * _trackAngleY;

public:
    TrackHistos(SimpleStandardEvent&, RootMonitor*);
    void Fill(const SimpleStandardEvent & ev);

private:
    uint8_t nPlanes;
};


#endif //EUDAQ_TRACKHISTOS_HH
