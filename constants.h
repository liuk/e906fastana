#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <TString.h>
#include <TMath.h>

const int nTargets = 7;
const TString targetName[] = {"", "H", "Empty", "D", "None", "Fe", "C", "W"};

const double Avo = 6.02214129E23;

const double A_H = 1.0074; 
const double A_D = 2.01410;
const double A_Fe = 55.845;
const double A_C = 12.0107;
const double A_W = 183.84;
const double A[] = {0., A_H, 0., A_D, 0., A_Fe, A_C, A_W};

const double Z_H = 1.;
const double Z_D = 1.;
const double Z_Fe = 26.;
const double Z_C = 6.;
const double Z_W = 74.;
const double Z[] = {0., Z_H, 0., Z_D, 0., Z_Fe, Z_C, Z_W};

const double rho_H = 0.07065;
const double rho_D = 0.1617;
const double rho_Fe = 7.874;
const double rho_C = 1.802;
const double rho_W = 19.3;
const double rho[] = {0., rho_H, 0., rho_D, 0., rho_Fe, rho_C, rho_W};

const double lambda_H = 52.0/rho_H;
const double lambda_D = 71.8/rho_D;
const double lambda_Fe = 132.1/rho_Fe;
const double lambda_C = 85.8/rho_C;
const double lambda_W = 191.9/rho_W;
const double lambda[] = {0., lambda_H, 0., lambda_D, 0., lambda_Fe, lambda_C, lambda_W};

const double L_H = 20.*2.54;
const double L_D = 20.*2.54;
const double L_Fe = 0.25*3.*2.54;
const double L_C = 0.436*3.*2.54;
const double L_W = 0.125*3.*2.54;
const double L[] = {0., L_H, 0., L_D, 0., L_Fe, L_C, L_W};

const double ratio_H = 1. - TMath::Exp(-L_H/lambda_H);
const double ratio_D = 1. - TMath::Exp(-L_D/lambda_D);
const double ratio_Fe = 1. - TMath::Exp(-L_Fe/lambda_Fe);
const double ratio_C = 1. - TMath::Exp(-L_C/lambda_C);
const double ratio_W = 1. - TMath::Exp(-L_W/lambda_W);
const double ratioBeam[] = {0., ratio_H, 0., ratio_D, 0., ratio_Fe, ratio_C, ratio_W};

#ifdef ALL
// all data
const double NProtons_H = 9.78995e+16;
const double NProtons_Empty = 1.56605e+16;
const double NProtons_D = 4.64828e+16;
const double NProtons_None = 1.85123e+16;
const double NProtons_Fe = 9.83039e+15;
const double NProtons_C = 2.48979e+16;
const double NProtons_W = 1.00265e+16;
#endif

#ifdef T57
//#57 only
const double NProtons_H = 4.18431e+16;
const double NProtons_Empty = 4.57502e+15;
const double NProtons_D = 2.0824e+16;
const double NProtons_None = 6.49558e+15;
const double NProtons_Fe = 4.25987e+15;
const double NProtons_C = 1.32132e+16;
const double NProtons_W = 4.38142e+15;
#endif

#ifdef T59
//#59 only
const double NProtons_H = 5.42553e+15;
const double NProtons_Empty = 5.88579e+14;
const double NProtons_D = 2.50563e+15;
const double NProtons_None = 1.1548e+15;
const double NProtons_Fe = 5.52929e+14;
const double NProtons_C = 1.659e+15;
const double NProtons_W = 5.44394e+14;
#endif

#ifdef T62
//#62 only
const double NProtons_H = 5.06309e+16;
const double NProtons_Empty = 1.04969e+16;
const double NProtons_D = 2.31532e+16;
const double NProtons_None = 1.08619e+16;
const double NProtons_Fe = 5.01759e+15;
const double NProtons_C = 1.00256e+16;
const double NProtons_W = 5.1007e+15;
#endif

#ifdef RUN2
//run2 only
const double NProtons_H = 4.72686e+16;
const double NProtons_Empty = 5.1636e+15;
const double NProtons_D = 2.33296e+16;
const double NProtons_None = 7.65038e+15;
const double NProtons_Fe = 4.8128e+15;
const double NProtons_C = 1.48722e+16;
const double NProtons_W = 4.92581e+15;
#endif

const double NProtons[] = {0., NProtons_H, NProtons_Empty, NProtons_D, NProtons_None, NProtons_Fe, NProtons_C, NProtons_W};

//Normalization factor






#endif