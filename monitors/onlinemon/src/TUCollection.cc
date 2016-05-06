/* ---------------------------------------------------------------------------------
** OSU Trigger Logic Unit EUDAQ Implementation
** 
**
** <TUCollection>.cc
** 
** Date: May 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#include "TUCollection.hh"
#include "OnlineMon.hh"

//ROOT Includes
#include <TH2I.h>
#include <TFile.h>

//STL includes
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

//project includes
#include "SimpleStandardEvent.hh"
#include "TUHistos.hh"


TUCollection::TUCollection(): BaseCollection(){
  tuevhistos = new TUHistos();
  histos_init = false;
  std::cout << " Initialising TU Collection" << std::endl;
  //CollectionType = TU_COLLECTION_TYPE;
}

TUCollection::~TUCollection(){}


void TUCollection::Write(TFile *file){
  if (file==NULL){
    cout << "TUCollection::Write File pointer is NULL"<<endl;
    exit(-1);
  }

  if (gDirectory!=NULL){
    gDirectory->mkdir("TU");
    gDirectory->cd("TU");
    tuevhistos->Write();
    gDirectory->cd("..");
  }

}

void TUCollection::Calculate(const unsigned int /*currentEventNumber*/){

}

void TUCollection::Reset(){
  tuevhistos->Reset();
}


void TUCollection::Fill(const SimpleStandardEvent &simpev){
  if (histos_init==false){
    bookHistograms(simpev);
    histos_init=true;
  }

  tuevhistos->Fill(simpev.getTUEvent(0), simpev.getEvent_number());
}


void TUCollection::bookHistograms(const SimpleStandardEvent & /*simpev*/){
  if (_mon != NULL){
    string performance_folder_name="TU";

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Coincidence Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Coincidence Count"), tuevhistos->getCoincidenceCountHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Coincidence Count No Scintillator"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Coincidence Count No Scintillator"), tuevhistos->getCoincidenceCountNoScintHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Prescaler Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Prescaler Count"), tuevhistos->getPrescalerCountHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Prescaler Xor Pulser Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Prescaler Xor Pulser Count"), tuevhistos->getPrescalerXPulserHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Accepted Prescaled Events"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Accepted Prescaled Events"), tuevhistos->getAcceptedPrescaledEventsHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Accepted Pulser Events"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Accepted Pulser Events"), tuevhistos->getAcceptedPulserEventsHisto());
    
    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Event Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Event Count"), tuevhistos->getEventCountHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Average Beam Current"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Average Beam Current"), tuevhistos->getAvgBeamCurrentHisto());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Plane Scaler 1"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Plane Scaler 1"), tuevhistos->getScaler1Histo());

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Plane Scaler 2"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Plane Scaler 2"), tuevhistos->getScaler2Histo());

    _mon->getOnlineMon()->makeTreeItemSummary(performance_folder_name.c_str()); //make summary page

  }
}



TUHistos * TUCollection::getTUHistos(){
  return tuevhistos;
}
