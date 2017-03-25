#include <iostream>
#include <map>

#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>

#include "kTracker/SRawEvent.h"
#include "kTracker/SRecEvent.h"

#include "DataStruct.h"

using namespace std;

int main(int argc, char* argv[])
{
    //name of the TTree
    TString treename = argv[3];

    //mode switches
    bool mcdata = false;
    bool mixdata = treename.Contains("mix");
    bool lsdata  = treename.Contains("pp") || treename.Contains("mm");

    // input data structure
    SRecEvent* recEvent = new SRecEvent;

    TFile* dataFile = new TFile(argv[1], "READ");
    TTree* dataTree = (TTree*)dataFile->Get(treename.Data());
    dataTree->SetBranchAddress("recEvent", &recEvent);

    //check if raw event exists
    SRawEvent* rawEvent = NULL;
    if(dataTree->FindBranch("rawEvent") != NULL)
    {
        mcdata = TString(dataTree->FindBranch("rawEvent")->GetClassName()) == "SRawMCEvent";
        rawEvent = mcdata ? (new SRawMCEvent) : (new SRawEvent);
        dataTree->SetBranchAddress("rawEvent", &rawEvent);
    }

    //check if original event info exists, if not, read event input
    SRawEvent* orgEvent = NULL;
    if(dataTree->FindBranch("orgEvent") != NULL)
    {
        orgEvent = mcdata ? (new SRawMCEvent) : (new SRawEvent);
        dataTree->SetBranchAddress("orgEvent", &orgEvent);
    }

    // output data structure
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

    //Initialize spill information accordingly
    spill.skipflag = mcdata || mixdata;
    map<int, Spill> spillBank;
    if(!spill.skipflag)
    {
        TFile* spillFile = new TFile(argv[4]);
        TTree* spillTree = (TTree*)spillFile->Get("save");

        spillTree->SetBranchAddress("spill", &p_spill);
        for(int i = 0; i < spillTree->GetEntries(); ++i)
        {
            spillTree->GetEntry(i);
            if(spill.goodSpill()) spillBank.insert(map<int, Spill>::value_type(spill.spillID, spill));
        }
    }

    //start reading data
    int nDimuons = 0;
    double x_dummy, y_dummy, z_dummy;
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);
        if(i % 1000 == 0)
        {
            cout << "Converting " << i << "/" << dataTree->GetEntries() << endl;
        }

        //general event level info
        event.runID = recEvent->getRunID();
        event.spillID = recEvent->getSpillID();
        event.eventID = recEvent->getEventID();
        event.status = recEvent->getRecStatus();

        //check spill valid first
        if(!spill.skipflag && event.spillID != spill.spillID)
        {
            if(spillBank.find(event.spillID) == spillBank.end()) continue;
            spill = spillBank[event.spillID];
        }

        //further event info
        if(mcdata)
        {
            event.MATRIX1 = rawEvent->isEmuTriggered() ? 1 : -1;
            event.weight = ((SRawMCEvent*)rawEvent)->weight;
            for(int j = -16; j <= 16; ++j) event.intensity[j+16] = rawEvent->getIntensity(j);
            event.intensityP = 0.;
            event.source1 = -1;
            event.source2 = -1;
            if(orgEvent != NULL)
            {

                event.occupancy[0] = orgEvent->getNHitsInD1();
                event.occupancy[1] = orgEvent->getNHitsInD2();
                event.occupancy[2] = orgEvent->getNHitsInD3();
                event.occupancy[3] = orgEvent->getNHitsInH1();
                event.occupancy[4] = orgEvent->getNHitsInH2();
                event.occupancy[5] = orgEvent->getNHitsInH3();
                event.occupancy[6] = orgEvent->getNHitsInH4();
                event.occupancy[7] = orgEvent->getNHitsInP1();
                event.occupancy[8] = orgEvent->getNHitsInP2();
            }
            else
            {
                for(int j = 0; j < 9; ++j) event.occupancy[j] = 0;
            }
        }
        else
        {
            event.MATRIX1 = recEvent->isTriggeredBy(SRawEvent::MATRIX1) ? 1 : -1;
            event.weight = 1.;
            event.source1 = -1;
            event.source2 = -1;
            if(orgEvent != NULL && orgEvent->getEventID() == recEvent->getEventID()) //notice the short circuiting
            {
                for(int j = -16; j <= 16; ++j) event.intensity[j+16] = rawEvent->getIntensity(j);
                event.occupancy[0] = orgEvent->getNHitsInD1();
                event.occupancy[1] = orgEvent->getNHitsInD2();
                event.occupancy[2] = orgEvent->getNHitsInD3();
                event.occupancy[3] = orgEvent->getNHitsInH1();
                event.occupancy[4] = orgEvent->getNHitsInH2();
                event.occupancy[5] = orgEvent->getNHitsInH3();
                event.occupancy[6] = orgEvent->getNHitsInH4();
                event.occupancy[7] = orgEvent->getNHitsInP1();
                event.occupancy[8] = orgEvent->getNHitsInP2();
            }
            else
            {
                for(int j = 0; j < 33; ++j) event.intensity[j] = 0.;
                for(int j = 0; j < 9; ++j)  event.occupancy[j] = 0;
            }
        }

        for(int j = 0; j < recEvent->getNDimuons(); ++j)
        {
            ++nDimuons;
            SRecDimuon recDimuon = recEvent->getDimuon(j);

            dimuon.dimuonID = nDimuons;
            dimuon.posTrackID = recDimuon.trackID_pos;
            dimuon.negTrackID = recDimuon.trackID_neg;
            dimuon.px1 = recDimuon.p_pos.Px();
            dimuon.py1 = recDimuon.p_pos.Py();
            dimuon.pz1 = recDimuon.p_pos.Pz();
            dimuon.px2 = recDimuon.p_neg.Px();
            dimuon.py2 = recDimuon.p_neg.Py();
            dimuon.pz2 = recDimuon.p_neg.Pz();
            dimuon.dx  = recDimuon.vtx.X();
            dimuon.dy  = recDimuon.vtx.Y();
            dimuon.dz  = recDimuon.vtx.Z();
            dimuon.dpx = dimuon.px1 + dimuon.px2;
            dimuon.dpy = dimuon.py1 + dimuon.py2;
            dimuon.dpz = dimuon.pz1 + dimuon.pz2;
            dimuon.mass  = recDimuon.mass;
            dimuon.xF    = recDimuon.xF;
            dimuon.x1    = recDimuon.x1;
            dimuon.x2    = recDimuon.x2;
            dimuon.pT    = recDimuon.pT;
            dimuon.costh = recDimuon.costh;
            dimuon.phi   = recDimuon.phi;
            dimuon.chisq_dimuon    = recDimuon.chisq_kf;
            dimuon.trackSeparation = recDimuon.vtx_pos.Z() - recDimuon.vtx_neg.Z();

            //if(!dimuon.goodDimuon()) continue;
            if(dimuon.mass < 0.5 || dimuon.mass > 10. || dimuon.chisq_dimuon > 50. || dimuon.x1 < 0. || dimuon.x1 > 1.
               || dimuon.x2 < 0. || dimuon.x2 > 1. || dimuon.xF < -1. || dimuon.xF > 1. || fabs(dimuon.dx) > 2.
               || fabs(dimuon.dy) > 2. || dimuon.dz < -300. || dimuon.dz > 300. || fabs(dimuon.dpx) > 3.
               || fabs(dimuon.dpy) > 3. || dimuon.dpz < 30. || dimuon.dpz > 120. || fabs(dimuon.trackSeparation) > 300.) continue;

            SRecTrack recPosTrack = recEvent->getTrack(dimuon.posTrackID);
            posTrack.trackID   = recDimuon.trackID_pos;
            posTrack.charge    = 1;
            posTrack.nHits     = recPosTrack.getNHits();
            posTrack.nHitsSt1  = recPosTrack.getNHitsInStation(1);
            posTrack.nHitsSt2  = recPosTrack.getNHitsInStation(2);
            posTrack.nHitsSt3  = recPosTrack.getNHitsInStation(3);
            posTrack.nHitsSt4H = recPosTrack.getNHitsInPTY();
            posTrack.nHitsSt4V = recPosTrack.getNHitsInPTX();
            posTrack.kmstatus  = recPosTrack.isKalmanFitted();
            posTrack.chisq        = recPosTrack.getChisq();
            posTrack.chisq_dump   = recPosTrack.getChisqDump();
            posTrack.chisq_target = recPosTrack.getChisqTarget();
            posTrack.chisq_upstream = recPosTrack.getChisqUpstream();
            recPosTrack.getExpPositionFast(600., x_dummy, y_dummy);
            posTrack.x1 = x_dummy;
            posTrack.y1 = y_dummy;
            posTrack.z1 = 600.;
            recPosTrack.getExpPositionFast(1910., x_dummy, y_dummy);
            posTrack.x3 = x_dummy;
            posTrack.y3 = y_dummy;
            posTrack.z3 = 1910.;
            posTrack.x0 = recPosTrack.getVtxPar(0);
            posTrack.y0 = recPosTrack.getVtxPar(1);
            posTrack.z0 = recPosTrack.getVtxPar(2);
            posTrack.xT = recPosTrack.getTargetPos().X();
            posTrack.yT = recPosTrack.getTargetPos().Y();
            posTrack.zT = recPosTrack.getTargetPos().Z();
            posTrack.xD = recPosTrack.getDumpPos().X();
            posTrack.yD = recPosTrack.getDumpPos().Y();
            posTrack.zD = recPosTrack.getDumpPos().Z();
            recPosTrack.getExpMomentumFast(600., x_dummy, y_dummy, z_dummy);
            posTrack.px1 = x_dummy;
            posTrack.py1 = y_dummy;
            posTrack.pz1 = z_dummy;
            recPosTrack.getExpMomentumFast(1910., x_dummy, y_dummy, z_dummy);
            posTrack.px3 = x_dummy;
            posTrack.py3 = y_dummy;
            posTrack.pz3 = z_dummy;
            recPosTrack.getMomentumVertex(x_dummy, y_dummy, z_dummy);
            posTrack.px0 = x_dummy;
            posTrack.py0 = y_dummy;
            posTrack.pz0 = z_dummy;
            posTrack.pxT = recPosTrack.getTargetMom().X();
            posTrack.pyT = recPosTrack.getTargetMom().Y();
            posTrack.pzT = recPosTrack.getTargetMom().Z();
            posTrack.pxD = recPosTrack.getDumpMom().X();
            posTrack.pyD = recPosTrack.getDumpMom().Y();
            posTrack.pzD = recPosTrack.getDumpMom().Z();
            posTrack.roadID = recPosTrack.getTriggerRoad();
            posTrack.tx_PT  = recPosTrack.getPTSlopeX();
            posTrack.ty_PT  = recPosTrack.getPTSlopeY();
            posTrack.thbend = atan(posTrack.px3/posTrack.pz3) - atan(posTrack.px1/posTrack.pz1);
            posTrack.kmstatus = recPosTrack.getKalmanStatus();
            posTrack.pxv = recDimuon.p_pos.Px();
            posTrack.pyv = recDimuon.p_pos.Py();
            posTrack.pzv = recDimuon.p_pos.Pz();

            SRecTrack recNegTrack = recEvent->getTrack(dimuon.negTrackID);
            negTrack.trackID   = recDimuon.trackID_neg;
            negTrack.charge    = -1;
            negTrack.nHits     = recNegTrack.getNHits();
            negTrack.nHitsSt1  = recNegTrack.getNHitsInStation(1);
            negTrack.nHitsSt2  = recNegTrack.getNHitsInStation(2);
            negTrack.nHitsSt3  = recNegTrack.getNHitsInStation(3);
            negTrack.nHitsSt4H = recNegTrack.getNHitsInPTY();
            negTrack.nHitsSt4V = recNegTrack.getNHitsInPTX();
            negTrack.kmstatus  = recNegTrack.isKalmanFitted();
            negTrack.chisq        = recNegTrack.getChisq();
            negTrack.chisq_dump   = recNegTrack.getChisqDump();
            negTrack.chisq_target = recNegTrack.getChisqTarget();
            negTrack.chisq_upstream = recNegTrack.getChisqUpstream();
            recNegTrack.getExpPositionFast(600., x_dummy, y_dummy);
            negTrack.x1 = x_dummy;
            negTrack.y1 = y_dummy;
            negTrack.z1 = 600.;
            recNegTrack.getExpPositionFast(1910., x_dummy, y_dummy);
            negTrack.x3 = x_dummy;
            negTrack.y3 = y_dummy;
            negTrack.z3 = 1910.;
            negTrack.x0 = recNegTrack.getVtxPar(0);
            negTrack.y0 = recNegTrack.getVtxPar(1);
            negTrack.z0 = recNegTrack.getVtxPar(2);
            negTrack.xT = recNegTrack.getTargetPos().X();
            negTrack.yT = recNegTrack.getTargetPos().Y();
            negTrack.zT = recNegTrack.getTargetPos().Z();
            negTrack.xD = recNegTrack.getDumpPos().X();
            negTrack.yD = recNegTrack.getDumpPos().Y();
            negTrack.zD = recNegTrack.getDumpPos().Z();
            recNegTrack.getExpMomentumFast(600., x_dummy, y_dummy, z_dummy);
            negTrack.px1 = x_dummy;
            negTrack.py1 = y_dummy;
            negTrack.pz1 = z_dummy;
            recNegTrack.getExpMomentumFast(1910., x_dummy, y_dummy, z_dummy);
            negTrack.px3 = x_dummy;
            negTrack.py3 = y_dummy;
            negTrack.pz3 = z_dummy;
            recNegTrack.getMomentumVertex(x_dummy, y_dummy, z_dummy);
            negTrack.px0 = x_dummy;
            negTrack.py0 = y_dummy;
            negTrack.pz0 = z_dummy;
            negTrack.pxT = recNegTrack.getTargetMom().X();
            negTrack.pyT = recNegTrack.getTargetMom().Y();
            negTrack.pzT = recNegTrack.getTargetMom().Z();
            negTrack.pxD = recNegTrack.getDumpMom().X();
            negTrack.pyD = recNegTrack.getDumpMom().Y();
            negTrack.pzD = recNegTrack.getDumpMom().Z();
            negTrack.roadID = recNegTrack.getTriggerRoad();
            negTrack.tx_PT  = recNegTrack.getPTSlopeX();
            negTrack.ty_PT  = recNegTrack.getPTSlopeY();
            negTrack.thbend = atan(negTrack.px3/negTrack.pz3) - atan(negTrack.px1/negTrack.pz1);
            negTrack.kmstatus = recNegTrack.getKalmanStatus();
            negTrack.pxv = recDimuon.p_neg.Px();
            negTrack.pyv = recDimuon.p_neg.Py();
            negTrack.pzv = recDimuon.p_neg.Pz();

            saveTree->Fill();
        }

        recEvent->clear();
    }

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}
