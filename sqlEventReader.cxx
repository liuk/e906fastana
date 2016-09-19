#include <iostream>
#include <cmath>
#include <stdlib.h>

#include <TROOT.h>
#include <TTree.h>
#include <TFile.h>
#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>

#include "DataStruct.h"

using namespace std;

Event* gEvent;

int getInt(const char* row)
{
    if(row == NULL)
    {
        gEvent->log("Integer content is missing.");
        return -9999;
    }
    return atoi(row);
}

float getFloat(const char* row)
{
    if(row == NULL)
    {
        gEvent->log("Floating content is missing.");
        return -9999.;
    }
    return atof(row);
}

int main(int argc, char* argv[])
{
    // define the output file structure
    Event* p_event = new Event; Event& event = *p_event;
    gEvent = p_event;

    TFile* saveFile;
    TTree* saveTree;

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], getInt(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    char query[5000];
    sprintf(query, "SELECT runID,spillID FROM Spill WHERE dataQuality=0 ORDER BY spillID");

    TSQLResult* res = server->Query(query);
    int nSpills = res->GetRowCount();
    int currentRunID = -9999;
    for(int i = 0; i < nSpills; ++i)
    {
        if(i % 10 == 0)
        {
            cout << "Converting Spill " << i << "/" << nSpills << endl;
        }

        //basic RunID info
        TSQLRow* row = res->Next();
        event.runID = getInt(row->GetField(0));
        event.spillID = getInt(row->GetField(1));
        delete row;

        //create a new file for a new run
        if(event.runID != currentRunID)
        {
            if(currentRunID > 0)
            {
                saveFile->cd();
                saveTree->Write();
                saveFile->Close();
            }

            currentRunID = event.runID;

            cout << "Creating a new file for a new run " << currentRunID << endl;
            saveFile = new TFile(Form("%s_%06d.root", argv[2], currentRunID), "recreate");
            saveTree = new TTree("save", "save");

            saveTree->Branch("Event", &p_event, 256000, 99);
        }

        //query all the events in this spill
        sprintf(query, "SELECT a.`RF-16`,a.`RF-15`,a.`RF-14`,a.`RF-13`,a.`RF-12`,a.`RF-11`,a.`RF-10`,a.`RF-09`,"
            "a.`RF-08`,a.`RF-07`,a.`RF-06`,a.`RF-05`,a.`RF-04`,a.`RF-03`,a.`RF-02`,a.`RF-01`,a.`RF+00`,a.`RF+01`,"
            "a.`RF+02`,a.`RF+03`,a.`RF+04`,a.`RF+05`,a.`RF+06`,a.`RF+07`,a.`RF+08`,a.`RF+09`,a.`RF+10`,a.`RF+11`,"
            "a.`RF+12`,a.`RF+13`,a.`RF+14`,a.`RF+15`,a.`RF+16`,b.eventID,b.D1,b.D2,b.D3,b.H1,b.H2,b.H3,b.H4,b.P1,b.P2 "
            "FROM QIE AS a,Occupancy AS b,kEvent AS c WHERE a.runID=%d AND a.spillID=%d AND b.runID=%d AND b.spillID=%d "
            "AND c.runID=%d AND c.spillID=%d AND a.eventID=b.eventID AND a.eventID=c.eventID AND (c.status=0 OR c.status=-10)",
            event.runID, event.spillID, event.runID, event.spillID, event.runID, event.spillID);
        TSQLResult* res_event = server->Query(query);

        int nEvents = res_event->GetRowCount();
        for(int j = 0; j < nEvents; ++j)
        {
            TSQLRow* row_event = res_event->Next();
            event.eventID = getInt(row_event->GetField(33));
            for(int k = 0; k < 9; ++k)
            {
                event.occupancy[k] = getInt(row_event->GetField(k+34));
            }
            for(int k = 0; k < 33; ++k)
            {
                event.intensity[k] = getInt(row_event->GetField(k));
            }
            delete row_event;

            saveTree->Fill();
        }

        delete res_event;
    }
    delete res;
    cout << "sqlEventReader finished successfully." << endl;

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
