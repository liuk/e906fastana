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
    void log(TString info) { std::cout << "RunID " << runID << ", spillID " << spillID << ", eventID " << eventID << ": " << info << std::endl; }

public:
    int runID;
    int spillID;
    int eventID;

    int MATRIX1;
    float weight;
    float intensity;

    ClassDef(Event, 2)
};

class Dimuon : public TObject
{
public:
    bool goodDimuon();
    bool targetDimuon();
    bool dumpDimuon();

public:
    int dimuonID;
    int posTrackID, negTrackID;
    float chisq_dimuon;
    float trackSeparation;
    float dx, dy, dz, dpx, dpy, dpz;
    float px1, py1, pz1, px2, py2, pz2;
    float mass, xF, x1, x2, pT;

    ClassDef(Dimuon, 1)
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
    float liveG2SEM;
    float QIEUnit;

    int spillID;
    int targetPos;
    int TARGPOS_CONTROL;

    bool mcflag;

    ClassDef(Spill, 2)
};

class Track : public TObject
{
public:
    bool goodTrack();
    bool targetTrack();
    bool dumpTrack();

public:
    int trackID;
    int triggerID;
    int charge;
    int nHits;
    float chisq;
    float x_st1, y_st1, z_st1;
    float x_st3, y_st3, z_st3;
    float x_vertex, y_vertex, z_vertex;
    float x_target, y_target, z_target;
    float x_dump,   y_dump,   z_dump;
    float px_vertex, py_vertex, pz_vertex;
    float px_st1, py_st1, pz_st1;
    float px_st3, py_st3, pz_st3;
    float px0, py0, pz0;

    ClassDef(Track, 3)
};

#endif
