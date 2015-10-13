#include <iostream>
#include <fstream>
#include <cmath>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>

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

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* saveTree = dataTree->CloneTree(0);

    TString opts(argv[3]);
    bool mc = opts.Contains("mc");
    bool mix = opts.Contains("mix");
    bool like = opts.Contains("ls");
    bool target = opts.Contains("target");
    bool dump = opts.Contains("dump");
    int polarity = opts.Contains("rev") ? -1 : 1;

    bool reqSpill = !(mc && mix);
    bool reqEvent = !(mix && like);
    if(like) polarity = 0;

    //loop over all the events
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);

        //general cuts
        if(reqSpill && !spill.goodSpill()) continue;
        if(reqEvent && !event.goodEvent()) continue;
        if(!dimuon.goodDimuon(polarity)) continue;
        if(!((posTrack.roadID > 0)^(negTrack.roadID > 0))) continue;
        if(!(posTrack.goodTrack() && negTrack.goodTrack())) continue;

        if(target)
        {
            if(!dimuon.targetDimuon()) continue;
            if(!(posTrack.targetTrack() && negTrack.targetTrack())) continue;
        }
        if(dump)
        {
            if(!dimuon.dumpDimuon()) continue;
            if(!(posTrack.dumpTrack() && negTrack.dumpTrack())) continue;
        }

        saveTree->Fill();
    }

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
