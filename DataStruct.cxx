#include <cmath>
#include "DataStruct.h"

ClassImp(Event)
ClassImp(Dimuon)
ClassImp(Spill)
ClassImp(Track)

#define SPILLID_MIN_57 303215
#define SPILLID_MAX_57 370100
#define SPILLID_MIN_59 370110
#define SPILLID_MAX_59 388580
#define SPILLID_MIN_61 388611
#define SPILLID_MAX_61 391100
#define SPILLID_MIN_62 394308
#define SPILLID_MAX_62 482573
#define SPILLID_MIN_67 484946
#define SPILLID_MAX_67 676224
#define SPILLID_MIN_70 676498
#define SPILLID_MAX_70 696455
#define SPILLID_MIN_78 0
#define SPILLID_MAX_78 -1

Event::Event() : runID(-1), spillID(-1), eventID(-1), status(-1), MATRIX1(-1), weight(0.)
{
    for(int i = 0; i < 33; ++i) intensity[i] = 0.;
}

bool Event::goodEvent()
{
    return MATRIX1 > 0 && status == 0;
}

float Event::weightedIntensity(float unit, float ped)
{
    double weight[] = {0.000814246430361413, 0.0028662467149288, 0.00597015326639906, 0.0121262946061061,
                       0.0300863195179747, 0.0777262437180552, 0.159446650644417, 0.259932709364831,
                       0.36718876894966, 0.488159093692654, 0.678969311099113, 0.847788074599439, 0.956475273764143,
                       1.,
                       0.989173954042814, 0.897678016090413, 0.767828869998712, 0.647167321489559, 0.533894756174369,
                       0.448848741080746, 0.356435437171761, 0.263693103645649, 0.177964720504253,
                       0.108504562083177, 0.0540099990325891, 0.019218568399343, 0.00308302089003216};

    double sum = 0.;
    double wsum = 0.;
    for(int i = -13; i <= 13; ++i)
    {
        sum += (weight[i+13]*intensity[i+16]);
        wsum += weight[i+13];
    }

    return (sum/wsum - ped)*unit;
}

bool Dimuon::goodDimuon(int polarity)
{
    if(fabs(dx) > 0.3 || fabs(dy - 1.6) > 0.24) return false;
    if(dz < -290. || dz > 200.) return false;
    if(fabs(dpx) > 2.5 || fabs(dpy) > 2.5) return false;
    if(dpz < 36. || dpz > 115.) return false;
    if(x1 < 0. || x1 > 1.) return false;
    if(x2 < 0. || x2 > 1.) return false;
    if(xF < -0.2 || xF > 0.9) return false;
    if(fabs(trackSeparation) > 300.) return false;
    if(chisq_dimuon > 25.) return false;
    if(fabs(costh) > 0.5) return false;
    //if(polarity*px1 < 0. || polarity*px2 > 0.) return false;

    return true;
}

bool Dimuon::targetDimuon()
{
    if(dz < -250. || dz > -20.) return false;
    return true;
}

bool Dimuon::dumpDimuon()
{
    if(dz < 0. || dz > 150.) return false;
    return true;
}

Spill::Spill() : spillID(-1), quality(-1), targetPos(-1), TARGPOS_CONTROL(-1), nEvents(0), nTracks(0), nDimuons(0), skipflag(false)
{}

bool Spill::goodSpill()
{
    //return skipflag || (goodTargetPos() && goodScaler() && goodBeam() && goodBeamDAQ() && goodMagnet() && goodReco());
    return skipflag || (quality == 0);
}

bool Spill::goodTargetPos()
{
    if(targetPos != TARGPOS_CONTROL) return false;
    if(targetPos < 1 || targetPos > 7) return false;

    return true;
}

bool Spill::goodScaler()
{
    if(trigSet == 57 || trigSet == 59)
    {
        if(TSGo < 1E3 || TSGo > 8E3) return false;
        if(acceptedMatrix1 < 1E3 || acceptedMatrix1 > 8E3) return false;
        if(afterInhMatrix1 < 1E3 || afterInhMatrix1 > 3E4) return false;
        if(acceptedMatrix1/afterInhMatrix1 < 0.2 || acceptedMatrix1/afterInhMatrix1 > 0.9) return false;
    }
    else if(trigSet == 61)
    {
        if(TSGo < 1E3 || TSGo > 12E3) return false;
        if(acceptedMatrix1 < 1E3 || acceptedMatrix1 > 12E3) return false;
        if(afterInhMatrix1 < 1E3 || afterInhMatrix1 > 1E6) return false;
        if(acceptedMatrix1/afterInhMatrix1 < 0.0 || acceptedMatrix1/afterInhMatrix1 > 0.9) return false;
    }
    else if(trigSet == 62 || trigSet == 67 || trigSet == 70)
    {
        if(TSGo < 100 || TSGo > 6000) return false;
        if(acceptedMatrix1 < 100 || acceptedMatrix1 > 6000) return false;
        if(afterInhMatrix1 < 100 || afterInhMatrix1 > 10000) return false;
        if(acceptedMatrix1/afterInhMatrix1 < 0.2 || acceptedMatrix1/afterInhMatrix1 > 1.05) return false;
    }
    else
    {
        return false;
    }
    return true;
}

bool Spill::goodBeam()
{
    if(trigSet == 57 || trigSet == 59)
    {
        if(G2SEM < 2E12 || G2SEM > 1E13) return false;
        if(dutyFactor < 15. || dutyFactor > 60.) return false;
    }
    else if(trigSet == 61)
    {
        if(G2SEM < 2E12 || G2SEM > 1E13) return false;
        if(dutyFactor < 15. || dutyFactor > 60.) return false;
    }
    else if(trigSet == 62 || trigSet == 67 || trigSet == 70)
    {
        if(G2SEM < 2E12 || G2SEM > 1E13) return false;
        if(dutyFactor < 10. || dutyFactor > 60.) return false;
    }
    else
    {
        return false;
    }
    return true;
}

bool Spill::goodBeamDAQ()
{
    if(trigSet == 57 || trigSet == 59)
    {
        if(QIESum < 4E10 || QIESum > 1E12) return false;
        if(inhibitSum < 4E9 || inhibitSum > 1E11) return false;
        if(busySum < 4E9 || busySum > 1E11) return false;
    }
    else if(trigSet == 61)
    {
        if(QIESum < 4E10 || QIESum > 1E12) return false;
        if(inhibitSum < 4E9 || inhibitSum > 1E11) return false;
        if(busySum < 4E9 || busySum > 1E11) return false;
    }
    else if(trigSet == 62 || trigSet == 67 || trigSet == 70)
    {
        if(QIESum < 4E10 || QIESum > 1E12) return false;
        if(inhibitSum < 4E9 || inhibitSum > 2E11) return false;
        if(busySum < 4E9 || busySum > 1E11) return false;
    }
    else
    {
        return false;
    }
    return true;
}

bool Spill::goodMagnet()
{
    if(trigSet == 57 || trigSet == 59)
    {
        if(FMAG < 1950 || FMAG > 2050) return false;
        if(KMAG < 1550 || KMAG > 1650) return false;
    }
    else if(trigSet == 61)
    {
        if(FMAG < 200 || FMAG > 500) return false;
        if(KMAG < 1550 || KMAG > 1650) return false;
    }
    else if(trigSet == 62 || trigSet == 67 || trigSet == 70)
    {
        if(FMAG < 1950 || FMAG > 2050) return false;
        if(KMAG < 1550 || KMAG > 1650) return false;
    }
    else
    {
        return false;
    }
    return true;
}

bool Spill::goodReco()
{
    return nTracks > 0;
}

int Spill::triggerSet()
{
    if(spillID >= 303215 && spillID <= 310954) return -1;           //bad QIE RF+00
    //if(spillID >= 394287 && spillID <= 409540) return -1;           //Run3 commissioning
    //if(spillID >= 394287 && spillID <= 414555) return -1;           //LD2 filled with LH2
    //if(spillID >= 416207 && spillID <= 424180) return -1;           //Manual target control
    if(spillID >= 482574 && spillID <= 484924) return -1;           //Magnet flipped
    if(spillID >= 526201 && spillID <= 526364) return -1;           //bad QIE timing in 67
    //if(spillID >= 581369 && spillID <= 582460) return -1;           //KMag off in #67
    //if(spillID >= 684663 && spillID <= 689443) return -1;           //D3p chamber time window shifted

    if(spillID >= SPILLID_MIN_57 && spillID <= SPILLID_MAX_57) return 57;
    if(spillID >= SPILLID_MIN_59 && spillID <= SPILLID_MAX_59) return 59;
    if(spillID >= SPILLID_MIN_61 && spillID <= SPILLID_MAX_61) return 61;
    if(spillID >= SPILLID_MIN_62 && spillID <= SPILLID_MAX_62) return 62;
    if(spillID >= SPILLID_MIN_67 && spillID <= SPILLID_MAX_67) return 67;
    if(spillID >= SPILLID_MIN_70 && spillID <= SPILLID_MAX_70) return 70;
    if(spillID >= SPILLID_MIN_78 && spillID <= SPILLID_MAX_78) return 78;
    return -1;
}

float Spill::QIEUnit()
{
    return G2SEM/(QIESum - 588*360000*pedestal());
}

float Spill::pedestal()
{
    if(spillID >= 448470)
    {
        return 32.37;
    }
    else if(spillID <= 448017)
    {
        return 36.32;
    }
    else
    {
        return 34.35;
    }
}

float Spill::liveG2SEM()
{
    return (QIESum - inhibitSum - busySum)*QIEUnit();
}

void Spill::print()
{
    using namespace std;
    cout << " trigge set:        " << triggerSet() << endl;
    cout << " targetPos:         " << targetPos << "  " << TARGPOS_CONTROL << endl;
    cout << " TSGo:              " << TSGo << endl;
    cout << " acceptedMatrix1:   " << acceptedMatrix1 << endl;
    cout << " afterInhMatrix1:   " << afterInhMatrix1 << endl;
    cout << " NM3ION:            " << NM3ION << endl;
    cout << " G2SEM:             " << G2SEM << endl;
    cout << " QIESum:            " << QIESum << endl;
    cout << " inhibitSum:        " << inhibitSum << endl;
    cout << " busySum:           " << busySum << endl;
    cout << " dutyFactor:        " << dutyFactor << endl;
    cout << " liveG2SEM:         " << liveG2SEM() << endl;
    cout << " liveProton:        " << liveProton << endl;
    cout << " QIEUnit:           " << QIEUnit() << endl;
    cout << " Pedastal:          " << pedestal() << endl;
}

bool Track::goodTrack()
{
    if(nHits < 14) return false;
    if(chisq/(nHits - 5) > 12.) return false;
    if(fabs(yT - 1.6) > 14.) return false;
    if(fabs(yD - 1.6) > 21.) return false;
    if(fabs(pxT) > 4.5) return false;
    if(fabs(pyT) > 3.) return false;
    if(z0 < -400. || z0 > 200.) return false;
    if(roadID == 0) return false;
    if(pz1 < 10 || pz1 > 75) return false;

    return true;
}

bool Track::targetTrack()
{
    if(z0 <= -290. || z0 >= 10.) return false;
    if(chisq_target/chisq_dump > 0.9) return false;
    if(chisq_target > 25.) return false;
    if(xD*xD + (yD - 1.6)*(yD - 1.6) < 16.) return false;

    return true;
}

bool Track::dumpTrack()
{
    if(z0 <= 0. && z0 >= 150.) return false;
    if(chisq_target - chisq_dump < 10.) return false;
    if(targetTrack()) return false;

    return true;
}
