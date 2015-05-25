#include <iostream>

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
    // input data structure
    SRecEvent* recEvent = new SRecEvent;

    TFile* dataFile = new TFile(argv[2], "READ");
    TTree* dataTree = (TTree*)dataFile->Get(argv[1]);

    dataTree->SetBranchAddress("recEvent", &recEvent);
    
    //check if raw event exists
    bool mcEvent = false;
    bool dataEvent = false;
    SRawEvent* rawEvent;
    SRawMCEvent* rawMCEvent;
    if(dataTree->FindBranch("rawEvent") != NULL)
    {
        if(TString(dataTree->FindBranch("rawEvent")->GetClassName()) == "SRawMCEvent")
        {
            rawMCEvent = new SRawMCEvent;
            dataTree->SetBranchAddress("rawEvent", &rawMCEvent);
            mcEvent = true;
        }
        else
        {
            rawEvent = new SRawEvent;
            dataTree->SetBranchAddress("rawEvent", &rawEvent);
            dataEvent = true;
        }
    }

    // output data structure
    Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    Event* p_event = new Event; Event& event = *p_event; 
    Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
    Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

    TFile* saveFile = new TFile(argv[3], "recreate");
    TTree* saveTree = new TTree("save", "save");

    saveTree->Branch("dimuon", &p_dimuon, 256000, 99);
    saveTree->Branch("event", &p_event, 256000, 99);
    saveTree->Branch("spill", &p_spill, 256000, 99);
    saveTree->Branch("posTrack", &p_posTrack, 256000, 99);
    saveTree->Branch("negTrack", &p_negTrack, 256000, 99);

    int nDimuons = 0;
    double x_dummy, y_dummy, z_dummy;
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);
        if(i % 1000 == 0)
        {
            cout << "Converting " << i << "/" << dataTree->GetEntries() << endl;
        }

        event.runID = recEvent->getRunID();
        event.spillID = recEvent->getSpillID();
        event.eventID = recEvent->getEventID();
        event.intensity = dataEvent ? rawEvent->getIntensity() : 0;
        if(mcEvent) 
        {
            event.weight = rawMCEvent->weight;
            event.MATRIX1 = rawMCEvent->isEmuTriggered() ? 1 : -1;
        }
        else if(dataEvent)
        {
            event.MATRIX1 = recEvent->isTriggeredBy(SRawEvent::MATRIX1) ? 1 : -1;
        }
        else
        {
            event.MATRIX1 = 1;
        }

        spill.spillID = recEvent->getSpillID();
        spill.targetPos = recEvent->getTargetPos();
        spill.TARGPOS_CONTROL = recEvent->getTargetPos();

        for(int j = 0; j < recEvent->getNDimuons(); ++j)
        {
            ++nDimuons;
            SRecDimuon recDimuon = recEvent->getDimuon(j);

            dimuon.dimuonID = nDimuons;
            dimuon.posTrackID = recDimuon.trackID_pos;
            dimuon.negTrackID = recDimuon.trackID_neg;
            dimuon.chisq_dimuon = recDimuon.chisq_kf;
            dimuon.trackSeparation = recDimuon.vtx_pos.Z() - recDimuon.vtx_neg.Z();
            dimuon.mass = recDimuon.mass;
            dimuon.xF = recDimuon.xF;
            dimuon.x1 = recDimuon.x1;
            dimuon.x2 = recDimuon.x2;
            dimuon.pT = recDimuon.pT;
        
            dimuon.px1 = recDimuon.p_pos.Px();
            dimuon.py1 = recDimuon.p_pos.Py();
            dimuon.pz1 = recDimuon.p_pos.Pz();
            dimuon.px2 = recDimuon.p_neg.Px();
            dimuon.py2 = recDimuon.p_neg.Py();
            dimuon.pz2 = recDimuon.p_neg.Pz();

            dimuon.dx = recDimuon.vtx.X();
            dimuon.dy = recDimuon.vtx.Y();
            dimuon.dz = recDimuon.vtx.Z();
            dimuon.dpx = dimuon.px1 + dimuon.px2;
            dimuon.dpy = dimuon.py1 + dimuon.py2;
            dimuon.dpz = dimuon.pz1 + dimuon.pz2;

            if(!dimuon.goodDimuon()) continue;

            SRecTrack recPosTrack = recEvent->getTrack(dimuon.posTrackID);
            posTrack.trackID = recDimuon.trackID_pos;
            posTrack.triggerID = recPosTrack.getTriggerRoad();
            posTrack.charge = 1;
            posTrack.nHits = recPosTrack.getNHits();
            posTrack.chisq = recPosTrack.getChisq();
            posTrack.px0 = recDimuon.p_pos.Px();
            posTrack.py0 = recDimuon.p_pos.Py();
            posTrack.pz0 = recDimuon.p_pos.Pz();
            posTrack.x_vertex = recPosTrack.getVtxPar(0);
            posTrack.y_vertex = recPosTrack.getVtxPar(1);
            posTrack.z_vertex = recPosTrack.getVtxPar(2);
            posTrack.x_target = recPosTrack.getTargetPos().X();
            posTrack.y_target = recPosTrack.getTargetPos().Y();
            posTrack.z_target = recPosTrack.getTargetPos().Z();
            posTrack.x_dump = recPosTrack.getDumpPos().X();
            posTrack.y_dump = recPosTrack.getDumpPos().Y();
            posTrack.z_dump = recPosTrack.getDumpPos().Z();
            recPosTrack.getMomentumVertex(x_dummy, y_dummy, z_dummy);
            posTrack.px_vertex = x_dummy;
            posTrack.py_vertex = y_dummy;
            posTrack.pz_vertex = z_dummy;
            recPosTrack.getExpMomentumFast(650., x_dummy, y_dummy, z_dummy);
            posTrack.px_st1 = x_dummy;
            posTrack.py_st1 = y_dummy;
            posTrack.pz_st1 = z_dummy;
            recPosTrack.getExpMomentumFast(1900., x_dummy, y_dummy, z_dummy);
            posTrack.px_st3 = x_dummy;
            posTrack.py_st3 = y_dummy;
            posTrack.pz_st3 = z_dummy;          
            recPosTrack.getExpPositionFast(650., x_dummy, y_dummy);
            posTrack.x_st1 = x_dummy;
            posTrack.y_st1 = y_dummy;
            posTrack.z_st1 = 650.;
            recPosTrack.getExpPositionFast(1900., x_dummy, y_dummy);
            posTrack.x_st3 = x_dummy;
            posTrack.y_st3 = y_dummy;
            posTrack.z_st1 = 1900.;

            SRecTrack recNegTrack = recEvent->getTrack(dimuon.negTrackID);
            negTrack.trackID = recDimuon.trackID_neg;
            negTrack.triggerID = recNegTrack.getTriggerRoad();
            negTrack.charge = -1;
            negTrack.nHits = recNegTrack.getNHits();
            negTrack.chisq = recNegTrack.getChisq();
            negTrack.px0 = recDimuon.p_neg.Px();
            negTrack.py0 = recDimuon.p_neg.Py();
            negTrack.pz0 = recDimuon.p_neg.Pz();
            negTrack.x_vertex = recNegTrack.getVtxPar(0);
            negTrack.y_vertex = recNegTrack.getVtxPar(1);
            negTrack.z_vertex = recNegTrack.getVtxPar(2);
            negTrack.x_target = recNegTrack.getTargetPos().X();
            negTrack.y_target = recNegTrack.getTargetPos().Y();
            negTrack.z_target = recNegTrack.getTargetPos().Z();
            negTrack.x_dump = recNegTrack.getDumpPos().X();
            negTrack.y_dump = recNegTrack.getDumpPos().Y();
            negTrack.z_dump = recNegTrack.getDumpPos().Z();
            recNegTrack.getMomentumVertex(x_dummy, y_dummy, z_dummy);
            negTrack.px_vertex = x_dummy;
            negTrack.py_vertex = y_dummy;
            negTrack.pz_vertex = z_dummy;
            recNegTrack.getExpMomentumFast(650., x_dummy, y_dummy, z_dummy);
            negTrack.px_st1 = x_dummy;
            negTrack.py_st1 = y_dummy;
            negTrack.pz_st1 = z_dummy;
            recNegTrack.getExpMomentumFast(1900., x_dummy, y_dummy, z_dummy);
            negTrack.px_st3 = x_dummy;
            negTrack.py_st3 = y_dummy;
            negTrack.pz_st3 = z_dummy;          
            recNegTrack.getExpPositionFast(650., x_dummy, y_dummy);
            negTrack.x_st1 = x_dummy;
            negTrack.y_st1 = y_dummy;
            negTrack.z_st1 = 650.;
            recNegTrack.getExpPositionFast(1900., x_dummy, y_dummy);
            negTrack.x_st3 = x_dummy;
            negTrack.y_st3 = y_dummy;
            negTrack.z_st3 = 1900.;

            saveTree->Fill();
        }   
    }

    saveFile->cd();
    saveTree->Write();
    saveFile->Close();

    return EXIT_SUCCESS;
}