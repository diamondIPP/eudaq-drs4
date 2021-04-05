#include "dictionaries.h"
#include "constants.h"
#include "datasource_evt.h"
#include "Utils.hh"
#include <exception>
#include <fstream>

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
#  include "EUTelRunHeaderImpl.h"

#include "EUTelCMSPixelDetector.h"
using eutelescope::EUTELESCOPE;
#endif

#include <TMath.h>
#include <TString.h>
#include <TF1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TFile.h>
#include <TDirectory.h>

using namespace pxar;

namespace eudaq {

  struct VCALDict {
    int row;
    int col;
    float par0;
    float par1;
    float par2;
    float par3;
    float calibration_factor;
  };

  class CMSPixelHelper {
  public:
    std::map<std::string, float> roc_calibrations = {{"psi46v2", 65},
                                                     {"psi46digv21respin", 47},
                                                     {"proc600", 47}};

    CMSPixelHelper(std::string event_type) : do_conversion(true), m_event_type(event_type) { m_conv_cfg = new Configuration(""); }

    void set_conversion(bool val) { do_conversion = val; }

    bool get_conversion() { return do_conversion; }

    std::map<std::string, VCALDict> vcal_vals;
    TF1 *fFitfunction;
    std::vector<TH1F *> hEncode;
    std::vector<TH1F *> hUblack;
    std::vector<TH1F *> hBlack;
    std::vector<TH1F *> hc0;
    std::vector<TH1F *> hc1;
    std::vector<TH1F *> hr0;
    std::vector<TH1F *> hr1;
    std::vector<TH1F *> hcr;
    std::vector<TProfile *> pblack;
    std::vector<TProfile *> pUblack;
    std::vector<TH2F *> h2c0;
    std::vector<TH2F *> h2c1;
    std::vector<TH2F *> h2r0;
    std::vector<TH2F *> h2r1;
    std::vector<TH2F *> h2cr;
    std::vector<TH2F *> h2black;
    std::vector<TH2F *> h2uBlack;
    std::vector<TH2F *> h2lastDac;
    std::vector<float> *blackV;
    std::vector<float> *uBlackV;
    std::vector<int16_t> *levelSV;
    std::vector<float> *decodingOffsetVector2;

    TFile *DecodingFile;
    TDirectory *DecodingDirectory;

    void initializeFitfunction() { fFitfunction = new TF1("fitfunc", "[3]*(TMath::Erf((x-[0])/[1])+[2])", -4096, 4096); }

    float getCharge(VCALDict d, float val, float factor = 65.) const {
      fFitfunction->SetParameters(d.par0, d.par1, d.par2, d.par3);
      return d.calibration_factor * float(fFitfunction->GetX(val));
    }

    virtual void SetConfig(Configuration *conv_cfg) { m_conv_cfg = conv_cfg; }

    void Initialize(const Event &bore, const Configuration &cnf) {
      DeviceDictionary *devDict;
      std::string roctype = bore.GetTag("ROCTYPE", "");
      std::string tbmtype = bore.GetTag("TBMTYPE", "tbmemulator");

      m_detector = bore.GetTag("DETECTOR", "");
      std::string pcbtype = bore.GetTag("PCBTYPE", "");
      m_rotated_pcb = pcbtype.find("-rot") != std::string::npos;

      // Get the number of planes:
      m_nplanes = bore.GetTag("PLANES", 1);

      m_roctype = devDict->getInstance()->getDevCode(roctype);
      m_tbmtype = devDict->getInstance()->getDevCode(tbmtype);

      if (m_roctype == 0x0) { EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin"); }
      read_PHCalibrationData(cnf);
      initializeFitfunction();

      /** Decoding of the analogue telescope */
      m_conv_cfg->SetSection("Converter.telescopetree");
      do_decoding = m_conv_cfg->Get("decoding_offset_v", "").empty() and TString("DUT").CompareTo(TString(m_detector)) == 0;
      decodingOffsetVector = m_conv_cfg->Get("decoding_offset_v", std::vector<float>(16, m_conv_cfg->Get("decoding_offset", 0.)));
      level1Vector = m_conv_cfg->Get("decoding_l1_v", std::vector<float>(16, 0.));
      decodingAlphasVector = m_conv_cfg->Get("decoding_alphas_v", std::vector<float>(16, 0.));
      std::cout << "Using decoding offsets: " << to_string(decodingOffsetVector, ", ", 0, 3) << std::endl;
      std::cout << "Using decoding Level1: " << to_string(level1Vector, ", ", 0, 3) << std::endl;
      std::cout << "Using decoding alphas: " << to_string(decodingAlphasVector, ", ", 0, 3) << std::endl;

      std::cout << "CMSPixel Converter initialized with detector " << m_detector << ", Event Type " << m_event_type
                << ", TBM type " << tbmtype << " (" << static_cast<int>(m_tbmtype) << ")"
                << ", ROC type " << roctype << " (" << static_cast<int>(m_roctype) << ")" << std::endl;

      hEncode.clear();
      hUblack.clear();
      hBlack.clear();
      hc0.clear();
      hc1.clear();
      hr0.clear();
      hr1.clear();
      hcr.clear();
      pblack.clear();
      pUblack.clear();
      h2c0.clear();
      h2c1.clear();
      h2r0.clear();
      h2r1.clear();
      h2cr.clear();
      h2black.clear();
      h2uBlack.clear();
      h2lastDac.clear();

      if (do_decoding) {
        for (size_t it = 0; it < m_nplanes; it++) {
          hEncode.push_back(new TH1F(TString::Format("encoded_%d", int(it)), TString::Format("encoded_%d", int(it)), 1000, -500, 500));
          hUblack.push_back(new TH1F(TString::Format("uBlack_%d", int(it)), TString::Format("uBlack_%d", int(it)), 1000, -500, 500));
          hBlack.push_back(new TH1F(TString::Format("black_%d", int(it)), TString::Format("black_%d", int(it)), 1000, -500, 500));
          hc0.push_back(new TH1F(TString::Format("c0_%d", int(it)), TString::Format("c0_%d", int(it)), 1000, -500, 500));
          hc1.push_back(new TH1F(TString::Format("c1_%d", int(it)), TString::Format("c1_%d", int(it)), 1000, -500, 500));
          hr0.push_back(new TH1F(TString::Format("r0_%d", int(it)), TString::Format("r0_%d", int(it)), 1000, -500, 500));
          hr1.push_back(new TH1F(TString::Format("r1_%d", int(it)), TString::Format("r1_%d", int(it)), 1000, -500, 500));
          hcr.push_back(new TH1F(TString::Format("cr_%d", int(it)), TString::Format("cr_%d", int(it)), 1000, -500, 500));
          pblack.push_back(new TProfile(TString::Format("pblack_%d", int(it)), TString::Format("pblack_%d", int(it)), 10000, 0, 1000000, -500, 500));
          pUblack.push_back(new TProfile(TString::Format("pUblack_%d", int(it)), TString::Format("pUblack_%d", int(it)), 10000, 0, 1000000, -500, 500));
          h2c0.push_back(new TH2F(TString::Format("h2c0_%d", int(it)), TString::Format("h2c0_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2c1.push_back(new TH2F(TString::Format("h2c1_%d", int(it)), TString::Format("h2c1_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2r0.push_back(new TH2F(TString::Format("h2r0_%d", int(it)), TString::Format("h2r0_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2r1.push_back(new TH2F(TString::Format("h2r1_%d", int(it)), TString::Format("h2r1_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2cr.push_back(new TH2F(TString::Format("h2cr_%d", int(it)), TString::Format("h2cr_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2black.push_back(new TH2F(TString::Format("h2black_%d", int(it)), TString::Format("h2black_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2uBlack.push_back(new TH2F(TString::Format("h2uBlack_%d", int(it)), TString::Format("h2uBlack_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
          h2lastDac.push_back(new TH2F(TString::Format("h2lastDac_%d", int(it)), TString::Format("h2lastDac_%d", int(it)), 10000, 0, 1000000, 1000, -500, 500));
        }
      }
      uBlackV = new std::vector<float>();
      blackV = new std::vector<float>();
      levelSV = new std::vector<int16_t>();
      decodingOffsetVector2 = new std::vector<float>();
      uBlackV->resize(16, 0.);
      blackV->resize(16, 0.);
      levelSV->resize(16, 0);
      decodingOffsetVector2->resize(16, 0.);
      for (size_t it = 0; it < decodingOffsetVector.size(); it++)
        decodingOffsetVector2->at(it) = decodingOffsetVector[it];
    }

    void WriteDecoding() {
      TDirectory *g_dir = std::string(gDirectory->GetName()).find("DecodingHistos") != std::string::npos ? gDirectory : gDirectory->GetDirectory("DecodingHistos");
      g_dir->cd();
      for (size_t it = 0; it < hEncode.size(); it++)
        hEncode[it]->Write();
      for (size_t it = 0; it < hUblack.size(); it++)
        hUblack[it]->Write();
      for (size_t it = 0; it < hBlack.size(); it++)
        hBlack[it]->Write();
      for (size_t it = 0; it < hc0.size(); it++)
        hc0[it]->Write();
      for (size_t it = 0; it < hc1.size(); it++)
        hc1[it]->Write();
      for (size_t it = 0; it < hr0.size(); it++)
        hr0[it]->Write();
      for (size_t it = 0; it < hr1.size(); it++)
        hr1[it]->Write();
      for (size_t it = 0; it < hcr.size(); it++)
        hcr[it]->Write();
      for (size_t it = 0; it < pblack.size(); it++)
        pblack[it]->Write();
      for (size_t it = 0; it < pUblack.size(); it++)
        pUblack[it]->Write();
      for (size_t it = 0; it < h2c0.size(); it++)
        h2c0[it]->Write();
      for (size_t it = 0; it < h2c1.size(); it++)
        h2c1[it]->Write();
      for (size_t it = 0; it < h2r0.size(); it++)
        h2r0[it]->Write();
      for (size_t it = 0; it < h2r1.size(); it++)
        h2r1[it]->Write();
      for (size_t it = 0; it < h2cr.size(); it++)
        h2cr[it]->Write();
      for (size_t it = 0; it < h2black.size(); it++)
        h2black[it]->Write();
      for (size_t it = 0; it < h2uBlack.size(); it++)
        h2uBlack[it]->Write();
      for (size_t it = 0; it < h2lastDac.size(); it++)
        h2lastDac[it]->Write();
      for (size_t it = 0; it < hEncode.size(); it++)
        delete hEncode[it];
      for (size_t it = 0; it < hUblack.size(); it++)
        delete hUblack[it];
      for (size_t it = 0; it < hBlack.size(); it++)
        delete hBlack[it];
      for (size_t it = 0; it < hc0.size(); it++)
        delete hc0[it];
      for (size_t it = 0; it < hc1.size(); it++)
        delete hc1[it];
      for (size_t it = 0; it < hr0.size(); it++)
        delete hr0[it];
      for (size_t it = 0; it < hr1.size(); it++)
        delete hr1[it];
      for (size_t it = 0; it < hcr.size(); it++)
        delete hcr[it];
      for (size_t it = 0; it < pblack.size(); it++)
        delete pblack[it];
      for (size_t it = 0; it < pUblack.size(); it++)
        delete pUblack[it];
      for (size_t it = 0; it < h2c0.size(); it++)
        delete h2c0[it];
      for (size_t it = 0; it < h2c1.size(); it++)
        delete h2c1[it];
      for (size_t it = 0; it < h2r0.size(); it++)
        delete h2r0[it];
      for (size_t it = 0; it < h2r1.size(); it++)
        delete h2r1[it];
      for (size_t it = 0; it < h2cr.size(); it++)
        delete h2cr[it];
      for (size_t it = 0; it < h2black.size(); it++)
        delete h2black[it];
      for (size_t it = 0; it < h2uBlack.size(); it++)
        delete h2uBlack[it];
      for (size_t it = 0; it < h2lastDac.size(); it++)
        delete h2lastDac[it];
      std::cout << "saved histograms in root file under DecodingHistos" << std::endl;
      g_dir->cd("../");
    }

    std::string GetStats() {
      std::cout << "Getting decoding statistics for detector " << m_detector << std::endl;
      if (do_decoding) { WriteDecoding(); }
      return decoding_stats.getString();
    }

    void read_PHCalibrationData(const Configuration &cnf) {
      std::cout << "TRY TO READ PH CALIBRATION DATA... ";
      bool foundData;
      for (auto i: cnf.GetSections()) {
        if (i.find("Producer.") == -1) continue;
        cnf.SetSection(i);
        std::string roctype = cnf.Get("roctype", "roctype", "");
        if (roctype == "") continue;
        bool is_digital = !(roctype.find("dig") == -1);

        std::string fname = m_conv_cfg->GetKeys().size() > 0 ? m_conv_cfg->Get("phCalibrationFile", "") : "";
        if (fname == "") fname = cnf.Get("phCalibrationFile", "");
        if (fname == "") {
          fname = cnf.Get("dacFile", "");
          size_t found = fname.find_last_of("/");
          fname = fname.substr(0, found);
        }
        fname += (is_digital) ? "/phCalibrationFitErr" : "/phCalibrationGErfFit";
        std::string i2c = cnf.Get("i2c", "i2caddresses", "0");
        if (i.find("REF") != -1)
          foundData = read_PH_CalibrationFile("REF", fname, i2c, roc_calibrations.at(roctype));
        else if (i.find("ANA") != -1)
          foundData = read_PH_CalibrationFile("ANA", fname, i2c, roc_calibrations.at(roctype));
        else if (i.find("DIG") != -1)
          foundData = read_PH_CalibrationFile("DIG", fname, i2c, roc_calibrations.at(roctype));
        else if (i.find("TRP") != -1)
          foundData = read_PH_CalibrationFile("TRP", fname, i2c, roc_calibrations.at(roctype));
        else
          foundData = read_PH_CalibrationFile("DUT", fname, i2c, roc_calibrations.at(roctype));
      }
      /** only do a conversion if we found data */
      do_conversion = foundData;
    }

    bool read_PH_CalibrationFile(std::string roc_type, std::string fname, std::string i2cs, float factor) {
      std::vector<std::string> vec_i2c = split(i2cs, " ");
      size_t nRocs = vec_i2c.size();

      // getting vcal-charge translation from file
      //
      float par0, par1, par2, par3;
      int row, col;
      std::string dump;
      char trash[30];
      for (uint16_t iroc = 0; iroc < nRocs; iroc++) {
        std::string i2c = vec_i2c.at(iroc);
        FILE *fp;
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        TString filename = fname;
        filename += "_C" + i2c + ".dat";
        std::cout << "Filename in CMSPixelHelper: " << filename << std::endl;
        fp = fopen(filename, "r");

        VCALDict tmp_vcaldict;
        if (!fp) {
//          std::cout <<  "  DID NOT FIND A FILE TO GO FROM ADC TO CHARGE!!" << std::endl;
          return false;
        } else {
          // jump to fourth line
          for (uint8_t i = 0; i < 3; i++) if (!getline(&line, &len, fp)) return false;

          int q = 0;
          while (fscanf(fp, "%f %f %f %f %s %d %d", &par0, &par1, &par2, &par3, trash, &col, &row) == 7) {
            tmp_vcaldict.par0 = par0;
            tmp_vcaldict.par1 = par1;
            tmp_vcaldict.par2 = par2;
            tmp_vcaldict.par3 = par3;
            tmp_vcaldict.calibration_factor = factor;
            std::string identifier = roc_type + std::string(TString::Format("%01d%02d%02d", iroc, row, col));
            q++;
            vcal_vals[identifier] = tmp_vcaldict;
          }
          fclose(fp);
        }
      }
      return true;
    }

    void UpdateHeaderVectors(dtbEventDecoder decoder, std::vector<float> *tvectUB, std::vector<float> *tvectB, std::vector<int16_t> *tvectLS, std::vector<float> *tvectDOff) const {
      std::vector<float> tempUB = decoder.GetUBlack();
      std::vector<float> tempB = decoder.GetBlack();
      std::vector<int16_t> tempLS = decoder.GetLevelS();
      std::vector<float> tempDOFF = decoder.GetDecodingOffsets();
      for (size_t it = 0; it < tempUB.size() and it < tvectUB->size(); it++)
        (*tvectUB).at(it) = tempUB[it];
      for (size_t it = 0; it < tempB.size() and it < tvectB->size(); it++)
        (*tvectB).at(it) = tempB[it];
      for (size_t it = 0; it < tempLS.size() and it < tvectLS->size(); it++)
        (*tvectLS).at(it) = tempLS[it];
      for (size_t it = 0; it < tempDOFF.size() and it < tvectDOff->size(); it++)
        (*tvectDOff).at(it) = tempDOFF[it];
    }

    void FillDecodingVectors(dtbEventDecoder decoder, const uint32_t event_number) const {

      if (do_decoding) {
        for (size_t roc = 0; roc < m_nplanes; roc++) {
          for (short value : decoder.Getc0Vect(roc)) {
            hEncode[roc]->Fill(value);
            hc0[roc]->Fill(value);
            h2c0[roc]->Fill(event_number, value);
          }
          for (short value : decoder.Getc1Vect(roc)) {
            hEncode[roc]->Fill(value);
            hc1[roc]->Fill(value);
            h2c1[roc]->Fill(event_number, value);
          }
          for (short value : decoder.Getr0Vect(roc)) {
            hEncode[roc]->Fill(value);
            hr0[roc]->Fill(value);
            h2r0[roc]->Fill(event_number, value);
          }
          for (short value : decoder.Getr1Vect(roc)) {
            hEncode[roc]->Fill(value);
            hr1[roc]->Fill(value);
            h2r1[roc]->Fill(event_number, value);
          }
          for (short value : decoder.GetcrVect(roc)) {
            hEncode[roc]->Fill(value);
            hcr[roc]->Fill(value);
            h2cr[roc]->Fill(event_number, value);
          }
          for (short value : decoder.GetblackVect(roc)) {
            hEncode[roc]->Fill(value);
            hBlack[roc]->Fill(value);
            pblack[roc]->Fill(event_number, value);
            h2black[roc]->Fill(event_number, value);
          }
          for (short value : decoder.GetUblackVect(roc)) {
            hEncode[roc]->Fill(value);
            hUblack[roc]->Fill(value);
            pUblack[roc]->Fill(event_number, value);
            h2uBlack[roc]->Fill(event_number, value);
          }
          for (short value : decoder.GetLastDacVect(roc)) {
            h2lastDac[roc]->Fill(event_number, value);
          }
        }
      }
    }

    bool GetStandardSubEvent(StandardEvent &out, const Event &in) const {

      // If we receive the EORE print the collected statistics:
      if (out.IsEORE()) {
        std::cout << "is eore" << std::endl;
        // Set decoder to INFO level for statistics printout:
        if (do_decoding) {
          TDirectory *blaf = nullptr;
          if (TString::Format("DecodingHistos").CompareTo(gDirectory->GetName()) != 0) {
            blaf = (TDirectory *) gDirectory->GetDirectory("DecodingHistos");
          } else {
            blaf = gDirectory;
          }
          blaf->cd();
          for (size_t it = 0; it < hEncode.size(); it++)
            hEncode[it]->Write();
          for (size_t it = 0; it < hUblack.size(); it++)
            hUblack[it]->Write();
          for (size_t it = 0; it < hBlack.size(); it++)
            hBlack[it]->Write();
          for (size_t it = 0; it < hc0.size(); it++)
            hc0[it]->Write();
          for (size_t it = 0; it < hc1.size(); it++)
            hc1[it]->Write();
          for (size_t it = 0; it < hr0.size(); it++)
            hr0[it]->Write();
          for (size_t it = 0; it < hr1.size(); it++)
            hr1[it]->Write();
          for (size_t it = 0; it < hcr.size(); it++)
            hcr[it]->Write();
          for (size_t it = 0; it < pblack.size(); it++)
            pblack[it]->Write();
          for (size_t it = 0; it < pUblack.size(); it++)
            pUblack[it]->Write();
          for (size_t it = 0; it < h2c0.size(); it++)
            h2c0[it]->Write();
          for (size_t it = 0; it < h2c1.size(); it++)
            h2c1[it]->Write();
          for (size_t it = 0; it < h2r0.size(); it++)
            h2r0[it]->Write();
          for (size_t it = 0; it < h2r1.size(); it++)
            h2r1[it]->Write();
          for (size_t it = 0; it < h2cr.size(); it++)
            h2cr[it]->Write();
          for (size_t it = 0; it < h2black.size(); it++)
            h2black[it]->Write();
          for (size_t it = 0; it < h2uBlack.size(); it++)
            h2uBlack[it]->Write();
          for (size_t it = 0; it < h2lastDac.size(); it++)
            h2lastDac[it]->Write();
          for (size_t it = 0; it < hEncode.size(); it++)
            delete hEncode[it];
          for (size_t it = 0; it < hUblack.size(); it++)
            delete hUblack[it];
          for (size_t it = 0; it < hBlack.size(); it++)
            delete hBlack[it];
          for (size_t it = 0; it < hc0.size(); it++)
            delete hc0[it];
          for (size_t it = 0; it < hc1.size(); it++)
            delete hc1[it];
          for (size_t it = 0; it < hr0.size(); it++)
            delete hr0[it];
          for (size_t it = 0; it < hr1.size(); it++)
            delete hr1[it];
          for (size_t it = 0; it < hcr.size(); it++)
            delete hcr[it];
          for (size_t it = 0; it < pblack.size(); it++)
            delete pblack[it];
          for (size_t it = 0; it < pUblack.size(); it++)
            delete pUblack[it];
          for (size_t it = 0; it < h2c0.size(); it++)
            delete h2c0[it];
          for (size_t it = 0; it < h2c1.size(); it++)
            delete h2c1[it];
          for (size_t it = 0; it < h2r0.size(); it++)
            delete h2r0[it];
          for (size_t it = 0; it < h2r1.size(); it++)
            delete h2r1[it];
          for (size_t it = 0; it < h2cr.size(); it++)
            delete h2cr[it];
          for (size_t it = 0; it < h2black.size(); it++)
            delete h2black[it];
          for (size_t it = 0; it < h2uBlack.size(); it++)
            delete h2uBlack[it];
          for (size_t it = 0; it < h2lastDac.size(); it++)
            delete h2lastDac[it];
          std::cout << "saved histograms under DecodingHistos" << std::endl;
          blaf->cd("../");
        }
        std::cout << "Decoding statistics for detector " << m_detector << std::endl;
        pxar::Log::ReportingLevel() = pxar::Log::FromString("INFO");
        decoding_stats.dump();
        return true;
      }

      // Check if we have BORE or EORE:
      if (in.IsBORE() || in.IsEORE()) {
        std::cout << "is bore or eore" << std::endl;
        return true;
      }

      // Check ROC type from event tags:
      if (m_roctype == 0x0) {
        EUDAQ_ERROR("Invalid ROC type\n");
        return false;
      }

      const auto & in_raw = dynamic_cast<const RawDataEvent &>(in);
      // Check of we have more than one data block:
      if (in_raw.NumBlocks() > 1) {
        EUDAQ_ERROR("Only one data block is expected!");
        return false;
      }

      // Set decoder to reasonable verbosity (still informing about problems:
      pxar::Log::ReportingLevel() = pxar::Log::FromString("CRITICAL");
      //pxar::Log::ReportingLevel() = pxar::Log::FromString("DEBUGPIPES");

      // The pipeworks:
      evtSource src;
      passthroughSplitter splitter;
      dtbEventDecoder decoder;
      decoder.setLevel1s(std::vector<float>(level1Vector.begin(), level1Vector.end()));
      decoder.setAlphas(std::vector<float>(decodingAlphasVector.begin(), decodingAlphasVector.end()));
      decoder.setBlackOffsets(decodingOffsetVector);
      decoder.SetBlackVectors(*uBlackV, *blackV, *levelSV, *decodingOffsetVector2);
      dataSink<pxar::Event *> Eventpump;
      pxar::Event *evt;
      int n = 0;
      try {

        // Connect the data source and set up the pipe:
        src = evtSource(0, m_nplanes, 0, m_tbmtype, m_roctype);
        src >> splitter >> decoder >> Eventpump;

        // Transform from EUDAQ data, add it to the datasource:
        src.AddData(TransformRawData(in_raw.GetBlock(0)));
        // ...and pull it out at the other end:
        evt = Eventpump.Get();

        decoding_stats += decoder.getStatistics();
        UpdateHeaderVectors(decoder, uBlackV, blackV, levelSV, decodingOffsetVector2);
        FillDecodingVectors(decoder, in.GetEventNumber());
      }
      catch (std::exception &e) {
        EUDAQ_WARN("Decoding crashed at event (" + m_detector + ") " + to_string(in.GetEventNumber()) + ":");
        std::ofstream f;
        std::string runNumber = to_string(in.GetRunNumber());
        std::string filename = "Errors" + std::string(3 - runNumber.length(), '0') + runNumber + ".txt";
        f.open(filename, std::ios::out | std::ios::app);
        f << in.GetEventNumber() << "\n";
        f.close();
        std::cout << e.what() << std::endl;
        return false;
      }

      // Iterate over all planes and check for pixel hits:

      for (size_t roc = 0; roc < m_nplanes; roc++) {

        // We are using the event's "sensor" (m_detector) to distinguish DUT and REF:
        StandardPlane plane(roc, m_event_type, m_detector);
        plane.SetTrigCount(evt->triggerCount());
        plane.SetTrigPhase(evt->triggerPhase());

        // Initialize the plane size (zero suppressed), set the number of pixels
        // Check which carrier PCB has been used and book planes accordingly:
        if (m_rotated_pcb) { plane.SetSizeZS(ROC_NUMROWS, ROC_NUMCOLS, 0); }
        else { plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0); }
        plane.SetTLUEvent(0);

        // Store all decoded pixels belonging to this plane:
        for (std::vector<pxar::pixel>::iterator it = evt->pixels.begin(); it != evt->pixels.end(); ++it) {
          // Check if current pixel belongs on this plane:
          if (it->roc() == roc) {
            std::string identifier = (std::string) m_detector + (std::string) TString::Format("%01zu%02d%02d", roc, it->row(), it->column());

            float charge;
            if (do_conversion) {
              charge = getCharge(vcal_vals.find(identifier)->second, it->value());
              if (charge < 0) {
                EUDAQ_WARN(std::string("Invalid cluster charge -" + to_string(charge) + "/" + to_string(it->value())));
                charge = 0;
              }
            } else
              charge = it->value();

            //std::cout << "filling charge " <<it->value()<<" "<< charge << " "<<factor<<" "<<identifier<<std::endl;
            if (m_rotated_pcb) { plane.PushPixel(it->row(), it->column(), charge /*it->value()*/); }
            else { plane.PushPixel(it->column(), it->row(), charge /*it->value()*/); }
          }
        }

        // Add plane to the output event:
        out.AddPlane(plane);
      }
      return true;
    }

    #if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & conf) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      // Type of data: real.
      runHeader.setDAQHWName(EUTELESCOPE::DAQDATA);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const {

      if(source.IsBORE()) {
  std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent BORE " << source << std::endl;
  return true;
      } else if(source.IsEORE()) {
  std::cout << "CMSPixelConverterPlugin::GetLCIOSubEvent EORE " << source << std::endl;
  return true;
      }

      // Set event type Data Event (kDE):
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);

      // Prepare the data collection and check for its existence:
      LCCollectionVec * zsDataCollection;
      bool zsDataCollectionExists = false;
      try {
  /// FIXME choose another name for the collection!
  zsDataCollection = static_cast<LCCollectionVec*>(result.getCollection(m_event_type));
  zsDataCollectionExists = true;
      } catch(lcio::DataNotAvailableException& e) {
  zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      // Set the proper cell encoder
      CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);

      // Prepare a description of the setup
      std::vector<eutelescope::EUTelSetupDescription*> setupDescription;

      // Decode the raw data and retrieve the eudaq StandardEvent:
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);

      // Loop over all planes available in the data stream:
      for (size_t iPlane = 0; iPlane < tmp_evt.NumPlanes(); ++iPlane) {
  StandardPlane plane = static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

  // The current detector is ...
  eutelescope::EUTelPixelDetector * currentDetector = 0x0;
  if(plane.Sensor() == "DUT" || plane.Sensor() == "REF" || plane.Sensor() == "TRP") {

    currentDetector = new eutelescope::EUTelCMSPixelDetector;
    // FIXME what is that mode used for?
    std::string mode = "ZS";
    currentDetector->setMode(mode);
    if(result.getEventNumber() == 0) {
      setupDescription.push_back(new eutelescope::EUTelSetupDescription(currentDetector));
    }
  } else {
    EUDAQ_ERROR("Unrecognised sensor type in LCIO converter: " + plane.Sensor());
    return true;
  }

  zsDataEncoder["sensorID"] = plane.ID();
  zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

  // Get the total number of pixels
  size_t nPixel = plane.HitPixels();

  // Prepare a new TrackerData for the ZS data
  std::auto_ptr<lcio::TrackerDataImpl>zsFrame(new lcio::TrackerDataImpl);
  zsDataEncoder.setCellID(zsFrame.get());

  // This is the structure that will host the sparse pixels
  std::auto_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel> >
    sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

  // Prepare a sparse pixel to be added to the sparse data:
  std::auto_ptr<eutelescope::EUTelGenericSparsePixel> sparsePixel(new eutelescope::EUTelGenericSparsePixel);
  for(size_t iPixel = 0; iPixel < nPixel; ++iPixel) {

    // Fill the sparse pixel coordinates with decoded data:
    sparsePixel->setXCoord((size_t)plane.GetX(iPixel));
    sparsePixel->setYCoord((size_t)plane.GetY(iPixel));
    // Fill the pixel charge:

    sparsePixel->setSignal((int32_t)plane.GetPixel(iPixel));

    // Add the pixel to the readout frame:
    sparseFrame->addSparsePixel(sparsePixel.get());
  }

  // Now add the TrackerData to the collection
  zsDataCollection->push_back(zsFrame.release());
  delete currentDetector;

      } // loop over all planes

      // Add the collection to the event only if not empty and not yet there
      if(!zsDataCollectionExists){
  if(zsDataCollection->size() != 0) {
    result.addCollection(zsDataCollection, m_event_type);
  } else {
    delete zsDataCollection; // clean up if not storing the collection here
  }
      }

      return true;
    }
    #endif

  private:
    uint8_t m_roctype, m_tbmtype;
    size_t m_planeid;
    size_t m_nplanes;
    std::string m_detector;
    bool m_rotated_pcb;
    std::string m_event_type;
    mutable pxar::statistics decoding_stats;
    bool do_conversion;
    bool do_decoding;
    Configuration *m_conv_cfg;
    std::vector<float> decodingOffsetVector;
    std::vector<float> decodingAlphasVector;
    std::vector<float> level1Vector;

    static std::vector<uint16_t> TransformRawData(const std::vector<unsigned char> &block) {
      /** Transform data of form char* to vector<int16_t> */
      std::vector<uint16_t> rawData;
      if (block.size() < 2) { return rawData; }
      int i = 0;
      while (i < block.size() - 1) {
        uint16_t temp = ((uint16_t) block.data()[i + 1] << 8) | block.data()[i];
        rawData.push_back(temp);
        i += 2;
//        hEncode->Fill(temp);
      }
      return rawData;
    }
  };

}
