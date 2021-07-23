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
#include "OnlineMonWindow.hh"

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

  if (gDirectory){
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

  if(simpev.getNTUEvent() >= 1)
    tuevhistos->Fill(simpev.getTUEvent(0), simpev.getEvent_number());
  }


void TUCollection::bookHistograms(const SimpleStandardEvent & /*simpev*/){
  if (_mon != NULL){
    string performance_folder_name="TU";

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Coincidence Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Coincidence Count"), tuevhistos->getCoincidenceCountHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Coincidence Count No Scintillator"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Coincidence Count No Scintillator"), tuevhistos->getCoincidenceCountNoScintHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Prescaler Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Prescaler Count"), tuevhistos->getPrescalerCountHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Prescaler Xor Pulser Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Prescaler Xor Pulser Count"), tuevhistos->getPrescalerXPulserHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Accepted Prescaled Events"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Accepted Prescaled Events"), tuevhistos->getAcceptedPrescaledEventsHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Accepted Pulser Events"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Accepted Pulser Events"), tuevhistos->getAcceptedPulserEventsHisto(), "P");
    
    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Event Count"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Event Count"), tuevhistos->getEventCountHisto(), "P");

    _mon->getOnlineMon()->registerTreeItem((performance_folder_name+"/Average Beam Current"));
    _mon->getOnlineMon()->registerHisto( (performance_folder_name+"/Average Beam Current"), tuevhistos->getAvgBeamCurrentHisto(), "P");


    for (unsigned i(0); i <= tuevhistos->getNScaler(); i++){
      const char * name = tuevhistos->getScalerHisto(i)->GetName();
      _mon->getOnlineMon()->registerTreeItem(string(TString::Format("%s/%s Scaler", performance_folder_name.c_str(), name)));
      _mon->getOnlineMon()->registerHisto(string(TString::Format("%s/%s Scaler", performance_folder_name.c_str(), name)), tuevhistos->getScalerHisto(i), "P");
    }

    _mon->getOnlineMon()->makeTreeItemSummary(performance_folder_name.c_str()); //make summary page

  }
}



TUHistos * TUCollection::getTUHistos(){
  return tuevhistos;
}
