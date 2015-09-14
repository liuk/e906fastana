#include <iostream>
#include <cmath>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>

#include "constants.h"
#include "DataStruct.h"

using namespace std;

int main(int argc, char* argv[])
{
    // define the output file structure
    Spill* p_spill = new Spill; Spill& spill = *p_spill;

    TFile* dataFile = new TFile(argv[1], "READ");
    TTree* dataTree = (TTree*)dataFile->Get("save");

    dataTree->SetBranchAddress("spill", &p_spill);

    double np[nTargets + 1] = {0};
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);

        if(!spill.goodSpill()) continue;
        np[spill.targetPos] = np[spill.targetPos] + spill.G2SEM*(spill.QIESum - spill.inhibitSum - spill.busySum)/spill.QIESum;
    }

    for(int i = 1; i <= nTargets; ++i)
    {
        cout << "const double NProtons_" << targetName[i] << " = " << np[i] << ";" << endl;
    }

    return EXIT_SUCCESS;
}
