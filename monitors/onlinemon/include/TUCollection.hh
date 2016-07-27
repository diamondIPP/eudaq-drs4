/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUCollection>.hh
** 
** Date: May 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#ifndef TUCOLLECTION_HH_
#define TUCOLLECTION_HH_

#include "BaseCollection.hh"
#include <RQ_OBJECT.h>

class SimpleStandardEvent;
class TUHistos;
class RootMonitor;


class TUCollection:public BaseCollection{

  RQ_OBJECT("TUCollection")
  protected:
    void fillHistograms(const SimpleStandardEvent &ev);
    bool histos_init;

  public:
    TUCollection();
    virtual ~TUCollection();
    void Reset();
    virtual void Write(TFile *file);
    void Calculate(const unsigned int currentEventNumber);
    void bookHistograms(const SimpleStandardEvent &simpev);
    void setRootMonitor(RootMonitor *mon)  {_mon = mon; }
    void Fill(const SimpleStandardEvent &simpev);
    TUHistos* getTUHistos();

  private:
    TUHistos * tuevhistos;
};

#ifdef __CINT__
#pragma link C++ class TUCollection-;
#endif

#endif
