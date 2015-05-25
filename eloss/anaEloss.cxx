#include <iostream>
#include <fstream>
#include <cmath>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TGraphAsymmErrors.h>
#include <TF1.h>
#include <TAxis.h>

#include "constants.h"
#include "DataStruct.h"

using namespace std;

int main(int argc, char* argv[])
{
    // define the output file structure
    Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    Event* p_event = new Event; Event& event = *p_event; 
    Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
    Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

    TFile* dataFile = new TFile(argv[1], "READ");
    TTree* dataTree = (TTree*)dataFile->Get("save");

    dataTree->SetBranchAddress("dimuon", &p_dimuon);
    dataTree->SetBranchAddress("event", &p_event);
    dataTree->SetBranchAddress("spill", &p_spill);
    dataTree->SetBranchAddress("posTrack", &p_posTrack);
    dataTree->SetBranchAddress("negTrack", &p_negTrack);

    //manual bining
    const double x1min = 0.35;
    const double x1max = 1.0;
    const double x1bin[] = {x1min, 0.55, 0.61, 0.66, 0.71, 0.79, x1max};
    const int nx1bins = 6;

    int nevt_all[nTargets+1] = {0};
    int nevt[nTargets+1][nx1bins] = {0};
    double x1_sum[nTargets+1][nx1bins] = {0};
    double x2_sum[nTargets+1][nx1bins] = {0};
    double xf_sum[nTargets+1][nx1bins] = {0};
    double mass_sum[nTargets+1][nx1bins] = {0};

    double x1_avg[nTargets+1][nx1bins] = {0};
    double x2_avg[nTargets+1][nx1bins] = {0};
    double xf_avg[nTargets+1][nx1bins] = {0};
    double mass_avg[nTargets+1][nx1bins] = {0};

    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);

        //temporary cuts
        if(spill.spillID >= 416709 && spill.spillID <= 423255) continue;   //bad taget position
        if(spill.spillID >= 482574 && spill.spillID <= 484924) continue;   //flipped magnets
        //if(event.intensity > 10000.) continue;

        //general cuts
        if(!spill.goodSpill()) continue;
        if(!event.goodEvent()) continue;
        if(!dimuon.goodDimuon()) continue;
        if(!dimuon.targetDimuon()) continue;
        if(!(posTrack.goodTrack() && negTrack.goodTrack())) continue;
        if(!(posTrack.targetTrack() && negTrack.targetTrack())) continue;

        //analysis specific cuts
        if(dimuon.mass < 4.2) continue;
        if(dimuon.x1 < x1min || dimuon.x1 > x1max) continue;
        if(dimuon.x2 < 0.15) continue;

        nevt_all[spill.targetPos] += 1;
        for(int j = 0; j < nx1bins; ++j)
        {
            if(dimuon.x1 > x1bin[j] && dimuon.x1 <= x1bin[j+1])
            {
                nevt[0][j] += 1;
                nevt[spill.targetPos][j] += 1;
                x1_sum[spill.targetPos][j] += dimuon.x1;
                x2_sum[spill.targetPos][j] += dimuon.x2;
                xf_sum[spill.targetPos][j] += dimuon.xF;
                mass_sum[spill.targetPos][j] += dimuon.mass;

                break;
            }
        }
    }

    //calc kinematic average
    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            x1_avg[i][j] = x1_sum[i][j]/nevt[i][j];
            x2_avg[i][j] = x2_sum[i][j]/nevt[i][j];
            xf_avg[i][j] = xf_sum[i][j]/nevt[i][j];
            mass_avg[i][j] = mass_sum[i][j]/nevt[i][j]; 
        }
    }

    //calc raw yields per 1E16 proton
    double yield_raw[nTargets+1][nx1bins] = {0};
    double err_yield_raw[nTargets+1][nx1bins] = {0};

    for(int i = 1; i <= nTargets; ++i)
    {
        double np_reduced = NProtons[i]/1E16;
        for(int j = 0; j < nx1bins; ++j)
        {
            yield_raw[i][j] = nevt[i][j]/np_reduced;
            err_yield_raw[i][j] = sqrt(nevt[i][j])/np_reduced;
        }
    }

    //background subtraction
    double yield_bkgsub[nTargets+1][nx1bins] = {0};
    double err_yield_bkgsub[nTargets+1][nx1bins] = {0};

    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            if(i <= 3)
            {
                yield_bkgsub[i][j] = yield_raw[i][j] - yield_raw[2][j];
                err_yield_bkgsub[i][j] = sqrt(err_yield_raw[i][j]*err_yield_raw[i][j] + err_yield_raw[2][j]*err_yield_raw[2][j]);
            }
            else
            {
                yield_bkgsub[i][j] = yield_raw[i][j] - yield_raw[4][j];
                err_yield_bkgsub[i][j] = sqrt(err_yield_raw[i][j]*err_yield_raw[i][j] + err_yield_raw[4][j]*err_yield_raw[4][j]);           
            }
        }
    }

    //contamination correction
    double f_H_in_H = 1.;
    double f_D_in_H = 0.;
    double f_H_in_D = 0.0724;
    double f_D_in_D = 0.9276;

    double yield_pure[nTargets+1][nx1bins] = {0};
    double err_yield_pure[nTargets+1][nx1bins] = {0};
    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            yield_pure[i][j] = yield_bkgsub[i][j];
            err_yield_pure[i][j] = err_yield_bkgsub[i][j];
        }
    }

    for(int i = 0; i < nx1bins; ++i)
    {
        yield_pure[3][i] = (f_H_in_H*yield_bkgsub[3][i] - f_H_in_D*yield_bkgsub[1][i])/(f_D_in_D*f_H_in_H - f_D_in_H*f_H_in_D);
        err_yield_pure[3][i] = sqrt(f_H_in_H*f_H_in_H*err_yield_raw[3][i]*err_yield_raw[3][i] + f_H_in_D*f_H_in_D*err_yield_raw[1][i]*err_yield_raw[1][i]
            + (f_H_in_H - f_H_in_D)*(f_H_in_H - f_H_in_D)*err_yield_raw[2][i])/(f_D_in_D*f_H_in_H - f_D_in_H*f_H_in_D);
    }

    //calc per nucleon cross sections
    double xsec[nTargets+1][nx1bins] = {0};
    double err_xsec[nTargets+1][nx1bins] = {0};

    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            xsec[i][j] = yield_pure[i][j]/(rho[i]*lambda[i]*ratioBeam[i]);
            err_xsec[i][j] = err_yield_pure[i][j]/(rho[i]*lambda[i]*ratioBeam[i]);
        }
    }

    //calc the cross section ratio
    double xsec_ratio[nTargets+1][nx1bins] = {0};
    double err_xsec_ratio[nTargets+1][nx1bins] = {0};

    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            xsec_ratio[i][j] = xsec[i][j]/xsec[3][j];
            err_xsec_ratio[i][j] = xsec_ratio[i][j]*sqrt(err_xsec[i][j]*err_xsec[i][j]/xsec[i][j]/xsec[i][j] + err_xsec[3][j]*err_xsec[3][j]/xsec[3][j]/xsec[3][j]);
        }
    }

    //iso-scalar correction
    double R_np[nx1bins] = {0};
    double err_R_np[nx1bins] = {0};
    for(int i = 0; i < nx1bins; ++i)
    {
        R_np[i] = A[3]*xsec[3][i]/xsec[1][i]/A[1] - 1.;
        R_np[i] = yield_pure[3][i]/yield_pure[1][i] - 1.;
        err_R_np[i] = A[3]*xsec[3][i]/xsec[1][i]*sqrt(err_xsec[3][i]*err_xsec[3][i]/xsec[3][i]/xsec[3][i] + err_xsec[1][i]*err_xsec[1][i]/xsec[1][i]/xsec[1][i]);
    }

    double xsec_ratio_corr[nTargets+1][nx1bins] = {0};
    double err_xsec_ratio_corr[nTargets+1][nx1bins] = {0};
    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            xsec_ratio_corr[i][j] = xsec_ratio[i][j]*0.5*(1. + R_np[j])*A[i]/(Z[i] + (A[i] - Z[i])*R_np[j]);
            err_xsec_ratio_corr[i][j] = err_xsec_ratio[i][j];
        }
    }
    
    //print everything
    for(int i = 1; i <= nTargets; ++i)
    {
        cout << " ============= On Target " << targetName[i] << " with " << NProtons[i] << " live protons ============= " << nevt_all[i] << endl;
        for(int j = 0; j < nx1bins; ++j)
        {
            cout << " --- x1 bin " << j << " : " << x1bin[j] << " --- " << x1bin[j+1] << " --- " << endl;
            
            cout << "high mass " << nevt[i][j] << " events. x1_avg = " << x1_avg[i][j] << ", ";
            cout << "x2_avg = " << x2_avg[i][j] << ", ";
            cout << "xf_avg = " << xf_avg[i][j] << ", ";
            cout << "mass_avg = " << mass_avg[i][j] << endl;

            cout << "Raw yield per 1E16 protons =               " << yield_raw[i][j] << " +/- " << err_yield_raw[i][j] << endl;
            cout << "Bkg-subtracted yield per 1E16 protons =    " << yield_bkgsub[i][j] << " +/- " << err_yield_bkgsub[i][j] << endl;
            cout << "De-comtaminated yield per 1E16 protons =   " << yield_pure[i][j] << " +/- " << err_yield_pure[i][j] << endl;
            cout << "Per-neucleon cross section =               " << xsec[i][j] << " +/- " << err_xsec[i][j] << endl;
            cout << "Cross section ratio over LD2 =             " << xsec_ratio[i][j] << " +/- " << err_xsec_ratio[i][j] << endl;
            cout << "Iso-scalar corrected cross section ratio = " << xsec_ratio_corr[i][j] << " +/- " << err_xsec_ratio_corr[i][j] << endl;
            //cout << "Base-line from dbar/ubar = " << ((A[i] - Z[i])*R_np[j] + Z[i])/(R_np[j] + 1.)/A[i]*A[3] << endl;
 
            cout << endl; 
        }
    }

    //making plots
    double x[nTargets+1][nx1bins] = {0};
    double dxl[nTargets+1][nx1bins] = {0};
    double dxr[nTargets+1][nx1bins] = {0};
    double y[nTargets+1][nx1bins] = {0};
    double y_raw[nTargets+1][nx1bins] = {0};
    double dy[nTargets+1][nx1bins] = {0};
    double dy_raw[nTargets+1][nx1bins] = {0};
    TGraphAsymmErrors* gr[nTargets+1] = {0};
    TGraphAsymmErrors* gr_raw[nTargets+1] = {0};

    for(int i = 1; i <= nTargets; ++i)
    {
        for(int j = 0; j < nx1bins; ++j)
        {
            x[i][j] = x1_avg[i][j];
            dxl[i][j] = x1_avg[i][j] - x1bin[j];
            dxr[i][j] = x1bin[j+1] - x1_avg[i][j];

            y[i][j] = xsec_ratio_corr[i][j];
            dy[i][j] = err_xsec_ratio_corr[i][j];
            
            y_raw[i][j] = xsec_ratio[i][j];
            dy_raw[i][j] = err_xsec_ratio[i][j];
        }

        gr[i] = new TGraphAsymmErrors(nx1bins, x[i], y[i], dxl[i], dxr[i], dy[i], dy[i]);
        gr[i]->SetTitle(targetName[i].Data());
        gr[i]->SetMarkerColor(kBlue);
        gr[i]->SetLineColor(kBlue);
        gr[i]->SetMarkerStyle(21);
        gr[i]->GetXaxis()->SetLimits(x1min - 0.1, x1max + 0.1);
        gr[i]->SetMaximum(2.);
        gr[i]->SetMinimum(0.);

        gr_raw[i] = new TGraphAsymmErrors(nx1bins, x[i], y_raw[i], dxl[i], dxr[i], dy_raw[i], dy_raw[i]);
        gr_raw[i]->SetTitle(targetName[i].Data());
        gr_raw[i]->SetMarkerColor(kRed);
        gr_raw[i]->SetLineColor(kRed);
        gr_raw[i]->SetMarkerStyle(21);
        gr_raw[i]->GetXaxis()->SetLimits(x1min - 0.1, x1max + 0.1);
        gr_raw[i]->SetMaximum(2.);
        gr_raw[i]->SetMinimum(0.);      
    }

    TCanvas* c1 = new TCanvas("c1", "c1", 1500, 500);
    c1->Divide(3, 1, 0., 0.01);

    TF1 f("f", "1", x1min - 0.1, x1max + 0.1);
    f.SetLineStyle(kDashed);
    f.SetLineColor(kRed);
    f.SetLineWidth(1);

    c1->cd(1); gr[6]->Draw("AP"); gr_raw[6]->Draw("Psame"); f.Draw("same");
    c1->cd(2); gr[5]->Draw("AP"); gr_raw[5]->Draw("Psame"); f.Draw("same");
    c1->cd(3); gr[7]->Draw("AP"); gr_raw[7]->Draw("Psame"); f.Draw("same");

    c1->SaveAs(argv[2]);

    return EXIT_SUCCESS;
}