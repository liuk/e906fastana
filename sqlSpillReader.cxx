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

Spill* gSpill;

int getInt(const char* row, int default_val = -9999)
{
    if(row == NULL)
    {
        gSpill->log("Integer content is missing.");
        return default_val;
    }
    return atoi(row);
}

float getFloat(const char* row, float default_val= -9999.)
{
    if(row == NULL)
    {
        gSpill->log("Floating content is missing.");
        return default_val;
    }
    return atof(row);
}

int main(int argc, char* argv[])
{
    // define the output file structure
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    spill.skipflag = false;

    gSpill = p_spill;

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* saveTree = new TTree("save", "save");

    saveTree->Branch("spill", &p_spill, 256000, 99);

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], getInt(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    char query[5000];
    //sprintf(query, "SELECT spillID FROM Spill WHERE runID in (SELECT run FROM summary.production WHERE ktracked=1) ORDER BY spillID");
    sprintf(query, "SELECT runID,spillID,liveProton,targetPos,dataQuality FROM Spill WHERE spillID IS NOT NULL ORDER BY spillID");

    TSQLResult* res = server->Query(query);
    int nSpillsRow = res->GetRowCount();

    int nBadSpills = 0;
    for(int i = 0; i < nSpillsRow; ++i)
    {
        if(i % 100 == 0)
        {
            cout << "Converting spill " << i << "/" << nSpillsRow << endl;
        }

        //basic spillID info
        TSQLRow* row = res->Next();
        int runID = getInt(row->GetField(0));
        spill.spillID = getInt(row->GetField(1));
        spill.trigSet = spill.triggerSet();
        spill.liveProton = getFloat(row->GetField(2));
        spill.targetPos = getInt(row->GetField(3), -1);
        spill.quality = getInt(row->GetField(4), -1);
        delete row;

        /*
        //magnet configuration
        sprintf(query, "SELECT value FROM Beam WHERE spillID=%d AND name IN ('F:NM3S','F:NM4AN') ORDER BY name", spill.spillID);
        TSQLResult* res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 2)
        {
            spill.log(Form("lacks magnet info %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 2 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        TSQLRow* row_spill = res_spill->Next(); spill.FMAG = getFloat(row_spill->GetField(0)); delete row_spill;
        row_spill = res_spill->Next();          spill.KMAG = getInt(row_spill->GetField(0)); delete row_spill;
        delete res_spill;

        //EventID range
        sprintf(query, "SELECT MIN(eventID),MAX(eventID) FROM Event WHERE runID=%d AND spillID=%d", runID, spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 1)
        {
            spill.log(Form("lacks event table entry %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 1 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        row_spill = res_spill->Next();
        spill.eventID_min = getInt(row_spill->GetField(0));
        spill.eventID_max = getInt(row_spill->GetField(1));
        delete row_spill;
        delete res_spill;

        //target position
        sprintf(query, "SELECT a.targetPos,b.value,a.dataQuality,a.liveProton FROM Spill AS a,Target AS b WHERE a.spillID"
            "=%d AND b.spillID=%d AND b.name='TARGPOS_CONTROL'", spill.spillID, spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 1)
        {
            spill.log(Form("lacks target position info %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 1 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        row_spill = res_spill->Next();
        spill.targetPos       = getInt(row_spill->GetField(0));
        spill.TARGPOS_CONTROL = getInt(row_spill->GetField(1));
        spill.quality         = getInt(row_spill->GetField(2));
        spill.liveProton      = row_spill->GetField(3) == NULL ? -1. : getFloat(row_spill->GetField(3));
        delete row_spill;
        delete res_spill;

        //Reconstruction info
        sprintf(query, "SELECT (SELECT COUNT(*) FROM Event WHERE spillID=%d),"
                              "(SELECT COUNT(*) FROM kDimuon WHERE spillID=%d),"
                              "(SELECT COUNT(*) FROM kTrack WHERE spillID=%d)", spill.spillID, spill.spillID, spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 1)
        {
            spill.log(Form("lacks reconstructed tables %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 1 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        row_spill = res_spill->Next();
        spill.nEvents = getInt(row_spill->GetField(0));
        spill.nDimuons = getInt(row_spill->GetField(1));
        spill.nTracks = getInt(row_spill->GetField(2));
        delete row_spill;
        delete res_spill;

        //Beam/BeamDAQ
        sprintf(query, "SELECT a.value,b.NM3ION,b.QIESum,b.inhibit_block_sum,b.trigger_sum_no_inhibit,"
            "b.dutyFactor53MHz FROM Beam AS a,BeamDAQ AS b WHERE a.spillID=%d AND b.spillID=%d AND "
            "a.name='S:G2SEM'", spill.spillID, spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 1)
        {
            spill.log(Form("lacks Beam/BeamDAQ info %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 1 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        row_spill = res_spill->Next();
        spill.G2SEM      = getFloat(row_spill->GetField(0));
        spill.NM3ION     = getFloat(row_spill->GetField(1));
        spill.QIESum     = getFloat(row_spill->GetField(2));
        spill.inhibitSum = getFloat(row_spill->GetField(3));
        spill.busySum    = getFloat(row_spill->GetField(4));
        spill.dutyFactor = getFloat(row_spill->GetField(5));

        delete row_spill;
        delete res_spill;

        //Scalar table -- EOS
        sprintf(query, "SELECT value FROM Scaler WHERE spillType='EOS' AND spillID=%d AND scalerName "
            "in ('TSGo','AcceptedMatrix1','AfterInhMatrix1') ORDER BY scalerName", spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 3)
        {
            spill.log(Form("lacks scaler info %d", res_spill->GetRowCount()));
            res_spill->GetRowCount() > 3 ? ++nBadSpill_duplicate : ++nBadSpill_record;

            delete res_spill;
            continue;
        }

        row_spill = res_spill->Next(); spill.acceptedMatrix1 = getFloat(row_spill->GetField(0)); delete row_spill;
        row_spill = res_spill->Next(); spill.afterInhMatrix1 = getFloat(row_spill->GetField(0)); delete row_spill;
        row_spill = res_spill->Next(); spill.TSGo            = getFloat(row_spill->GetField(0)); delete row_spill;
        delete res_spill;

        //Scalar table -- BOS
        sprintf(query, "SELECT value FROM Scaler WHERE spillType='BOS' AND spillID=%d AND scalerName "
            "in ('TSGo','AcceptedMatrix1','AfterInhMatrix1') ORDER BY scalerName", spill.spillID);
        res_spill = server->Query(query);
        if(res_spill->GetRowCount() != 3)
        {
            spill.log(Form("lacks scaler info %d", res_spill->GetRowCount()));
            spill.acceptedMatrix1BOS = -1;
            spill.afterInhMatrix1BOS = -1;
            spill.TSGoBOS = -1;
        }
        else
        {
            row_spill = res_spill->Next(); spill.acceptedMatrix1BOS = getFloat(row_spill->GetField(0)); delete row_spill;
            row_spill = res_spill->Next(); spill.afterInhMatrix1BOS = getFloat(row_spill->GetField(0)); delete row_spill;
            row_spill = res_spill->Next(); spill.TSGoBOS            = getFloat(row_spill->GetField(0)); delete row_spill;
        }
        delete res_spill;
        */

        if(!spill.goodSpill())
        {
            ++nBadSpills;
            spill.log("spill bad");
            spill.print();
        }

        //Save
        saveTree->Fill();
    }
    delete res;

    cout << saveTree->GetEntries() << " good spills, " << nBadSpills << " bad spills." << endl;
    cout << "sqlSpillReader finished successfully." << endl;

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
