/*
 * WaveformCollection.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "WaveformCollection.hh"
#include "OnlineMon.hh"
#include "OnlineMonWindow.hh"

using namespace eudaq;

static int counting = 0;
static int events = 0;

bool WaveformCollection::isWaveformRegistered(SimpleStandardWaveform p) {

    return (_map.find(p) != _map.end());
}

void WaveformCollection::fillHistograms(const SimpleStandardWaveform &simpWaveform) {

    if (!isWaveformRegistered(simpWaveform))
    {
        registerWaveform(simpWaveform);
        isOneWaveformRegistered = true;
    }

    WaveformHistos *Waveform = _map[simpWaveform];
    Waveform->Fill(simpWaveform);

    ++counting;

}

void WaveformCollection::Write(TFile *file)
{
    if (file == nullptr)
        exit(-1);
    if (gDirectory) {
        gDirectory->mkdir("Waveforms");
        gDirectory->cd("Waveforms");

        for (auto &it : _map) {

            char sensorfolder[255] = "";
            sprintf(sensorfolder,"%s_%d", it.first.getName().c_str(), it.first.getID());
            gDirectory->mkdir(sensorfolder);
            gDirectory->cd(sensorfolder);
            it.second->Write();

            gDirectory->cd("..");
        }
        gDirectory->cd("..");
    }
}

WaveformHistos* WaveformCollection::getWaveformHistos(std::string sensor, int id) {

    for (auto &it : _map)
        if (it.first.getID() == id && it.first.getName() == sensor)
            return it.second;
    return nullptr;
}

void WaveformCollection::Calculate(const unsigned int currentEventNumber) {

    for (auto &it : _map)
        it.second->Calculate(currentEventNumber / _reduce);
}

void WaveformCollection::Reset() {

    for (auto &it : _map)
        it.second->Reset();
}

void WaveformCollection::Fill(const SimpleStandardEvent &simpev) {

    //	cout<<"WaveformCollection::Fill\t"<<simpev.getNPlanes()<<" "<<simpev.getNWaveforms()<<endl;
    for (int i_wf = 0; i_wf < simpev.getNWaveforms(); i_wf++) {
        const SimpleStandardWaveform & simpWaveform = simpev.getWaveform(i_wf);
        //		cout<<i_wf<<"\t"<<simpWaveform.getChannelName()<<endl;
        fillHistograms(simpWaveform);
    }
}

void WaveformCollection::registerSignalWaveforms(const SimpleStandardWaveform &p){

    registerDataWaveforms(p, "", "SignalEvents");
}

void WaveformCollection::registerPulserWaveforms(const SimpleStandardWaveform &p){

    registerDataWaveforms(p, "Pulser_", "PulserEvents");
}

void WaveformCollection::registerDataWaveforms(const SimpleStandardWaveform &p, string prefix, const string desc) {

    string folder = p.getName();
    string subfolder = string(TString::Format("Ch %i - %s", p.getID(), p.getChannelName().c_str()));
    string tree = join(folder, join(subfolder, desc));

    WaveformHistos * wf_histo = getWaveformHistos(p.getName(), p.getID());

    registerHistoItem(join(tree, "Full Average"),                   wf_histo->getHisto(prefix + "FullAverage"));
    registerHistoItem(join(tree, "Signal Spread"),                  wf_histo->getHisto(prefix + "Signal"));
    registerHistoItem(join(tree, "Pedestal Spread"),                wf_histo->getHisto(prefix + "Pedestal"));
    registerHistoItem(join(tree, "Pulser Spread"),                  wf_histo->getHisto(prefix + "Pulser"));
    registerHistoItem(join(tree, "Signal Minus Pedestal"),          wf_histo->getHisto(prefix + "SignalMinusPedestal"));
    registerHistoItem(join(tree, "Signal Profile"),                 wf_histo->getProfile(prefix + "Signal"));
    registerHistoItem(join(tree, "Pedestal Profile"),               wf_histo->getProfile(prefix + "Pedestal"));
    registerHistoItem(join(tree, "Pulser Profile"),                 wf_histo->getProfile(prefix + "Pulser"));
    registerHistoItem(join(tree, "SignalMinusPedestalProfile"),     wf_histo->getProfile(prefix + "SignalMinusPedestal"));
    registerHistoItem(join(tree, "TimeSignalMinusPedestalProfile"), wf_histo->getTimeProfile(prefix + "SignalMinusPedestal"));

#ifdef DEBUG
    cout << "DEBUG "<< p.getName().c_str() <<endl;
    cout << "DEBUG "<< folder << " "<<tree<<  endl;
#endif

    registerHistoItem(join(tree, "Integral/Signal"), wf_histo->getHisto(prefix + "SignalIntegral"));
    registerHistoItem(join(tree, "Integral/Pedestal"), wf_histo->getHisto(prefix + "PedestalIntegral"));
    registerHistoItem(join(tree, "Integral/Pulser"), wf_histo->getHisto(prefix + "PulserIntegral"));

    /** make sub folder summary */
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Signal Spread"));
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Pulser Spread"));
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Integral/Signal"));
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Integral/Pulser"));
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Signal Profile"));
    _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Pulser Profile"));

    /** make channel summary */
    if (desc.find("Signal") != string::npos){
        _mon->getOnlineMon()->addTreeItemSummary(join(folder, subfolder), join(tree, "Integral/Signal"));
        _mon->getOnlineMon()->addTreeItemSummary(join(folder, subfolder), join(tree, "Signal Profile"));
    }

    _mon->getOnlineMon()->makeTreeItemSummary(join(tree, "Integral"));  //make Integral summary page
}


void WaveformCollection::registerWaveform(const SimpleStandardWaveform &p) {

    uint8_t wf_type = getWaveFormType(p);
    cout << "WaveformCollection::registerWaveform: \"" << p.getName() << "\", Channel: " << p.getID() << ", Name: \"" << p.getChannelName() << "\", wf type: "
         << int(wf_type) << endl;
    WaveformHistos *tmphisto = new WaveformHistos(p, _mon);
    tmphisto->SetOptions(_WaveformOptions);
    _map[p] = tmphisto;

    if (_mon != nullptr) {
        if (_mon->getOnlineMon() == nullptr)
            return;  // don't register items

        /** =============== SIGNAL WAVEFORMS ================================*/
        if (wf_type == 0) {
            registerSignalWaveforms(p);
            registerPulserWaveforms(p);
        }
        string folder = p.getName();
        string tree = join(folder, string(TString::Format("Ch %i - %s", p.getID(), p.getChannelName().c_str())));
        /** =============== WAVEFORM STACKS ================================*/
        registerHistoItem(join(tree, "Raw Waveform"), getWaveformHistos(p.getName(), p.getID())->getWaveformGraph(0), "L");
        if (wf_type==0)
            registerHistoStackItem(eudaq::join(tree, "SignalEvents/RawWaveformStackGood"), getWaveformHistos(p.getName(), p.getID())->getGoodWaveformStack(),
                                   "nostack");
        registerHistoStackItem(join(tree, "Raw Waveform Stack"), getWaveformHistos(p.getName(), p.getID())->getWaveformStack(), "nostack");
        /** =============== CATEGORIES ================================*/
        registerHistoItem(join(tree, "Categories"), (TH2F*)getWaveformHistos(p.getName(),p.getID())->getHisto("CategoryVsEvent"), "colz");

#ifdef DEBUG
        cout << "DEBUG "<< p.getName().c_str() <<endl;
        cout << "DEBUG "<< folder << " "<<tree<<  endl;
#endif

        /** =============== SUMMARY ================================*/
        _mon->getOnlineMon()->addTreeItemSummary(folder, join(tree, "Raw Waveform Stack"));
        _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Raw Waveform"));
        _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Raw Waveform Stack"));
//        _mon->getOnlineMon()->addTreeItemSummary(tree, join(tree, "Categories"));
    }
}

void WaveformCollection::registerHistoItem(std::string name, TH1 * histo, const std::string opt, const unsigned log_y) {

    _mon->getOnlineMon()->registerTreeItem(name);
    _mon->getOnlineMon()->registerHisto(name, histo, opt, log_y);
}

void WaveformCollection::registerHistoStackItem(std::string name, THStack * stack, const std::string opt, const unsigned log_y){

    _mon->getOnlineMon()->registerTreeItem(name);
    _mon->getOnlineMon()->registerHistoStack(name, stack, opt, log_y);
}

uint8_t WaveformCollection::getWaveFormType(const SimpleStandardWaveform &p) {

    if (p.getChannelName().find("Pulser") != string::npos)
        return 1;
    else if (p.getChannelName().find("FORC") != string::npos)
        return 2;
    return 0;  // 0 = diamond
}