/*
 * WaveformCollection.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "WaveformCollection.hh"
#include "OnlineMon.hh"

static int counting = 0;
static int events = 0;

bool WaveformCollection::isWaveformRegistered(SimpleStandardWaveform p)
{
    std::map<SimpleStandardWaveform,WaveformHistos*>::iterator it;
    it = _map.find(p);
    return (it != _map.end());
}

void WaveformCollection::fillHistograms(const SimpleStandardWaveform &simpWaveform)
{

    if (!isWaveformRegistered(simpWaveform))
    {
        registerWaveform(simpWaveform);
        isOneWaveformRegistered = true;
    }

    WaveformHistos *Waveform = _map[simpWaveform];
    Waveform->Fill(simpWaveform);

    ++counting;

}

void WaveformCollection::bookHistograms(const SimpleStandardEvent &simpev)
{
    for (int Waveform = 0; Waveform < simpev.getNWaveforms(); Waveform++)
    {
        SimpleStandardWaveform simpWaveform = simpev.getWaveform(Waveform);
        if (!isWaveformRegistered(simpWaveform))
        {
            registerWaveform(simpWaveform);
        }
    }
}


void WaveformCollection::Write(TFile *file)
{
    if (file==NULL)
    {
        //cout << "WaveformCollection::Write File pointer is NULL"<<endl;
        exit(-1);
    }
    if (gDirectory!=NULL) //check if this pointer exists
    {
        gDirectory->mkdir("Waveforms");
        gDirectory->cd("Waveforms");


        std::map<SimpleStandardWaveform,WaveformHistos*>::iterator it;
        for (it = _map.begin(); it != _map.end(); ++it) {

            char sensorfolder[255] = "";
            sprintf(sensorfolder,"%s_%d",it->first.getName().c_str(), it->first.getID());
            //cout << "Making new subfolder " << sensorfolder << endl;
            gDirectory->mkdir(sensorfolder);
            gDirectory->cd(sensorfolder);
            it->second->Write();

            //gDirectory->ls();
            gDirectory->cd("..");
        }
        gDirectory->cd("..");
    }
}

WaveformHistos* WaveformCollection::getWaveformHistos(std::string sensor, int id) {
    //in the past:
    //	SimpleStandardWaveform wf(sensor,id,1024);
    //	return _map[wf];
    std::map<SimpleStandardWaveform,WaveformHistos*>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it)
        if (it->first.getID() == id && it->first.getName()== sensor)
            return it->second;
    return 0;
}


void WaveformCollection::Calculate(const unsigned int currentEventNumber)
{
    //	cout<<"WaveformCollection::Calculate"<< currentEventNumber<<" "<<_reduce<<" "<< (currentEventNumber % 1000*_reduce == 0)<<endl;
    if (( true))//currentEventNumber % _reduce == 0))
    {
        std::map<SimpleStandardWaveform,WaveformHistos*>::iterator it;
        for (it = _map.begin(); it != _map.end(); ++it)
        {
            //			std::cout << "WaveformCollection::Calculating "<< currentEventNumber<<std::endl;
            it->second->Calculate(currentEventNumber/_reduce);
        }
    }
}

void WaveformCollection::Reset()
{
    std::map<SimpleStandardWaveform,WaveformHistos*>::iterator it;
    for (it = _map.begin(); it != _map.end(); ++it)
    {
        (*it).second->Reset();
    }
}


void WaveformCollection::Fill(const SimpleStandardEvent &simpev)
{
    //	cout<<"WaveformCollection::Fill\t"<<simpev.getNPlanes()<<" "<<simpev.getNWaveforms()<<endl;
    for (int Waveform = 0; Waveform < simpev.getNWaveforms(); Waveform++) {
        const SimpleStandardWaveform&  simpWaveform = simpev.getWaveform(Waveform);
        //		cout<<Waveform<<"\t"<<simpWaveform.getChannelName()<<endl;
        fillHistograms(simpWaveform);
    }

}

void WaveformCollection::registerSignalWaveforms(const SimpleStandardWaveform &p){
    registerDataWaveforms(p,"","SignalEvents");
}

void WaveformCollection::registerDataWaveforms(const SimpleStandardWaveform &p,string prefix, string desc){
    char tree[1024], folder[1024];

    WaveformHistos* wf_histo =getWaveformHistos(p.getName(),p.getID());
    string main_path = (string)TString::Format("%s/Ch %i - %s/%s",p.getName().c_str(),p.getID(),p.getChannelName().c_str(),desc.c_str());

    sprintf(tree,"%s/FullAverage",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"FullAverage"), "",0);

    sprintf(tree,"%s/Signal",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"Signal"), "",0);


    sprintf(tree,"%s/Pedestal",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"Pedestal"), "",0);

    sprintf(tree,"%s/Pulser",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"Pulser"), "",0);

    sprintf(tree,"%s/SignalMinusPedestal",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"SignalMinusPedestal"), "",0);

    sprintf(tree,"%s/SignalProfile",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getProfile(prefix+"Signal"), "",0);

    sprintf(tree,"%s/PedestalProfile",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getProfile(prefix+"Pedestal"), "",0);

    sprintf(tree,"%s/PulserProfile",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getProfile(prefix+"Pulser"), "",0);

    sprintf(tree,"%s/SignalMinusPedestalProfile",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getProfile(prefix+"SignalMinusPedestal"), "",0);

    //    sprintf(folder,"%s/Signal",p.getName().c_str());
    sprintf(folder,"%s",main_path.c_str());
#ifdef DEBUG
    cout << "DEBUG "<< p.getName().c_str() <<endl;
    cout << "DEBUG "<< folder << " "<<tree<<  endl;
#endif
    _mon->getOnlineMon()->addTreeItemSummary(folder,tree);

    sprintf(tree,"%s",main_path.c_str());//p.getName().c_str(),p.getID(),p.getChannelName().c_str());
    _mon->getOnlineMon()->makeTreeItemSummary(tree); //make summary page


    sprintf(tree,"%s/Integral/Signal",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"SignalIntegral"), "",0);

    sprintf(tree,"%s/Integral/Pedestal",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"PedestalIntegral"), "",0);

    sprintf(tree,"%s/Integral/Pulser",main_path.c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,wf_histo->getHisto(prefix+"PulserIntegral"), "",0);
    sprintf(folder,"%s/Integral",main_path.c_str());
    _mon->getOnlineMon()->addTreeItemSummary(folder,tree);

    sprintf(tree,"%s/Integral",main_path.c_str());//p.getName().c_str(),p.getID(),p.getChannelName().c_str());
    _mon->getOnlineMon()->makeTreeItemSummary(tree); //make summary page
}

void WaveformCollection::registerGlobalWaveforms(const SimpleStandardWaveform &p){
    char tree[1024], folder[1024];
    sprintf(tree,"%s/Ch %i - %s/PulserEvents",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
    std::cout<<tree<<endl;
    WaveformHistos* wf_histo =getWaveformHistos(p.getName(),p.getID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,
            wf_histo->getHisto("PulserEvents"), "",0);

    sprintf(tree,"%s/Ch %i - %s/PulserEventsProfile",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,
            getWaveformHistos(p.getName(),p.getID())->getProfile("PulserEvents"), "",0);

    sprintf(tree,"%s/Ch %i - %s/nFlatLineEvents",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
    std::cout<<tree<<endl;
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,
            getWaveformHistos(p.getName(),p.getID())->getHisto("nFlatLineEvents"), "",0);

}

void WaveformCollection::registerPulserWaveforms(const SimpleStandardWaveform &p){
    registerDataWaveforms(p,"Pulser_","PulserEvents");
}

void WaveformCollection::registerWaveform(const SimpleStandardWaveform &p) {
    //	cout<<"WaveformCollection::registerWaveform \t\""<<p.getName()<<"\" "<<p.getID()<<" \""<<p.getChannelName()<<"\" mon:"<<_mon<<endl;
    WaveformHistos *tmphisto = new WaveformHistos(p,_mon);
    tmphisto->SetOptions(_WaveformOptions);
    _map[p] = tmphisto;
    //std::cout << "Registered Waveform: " << p.getName() << " " << p.getID() << std::endl;
    //WaveformRegistered(p.getName(),p.getID());
    if (_mon != NULL)
    {
        if (_mon->getOnlineMon()==NULL)
        {
            return; // don't register items
        }
        //		cout << "WaveformCollection:: Monitor running in online-mode" << endl;
        char tree[1024], folder[1024];
        WaveformHistos* wf_histo =getWaveformHistos(p.getName(),p.getID());
        registerGlobalWaveforms(p);

        registerSignalWaveforms(p);
        //
        //=====================================================================
        //=============== WAVEFORM STACKS =====================================
        //=====================================================================
        sprintf(tree,"%s/Ch %i - %s/RawWaveform",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
        std::cout<<tree<<endl;
        _mon->getOnlineMon()->registerTreeItem(tree);
        _mon->getOnlineMon()->registerHisto(tree,getWaveformHistos(p.getName(),p.getID())->getWaveformGraph(0), "L",0);

        sprintf(tree,"%s/Ch %i - %s/RawWaveformStack",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
        std::cout<<tree<<endl;
        _mon->getOnlineMon()->registerTreeItem(tree);
        _mon->getOnlineMon()->registerHistoStack(tree,getWaveformHistos(p.getName(),p.getID())->getWaveformStack(), "nostack",0);

        sprintf(folder,"%s",p.getName().c_str());
#ifdef DEBUG
        cout << "DEBUG "<< p.getName().c_str() <<endl;
        cout << "DEBUG "<< folder << " "<<tree<<  endl;
#endif
        _mon->getOnlineMon()->addTreeItemSummary(folder,tree);

        sprintf(tree,"%s/Ch %i - %s",p.getName().c_str(),p.getID(),p.getChannelName().c_str());
        _mon->getOnlineMon()->makeTreeItemSummary(tree); //make summary page

        registerPulserWaveforms(p);
    }
}
