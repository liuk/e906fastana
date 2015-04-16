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
  	TSQLServer* server = TSQLServer::Connect(Form("mysql://%s", argv[3]), "", "");
  	server->Exec(Form("USE %s", argv[1]));
  	cout << "Reading schema " << argv[1] << " and save to " << argv[2] << endl;

  	char query[2000];
  	sprintf(query, "SELECT dimuonID,runID,spillID,eventID,posTrackID,negTrackID,dx,dy,dz,"
  		"dpx,dpy,dpz,mass,xF,xB,xT,trackSeparation,chisq_dimuon,px1,py1,pz1,px2,py2,pz2 "
  		"FROM kDimuon WHERE mass>0.5 AND mass<10. AND chisq_dimuon<25. AND xB>0. AND xB<1. AND "
  		"xT>0. AND xT<1. AND xF>-1. AND xF<1. AND ABS(dx)<2. AND ABS(dy)<2. AND dz>-300. "
  		"AND dz<-90. AND ABS(dpx)<3. AND ABS(dpy)<3. AND dpz>30. AND dpz<120. AND "
  		"ABS(trackSeparation)<200. ORDER BY spillID");

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
	    dimuon.pT           = sqrt(dimuon.dpx*dimuon.dpx + dimuon.dpy*dimuon.dpy);

	    delete row_dimuon;

	    //spill info and cuts
	    if(event.spillID > spill.spillID) //we have a new spill here
	    {
	    	spill.spillID = event.spillID;
	    	badSpillFlag = false;

	    	//target position
	    	sprintf(query, "SELECT a.targetPos,b.value FROM Spill AS a,Target AS b WHERE a.spillID"
	    		"=%d AND b.spillID=%d AND b.name='TARGPOS_CONTROL'", event.spillID, event.spillID);
	    	TSQLResult* res_spill = server->Query(query);
	    	if(res_spill->GetRowCount() != 1)
	    	{
	    		++nBadSpill_record;
	    		badSpillFlag = true;
	    		event.log("lacks target position info");

	    		delete res_spill;
	    		continue;
	    	}

	    	TSQLRow* row_spill = res_spill->Next();
	    	spill.targetPos       = atoi(row_spill->GetField(0));
	    	spill.TARGPOS_CONTROL = atoi(row_spill->GetField(1));
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

	    	spill.liveG2SEM  = spill.G2SEM*(spill.QIESum - spill.inhibitSum - spill.busySum)/spill.QIESum;
	    	spill.QIEUnit    = spill.G2SEM/spill.QIESum;

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
	    		event.log("spill bad");
	    		spill.print();
	    		continue;
	    	}
	    }
	    if(badSpillFlag) continue;

	    //event info
	    sprintf(query, "SELECT a.`RF+00`,b.MATRIX1 FROM QIE AS a,Event AS b WHERE a.runID=%d "
	    	"AND a.eventID=%d AND b.runID=%d AND b.eventID=%d", event.runID, event.eventID, event.runID, event.eventID);
	    TSQLResult* res_event = server->Query(query);
	    if(res_event->GetRowCount() != 1)
	    {
	    	event.log("lacks QIE/Trigger info");

	    	delete res_event;
	    	continue;
	    }

	    TSQLRow* row_event = res_event->Next();
	    event.intensity = spill.QIEUnit*atof(row_event->GetField(0));
	    event.MATRIX1 = atoi(row_event->GetField(1));
	    delete row_event;
	    delete res_event;

	    //track info
	    sprintf(query, "SELECT numHits,chisq,x1,y1,z1,x3,y3,z3,x0,y0,z0,x_target,y_target,"
	    	"z_target,x_dump,y_dump,z_dump,px1,py1,pz1,px3,py3,pz3,px0,py0,pz0 FROM kTrack "
	    	"WHERE runID=%d AND eventID=%d AND trackID in (%d,%d) ORDER BY charge DESC", 
	    	event.runID, event.eventID, dimuon.posTrackID, dimuon.negTrackID);

	    TSQLResult* res_track = server->Query(query);
	    if(res_track->GetRowCount() != 2)
	    {
	    	event.log("has less than 2 tracks");

	    	delete res_track;
	    	continue;
	    }

	    TSQLRow* row_track = res_track->Next();
	    posTrack.trackID  = dimuon.posTrackID;
	    posTrack.charge   = 1;
	    posTrack.nHits    = atoi(row_track->GetField(0));
	    posTrack.chisq    = atof(row_track->GetField(1));
	    posTrack.x_st1    = atof(row_track->GetField(2));
	    posTrack.y_st1    = atof(row_track->GetField(3));
	    posTrack.z_st1    = atof(row_track->GetField(4));
	    posTrack.x_st3    = atof(row_track->GetField(5));
	    posTrack.y_st3    = atof(row_track->GetField(6));
	    posTrack.z_st3    = atof(row_track->GetField(7));
	    posTrack.x_vertex = atof(row_track->GetField(8));
	    posTrack.y_vertex = atof(row_track->GetField(9));
	    posTrack.z_vertex = atof(row_track->GetField(10));
	    posTrack.x_target = atof(row_track->GetField(11));
	    posTrack.y_target = atof(row_track->GetField(12));
	    posTrack.z_target = atof(row_track->GetField(13));
	    posTrack.x_dump   = atof(row_track->GetField(14));
	    posTrack.y_dump   = atof(row_track->GetField(15));
	    posTrack.z_dump   = atof(row_track->GetField(16));
	    posTrack.px_st1   = atof(row_track->GetField(17));
	    posTrack.py_st1   = atof(row_track->GetField(18));
	    posTrack.pz_st1   = atof(row_track->GetField(19));
	    posTrack.px_st3   = atof(row_track->GetField(20));
	    posTrack.py_st3   = atof(row_track->GetField(21));
	    posTrack.pz_st3   = atof(row_track->GetField(22));
	    posTrack.px_vertex = atof(row_track->GetField(23));
	    posTrack.py_vertex = atof(row_track->GetField(24));
	    posTrack.pz_vertex = atof(row_track->GetField(25));
	    posTrack.px0       = dimuon.px1;
	    posTrack.py0       = dimuon.py1;
	    posTrack.pz0       = dimuon.pz1;
	    delete row_track;

	    row_track = res_track->Next();
	    negTrack.trackID  = dimuon.negTrackID;
	    negTrack.charge   = -1;
	    negTrack.nHits    = atoi(row_track->GetField(0));
	    negTrack.chisq    = atof(row_track->GetField(1));
	    negTrack.x_st1    = atof(row_track->GetField(2));
	    negTrack.y_st1    = atof(row_track->GetField(3));
	    negTrack.z_st1    = atof(row_track->GetField(4));
	    negTrack.x_st3    = atof(row_track->GetField(5));
	    negTrack.y_st3    = atof(row_track->GetField(6));
	    negTrack.z_st3    = atof(row_track->GetField(7));
	    negTrack.x_vertex = atof(row_track->GetField(8));
	    negTrack.y_vertex = atof(row_track->GetField(9));
	    negTrack.z_vertex = atof(row_track->GetField(10));
	    negTrack.x_target = atof(row_track->GetField(11));
	    negTrack.y_target = atof(row_track->GetField(12));
	    negTrack.z_target = atof(row_track->GetField(13));
	    negTrack.x_dump   = atof(row_track->GetField(14));
	    negTrack.y_dump   = atof(row_track->GetField(15));
	    negTrack.z_dump   = atof(row_track->GetField(16));
	    negTrack.px_st1   = atof(row_track->GetField(17));
	    negTrack.py_st1   = atof(row_track->GetField(18));
	    negTrack.pz_st1   = atof(row_track->GetField(19));
	    negTrack.px_st3   = atof(row_track->GetField(20));
	    negTrack.py_st3   = atof(row_track->GetField(21));
	    negTrack.pz_st3   = atof(row_track->GetField(22));
	    negTrack.px_vertex = atof(row_track->GetField(23));
	    negTrack.py_vertex = atof(row_track->GetField(24));
	    negTrack.pz_vertex = atof(row_track->GetField(25));
	    negTrack.px0       = dimuon.px2;
	    negTrack.py0       = dimuon.py2;
	    negTrack.pz0       = dimuon.pz2;
	    delete row_track;	    
	    delete res_track;

	    //Now save everything
	    saveTree->Fill();
	    if(saveTree->GetEntries() % 1000 == 0) saveTree->AutoSave("self");
  	}

  	cout << "sqlReader finished successfully." << endl;
  	cout << nGoodSpill << " good spills, " << nBadSpill_record << " spills have insufficient info in database, " << nBadSpill_quality << " rejected because of bad quality." << endl;

  	saveFile->cd();
  	saveTree->Write();
  	spillTree->Write();
  	saveFile->Close();

	return EXIT_SUCCESS;
}
