#include <iostream>
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

int main(int argc, char* argv[])
{
    // define the output file structure
    Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    Event* p_event = new Event; Event& event = *p_event;
    Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
    Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

    TFile* saveFile = new TFile(argv[2], "recreate");
    TTree* saveTree = new TTree("save", "save");

    saveTree->Branch("dimuon", &p_dimuon, 256000, 99);
    saveTree->Branch("event", &p_event, 256000, 99);
    saveTree->Branch("spill", &p_spill, 256000, 99);
    saveTree->Branch("posTrack", &p_posTrack, 256000, 99);
    saveTree->Branch("negTrack", &p_negTrack, 256000, 99);

    TTree* spillTree = new TTree("spill", "spill");
    spillTree->Branch("spill", &p_spill, 256000, 99);

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], atoi(argv[4])), "seaguest", "qqbar2mu+mu-");
    server->Exec(Form("USE %s", argv[1]));
    cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

    //define the source tables
    TString suffix = "";
    if(argc > 5) suffix = argv[5];

    //decide how to access event/spill level information
    bool mcdata = server->HasTable("mDimuon");
    bool mixdata = suffix.Contains("Mix");
    spill.skipflag = mcdata || mixdata;

    char query[2000];
    sprintf(query, "SELECT dimuonID,runID,spillID,eventID,posTrackID,negTrackID,dx,dy,dz,"
        "dpx,dpy,dpz,mass,xF,xB,xT,trackSeparation,chisq_dimuon,px1,py1,pz1,px2,py2,pz2,costh,phi,targetPos "
        "FROM kDimuon%s WHERE mass>0.5 AND mass<10. AND chisq_dimuon<25. AND xB>0. AND xB<1. AND "
        "xT>0. AND xT<1. AND xF>-1. AND xF<1. AND ABS(dx)<2. AND ABS(dy)<2. AND dz>-300. "
        "AND dz<300. AND ABS(dpx)<3. AND ABS(dpy)<3. AND dpz>30. AND dpz<120. AND "
        "ABS(trackSeparation)<300. ORDER BY spillID", suffix.Data());

    TSQLResult* res_dimuon = server->Query(query);
    int nDimuonsRaw = res_dimuon->GetRowCount();

    spill.spillID = -1;
    int nGoodSpill = 0;
    int nBadSpill_record = 0;
    int nBadSpill_quality = 0;
    bool badSpillFlag = false;
    for(int i = 0; i < nDimuonsRaw; ++i)
    {
        if(i % 1000 == 0)
        {
            cout << "Converting dimuon " << i << "/" << nDimuonsRaw << endl;
        }

        //basic dimuon info
        TSQLRow* row_dimuon = res_dimuon->Next();

        dimuon.dimuonID     = atoi(row_dimuon->GetField(0));
        event.runID         = atoi(row_dimuon->GetField(1));
        event.spillID       = atoi(row_dimuon->GetField(2));
        event.eventID       = atoi(row_dimuon->GetField(3));
        dimuon.posTrackID   = atoi(row_dimuon->GetField(4));
        dimuon.negTrackID   = atoi(row_dimuon->GetField(5));
        dimuon.dx           = atof(row_dimuon->GetField(6));
        dimuon.dy           = atof(row_dimuon->GetField(7));
        dimuon.dz           = atof(row_dimuon->GetField(8));
        dimuon.dpx          = atof(row_dimuon->GetField(9));
        dimuon.dpy          = atof(row_dimuon->GetField(10));
        dimuon.dpz          = atof(row_dimuon->GetField(11));
        dimuon.mass         = atof(row_dimuon->GetField(12));
        dimuon.xF           = atof(row_dimuon->GetField(13));
        dimuon.x1           = atof(row_dimuon->GetField(14));
        dimuon.x2           = atof(row_dimuon->GetField(15));
        dimuon.trackSeparation = atof(row_dimuon->GetField(16));
        dimuon.chisq_dimuon = atof(row_dimuon->GetField(17));
        dimuon.px1          = atof(row_dimuon->GetField(18));
        dimuon.py1          = atof(row_dimuon->GetField(19));
        dimuon.pz1          = atof(row_dimuon->GetField(20));
        dimuon.px2          = atof(row_dimuon->GetField(21));
        dimuon.py2          = atof(row_dimuon->GetField(22));
        dimuon.pz2          = atof(row_dimuon->GetField(23));
        dimuon.costh        = atof(row_dimuon->GetField(24));
        dimuon.phi          = atof(row_dimuon->GetField(25));
        dimuon.pT           = sqrt(dimuon.dpx*dimuon.dpx + dimuon.dpy*dimuon.dpy);
        int targetPos       = atoi(row_dimuon->GetField(26));

        delete row_dimuon;

        //spill info and cuts
        if(!(mcdata || mixdata) && event.spillID != spill.spillID) //we have a new spill here
        {
            event.log("New spill!");

            spill.spillID = event.spillID;
            badSpillFlag = false;

            //run configuration
            sprintf(query, "SELECT value FROM Run WHERE runID=%d AND name IN ('KMAG-Avg','MATRIX3Prescale') ORDER BY name", event.runID);
            TSQLResult* res_spill = server->Query(query);
            if(res_spill->GetRowCount() != 2)
            {
                ++nBadSpill_record;
                spill.log("lacks Run table info");

                delete res_spill;
                continue;
            }

            TSQLRow* row_spill = res_spill->Next();  spill.KMAG = atof(row_spill->GetField(0)); delete row_spill;
            row_spill = res_spill->Next(); spill.MATRIX3Prescale = atoi(row_spill->GetField(0)); delete row_spill;
            delete res_spill;

            //target position
            sprintf(query, "SELECT a.targetPos,b.value,a.dataQuality,a.liveProton FROM Spill AS a,Target AS b WHERE a.spillID"
                "=%d AND b.spillID=%d AND b.name='TARGPOS_CONTROL' AND a.liveProton IS NOT NULL", event.spillID, event.spillID);
            res_spill = server->Query(query);
            if(res_spill->GetRowCount() != 1)
            {
                ++nBadSpill_record;
                badSpillFlag = true;
                event.log("lacks target position info");

                delete res_spill;
                continue;
            }

            row_spill = res_spill->Next();
            spill.targetPos       = atoi(row_spill->GetField(0));
            spill.TARGPOS_CONTROL = atoi(row_spill->GetField(1));
            spill.quality         = atoi(row_spill->GetField(2));
            spill.liveProton      = atof(row_spill->GetField(3));
            delete row_spill;
            delete res_spill;

            //Reconstruction info
            sprintf(query, "SELECT (SELECT COUNT(*) FROM Event WHERE spillID=%d),"
                                  "(SELECT COUNT(*) FROM kDimuon WHERE spillID=%d),"
                                  "(SELECT COUNT(*) FROM kTrack WHERE spillID=%d)", spill.spillID, spill.spillID, spill.spillID);
            res_spill = server->Query(query);
            if(res_spill->GetRowCount() != 1)
            {
                ++nBadSpill_record;
                spill.log("lacks reconstructed tables");

                delete res_spill;
                continue;
            }

            row_spill = res_spill->Next();
            spill.nEvents = atoi(row_spill->GetField(0));
            spill.nDimuons = atoi(row_spill->GetField(1));
            spill.nTracks = atoi(row_spill->GetField(2));
            delete row_spill;
            delete res_spill;

            //Beam/BeamDAQ
            sprintf(query, "SELECT a.value,b.NM3ION,b.QIESum,b.inhibit_block_sum,b.trigger_sum_no_inhibit,"
                "b.dutyFactor53MHz FROM Beam AS a,BeamDAQ AS b WHERE a.spillID=%d AND b.spillID=%d AND "
                "a.name='S:G2SEM'", event.spillID, event.spillID);
            res_spill = server->Query(query);
            if(res_spill->GetRowCount() != 1)
            {
                ++nBadSpill_record;
                badSpillFlag = true;
                event.log("lacks Beam/BeamDAQ info");

                delete res_spill;
                continue;
            }

            row_spill = res_spill->Next();
            spill.G2SEM      = atof(row_spill->GetField(0));
            spill.NM3ION     = atof(row_spill->GetField(1));
            spill.QIESum     = atof(row_spill->GetField(2));
            spill.inhibitSum = atof(row_spill->GetField(3));
            spill.busySum    = atof(row_spill->GetField(4));
            spill.dutyFactor = atof(row_spill->GetField(5));

            delete row_spill;
            delete res_spill;

            //Scalar table
            sprintf(query, "SELECT value FROM Scaler WHERE spillType='EOS' AND spillID=%d AND scalerName "
                "in ('TSGo','AcceptedMatrix1','AfterInhMatrix1') ORDER BY scalerName", event.spillID);
            res_spill = server->Query(query);
            if(res_spill->GetRowCount() != 3)
            {
                ++nBadSpill_record;
                badSpillFlag = true;
                event.log("lacks scaler info");

                delete res_spill;
                continue;
            }

            row_spill = res_spill->Next(); spill.acceptedMatrix1 = atof(row_spill->GetField(0)); delete row_spill;
            row_spill = res_spill->Next(); spill.afterInhMatrix1 = atof(row_spill->GetField(0)); delete row_spill;
            row_spill = res_spill->Next(); spill.TSGo            = atof(row_spill->GetField(0)); delete row_spill;
            delete res_spill;

            if(spill.goodSpill())
            {
                spillTree->Fill();
                ++nGoodSpill;

                if(nGoodSpill % 100 == 0) spillTree->AutoSave("self");
            }
            else
            {
                ++nBadSpill_quality;
                badSpillFlag = true;
                spill.print();
                continue;
            }
        }
        else if(mixdata)
        {
            spill.targetPos = targetPos;
            spill.TARGPOS_CONTROL = targetPos;
        }
        if(badSpillFlag) continue;

        //event info
        if(!(mcdata || mixdata))
        {
            sprintf(query, "SELECT a.`RF-16`,a.`RF-15`,a.`RF-14`,a.`RF-13`,a.`RF-12`,a.`RF-11`,a.`RF-10`,a.`RF-09`,"
                "a.`RF-08`,a.`RF-07`,a.`RF-06`,a.`RF-05`,a.`RF-04`,a.`RF-03`,a.`RF-02`,a.`RF-01`,a.`RF+00`,a.`RF+01`,"
                "a.`RF+02`,a.`RF+03`,a.`RF+04`,a.`RF+05`,a.`RF+06`,a.`RF+07`,a.`RF+08`,a.`RF+09`,a.`RF+10`,a.`RF+11`,"
                "a.`RF+12`,a.`RF+13`,a.`RF+14`,a.`RF+15`,a.`RF+16`,"
                "b.MATRIX1,c.status FROM QIE AS a,Event AS b,kEvent AS c WHERE a.runID=%d "
                "AND a.eventID=%d AND b.runID=%d AND b.eventID=%d AND c.runID=%d AND c.eventID=%d",
                event.runID, event.eventID, event.runID, event.eventID, event.runID, event.eventID);
            TSQLResult* res_event = server->Query(query);
            if(res_event->GetRowCount() != 1)
            {
                event.log("lacks QIE/Trigger/kEvent info");

                delete res_event;
                continue;
            }

            TSQLRow* row_event = res_event->Next();
            for(int j = 0; j < 33; ++j)
            {
                event.intensity[j] = atof(row_event->GetField(j));
            }
            event.MATRIX1 = atoi(row_event->GetField(33));
            event.status = atoi(row_event->GetField(34));
            event.weight = 1.;
            delete row_event;
            delete res_event;
        }
        else if(mcdata)
        {
            sprintf(query, "SELECT a.MATRIX1,b.targetPos,c.sigWeight,d.status FROM Event AS a,Spill AS b,mDimuon AS c,"
                "kEvent AS d WHERE a.runID=%d AND a.eventID=%d AND b.runID=%d AND b.eventID=%d AND"
                " c.runID=%d AND c.eventID=%d AND d.runID=%d AND d.eventID=%d", event.runID, event.eventID,
                event.runID, event.eventID, event.runID, event.eventID, event.runID, event.eventID);
            TSQLResult* res_event = server->Query(query);
            if(res_event->GetRowCount() != 1)
            {
                event.log("lacks Trigger/Target info");

                delete res_event;
                continue;
            }

            TSQLRow* row_event = res_event->Next();
            event.intensity[16] = 1.;
            event.MATRIX1 = atoi(row_event->GetField(0));
            event.weight = atof(row_event->GetField(2));
            event.status = atoi(row_event->GetField(3));
            spill.targetPos = atoi(row_event->GetField(1));
            spill.TARGPOS_CONTROL = spill.targetPos;
            delete row_event;
            delete res_event;
        }

        //track info
        sprintf(query, "SELECT numHits,numHitsSt1,numHitsSt2,numHitsSt3,numHitsSt4H,numHitsSt4V,"
            "chisq,chisq_target,chisq_dump,chisq_upstream,x1,y1,z1,x3,y3,z3,x0,y0,z0,xT,yT,"
            "xD,yD,px1,py1,pz1,px3,py3,pz3,px0,py0,pz0,pxT,pyT,pzT,pxD,pyD,pzD,roadID,"
            "tx_PT,ty_PT,thbend FROM kTrack%s "
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
        posTrack.nHits     = atoi(row_track->GetField(0));
        posTrack.nHitsSt1  = atoi(row_track->GetField(1));
        posTrack.nHitsSt2  = atoi(row_track->GetField(2));
        posTrack.nHitsSt3  = atoi(row_track->GetField(3));
        posTrack.nHitsSt4H = atoi(row_track->GetField(4));
        posTrack.nHitsSt4V = atoi(row_track->GetField(5));
        posTrack.chisq          = atof(row_track->GetField(6));
        posTrack.chisq_target   = atof(row_track->GetField(7));
        posTrack.chisq_dump     = atof(row_track->GetField(8));
        posTrack.chisq_upstream = atof(row_track->GetField(9));
        posTrack.x1    = atof(row_track->GetField(10));
        posTrack.y1    = atof(row_track->GetField(11));
        posTrack.z1    = atof(row_track->GetField(12));
        posTrack.x3    = atof(row_track->GetField(13));
        posTrack.y3    = atof(row_track->GetField(14));
        posTrack.z3    = atof(row_track->GetField(15));
        posTrack.x0    = atof(row_track->GetField(16));
        posTrack.y0    = atof(row_track->GetField(17));
        posTrack.z0    = atof(row_track->GetField(18));
        posTrack.xT    = atof(row_track->GetField(19));
        posTrack.yT    = atof(row_track->GetField(20));
        posTrack.zT    = -129.;
        posTrack.xD    = atof(row_track->GetField(21));
        posTrack.yD    = atof(row_track->GetField(22));
        posTrack.zD    = 42.;
        posTrack.px1   = atof(row_track->GetField(23));
        posTrack.py1   = atof(row_track->GetField(24));
        posTrack.pz1   = atof(row_track->GetField(25));
        posTrack.px3   = atof(row_track->GetField(26));
        posTrack.py3   = atof(row_track->GetField(27));
        posTrack.pz3   = atof(row_track->GetField(28));
        posTrack.px0   = atof(row_track->GetField(29));
        posTrack.py0   = atof(row_track->GetField(30));
        posTrack.pz0   = atof(row_track->GetField(31));
        posTrack.pxT   = atof(row_track->GetField(32));
        posTrack.pyT   = atof(row_track->GetField(33));
        posTrack.pzT   = atof(row_track->GetField(34));
        posTrack.pxD   = atof(row_track->GetField(35));
        posTrack.pyD   = atof(row_track->GetField(36));
        posTrack.pzD   = atof(row_track->GetField(37));
        posTrack.roadID = atoi(row_track->GetField(38));
        posTrack.tx_PT  = atof(row_track->GetField(39));
        posTrack.ty_PT  = atof(row_track->GetField(40));
        posTrack.thbend = atof(row_track->GetField(41));
        posTrack.pxv    = dimuon.px1;
        posTrack.pyv    = dimuon.py1;
        posTrack.pzv    = dimuon.pz1;
        delete row_track;

        row_track = res_track->Next();
        negTrack.trackID   = dimuon.negTrackID;
        negTrack.charge    = 1;
        negTrack.nHits     = atoi(row_track->GetField(0));
        negTrack.nHitsSt1  = atoi(row_track->GetField(1));
        negTrack.nHitsSt2  = atoi(row_track->GetField(2));
        negTrack.nHitsSt3  = atoi(row_track->GetField(3));
        negTrack.nHitsSt4H = atoi(row_track->GetField(4));
        negTrack.nHitsSt4V = atoi(row_track->GetField(5));
        negTrack.chisq          = atof(row_track->GetField(6));
        negTrack.chisq_target   = atof(row_track->GetField(7));
        negTrack.chisq_dump     = atof(row_track->GetField(8));
        negTrack.chisq_upstream = atof(row_track->GetField(9));
        negTrack.x1    = atof(row_track->GetField(10));
        negTrack.y1    = atof(row_track->GetField(11));
        negTrack.z1    = atof(row_track->GetField(12));
        negTrack.x3    = atof(row_track->GetField(13));
        negTrack.y3    = atof(row_track->GetField(14));
        negTrack.z3    = atof(row_track->GetField(15));
        negTrack.x0    = atof(row_track->GetField(16));
        negTrack.y0    = atof(row_track->GetField(17));
        negTrack.z0    = atof(row_track->GetField(18));
        negTrack.xT    = atof(row_track->GetField(19));
        negTrack.yT    = atof(row_track->GetField(20));
        negTrack.zT    = -129.;
        negTrack.xD    = atof(row_track->GetField(21));
        negTrack.yD    = atof(row_track->GetField(22));
        negTrack.zD    = 42.;
        negTrack.px1   = atof(row_track->GetField(23));
        negTrack.py1   = atof(row_track->GetField(24));
        negTrack.pz1   = atof(row_track->GetField(25));
        negTrack.px3   = atof(row_track->GetField(26));
        negTrack.py3   = atof(row_track->GetField(27));
        negTrack.pz3   = atof(row_track->GetField(28));
        negTrack.px0   = atof(row_track->GetField(29));
        negTrack.py0   = atof(row_track->GetField(30));
        negTrack.pz0   = atof(row_track->GetField(31));
        negTrack.pxT   = atof(row_track->GetField(32));
        negTrack.pyT   = atof(row_track->GetField(33));
        negTrack.pzT   = atof(row_track->GetField(34));
        negTrack.pxD   = atof(row_track->GetField(35));
        negTrack.pyD   = atof(row_track->GetField(36));
        negTrack.pzD   = atof(row_track->GetField(37));
        negTrack.roadID = atoi(row_track->GetField(38));
        negTrack.tx_PT  = atof(row_track->GetField(39));
        negTrack.ty_PT  = atof(row_track->GetField(40));
        negTrack.thbend = atof(row_track->GetField(41));
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
    if(!spill.skipflag) cout << nGoodSpill << " good spills, " << nBadSpill_record << " spills have insufficient info in database, " << nBadSpill_quality << " rejected because of bad quality." << endl;

    saveFile->cd();
    saveTree->Write();
    spillTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
