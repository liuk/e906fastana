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

    map<int, pair<int, int> > eventIDsource;

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], atoi(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    //cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    for(map<int, vector<int> >::iterator iter = eventIDlist.begin(); iter != eventIDlist.end(); ++iter)
    {
        int runID = iter->first;
        cout << "Reading run " << runID << endl;

        TString query = Form("SELECT a.eventID,b.charge,b.eventID FROM kTrackMix AS a,kTrack AS b WHERE "
                    "a.chisq=b.chisq AND a.numHits=b.numHits AND a.charge=b.charge AND a.pz1=b.pz1 "
                    "AND a.runID=%d AND b.runID=%d AND a.eventID IN (", runID, runID);
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

        //Now read database
        eventIDsource.clear();

        TSQLResult* res_source = server->Query(query.Data());
        for(int j = 0; j < res_source->GetRowCount(); ++j)
        {
            TSQLRow* row_source = res_source->Next();
            int eventID = atoi(row_source->GetField(0));
            int charge = atoi(row_source->GetField(1));
            int source = atoi(row_source->GetField(2));

            if(charge > 0) eventIDsource[eventID].first = source;
            if(charge < 0) eventIDsource[eventID].second = source;

            delete row_source;
        }
        delete res_source;

        //Get QIE info
        for(map<int, pair<int, int> >::iterator iter = eventIDsource.begin(); iter != eventIDsource.end(); ++iter)
        {
            query = Form("SELECT AVG(Intensity_p),AVG(`RF-16`),AVG(`RF-15`),AVG(`RF-14`),AVG(`RF-13`),AVG(`RF-12`),AVG(`RF-11`),AVG(`RF-10`),AVG(`RF-09`),"
                "AVG(`RF-08`),AVG(`RF-07`),AVG(`RF-06`),AVG(`RF-05`),AVG(`RF-04`),AVG(`RF-03`),AVG(`RF-02`),AVG(`RF-01`),AVG(`RF+00`),AVG(`RF+01`),"
                "AVG(`RF+02`),AVG(`RF+03`),AVG(`RF+04`),AVG(`RF+05`),AVG(`RF+06`),AVG(`RF+07`),AVG(`RF+08`),AVG(`RF+09`),AVG(`RF+10`),AVG(`RF+11`),"
                "AVG(`RF+12`),AVG(`RF+13`),AVG(`RF+14`),AVG(`RF+15`),AVG(`RF+16`) FROM QIE WHERE runID=%d and (eventID=%d or eventID=%d)",
                runID, iter->second.first, iter->second.second);

            cout << iter->first << "  " << iter->second.first << "  " << iter->second.second << endl;

            TSQLResult* res_QIE = server->Query(query.Data());
            if(res_QIE->GetRowCount() != 1)
            {
                delete res_QIE;
                continue;
            }

            TSQLRow* row_QIE = res_QIE->Next();
            event.intensityP = row_QIE->GetField(0) == NULL ? 0. : 0.5*atof(row_QIE->GetField(0));
            for(int j = 0; j < 33; ++j) event.intensity[j] = row_QIE->GetField(j+1) == NULL ? 0. : 0.5*atof(row_QIE->GetField(j+1));

            event.runID = runID;
            event.spillID = 0;
            event.eventID = iter->first;

            saveTree->Fill();

            delete row_QIE;
            delete res_QIE;
        }
    }


    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
