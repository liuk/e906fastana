#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <map>
#include <vector>

#include <TROOT.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>

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

    TFile* sourceFile = new TFile(argv[1], "READ");
    TTree* sourceTree = (TTree*)sourceFile->Get("save");

    sourceTree->SetBranchAddress("event", &p_event);

    map<pair<int, int>, Event> eventInfo;
    for(int i = 0; i < sourceTree->GetEntries(); ++i)
    {
        sourceTree->GetEntry(i);

        //cout << i << "  " << "  " << event.runID << "  " << event.eventID << endl;
        eventInfo.insert(map<pair<int, int>, Event>::value_type(make_pair(event.runID, event.eventID), event));
        //eventInfo[make_pair(event.runID, event.eventID)] = event;
    }

    TFile* dataFile = new TFile(argv[2], "READ");
    TTree* dataTree = (TTree*)dataFile->Get("save");

    dataTree->SetBranchAddress("dimuon", &p_dimuon);
    dataTree->SetBranchAddress("event", &p_event);
    dataTree->SetBranchAddress("spill", &p_spill);
    dataTree->SetBranchAddress("posTrack", &p_posTrack);
    dataTree->SetBranchAddress("negTrack", &p_negTrack);

    TFile* saveFile = new TFile(argv[3], "recreate");
    TTree* saveTree = dataTree->CloneTree(0, "fast");

    Event* p_eventNew = new Event; Event& eventNew = *p_eventNew;

    saveTree->Branch("eventNew", &p_eventNew, 256000, 99);

    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);
        eventNew = event;

        //cout << i << "  " << "  " << event.runID << "  " << event.eventID << endl;
        if(eventInfo.find(make_pair(event.runID, event.eventID)) != eventInfo.end())
        {
            Event ref = eventInfo[make_pair(event.runID, event.eventID)];

            eventNew.intensityP = ref.intensityP;
            for(int j = 0; j < 33; ++j) event.intensity[j] = ref.intensity[j];
        }

        saveTree->Fill();
    }

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
