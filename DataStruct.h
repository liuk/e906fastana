#ifndef _EVENT_STRUCT_H
#define _EVENT_STRUCT_H

#include <iostream>

#include <TObject.h>
#include <TROOT.h>
#include <TString.h>

#define PEDESTAL 36.791

class Event : public TObject
{
public:
    Event();
    bool goodEvent();
    float weightedIntensity(float unit = 1.);

    void log(TString info) { std::cout << "RunID " << runID << ", spillID " << spillID << ", eventID " << eventID << ": " << info << std::endl; }

public:
    int runID;
    int spillID;
    int eventID;
    int status;

    int MATRIX1;
    float weight;

    float occupancy[15];      // 0 - 14 stands for D1 D2 D3 H1 H2 H3 H4 P1 P2 D1L D1R D2L D2R D3L D3R
    float intensity[33];

    int sourceID1, sourceID2;  //source of the eventID for mixing

    ClassDef(Event, 5)
};

class Dimuon : public TObject
{
public:
    bool goodDimuon(int polarity = 1);
    bool targetDimuon();
    bool dumpDimuon();

public:
    int dimuonID;
    int posTrackID, negTrackID;
    float chisq_dimuon;
    float trackSeparation;
    float dx, dy, dz, dpx, dpy, dpz;
    float px1, py1, pz1, px2, py2, pz2;
    float mass, xF, x1, x2, pT, costh, phi;

    ClassDef(Dimuon, 2)
};

class Spill : public TObject
{
public:
    Spill();
    bool goodSpill();
    bool goodTargetPos();
    bool goodScaler();
    bool goodBeam();
    bool goodBeamDAQ();
    bool goodMagnet();
    bool goodReco();

    int triggerSet();

    void print();
    void log(TString info) { std::cout << "SpillID " << spillID << ": " << info << std::endl; }

    float QIEUnit();
    float liveG2SEM();

public:
    float TSGo;
    float acceptedMatrix1;
    float afterInhMatrix1;
    float TSGoBOS;
    float acceptedMatrix1BOS;
    float afterInhMatrix1BOS;
    float NM3ION;
    float G2SEM;
    float QIESum;
    float inhibitSum;
    float busySum;
    float dutyFactor;
    float liveProton;

    int trigSet;
    int spillID;
    int quality;
    int targetPos;
    int TARGPOS_CONTROL;

    int nEvents;
    int nTracks;
    int nDimuons;

    int eventID_min;
    int eventID_max;

    float KMAG;
    float FMAG;

    bool skipflag;     // will be true for MC data or random-mixing data, to overide the spill quality check

    ClassDef(Spill, 6)
};

class Track : public TObject
{
public:
    bool goodTrack();
    bool targetTrack();
    bool dumpTrack();

public:
    int trackID;
    int roadID;
    int charge;
    int nHits, nHitsSt1, nHitsSt2, nHitsSt3;
    int nHitsSt4H, nHitsSt4V;
    int kmstatus;
    float chisq;
    float chisq_dump, chisq_target, chisq_upstream;  // chi squared when force the track to cross z = 40, -130, -480 cm
    float x1, y1, z1;
    float x3, y3, z3;
    float x0, y0, z0;         //vertex position as defined by DCA
    float xT, yT, zT;         //projection at z_target
    float xD, yD, zD;         //projection at z_dump
    float px0, py0, pz0;      //momentum when constrained to (0, 0, z_vertex)
    float px1, py1, pz1;      //momentum at z_st1 (650 cm)
    float px3, py3, pz3;      //momentum at z_st3 (1900 cm)
    float pxT, pyT, pzT;      //momentum when constrained to z_target
    float pxD, pyD, pzD;      //momentum when constrained to z_dump
    float pxv, pyv, pzv;      //momentum when constrained to dimuon vertex
    float tx_PT, ty_PT;       //slope of track segments at prop. tubes
    float thbend;             //bend angle in KMAG

    ClassDef(Track, 6)
};

#endif
