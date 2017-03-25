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

int getInt(const char* row)
{
    if(row == NULL)
    {
        gSpill->log("Integer content is missing.");
        return -9999;
    }
    return atoi(row);
}

float getFloat(const char* row)
{
    if(row == NULL)
    {
        gSpill->log("Floating content is missing.");
        return -9999.;
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

    TString query = "SELECT runID,spillID FROM Spill ORDER BY spillID";

    TSQLResult* res = server->Query(query.Data());
    int nSpillsRow = res->GetRowCount();

    int nBadSpill_record = 0;
    int nBadSpill_duplicate = 0;
    int nBadSpill_quality = 0;
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
        delete row;

        //magnet configuration
        TString query = Form("SELECT value FROM Beam WHERE spillID=%d AND name IN ('F:NM3S','F:NM4AN') ORDER BY name", spill.spillID);
        TSQLResult* res_spill = server->Query(query.Data());
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
        query = Form("SELECT MIN(eventID),MAX(eventID) FROM Event WHERE runID=%d AND spillID=%d", runID, spill.spillID);
        res_spill = server->Query(query.Data());
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
        query = Form("SELECT a.targetPos,b.value,a.dataQuality,a.liveProton FROM Spill AS a,Target AS b WHERE a.spillID"
            "=%d AND b.spillID=%d AND b.name='TARGPOS_CONTROL'", spill.spillID, spill.spillID);
        res_spill = server->Query(query.Data());
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
        spill.liveProton      = getFloat(row_spill->GetField(3));
        delete row_spill;
        delete res_spill;

        //Reconstruction info
        query = Form("SELECT (SELECT COUNT(*) FROM Event WHERE spillID=%d),(SELECT COUNT(*) FROM kDimuon WHERE spillID=%d),"
            "(SELECT COUNT(*) FROM kTrack WHERE spillID=%d)", spill.spillID, spill.spillID, spill.spillID);
        res_spill = server->Query(query.Data());
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
        query = Form("SELECT a.value,b.NM3ION,b.QIESum,b.inhibit_block_sum,b.trigger_sum_no_inhibit,"
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
        query = Form("SELECT value FROM Scaler WHERE spillType='EOS' AND spillID=%d AND scalerName "
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
        query = Form("SELECT value FROM Scaler WHERE spillType='BOS' AND spillID=%d AND scalerName "
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

        if(!spill.goodSpill())
        {
            ++nBadSpill_quality;
            spill.log("spill bad");
            spill.print();
        }

        //Save
        saveTree->Fill();
    }
    delete res;

    cout << "sqlSpillReader finished successfully." << endl;
    cout << saveTree->GetEntries() << " good spills, " << nBadSpill_record << " spills have insufficient info in database, "
         << nBadSpill_duplicate << " spills have duplicate entries in database, " << nBadSpill_quality << " rejected because of bad quality." << endl;

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
