//
// Created by micha on 30.03.20.
//
#ifndef EUDAQ_CMSDECODER_HH
#define EUDAQ_CMSDECODER_HH

#include <TString.h>
#include <TF1.h>
#include "FileWriter.hh"
class TFile;
class TDirectory;
class TH1F;
class TF1;

namespace eudaq {

class CMSDecoder : public FileWriter {

public:
  explicit CMSDecoder(const std::string &);
  void WriteEvent(const DetectorEvent & ev) override;
  void StartRun(unsigned /*run_number*/) override;
  long GetMaxEventNumber() override { return f_max_event_number; }
  void Run() override;
  std::string GetStats(const DetectorEvent &dev) override;

  std::vector<float> GetBlackOffsets() override { return *m_black_offsets; }
  std::vector<float> GetLeve1Offsets() override { return *m_level1_offsets; }
  std::vector<float> GetAlphas() override { return *m_alphas; }
  void PrintResults() override;
  void SaveResults() override;

private:
  uint64_t FileBytes() const override { return 0; }
  TString f_file_name;
  size_t f_max_event_number;
  size_t f_event_number;
  TFile * m_tfile;
  TDirectory * m_thdir;
  std::vector<float> * m_black_offsets;
  std::vector<float> * m_level1_offsets;

  std::vector<float> * m_alphas;
  static TString * GetFitString(int, int, float);
  static int FindFirstBinAbove(TH1F*, float, int, int);
  static int FindLastBinAbove(TH1F*, float, int, int);
  static TF1 * CreateFit(TH1F *blah, TH1F *hblack, int NL, int NSp, int NTotal, int rocn);
  static float GetTimeCorrectedValue(float, float, float);
};

}

#endif //EUDAQ_CMSDECODER_HH
