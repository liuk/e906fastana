#ifndef _EVENT_STRUCT_H
#define _EVENT_STRUCT_H

#include <iostream>

#include <TObject.h>
#include <TROOT.h>
#include <TString.h>

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
    float intensity[33];

    ClassDef(Event, 4)
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
    bool goodTSGo();
    bool goodScaler();
    bool goodBeam();
    bool goodBeamDAQ();

    int triggerSet();

    void print();
    void log(TString info) { std::cout << "SpillID " << spillID << ": " << info << std::endl; }

    float QIEUnit();
    float liveG2SEM();

public:
    float TSGo;
    float acceptedMatrix1;
    float afterInhMatrix1;
    float NM3ION;
    float G2SEM;
    float QIESum;
    float inhibitSum;
    float busySum;
    float dutyFactor;
    float liveProton;

    int spillID;
    int quality;
    int targetPos;
    int TARGPOS_CONTROL;

    int nEvents;
    int nTracks;
    int nDimuons;

    float KMAG;
    int MATRIX3Prescale;

    bool skipflag;     // will be true for MC data or random-mixing data

    ClassDef(Spill, 5)
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

    ClassDef(Track, 5)
};

#endif
