#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>

#include <TROOT.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
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
    Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    Event* p_event = new Event; Event& event = *p_event;
    Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
    Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

    gEvent = p_event;

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* saveTree = new TTree("save", "save");

    saveTree->Branch("dimuon", &p_dimuon, 256000, 99);
    saveTree->Branch("event", &p_event, 256000, 99);
    saveTree->Branch("spill", &p_spill, 256000, 99);
    saveTree->Branch("posTrack", &p_posTrack, 256000, 99);
    saveTree->Branch("negTrack", &p_negTrack, 256000, 99);

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], atoi(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    //define the source tables, and run range
    TString suffix = argv[5];
    TString runIDMin = argv[6];
    TString runIDMax = argv[7];

    //decide how to access event/spill level information
    spill.skipflag = suffix.Contains("Mix");

    //pre-load spill information if provided
    map<int, Spill> spillBank;
    if(!spill.skipflag)
    {
        TFile* spillFile = new TFile(argv[8]);
        TTree* spillTree = (TTree*)spillFile->Get("save");
        spillTree->SetBranchAddress("spill", &p_spill);

        for(int i = 0; i < spillTree->GetEntries(); ++i)
        {
            spillTree->GetEntry(i);
            if(spill.goodSpill())  spillBank.insert(map<int, Spill>::value_type(spill.spillID, spill));
        }
    }

    TString query = Form("SELECT dimuonID,runID,spillID,eventID,posTrackID,negTrackID,dx,dy,dz,"
        "dpx,dpy,dpz,mass,xF,xB,xT,trackSeparation,chisq_dimuon,px1,py1,pz1,px2,py2,pz2,costh,phi,targetPos "
        "FROM kDimuon%s WHERE runID>=%s AND runID<=%s AND mass>0.5 AND mass<10. AND chisq_dimuon<50. AND xB>0. AND xB<1. AND "
        "xT>0. AND xT<1. AND xF>-1. AND xF<1. AND ABS(dx)<2. AND ABS(dy)<2. AND dz>-300. "
        "AND dz<300. AND ABS(dpx)<3. AND ABS(dpy)<3. AND dpz>30. AND dpz<120. AND "
        "ABS(trackSeparation)<300. ORDER BY spillID", suffix.Data(), runIDMin.Data(), runIDMax.Data());

    TSQLResult* res_dimuon = server->Query(query.Data());
    int nDimuonsRaw = res_dimuon->GetRowCount();

    spill.spillID = -1;
    bool badSpillFlag = false;
    for(int i = 0; i < nDimuonsRaw; ++i)
    {
        if(i % 1000 == 0)
        {
            cout << "Converting dimuon " << i << "/" << nDimuonsRaw << endl;
        }

        //basic dimuon info
        TSQLRow* row_dimuon = res_dimuon->Next();

        dimuon.dimuonID     = getInt(row_dimuon->GetField(0));
        event.runID         = getInt(row_dimuon->GetField(1));
        event.spillID       = getInt(row_dimuon->GetField(2));
        event.eventID       = getInt(row_dimuon->GetField(3));
        dimuon.posTrackID   = getInt(row_dimuon->GetField(4));
        dimuon.negTrackID   = getInt(row_dimuon->GetField(5));
        dimuon.dx           = getFloat(row_dimuon->GetField(6));
        dimuon.dy           = getFloat(row_dimuon->GetField(7));
        dimuon.dz           = getFloat(row_dimuon->GetField(8));
        dimuon.dpx          = getFloat(row_dimuon->GetField(9));
        dimuon.dpy          = getFloat(row_dimuon->GetField(10));
        dimuon.dpz          = getFloat(row_dimuon->GetField(11));
        dimuon.mass         = getFloat(row_dimuon->GetField(12));
        dimuon.xF           = getFloat(row_dimuon->GetField(13));
        dimuon.x1           = getFloat(row_dimuon->GetField(14));
        dimuon.x2           = getFloat(row_dimuon->GetField(15));
        dimuon.trackSeparation = getFloat(row_dimuon->GetField(16));
        dimuon.chisq_dimuon = getFloat(row_dimuon->GetField(17));
        dimuon.px1          = getFloat(row_dimuon->GetField(18));
        dimuon.py1          = getFloat(row_dimuon->GetField(19));
        dimuon.pz1          = getFloat(row_dimuon->GetField(20));
        dimuon.px2          = getFloat(row_dimuon->GetField(21));
        dimuon.py2          = getFloat(row_dimuon->GetField(22));
        dimuon.pz2          = getFloat(row_dimuon->GetField(23));
        dimuon.costh        = getFloat(row_dimuon->GetField(24));
        dimuon.phi          = getFloat(row_dimuon->GetField(25));
        dimuon.pT           = sqrt(dimuon.dpx*dimuon.dpx + dimuon.dpy*dimuon.dpy);
        int targetPos       = getInt(row_dimuon->GetField(26));

        delete row_dimuon;

        //spill info and cuts
        if(!spill.skipflag && event.spillID != spill.spillID) //we have a new spill here
        {
            event.log("New spill!");
            spill.spillID = event.spillID;

            //check if the spill exists in pre-loaded spills
            if(spillBank.find(event.spillID) == spillBank.end())
            {
                event.log("spill does not exist!");
                badSpillFlag = true;
            }
            else
            {
                spill = spillBank[event.spillID];
                badSpillFlag = false;
            }
        }
        else
        {
            spill.spillID = event.spillID;
            spill.targetPos = targetPos;
            spill.TARGPOS_CONTROL = targetPos;
        }
        if(badSpillFlag) continue;

        //event info
        if(!spill.skipflag) //it's real data
        {
            TString query = Form("SELECT a.`RF-16`,a.`RF-15`,a.`RF-14`,a.`RF-13`,a.`RF-12`,a.`RF-11`,a.`RF-10`,a.`RF-09`,"
                "a.`RF-08`,a.`RF-07`,a.`RF-06`,a.`RF-05`,a.`RF-04`,a.`RF-03`,a.`RF-02`,a.`RF-01`,a.`RF+00`,a.`RF+01`,"
                "a.`RF+02`,a.`RF+03`,a.`RF+04`,a.`RF+05`,a.`RF+06`,a.`RF+07`,a.`RF+08`,a.`RF+09`,a.`RF+10`,a.`RF+11`,"
                "a.`RF+12`,a.`RF+13`,a.`RF+14`,a.`RF+15`,a.`RF+16`,a.Intensity_p,d.D1,d.D2,d.D3,d.H1,d.H2,d.H3,d.H4,d.P1,d.P2,"
                "b.MATRIX1,c.status,c.source1,c.source2 FROM QIE AS a,Event AS b,kEvent AS c,Occupancy AS d WHERE a.runID=%d "
                "AND a.eventID=%d AND b.runID=%d AND b.eventID=%d AND c.runID=%d AND c.eventID=%d AND d.runID=%d AND d.eventID=%d",
                event.runID, event.eventID, event.runID, event.eventID, event.runID, event.eventID, event.runID, event.eventID);
            TSQLResult* res_event = server->Query(query.Data());
            if(res_event->GetRowCount() != 1)
            {
                event.log("lacks QIE/Trigger/kEvent/Occupancy info");

                delete res_event;
                continue;
            }

            TSQLRow* row_event = res_event->Next();
            for(int j = 0; j < 33; ++j)
            {
                event.intensity[j] = getFloat(row_event->GetField(j));
            }
            event.intensityP = getFloat(row_event->GetField(33));
            for(int j = 0; j < 9; ++j) event.occupancy[j] = getInt(row_event->GetField(34+j));
            event.MATRIX1 = getInt(row_event->GetField(43));
            event.status = getInt(row_event->GetField(44));
            event.source1 = getInt(row_event->GetField(45));
            event.source2 = getInt(row_event->GetField(46));
            event.weight = 1.;
            delete row_event;
            delete res_event;
        }
        else //mix data
        {
            TString query = Form("SELECT status,source1,source2 FROM kEvent%s WHERE runID=%d AND eventID=%d", suffix.Data(), event.runID, event.eventID);
            TSQLResult* res_eventmix = server->Query(query.Data());
            TSQLRow* row_eventmix = res_eventmix->Next();
            event.MATRIX1 = 1;
            event.weight = 1.;
            event.status = getInt(row_eventmix->GetField(0));
            event.source1 = getInt(row_eventmix->GetField(1));
            event.source2 = getInt(row_eventmix->GetField(2));

            delete res_eventmix;
            delete row_eventmix;

            query = Form("SELECT AVG(Intensity_p),AVG(`RF-16`),AVG(`RF-15`),AVG(`RF-14`),AVG(`RF-13`),AVG(`RF-12`),AVG(`RF-11`),AVG(`RF-10`),AVG(`RF-09`),"
                "AVG(`RF-08`),AVG(`RF-07`),AVG(`RF-06`),AVG(`RF-05`),AVG(`RF-04`),AVG(`RF-03`),AVG(`RF-02`),AVG(`RF-01`),AVG(`RF+00`),AVG(`RF+01`),"
                "AVG(`RF+02`),AVG(`RF+03`),AVG(`RF+04`),AVG(`RF+05`),AVG(`RF+06`),AVG(`RF+07`),AVG(`RF+08`),AVG(`RF+09`),AVG(`RF+10`),AVG(`RF+11`),"
                "AVG(`RF+12`),AVG(`RF+13`),AVG(`RF+14`),AVG(`RF+15`),AVG(`RF+16`) FROM QIE WHERE runID=%d AND (eventID=%d or eventID=%d)", event.runID,
                event.source1, event.source2);
            TSQLResult* res_eventmix2 = server->Query(query);
            TSQLRow* row_eventmix2 = res_eventmix2->Next();

            event.intensityP = getFloat(row_eventmix2->GetField(0));
            for(int j = 0; j < 33; ++j) event.intensity[j] = getFloat(row_eventmix2->GetField(j+1));

            delete row_eventmix2;
            delete res_eventmix2;

            query = Form("SELECT AVG(D1),AVG(D2),AVG(D3),AVG(H1),AVG(H2),AVG(H3),AVG(H4),AVG(P1),AVG(P2) FROM Occupancy WHERE runID=%d AND "
                "(eventID=%d or eventID=%d)", event.runID, event.source1, event.source2);
            TSQLResult* res_eventmix3 = server->Query(query);
            TSQLRow* row_eventmix3 = res_eventmix2->Next();

            for(int j = 0; j < 9; ++j) event.occupancy[j] = getFloat(row_eventmix3->GetField(j));

            delete row_eventmix3;
            delete res_eventmix3;
        }

        //track info
        TString query = Form("SELECT numHits,numHitsSt1,numHitsSt2,numHitsSt3,numHitsSt4H,numHitsSt4V,"
            "chisq,chisq_target,chisq_dump,chisq_upstream,x1,y1,z1,x3,y3,z3,x0,y0,z0,xT,yT,"
            "xD,yD,px1,py1,pz1,px3,py3,pz3,px0,py0,pz0,pxT,pyT,pzT,pxD,pyD,pzD,roadID,"
            "tx_PT,ty_PT,thbend,kmstatus,z0x,z0y FROM kTrack%s "
            "WHERE runID=%d AND eventID=%d AND trackID in (%d,%d) ORDER BY charge DESC",
            suffix.Data(), event.runID, event.eventID, dimuon.posTrackID, dimuon.negTrackID);

        TSQLResult* res_track = server->Query(query);
        if(res_track->GetRowCount() != 2)
        {
            event.log("has less than 2 tracks");

            delete res_track;
            continue;
        }

        TSQLRow* row_track = res_track->Next();
        posTrack.trackID   = dimuon.posTrackID;
        posTrack.charge    = 1;
        posTrack.nHits     = getInt(row_track->GetField(0));
        posTrack.nHitsSt1  = getInt(row_track->GetField(1));
        posTrack.nHitsSt2  = getInt(row_track->GetField(2));
        posTrack.nHitsSt3  = getInt(row_track->GetField(3));
        posTrack.nHitsSt4H = getInt(row_track->GetField(4));
        posTrack.nHitsSt4V = getInt(row_track->GetField(5));
        posTrack.chisq          = getFloat(row_track->GetField(6));
        posTrack.chisq_target   = getFloat(row_track->GetField(7));
        posTrack.chisq_dump     = getFloat(row_track->GetField(8));
        posTrack.chisq_upstream = getFloat(row_track->GetField(9));
        posTrack.x1    = getFloat(row_track->GetField(10));
        posTrack.y1    = getFloat(row_track->GetField(11));
        posTrack.z1    = getFloat(row_track->GetField(12));
        posTrack.x3    = getFloat(row_track->GetField(13));
        posTrack.y3    = getFloat(row_track->GetField(14));
        posTrack.z3    = getFloat(row_track->GetField(15));
        posTrack.x0    = getFloat(row_track->GetField(16));
        posTrack.y0    = getFloat(row_track->GetField(17));
        posTrack.z0    = getFloat(row_track->GetField(18));
        posTrack.xT    = getFloat(row_track->GetField(19));
        posTrack.yT    = getFloat(row_track->GetField(20));
        posTrack.zT    = -129.;
        posTrack.xD    = getFloat(row_track->GetField(21));
        posTrack.yD    = getFloat(row_track->GetField(22));
        posTrack.zD    = 42.;
        posTrack.px1   = getFloat(row_track->GetField(23));
        posTrack.py1   = getFloat(row_track->GetField(24));
        posTrack.pz1   = getFloat(row_track->GetField(25));
        posTrack.px3   = getFloat(row_track->GetField(26));
        posTrack.py3   = getFloat(row_track->GetField(27));
        posTrack.pz3   = getFloat(row_track->GetField(28));
        posTrack.px0   = getFloat(row_track->GetField(29));
        posTrack.py0   = getFloat(row_track->GetField(30));
        posTrack.pz0   = getFloat(row_track->GetField(31));
        posTrack.pxT   = getFloat(row_track->GetField(32));
        posTrack.pyT   = getFloat(row_track->GetField(33));
        posTrack.pzT   = getFloat(row_track->GetField(34));
        posTrack.pxD   = getFloat(row_track->GetField(35));
        posTrack.pyD   = getFloat(row_track->GetField(36));
        posTrack.pzD   = getFloat(row_track->GetField(37));
        posTrack.roadID = getInt(row_track->GetField(38));
        posTrack.tx_PT  = getFloat(row_track->GetField(39));
        posTrack.ty_PT  = getFloat(row_track->GetField(40));
        posTrack.thbend = getFloat(row_track->GetField(41));
        posTrack.kmstatus = getInt(row_track->GetField(42));
        posTrack.z0x    = getInt(row_track->GetField(43));
        posTrack.z0y    = getInt(row_track->GetField(44));
        posTrack.pxv    = dimuon.px1;
        posTrack.pyv    = dimuon.py1;
        posTrack.pzv    = dimuon.pz1;
        delete row_track;

        row_track = res_track->Next();
        negTrack.trackID   = dimuon.negTrackID;
        negTrack.charge    = 1;
        negTrack.nHits     = getInt(row_track->GetField(0));
        negTrack.nHitsSt1  = getInt(row_track->GetField(1));
        negTrack.nHitsSt2  = getInt(row_track->GetField(2));
        negTrack.nHitsSt3  = getInt(row_track->GetField(3));
        negTrack.nHitsSt4H = getInt(row_track->GetField(4));
        negTrack.nHitsSt4V = getInt(row_track->GetField(5));
        negTrack.chisq          = getFloat(row_track->GetField(6));
        negTrack.chisq_target   = getFloat(row_track->GetField(7));
        negTrack.chisq_dump     = getFloat(row_track->GetField(8));
        negTrack.chisq_upstream = getFloat(row_track->GetField(9));
        negTrack.x1    = getFloat(row_track->GetField(10));
        negTrack.y1    = getFloat(row_track->GetField(11));
        negTrack.z1    = getFloat(row_track->GetField(12));
        negTrack.x3    = getFloat(row_track->GetField(13));
        negTrack.y3    = getFloat(row_track->GetField(14));
        negTrack.z3    = getFloat(row_track->GetField(15));
        negTrack.x0    = getFloat(row_track->GetField(16));
        negTrack.y0    = getFloat(row_track->GetField(17));
        negTrack.z0    = getFloat(row_track->GetField(18));
        negTrack.xT    = getFloat(row_track->GetField(19));
        negTrack.yT    = getFloat(row_track->GetField(20));
        negTrack.zT    = -129.;
        negTrack.xD    = getFloat(row_track->GetField(21));
        negTrack.yD    = getFloat(row_track->GetField(22));
        negTrack.zD    = 42.;
        negTrack.px1   = getFloat(row_track->GetField(23));
        negTrack.py1   = getFloat(row_track->GetField(24));
        negTrack.pz1   = getFloat(row_track->GetField(25));
        negTrack.px3   = getFloat(row_track->GetField(26));
        negTrack.py3   = getFloat(row_track->GetField(27));
        negTrack.pz3   = getFloat(row_track->GetField(28));
        negTrack.px0   = getFloat(row_track->GetField(29));
        negTrack.py0   = getFloat(row_track->GetField(30));
        negTrack.pz0   = getFloat(row_track->GetField(31));
        negTrack.pxT   = getFloat(row_track->GetField(32));
        negTrack.pyT   = getFloat(row_track->GetField(33));
        negTrack.pzT   = getFloat(row_track->GetField(34));
        negTrack.pxD   = getFloat(row_track->GetField(35));
        negTrack.pyD   = getFloat(row_track->GetField(36));
        negTrack.pzD   = getFloat(row_track->GetField(37));
        negTrack.roadID = getInt(row_track->GetField(38));
        negTrack.tx_PT  = getFloat(row_track->GetField(39));
        negTrack.ty_PT  = getFloat(row_track->GetField(40));
        negTrack.thbend = getFloat(row_track->GetField(41));
        negTrack.kmstatus = getInt(row_track->GetField(42));
        negTrack.z0x    = getInt(row_track->GetField(43));
        negTrack.z0y    = getInt(row_track->GetField(44));
        negTrack.pxv    = dimuon.px2;
        negTrack.pyv    = dimuon.py2;
        negTrack.pzv    = dimuon.pz2;
        delete row_track;
        delete res_track;

        //Now save everything
        saveTree->Fill();
        if(saveTree->GetEntries() % 1000 == 0) saveTree->AutoSave("self");
    }
    delete res_dimuon;

    cout << Form("sqlReader finished reading %s tables successfully.", suffix.Data()) << endl;

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
