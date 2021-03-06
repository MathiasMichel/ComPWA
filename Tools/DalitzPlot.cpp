// Copyright (c) 2013, 2017 The ComPWA Team.
// This file is part of the ComPWA framework, check
// https://github.com/ComPWA/ComPWA/license.txt for details.

#include <stdio.h>
#include <numeric>

#include "Math/ProbFuncMathCore.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TCanvas.h"

#include "Core/DataPoint.hpp"
#include "Core/Logging.hpp"
#include "Core/ProgressBar.hpp"
#include "Physics/PartialAmplitude.hpp"
#include "Physics/HelicityFormalism/HelicityKinematics.hpp"

#include "Tools/DalitzPlot.hpp"

#include "Tools/HistTools.hpp"

using namespace ComPWA;
using namespace ComPWA::Physics::HelicityFormalism;
using namespace ComPWA::Tools;

void phspContour(unsigned int xsys, unsigned int ysys, unsigned int n,
                 double *xcoord, double *ycoord) {

  unsigned int num = n;
  if (num % 2 != 0) {
    num -= 1;
    LOG(INFO) << "DalitzKinematics::phspContour() | "
                 "Setting size to a even number. Assure that the size of "
                 "your arrays is "
              << num * 2 + 1 << "!";
  }

  //  std::pair<double,double> xlimits = GetMinMax(xsys);
  //  double binw = (xlimits.second-xlimits.first)/(double)(num);
  //  double ymin,ymax,x;
  //  unsigned int i=0;
  //
  //  for (; i<num; i++)
  //    {
  //    x = i*binw + xlimits.first;
  //    while(x<=xlimits.first) { x+=binw/100; }
  //    while(x>=xlimits.second) { x-=binw/100; }
  //    auto lim = GetMinMaxLocal(ysys,xsys,x);
  //    //      ymin = invMassMin(ysys,xsys,x);
  //    //      ymax = invMassMax(ysys,xsys,x);
  //
  //    xcoord[i]=x; ycoord[i]=lim.first;
  //    xcoord[num*2-i]=x; ycoord[num*2-i]=lim.second;
  //    }
  //  //adding last datapoint
  //  x = i*binw + xlimits.first;
  //  while(x<=xlimits.first) { x+=binw/100; }
  //  while(x>=xlimits.second) { x-=binw/100; }
  //
  //  xcoord[i]=x; ycoord[i]=GetMinMaxLocal(ysys,xsys,x).first;
  return;
}

DalitzPlot::DalitzPlot(std::shared_ptr<Kinematics> kin, std::string name,
                       int bins)
    : Name(name), kin_(kin), _isFilled(0), _bins(bins), _globalScale(1.0),
      _correctForEfficiency(false),
      h_weights("h_weights", "h_weights", bins, 0, 1.01),
      dataDiagrams(kin, "data", "Data", bins),
      phspDiagrams(kin, "phsp", "Phase-space", bins),
      fitHitMissDiagrams(kin, "fitHitMiss", "HitMiss", bins) {
  gStyle->SetOptStat(10); // entries only
  //  gStyle->SetOptStat(1000001); //name and integral
  gStyle->SetOptTitle(0);

  // Full intensity blue
  // Phase-space green
  phspDiagrams.setColor(kGreen);

  phspDiagrams.setStats(0);

  //=== generate contour
  double xpoints[4001], ypoints[4001];
  phspContour(0, 1, 2000, xpoints, ypoints);
  m23m13_contour = TGraph(4001, xpoints, ypoints);
  m23m13_contour.SetMarkerStyle(1);
  m23m13_contour.SetLineColor(kRed);
  m23m13_contour.SetMarkerColor(kRed);
  m23m13_contour.SetMarkerSize(0.0);
  m23m13_contour.SetTitle("phspContour");
  m23m13_contour.SetFillColor(kWhite);
  phspContour(0, 2, 2000, xpoints, ypoints);
  m23m12_contour = TGraph(4001, xpoints, ypoints);
  m23m12_contour.SetMarkerStyle(1);
  m23m12_contour.SetLineColor(kRed);
  m23m12_contour.SetMarkerColor(kRed);
  m23m12_contour.SetMarkerSize(0.0);
  m23m12_contour.SetTitle("phspContour");
  m23m12_contour.SetFillColor(kWhite);
  phspContour(2, 1, 2000, xpoints, ypoints);
  m12m13_contour = TGraph(4001, xpoints, ypoints);
  m12m13_contour.SetMarkerStyle(1);
  m12m13_contour.SetLineColor(kRed);
  m12m13_contour.SetMarkerColor(kRed);
  m12m13_contour.SetMarkerSize(0.0);
  m12m13_contour.SetTitle("phspContour");
  m12m13_contour.SetFillColor(kWhite);
}

DalitzPlot::~DalitzPlot() {}

void DalitzPlot::setFitAmp(std::shared_ptr<ComPWA::AmpIntensity> intens,
                           std::string title, Color_t color) {
  _plotComponents.clear();
  _plotComponents.push_back(intens);
  _plotHistograms.push_back(
      DalitzHisto(kin_, intens->name(), title, _bins, color));
  _plotHistograms.back().setStats(0);
  _plotLegend.push_back("Fit");
  _isFilled = 0;
}

void DalitzPlot::fill(std::shared_ptr<Kinematics> kin) {
  // TODO: reset diagrams here

  //===== Fill data histograms
  if (s_data) {
    for (unsigned int i = 0; i < s_data->numEvents(); i++) { // loop over data
      Event event(s_data->event(i));

      double eff = 1.0;
      if (_correctForEfficiency)
        eff = event.efficiency();
      if (eff == 0.0) {
        LOG(ERROR) << "DalitzPlot::Fill() | Loop over "
                      "data sample: An event with zero efficiency was found! "
                      "This should not happen! We skip it!";
        continue;
      }
      double evWeight = event.weight();

      dataDiagrams.fill(kin, event, evWeight * 1 / eff);
      h_weights.Fill(evWeight * 1 / eff);
    }
    _globalScale = dataDiagrams.integral();
  }

  //===== Plot amplitude
  if (s_phsp) {
    LOG(INFO)
        << "PlotData::plot | Plotting phase space sample and intensity...";

    double weightsSum = 0.0;

    // Loop over all events in phase space sample
    ProgressBar bar(s_phsp->numEvents());
    for (unsigned int i = 0; i < s_phsp->numEvents();
         i++) { // loop over phsp MC
      bar.next();
      Event event(s_phsp->event(i));
      double evWeight = event.weight();
      double eff = 1.0;
      if (_correctForEfficiency)
        eff = event.efficiency();
      if (eff == 0.0) {
        LOG(ERROR) << "DalitzPlot::Fill() | Loop over "
                      "phsp sample: An event with zero efficiency was found! "
                      "This should not happen! We skip it!";
        continue;
      }

      double evBase = evWeight / eff;
      weightsSum += evBase;

      /* Construct DataPoint from Event to check if dataPoint is within the
       * phase space boundaries */
      DataPoint point;
      try {
        kin->convert(event, point);
      } catch (BeyondPhsp &ex) { // event outside phase, remove
        continue;
      }

      // Fill diagrams with pure phase space events
      phspDiagrams.fill(kin, event, evBase); // scale phsp to data size

      // Loop over all components that we want to plot
      for (int t = 0; t < _plotHistograms.size(); t++)
        _plotHistograms.at(t).fill(
            kin, event, _plotComponents.at(t)->intensity(point) * evBase);
    }

    // Scale histograms to match data sample
    phspDiagrams.scale(_globalScale / phspDiagrams.integral());
    double scale = _globalScale / _plotHistograms.at(0).integral();
    _plotHistograms.at(0).scale(scale);

    for (int t = 1; t < _plotHistograms.size(); t++) {
      _plotHistograms.at(t).scale(scale);
    }
  }

  //===== Plot hit&miss data
  if (s_hitMiss) {
    for (unsigned int i = 0; i < s_hitMiss->numEvents();
         i++) { // loop over data
      Event event(s_hitMiss->event(i));
      double eff = 1.0;
      if (_correctForEfficiency)
        eff = event.efficiency();
      if (eff == 0.0) {
        LOG(ERROR) << "DalitzPlot::Fill() | Loop over "
                      "Hit&Miss sample: An event with zero efficiency was "
                      "found! This should not happen! We skip it!";
        continue;
      }
      double evWeight = event.weight();

      fitHitMissDiagrams.fill(kin, event, evWeight * 1 / eff);
    }
  }

  _isFilled = 1;
}

void DalitzPlot::plot() {
  if (!_isFilled)
    fill(kin_);

  //----- plotting 2D dalitz distributions -----
  TCanvas *c1 = new TCanvas("dalitz", "dalitz", 50, 50, 1600, 1600);
  c1->Divide(2, 2);
  c1->cd(1);
  dataDiagrams.getHistogram2D(0)->Draw("COLZ");
  m23m13_contour.Draw("P");
  dataDiagrams.getHistogram2D(0)->SetLineWidth(1);
  c1->cd(2);
  fitHitMissDiagrams.getHistogram2D(0)->Draw("COLZ");
  m23m13_contour.Draw("P");
  c1->cd(3);
  _plotHistograms.at(0).getHistogram2D(0)->Draw("COLZ");
  m23m13_contour.Draw("P");

  //----- plotting invariant mass distributions -----
  TCanvas *c2 = new TCanvas("invmass", "invmass", 50, 50, 2400, 800);
  c2->Divide(3, 1);
  c2->cd(1);
  CreateHist(0); // Plotting mKKsq
  c2->cd(2);
  CreateHist(1); // Plotting mKSK+sq
  c2->cd(3);
  CreateHist(2); // Plotting mKSK+sq
  c2->cd(3);
  TLegend *leg = new TLegend(0.15, 0.6, 0.50, 0.85);
  leg->AddEntry(dataDiagrams.getHistogram(2), "Data");
  leg->AddEntry(_plotHistograms.at(0).getHistogram(2), "Model");
  for (int i = 1; i < _plotComponents.size(); i++)
    leg->AddEntry(_plotHistograms.at(i).getHistogram(2),
                  TString(_plotLegend.at(i)));
  leg->SetFillStyle(0);
  leg->Draw(); // Plot legend

  //----- plotting signal amplitude contributions -----
  TCanvas *c5 =
      new TCanvas("signalInvmass", "signalInvmass", 50, 50, 2400, 800);
  c5->Divide(3, 1);
  c5->cd(1);
  CreateHist2(0); // Plotting mKKsq
  c5->cd(2);
  CreateHist2(1); // Plotting mKSK+sq
  c5->cd(3);
  CreateHist2(2); // Plotting mKSK+sq

  //----- Helicity angle distributions -----
  //  TCanvas *c3 =
  //      new TCanvas("helicityAngle", "helicity angle", 50, 50, 2400, 1600);
  //  c3->Divide(3, 2);
  //  c3->cd(1);
  //  CreateHist(3);
  //  c3->cd(2);
  //  CreateHist(4);
  //  c3->cd(3);
  //  CreateHist(5);
  //  c3->cd(4);
  //  CreateHist(6);
  //  c3->cd(5);
  //  CreateHist(7);
  //  c3->cd(6);
  //  CreateHist(8);

  //----- Weights distributions -----
  TCanvas *c4 = new TCanvas("weights", "weights", 50, 50, 2400, 800);
  c4->cd();
  h_weights.Draw();

  //----- Write to TFile -----
  TFile *tf2 = new TFile(Name + ".root", "recreate");
  if (tf2->IsZombie()) {
    std::cout << "Error opening output file" << std::endl;
    exit(-1);
  }
  m12m13_contour.Write("m12m13_contour", TObject::kOverwrite, 0);
  m23m12_contour.Write("m23m12_contour", TObject::kOverwrite, 0);
  m23m13_contour.Write("m23m13_contour", TObject::kOverwrite, 0);
  c1->Write("dalitz", TObject::kOverwrite, 0);
  c2->Write("invmass", TObject::kOverwrite, 0);
  c5->Write("signalInvmass", TObject::kOverwrite, 0);
  //  c3->Write("helicityAngle", TObject::kOverwrite, 0);

  // Save data trees and histograms
  tf2->mkdir("hist");
  tf2->cd("hist");
  dataDiagrams.write();
  phspDiagrams.write();
  fitHitMissDiagrams.write();

  for (int t = 0; t < _plotHistograms.size(); t++)
    _plotHistograms.at(t).write();

  // Write some canvas to single files
  c2->Print(Name + "-invmass.root");
  c2->Print(Name + "-invmass.pdf");

  tf2->Close();

  return;
}

void DalitzPlot::CreateHist(unsigned int id) {
  TPad *pad;
  std::vector<TH1D *> v;
  std::vector<TString> options;
  if (s_data) {
    v.push_back(dataDiagrams.getHistogram(id));
    options.push_back("E1");
  }
  for (int t = 0; t < _plotHistograms.size(); t++) {
    v.push_back(_plotHistograms.at(t).getHistogram(id));
    options.push_back("Sames,Hist");
  }

  pad = drawPull(v, options);
}

void DalitzPlot::CreateHist2(unsigned int id) {
  TPad *pad;
  std::vector<TH1D *> v;
  std::vector<TString> options;

  v.push_back(_plotHistograms.at(0).getHistogram(id));
  options.push_back("Hist");
  for (int t = 1; t < _plotHistograms.size(); t++) {
    v.push_back(_plotHistograms.at(t).getHistogram(id));
    options.push_back("Sames,Hist");
  }

  pad = drawHist(v, options);
}

//===================== DalitzHisto =====================
DalitzHisto::DalitzHisto(std::shared_ptr<Kinematics> kin, std::string name,
                         std::string title, unsigned int bins, Color_t color)
    : Name(name), Title(title), NumBins(bins), Integral(0.0), Color(color) {

  // we have to explicitly cast to HelicityKinematics in order to get
  // the invariant mass boundaries
  auto helkin = std::dynamic_pointer_cast<HelicityKinematics>(kin);

  // Initialize TTree
  Tree = std::unique_ptr<TTree>(new TTree(TString(Name), TString(Title)));

  // Adding branches to TTree
  Tree->Branch(TString(Name), &BranchPoint);
  Tree->Branch("efficiency", &BranchEff, "eff/D");
  Tree->Branch("weight", &BranchWeight, "weight/D");

  char label[60];

  // mass23sq
  unsigned int sys23(helkin->addSubSystem({1}, {2}, {0}, {}));
  auto m23sq_limit = helkin->invMassBounds(sys23);
  double m23sq_min = m23sq_limit.first;
  double m23sq_max = m23sq_limit.second;

  Arr.push_back(
      TH1D("m23sq", "m_{23}^{2} [GeV/c^{2}]", NumBins, m23sq_min, m23sq_max));
  double binWidth = (double)(m23sq_min - m23sq_max) / NumBins;
  sprintf(label, "Entries /%f.3", binWidth);
  Arr.back().GetYaxis()->SetTitle("# [" + TString(label) + "]");
  Arr.back().GetXaxis()->SetTitle("m_{23}^{2} [GeV/c^{2}]");
  Arr.back().Sumw2();
  // mass13sq
  unsigned int sys13(helkin->addSubSystem({0}, {2}, {1}, {}));
  auto m13sq_limit = helkin->invMassBounds(sys13);
  double m13sq_min = m13sq_limit.first;
  double m13sq_max = m13sq_limit.second;

  Arr.push_back(
      TH1D("m13sq", "m_{13}^{2} [GeV/c^{2}]", NumBins, m13sq_min, m13sq_max));
  binWidth = (double)(m13sq_min - m13sq_max) / NumBins;
  sprintf(label, "Entries /%f.3", binWidth);
  Arr.back().GetYaxis()->SetTitle("# [" + TString(label) + "]");
  Arr.back().GetXaxis()->SetTitle("m_{13}^{2} [GeV/c^{2}]");
  Arr.back().Sumw2();
  // mass12sq
  unsigned int sys12(helkin->addSubSystem({0}, {1}, {2}, {}));
  auto m12sq_limit = helkin->invMassBounds(sys12);
  double m12sq_min = m12sq_limit.first;
  double m12sq_max = m12sq_limit.second;

  Arr.push_back(
      TH1D("m12sq", "m_{12}^{2} [GeV/c^{2}]", NumBins, m12sq_min, m12sq_max));
  binWidth = (double)(m12sq_min - m12sq_max) / NumBins;
  sprintf(label, "Entries /%f.3", binWidth);
  Arr.back().GetYaxis()->SetTitle("# [" + TString(label) + "]");
  Arr.back().GetXaxis()->SetTitle("m_{12}^{2} [GeV/c^{2}]");
  Arr.back().Sumw2();

  Arr2D.push_back(TH2D(TString(name + "_m23sqm13sq"), TString(title), NumBins,
                       m23sq_min, m23sq_max, NumBins, m13sq_min, m13sq_max));
  Arr2D.push_back(TH2D(TString(name + "_m23sqm12sq"), TString(title), NumBins,
                       m23sq_min, m23sq_max, NumBins, m12sq_min, m12sq_max));
  Arr2D.push_back(TH2D(TString(name + "_m12sqm13sq"), TString(title), NumBins,
                       m12sq_min, m12sq_max, NumBins, m13sq_min, m13sq_max));
  Arr2D.push_back(TH2D(TString(name + "_m23sqCosTheta"), TString(title),
                       NumBins, m23sq_min, m23sq_max, NumBins, -1, 1));

  Arr2D.at(0).GetXaxis()->SetTitle("m_{KK}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(0).GetYaxis()->SetTitle("m_{K_{S}K^{+}}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(1).GetXaxis()->SetTitle("m_{KK}^{2} [GeV^{2}/c^{4}");
  Arr2D.at(1).GetYaxis()->SetTitle("m_{K_{S}K^{-}}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(2).GetXaxis()->SetTitle("m_{K_{S}K^{-}}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(2).GetYaxis()->SetTitle("m_{K_{S}K^{+}}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(3).GetXaxis()->SetTitle("m_{KK}^{2} [GeV^{2}/c^{4}]");
  Arr2D.at(3).GetYaxis()->SetTitle("#cos(#Theta)_{KK}");

  auto itr = Arr2D.begin();
  for (; itr != Arr2D.end(); ++itr) {
    (*itr).GetXaxis()->SetNdivisions(508);
    (*itr).GetZaxis()->SetTitle("Entries");
  }

  setColor(color);
  return;
}

void DalitzHisto::fill(std::shared_ptr<Kinematics> kin, Event &event,
                       double w) {

  double weight = event.weight() * w; // use event weights?

  Integral += weight;

  auto helkin = std::dynamic_pointer_cast<HelicityKinematics>(kin);

  unsigned int sysId23 = helkin->addSubSystem({1}, {2}, {0}, {});
  unsigned int sysId13 = helkin->addSubSystem({0}, {2}, {1}, {});
  unsigned int sysId12 = helkin->addSubSystem({0}, {1}, {2}, {});

  DataPoint point;
  try {
    kin->convert(event, point);
  } catch (std::exception &ex) {
    return;
  }

  double m23sq = point.value(3 * sysId23);
  double cos23 = point.value(3 * sysId23 + 1);
  double m13sq = point.value(3 * sysId13);
  //  double cos13 = point.getVal(3*sysId13+1);
  double m12sq = point.value(3 * sysId12);
  //  double cos12 = point.getVal(3*sysId12+1);

  Arr.at(0).Fill(m23sq, weight);
  Arr.at(1).Fill(m13sq, weight);
  Arr.at(2).Fill(m12sq, weight);

  Arr2D.at(0).Fill(m23sq, m13sq, weight);
  Arr2D.at(1).Fill(m23sq, m12sq, weight);
  Arr2D.at(2).Fill(m12sq, m13sq, weight);
  Arr2D.at(3).Fill(m23sq, cos23, weight);
}

void DalitzHisto::setStats(bool b) {
  auto n = Arr.size();
  for (int i = 0; i < n; ++i) {
    Arr.at(i).SetStats(b);
  }
  auto n2 = Arr2D.size();
  for (int i = 0; i < n2; ++i) {
    Arr2D.at(i).SetStats(b);
  }
}

void DalitzHisto::scale(double w) {
  auto n = Arr.size();
  for (int i = 0; i < n; ++i) {
    Arr.at(i).Scale(w);
  }
  auto n2 = Arr2D.size();
  for (int i = 0; i < n2; ++i) {
    Arr2D.at(i).Scale(w);
  }
}

void DalitzHisto::setColor(Color_t color) {
  auto n = Arr.size();
  for (int i = 0; i < n; ++i) {
    Arr.at(i).SetLineColor(color);
    Arr.at(i).SetMarkerColor(color);
  }
}

TH1D *DalitzHisto::getHistogram(unsigned int num) { return &Arr.at(num); }

TH2D *DalitzHisto::getHistogram2D(unsigned int num) { return &Arr2D.at(num); }

void DalitzHisto::write() {
  Tree->Write(TString(Name) + "Tree");
  gDirectory->mkdir(TString(Name) + "_hist");
  gDirectory->cd(TString(Name) + "_hist");
  auto n = Arr.size();
  for (int i = 0; i < n; ++i) {
    Arr.at(i).Write();
  }
  auto n2 = Arr2D.size();
  for (int i = 0; i < n2; ++i) {
    Arr2D.at(i).Write();
  }
  gDirectory->cd("../");
}
