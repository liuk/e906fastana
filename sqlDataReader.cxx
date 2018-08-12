#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <map>

#include <TROOT.h>
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include <TSQLServer.h>
#include <TSQLResult.h>
#include <TSQLRow.h>
#include <TString.h>

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

    //Temporary cache
    map<int, Dimuon> dimuonBank;  //key - eventID
    map<int, Spill>  spillBank;   //key - spillID
    map<int, Track>  posTrackBank; //key - eventID
    map<int, Track>  negTrackBank; //key - eventID
    map<int, Event>  eventBank;    //key - eventID

    //Connect to server
    TSQLServer* server = TSQLServer::Connect(Form("mysql://%s:%d", argv[3], atoi(argv[4])), "seaguest", "qqbar2mu+mu-");
    cout << "Reading run " << argv[1] << " and save to " << argv[2] << endl;

    //define the source tables
    TString suffix = "";
    if(argc > 6) suffix = argv[6];

    //decide how to access event/spill level information
    bool mcdata  = false; 
    bool mixdata = suffix.Contains("Mix");

    //pre-load spill information if provided
    TFile* spillFile = new TFile(argv[5]);
    TTree* spillTree = (TTree*)spillFile->Get("save");
    spillTree->SetBranchAddress("spill", &p_spill);

    for(int i = 0; i < spillTree->GetEntries(); ++i)
    {
        spillTree->GetEntry(i);
        spillBank.insert(map<int, Spill>::value_type(spill.spillID, spill));
    }

    TString query = Form("SELECT dimuonID,runID,spillID,eventID,posTrackID,negTrackID,dx,dy,dz,"
        "dpx,dpy,dpz,mass,xF,xB,xT,trackSeparation,chisq_dimuon,px1,py1,pz1,px2,py2,pz2,costh,phi,targetPos "
        "FROM run_%s_R008.kDimuon%s WHERE mass>0.5 AND mass<10. AND chisq_dimuon<25. AND xB>0. AND xB<1. AND "
        "xT>0. AND xT<1. AND xF>-1. AND xF<1. AND ABS(dx)<2. AND ABS(dy)<2. AND dz>-300. "
        "AND dz<10. AND ABS(dpx)<3. AND ABS(dpy)<3. AND dpz>30. AND dpz<120. AND "
        "ABS(trackSeparation)<300. ORDER BY spillID,eventID", argv[1], suffix.Data());
    
    TSQLResult* res_dimuon = server->Query(query.Data());
    int nDimuonsRaw = res_dimuon->GetRowCount();
    cout << "Dimuon query: " << query << " : " << nDimuonsRaw << endl;
    for(int i = 0; i < nDimuonsRaw; ++i)
    {
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

        //check spill
        if(!mixdata)
        {
            if(spillBank.find(event.spillID) == spillBank.end()) continue;
        }

        dimuonBank[event.eventID] = dimuon;
        eventBank[event.eventID]  = event;
    }
    delete res_dimuon;

    if(!(mixdata || mcdata))  //this is normal data
    {
        TString query = Form("SELECT a.`RF-16`,a.`RF-15`,a.`RF-14`,a.`RF-13`,a.`RF-12`,a.`RF-11`,a.`RF-10`,a.`RF-09`,"
                "a.`RF-08`,a.`RF-07`,a.`RF-06`,a.`RF-05`,a.`RF-04`,a.`RF-03`,a.`RF-02`,a.`RF-01`,a.`RF+00`,a.`RF+01`,"
                "a.`RF+02`,a.`RF+03`,a.`RF+04`,a.`RF+05`,a.`RF+06`,a.`RF+07`,a.`RF+08`,a.`RF+09`,a.`RF+10`,a.`RF+11`,"
                "a.`RF+12`,a.`RF+13`,a.`RF+14`,a.`RF+15`,a.`RF+16`,b.MATRIX1,c.status,b.eventID,d.D1,"
                "d.D2,d.D3,d.H1,d.H2,d.H3,d.H4,d.P1,d.P2,d.D1L,d.D1R,d.D2L,d.D2R,d.D3L,d.D3R FROM run_%s_R007.QIE AS a,"
                "run_%s_R007.Event AS b,run_%s_R008.kEvent AS c,run_%s_R007.Occupancy AS d WHERE a.eventID=b.eventID AND "
                "b.eventID=c.eventID AND c.eventID=d.eventID ORDER BY b.eventID", argv[1], argv[1], argv[1], argv[1]);

        TSQLResult* res_event = server->Query(query.Data());
        int nEventRows = res_event->GetRowCount();
        cout << "Normal event query: " << query << " : " << nEventRows << endl;
        for(int i = 0; i < nEventRows; ++i)
        {
            TSQLRow* row_event = res_event->Next();
            int eventID = atoi(row_event->GetField(35));
            if(eventBank.find(eventID) == eventBank.end())
            {
                delete row_event;
                continue;
            }

            for(int j = 0; j < 33; ++j) eventBank[eventID].intensity[j] = atof(row_event->GetField(j));
            eventBank[eventID].MATRIX1 = atoi(row_event->GetField(33));
            eventBank[eventID].sourceID1 = -1;
            eventBank[eventID].sourceID2 = -1;
            eventBank[eventID].status  = atoi(row_event->GetField(34));
            for(int j = 0; j < 15; ++j) eventBank[eventID].occupancy[j] = atof(row_event->GetField(j+36));
            eventBank[eventID].weight  = 1.;

            delete row_event;
        }
        delete res_event;
    }
    else if(mixdata) //for mix data
    {
        TString query = Form("SELECT a.`RF-16`,a.`RF-15`,a.`RF-14`,a.`RF-13`,a.`RF-12`,a.`RF-11`,a.`RF-10`,a.`RF-09`,"
                "a.`RF-08`,a.`RF-07`,a.`RF-06`,a.`RF-05`,a.`RF-04`,a.`RF-03`,a.`RF-02`,a.`RF-01`,a.`RF+00`,a.`RF+01`,"
                "a.`RF+02`,a.`RF+03`,a.`RF+04`,a.`RF+05`,a.`RF+06`,a.`RF+07`,a.`RF+08`,a.`RF+09`,a.`RF+10`,a.`RF+11`,"
                "a.`RF+12`,a.`RF+13`,a.`RF+14`,a.`RF+15`,a.`RF+16`,b.source1,b.status,b.eventID,d.D1,"
                "d.D2,d.D3,d.H1,d.H2,d.H3,d.H4,d.P1,d.P2,d.D1L,d.D1R,d.D2L,d.D2R,d.D3L,d.D3R,d.spillID FROM run_%s_R007.QIE AS a,"
                "run_%s_R008.kEvent%s AS b,run_%s_R007.Occupancy AS d WHERE a.eventID=b.source1 AND "
                "d.eventID=b.source1 ORDER BY b.eventID", argv[1], argv[1], suffix.Data(), argv[1]);

        TSQLResult* res_event1 = server->Query(query.Data());
        int nEventRows = res_event1->GetRowCount();
        cout << "Mix event query 1: " << query << " : " << nEventRows << endl;
        for(int i = 0; i < nEventRows; ++i)
        {
            TSQLRow* row_event = res_event1->Next();
            int eventID = atoi(row_event->GetField(35));
            if(eventBank.find(eventID) == eventBank.end())
            {
                delete row_event;
                continue;
            }

            for(int j = 0; j < 33; ++j) eventBank[eventID].intensity[j] = 0.5*atof(row_event->GetField(j));
            eventBank[eventID].MATRIX1 = 1;
            eventBank[eventID].sourceID1 = atoi(row_event->GetField(33));
            eventBank[eventID].status  = atoi(row_event->GetField(34));
            for(int j = 0; j < 15; ++j) eventBank[eventID].occupancy[j] = atof(row_event->GetField(j+36));
            eventBank[eventID].spillID = atoi(row_event->GetField(51));
            eventBank[eventID].weight  = 1.;

            delete row_event;
        }
        delete res_event1;

        query.ReplaceAll("source1", "source2");
        TSQLResult* res_event2 = server->Query(query.Data());
        nEventRows = res_event2->GetRowCount();
        cout << "Mix event query 2: " << query << " : " << nEventRows << endl;
        for(int i = 0; i < nEventRows; ++i)
        {
            TSQLRow* row_event = res_event2->Next();
            int eventID = atoi(row_event->GetField(35));
            if(eventBank.find(eventID) == eventBank.end())
            {
                delete row_event;
                continue;
            }

            eventBank[eventID].sourceID2 = atoi(row_event->GetField(33));
            for(int j = 0; j < 33; ++j) eventBank[eventID].intensity[j] += 0.5*atof(row_event->GetField(j));
            for(int j = 0; j < 15; ++j) eventBank[eventID].occupancy[j] += 0.5*atof(row_event->GetField(j+36));

            delete row_event;
        }
        delete res_event2;
    }

    //track info
    query = Form("SELECT numHits,numHitsSt1,numHitsSt2,numHitsSt3,numHitsSt4H,numHitsSt4V,"
            "chisq,chisq_target,chisq_dump,chisq_upstream,x1,y1,z1,x3,y3,z3,x0,y0,z0,xT,yT,"
            "xD,yD,px1,py1,pz1,px3,py3,pz3,px0,py0,pz0,pxT,pyT,pzT,pxD,pyD,pzD,roadID,"
            "tx_PT,ty_PT,thbend,kmstatus,trackID,eventID,charge FROM run_%s_R008.kTrack%s ORDER BY eventID",
            argv[1], suffix.Data());

    TSQLResult* res_track = server->Query(query.Data());
    int nTrackRows = res_track->GetRowCount();
    cout << "Track query: " << query << " : " << nTrackRows << endl;
    for(int i = 0; i < nTrackRows; ++i)
    {
        TSQLRow* row_track = res_track->Next();

        Track track;
        track.trackID   = atoi(row_track->GetField(43));
        track.charge    = atoi(row_track->GetField(45));
        track.nHits     = atoi(row_track->GetField(0));
        track.nHitsSt1  = atoi(row_track->GetField(1));
        track.nHitsSt2  = atoi(row_track->GetField(2));
        track.nHitsSt3  = atoi(row_track->GetField(3));
        track.nHitsSt4H = atoi(row_track->GetField(4));
        track.nHitsSt4V = atoi(row_track->GetField(5));
        track.chisq          = atof(row_track->GetField(6));
        track.chisq_target   = atof(row_track->GetField(7));
        track.chisq_dump     = atof(row_track->GetField(8));
        track.chisq_upstream = atof(row_track->GetField(9));
        track.x1    = atof(row_track->GetField(10));
        track.y1    = atof(row_track->GetField(11));
        track.z1    = atof(row_track->GetField(12));
        track.x3    = atof(row_track->GetField(13));
        track.y3    = atof(row_track->GetField(14));
        track.z3    = atof(row_track->GetField(15));
        track.x0    = atof(row_track->GetField(16));
        track.y0    = atof(row_track->GetField(17));
        track.z0    = atof(row_track->GetField(18));
        track.xT    = atof(row_track->GetField(19));
        track.yT    = atof(row_track->GetField(20));
        track.zT    = -129.;
        track.xD    = atof(row_track->GetField(21));
        track.yD    = atof(row_track->GetField(22));
        track.zD    = 42.;
        track.px1   = atof(row_track->GetField(23));
        track.py1   = atof(row_track->GetField(24));
        track.pz1   = atof(row_track->GetField(25));
        track.px3   = atof(row_track->GetField(26));
        track.py3   = atof(row_track->GetField(27));
        track.pz3   = atof(row_track->GetField(28));
        track.px0   = atof(row_track->GetField(29));
        track.py0   = atof(row_track->GetField(30));
        track.pz0   = atof(row_track->GetField(31));
        track.pxT   = atof(row_track->GetField(32));
        track.pyT   = atof(row_track->GetField(33));
        track.pzT   = atof(row_track->GetField(34));
        track.pxD   = atof(row_track->GetField(35));
        track.pyD   = atof(row_track->GetField(36));
        track.pzD   = atof(row_track->GetField(37));
        track.roadID = atoi(row_track->GetField(38));
        track.tx_PT  = atof(row_track->GetField(39));
        track.ty_PT  = atof(row_track->GetField(40));
        track.thbend = atof(row_track->GetField(41));
        track.kmstatus = atoi(row_track->GetField(42));
        track.pxv    = dimuon.px1;
        track.pyv    = dimuon.py1;
        track.pzv    = dimuon.pz1;
        int eventID = atoi(row_track->GetField(44));
        delete row_track;
        
        if(eventBank.find(eventID) == eventBank.end())
        {
            continue;
        }

        if(track.charge > 0)
        {
            posTrackBank[eventID] = track;
        }
        else
        {
            negTrackBank[eventID] = track;
        }
    }
    delete res_track;

    //save the data
    for(map<int, Event>::iterator iter = eventBank.begin(); iter != eventBank.end(); ++iter)
    {
        int eventID = iter->first;
        if(eventID != iter->second.eventID) continue;
        if(dimuonBank.find(eventID) == dimuonBank.end()) continue;
        if(posTrackBank.find(eventID) == posTrackBank.end()) continue;
        if(spillBank.find(iter->second.spillID) == spillBank.end()) continue;

        dimuon = dimuonBank[eventID];
        event  = iter->second;
        posTrack = posTrackBank[eventID];
        negTrack = negTrackBank[eventID];
        spill    = spillBank[event.spillID];

        if(mixdata || mcdata) spill.skipflag = true;

        saveTree->Fill(); 
    }

    cout << "Summary: " << endl;
    cout << "DimuonBank:   " << dimuonBank.size() << endl;
    cout << "EventBank:    " << eventBank.size() << endl;
    cout << "SpillBank:    " << spillBank.size() << endl;
    cout << "PosTrackBank: " << posTrackBank.size() << endl;
    cout << "NegTrackBank: " << negTrackBank.size() << endl;

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    cout << "sqlDataReader finishes successfully." << endl;

    return EXIT_SUCCESS;
}
