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
    Event* p_event = new Event; Event& event = *p_event;

    TFile* dataFile = new TFile(argv[5], "READ");
    TTree* dataTree = (TTree*)dataFile->Get("save");

    dataTree->SetBranchAddress("event", &p_event);

    map<int, vector<int> > eventIDlist;

    int runIDmin = atoi(argv[6]);
    int runIDmax = atoi(argv[7]);
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);
        if(event.runID < runIDmin || event.runID > runIDmax) continue;
        eventIDlist[event.runID].push_back(event.eventID);
    }

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* saveTree = new TTree("save", "save");

    saveTree->Branch("event", &p_event, 256000, 99);

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], atoi(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    //cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    for(map<int, vector<int> >::iterator iter = eventIDlist.begin(); iter != eventIDlist.end(); ++iter)
    {
        int runID = iter->first;
        cout << "Reading run " << runID << endl;

        TString query = Form("SELECT eventID,spillID,Intensity_p,`RF-16`,`RF-15`,`RF-14`,`RF-13`,`RF-12`,`RF-11`,`RF-10`,`RF-09`,"
            "`RF-08`,`RF-07`,`RF-06`,`RF-05`,`RF-04`,`RF-03`,`RF-02`,`RF-01`,`RF+00`,`RF+01`,"
            "`RF+02`,`RF+03`,`RF+04`,`RF+05`,`RF+06`,`RF+07`,`RF+08`,`RF+09`,`RF+10`,`RF+11`,"
            "`RF+12`,`RF+13`,`RF+14`,`RF+15`,`RF+16` FROM QIE WHERE runID=%d and eventID IN (", runID);
        for(int i = 0; i < iter->second.size(); ++i)
        {
            query = query + Form("%d", iter->second[i]);
            if(i != iter->second.size()-1)
            {
                query = query + ",";
            }
            else
            {
                query = query + ")";
            }
        }
        cout << query << endl;

        TSQLResult* res_QIE = server->Query(query.Data());
        for(int i = 0; i < res_QIE->GetRowCount(); ++i)
        {
            TSQLRow* row_QIE = res_QIE->Next();
            event.eventID = row_QIE->GetField(0) == NULL ? 0 : atoi(row_QIE->GetField(0));
            event.spillID = row_QIE->GetField(1) == NULL ? 0 : atoi(row_QIE->GetField(1));
            event.intensityP = row_QIE->GetField(2) == NULL ? 0. : atof(row_QIE->GetField(2));
            for(int j = 0; j < 33; ++j) event.intensity[j] = row_QIE->GetField(j+3) == NULL ? 0. : atof(row_QIE->GetField(j+3));

            event.runID = runID;

            saveTree->Fill();
            delete row_QIE;
        }
        delete res_QIE;
    }

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
