//
// Created by micha on 30.03.20.
//
#include <Math/MinimizerOptions.h>
#include <TMath.h>
#include "eudaq/CMSDecoder.hh"
#include "eudaq/PluginManager.hh"
#include "TFile.h"
#include "TH1F.h"


using namespace std;
using namespace eudaq;

namespace { static RegisterFileWriter<CMSDecoder> reg("cmsdecoder"); }

CMSDecoder::CMSDecoder(const std::string & /*unused*/): f_file_name(""), f_max_event_number(100000), f_event_number(0), m_tfile(nullptr), m_thdir(nullptr) {

  m_black_offsets = new vector<float>(0);
  m_level1_offsets = new vector<float>(0);
  m_alphas = new vector<float>(0);
}

void CMSDecoder::StartRun(unsigned run_number){
  f_file_name = TString::Format("decoding_%d.root", run_number);
  m_tfile = new TFile(f_file_name, "RECREATE");
  cout << "\nCreating file \"decoding_" << int(run_number) << ".root\"" << endl;
  m_thdir = m_tfile->mkdir("DecodingHistos");
  m_thdir->cd();
  cout << "Created ROOT file directory \"DecodingHistos\"" << endl;
}

void CMSDecoder::WriteEvent(const DetectorEvent & ev) {
  if (ev.IsBORE()) {
    eudaq::PluginManager::SetConfig(ev, m_config);
    eudaq::PluginManager::Initialize(ev);
    cout << "loading the first event..." << endl;
    return;
  }
  if (ev.IsEORE()) {
    eudaq::PluginManager::ConvertToStandard(ev);
    cout << "loaded the last event." << endl;
    return;
  }

  StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
}

void CMSDecoder::Run() {
  m_tfile->cd();
  if (m_tfile->IsOpen()) { m_tfile->Close(); }
  auto file = new TFile(f_file_name, "UPDATE");  // reload file because of instability of the old one
  auto tdir = dynamic_cast<TDirectory*>(file->Get("DecodingHistos"));
  auto * blacks = new std::vector<float>(0);
  for(size_t i_plane = 0; i_plane < 16; i_plane++) {
    if (tdir->Get(TString::Format("cr_%d", int(i_plane))) != nullptr) {
      auto * h = dynamic_cast<TH1F *>( tdir->Get(TString::Format("cr_%d", int(i_plane))));
      auto *hblack = dynamic_cast<TH1F *>( tdir->Get(TString::Format("black_%d", int(i_plane))));
      auto *hublack = dynamic_cast<TH1F *>( tdir->Get(TString::Format("uBlack_%d", int(i_plane))));
      TF1 * fit = CreateFit(h, hblack, 3, 6, 6, int(i_plane));
      ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(100000);
      ROOT::Math::MinimizerOptions::SetDefaultTolerance(0.000001);
      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Migrad"); // 2.2 seconds good fitting parameters
//      ROOT::Math::MinimizerOptions::SetDefaultMaxIterations(10000000);
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiFit", "GSLLM"); // 0.6 sec large chi2
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Scan"); //
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2", "Simplex"); //
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Fumili2"); //
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","BFGS2"); // Too long more than 10 mins.
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","CONJFR"); // Too long more than 10 mins.
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLMultiMin","CONJPR"); // Too long more than 10 mins.
//      ROOT::Math::MinimizerOptions::SetDefaultMinimizer("GSLSimAn","SimAn"); // 142 sec
//      h->Fit(TString::Format("blaf1_%d", int(i_plane)).Data(), "0Q");
      h->Fit(TString::Format("blaf1_%d", int(i_plane)).Data(), "0Q");
      auto black(float(hblack->GetMean()));
      auto uBlack(float(hublack->GetMean()));
      auto delta(float(fit->GetParameter(1)));
      auto l1Spacing(float(fit->GetParameter(2) + fit->GetParameter(1)));
      /** check if the first fitted peak is the extra one */
      float p0 = fit->GetParameter(5) > fit->GetParameter(4) * 5 ? float(fit->GetParameter(0) + delta) : float(fit->GetParameter(0));
//      auto Level1(float(l1Spacing + p0 + 4 * delta));
      auto Level1(float(l1Spacing + p0));
      auto alpha(float(delta / l1Spacing));
      auto uBlackCorrected = GetTimeCorrectedValue(uBlack, 0, alpha);
      auto blackCorrected = GetTimeCorrectedValue(black, uBlackCorrected, alpha);
      m_alphas->push_back(alpha);
      blacks->push_back(blackCorrected);
      m_level1_offsets->push_back(Level1);
      m_black_offsets->push_back(Level1 - blackCorrected);
      tdir->cd();
      fit->Write();
      file->cd();
    }
  }
  file->Close();
}

float CMSDecoder::GetTimeCorrectedValue(float value, float prevValue, float alpha){
  return (value - prevValue * alpha) / (1 - alpha);
}

TString * CMSDecoder::GetFitString(int NL, int NSp, float sigmaSp) {

  auto * temps = new TString("0");
  for(int m = 0; m < NL; m++){
    for(int n = 0; n < NSp; n++){
      temps->Append(TString::Format("+([%d]*TMath::Exp(-TMath::Power((x-[0]-%d*[1]-%d*[2]-%f*[%d])/(TMath::Sqrt(2)*[3]),2)))", 4 + NSp * m + n, n, m, sigmaSp, 4 + NSp * (NL + m) + n));
    }
  }
  return temps;
}

TF1 * CMSDecoder::CreateFit(TH1F* blah, TH1F* hblack, int NL, int NSp, int NTotal, int rocn){
  /**
   * This method calculates the decoding offsets from a histogram.
   * @param blah: is the histogram with the encoded levels. Tipically it is CR because it has homogeneous peaks as it is used to decide left pixel or right pixel
   * @param hblack: is the black histogram used to estimate the real width of the levels
   * @param NL: is the number of Levels to fit. 3 is used as default
   * @param NSp: is the number of peaks on each Level. Noticeable when the timing was not set up correctly. For CR it should be 6.
   * @param NTotal: is the total number of Levels in the histogram. For CR it should be 6 (default)
   * @return A pointer to the fitted function.
   * */
  int NSp2 = NSp + 1; // assume there is an extra peak to help the fitter. The extra peak will have a lower scaling constant much smaller than the other ones.
  auto sigmaSp = float(hblack->GetRMS());
  TString *blast = GetFitString(NL, NSp2, float(sigmaSp * 2));
  int entries = int(blah->GetEntries());
  auto maxh = float(blah->GetMaximum());
  float fracth = 20;
//  int binl(int(blah->FindFirstBinAbove(0)));
//  int binh(int(blah->FindLastBinAbove(0)));
//  float xmin(float(blah->GetBinLowEdge(binl))), xmax(float(blah->GetBinLowEdge(binh + 1)));
//  Find levels limits
//  int binl0(FindFirstBinAbove(blah, maxh / fracth, binl, binh)), binh0(FindLastBinAbove(blah, maxh / fracth, binl, binh));
  int binl0(blah->FindFirstBinAbove(maxh / fracth));
  int binh0(blah->FindLastBinAbove(maxh / fracth));
  auto xmin0(float(blah->GetBinLowEdge(binl0)));
  auto xmax0(float(blah->GetBinLowEdge(binh0 + 1)));
  // Find an estimate for l1; the spacing between clusters
  auto widthp(float(2 * sigmaSp * TMath::Sqrt(2 * TMath::Log(fracth)))); // From FWPercent(p) = 2 * sigma * sqrt(2 * Ln(1/p)) for p = 1/fracth
  auto l1(float((xmax0 - xmin0) / (NTotal - 1.) - 2 * widthp));
  // Find First cluster limits
  int binlN(0);
  int binhN(0);
  int binhP(FindLastBinAbove(blah, maxh / fracth, binl0 + 1, int(blah->FindBin(xmin0 + l1))));
  float xminN(0);
  float xmaxN(0);
  auto xmaxP(float(blah->GetBinLowEdge(binhP + 1)));
  float xmax10(xmaxP);
  // Find cluster NL limits
  for(size_t it = 2; it <= NL; it++){
    binlN = FindFirstBinAbove(blah, maxh / fracth, binhP + 1, int(blah->FindBin(xmaxP + l1)));
    xminN = float(blah->GetBinLowEdge(binlN));
    binhN = FindLastBinAbove(blah, maxh / fracth, binlN + 1, int(blah->FindBin(xminN + l1)));
    xmaxN = float(blah->GetBinLowEdge(binhN + 1));
    binhP = binhN;
    xmaxP = xmaxN;
  }
  // Guess the offset on each cluster due to bad timing setting, a more accurate spacing between clusters, and the first position of the first cluster
  float delta0((xmax10 - xmin0 - widthp) / float(NSp2 - 1));
  float l1Guess((xminN - xmin0) / float(NL - 1));
  float p0(xmin0 + widthp / 2);
  // Create Fitting function
  TF1* tempf = new TF1(TString::Format("blaf1_%d",rocn).Data(), blast->Data(), xmin0, xmaxN);
  tempf->SetNpx(1000);
  tempf->SetParLimits(0, xmin0 - 50, xmin0 + delta0 / 4 + widthp);
  tempf->SetParLimits(1, 0, 1.5 * delta0);
  tempf->SetParLimits(2, l1Guess / 2, 1.5 * l1Guess);
  tempf->SetParLimits(3, 0.1, 3 * sigmaSp);
  tempf->SetParameter(0, p0);
  tempf->SetParameter(1, delta0);
  tempf->SetParameter(2, l1Guess);
  tempf->SetParameter(3, sigmaSp);
  for(int m = 0; m < NL; m++){
    for(int n = 0; n < NSp2; n++){
      tempf->SetParLimits(4 + NSp2 * m + n, 0, entries);
      tempf->SetParameter(4 + NSp2 * m + n, 0.8 * maxh);
      tempf->SetParLimits(4 + NSp2 * (NL + m) + n, -1, 1);
      tempf->SetParameter(4 + NSp2 * (NL + m) + n, 0.01);
    }
//        tempf->SetParLimits(4 + NSp2 * NL + m, -1, 1);
//        tempf->SetParameter(4 + NSp2 * NL + m, 0.01);
  }
  return tempf;
}

int CMSDecoder::FindFirstBinAbove(TH1F* h, float value, int bin_low, int bin_high){

  if (h->GetBinContent(bin_low) > value){
    return bin_low != 1 ? FindFirstBinAbove(h, value, bin_low - 1, bin_high) : bin_low; }
  if (h->GetBinContent(bin_low) <= value && h->GetBinContent(bin_high) <= value) {
    return FindFirstBinAbove(h, value, bin_low, bin_high + 1); }
  for (int i_bin = bin_low; i_bin <= bin_high; i_bin++) {
    if (h->GetBinContent(i_bin) > value) {
      return i_bin;}
  }
  return bin_high;
}

int CMSDecoder::FindLastBinAbove(TH1F* h, float value, int bin_low, int bin_high){

  if (h->GetBinContent(bin_high) > value){
    return bin_low != 1 ? FindLastBinAbove(h, value, bin_low, bin_high + 1) : bin_high; }
  for (int i_bin = bin_low; i_bin <= bin_high; i_bin++) {
    if (h->GetBinContent(i_bin) > value) {
      return bin_high + bin_low - i_bin;}
  }
  return bin_low;
}

std::string CMSDecoder::GetStats(const DetectorEvent &dev) {
  return PluginManager::GetStats(dev);
}

void CMSDecoder::PrintResults() {

  std::cout << "Calculated decoding offets: " << to_string(*m_black_offsets, ", ", 0, 3) << std::endl;
  std::cout << "Calculated levels 1: " << to_string(*m_level1_offsets, ", ", 0, 2) << std::endl;
  std::cout << "Calculated time compensations (alphas): " << to_string(*m_alphas, ", ", 0, 2) << std::endl;
}

void CMSDecoder::SaveResults() {

  m_config->SetSection("Converter.telescopetree");
  m_config->Set("decoding_offset_v", to_string(*m_black_offsets, ",", 0, 3));
  m_config->Set("decoding_l1_v", to_string(*m_level1_offsets, ",", 0, 2));
  m_config->Set("decoding_alphas_v", to_string(*m_alphas, ",", 0, 2));
  m_config->Save();
}

