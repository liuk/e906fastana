#include <cmath>
#include "DataStruct.h"

ClassImp(Event)
ClassImp(Dimuon)
ClassImp(Spill)
ClassImp(Track)

#define SPILLID_MIN_57 303278
#define SPILLID_MAX_57 369968
#define SPILLID_MIN_59 370239
#define SPILLID_MAX_59 388470
#define SPILLID_MIN_62 409563
#define SPILLID_MAX_62 484924

Event::Event() : runID(-1), spillID(-1), eventID(-1), MATRIX1(-1), weight(1.), intensity(0.)
{}

bool Event::goodEvent()
{
    return MATRIX1 > 0;
}

bool Dimuon::goodDimuon()
{
    if(fabs(dx) > 2. || fabs(dy) > 2.) return false;
    if(dz < -300. || dz > 150.) return false;
    if(fabs(dpx) > 3. || fabs(dpy) > 3.) return false;
    if(dpz < 30. || dpz > 120.) return false;
    if(x1 < 0. || x1 > 1.) return false;
    if(x2 < 0. || x2 > 1.) return false;
    if(xF < -1. || xF > 1.) return false;
    if(fabs(trackSeparation) > 200.) return false;
    if(chisq_dimuon > 25.) return false;
    if(px1 < 0. || px2 > 0.) return false;

    return true;
}

bool Dimuon::targetDimuon()
{
    if(dz > -90. || dz < -300.) return false;
    return true;
}

bool Dimuon::dumpDimuon()
{
    if(dz < 0. || dz > 150.) return false;
    return true;
}

Spill::Spill() : spillID(-1), targetPos(-1), TARGPOS_CONTROL(-1), mcflag(false)
{}

bool Spill::goodSpill()
{
    return mcflag || (goodTargetPos() && goodTSGo() && goodScaler() && goodBeam() && goodBeamDAQ());
}

bool Spill::goodTargetPos()
{
    if(targetPos != TARGPOS_CONTROL) return false;
    if(targetPos < 1 || targetPos > 7) return false;

    return true;
}

bool Spill::goodTSGo()
{
    int trigSet = triggerSet();
    if(trigSet == 57 || trigSet == 59)
    {
        if(TSGo < 1E3 || TSGo > 8E3) return false;
    }
    else if(trigSet == 62)
    {
        if(TSGo < 1E2 || TSGo > 6E3) return false;
    }
    else
    {
        return false;
    }

    return true;
}

bool Spill::goodScaler()
{
    int trigSet = triggerSet();
    if(trigSet == 57 || trigSet == 59)
    {
        if(acceptedMatrix1 < 1E3 || acceptedMatrix1 > 8E3) return false;
        if(afterInhMatrix1 < 1E3 || afterInhMatrix1 > 3E4) return false;
        if(acceptedMatrix1/afterInhMatrix1 < 0.2 || acceptedMatrix1/afterInhMatrix1 > 0.9) return false;
    }
    else if(trigSet == 62)
    {
        if(acceptedMatrix1 < 1E2 || acceptedMatrix1 > 6E3) return false;
        if(afterInhMatrix1 < 1E2 || afterInhMatrix1 > 1E4) return false;
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
    int trigSet = triggerSet();
    if(trigSet == 57 || trigSet == 59)
    {
        //if(NM3ION < 2E12 || NM3ION > 1E13) return false;
        if(G2SEM < 2E12 || G2SEM > 1E13) return false;
        if(dutyFactor < 15. || dutyFactor > 60.) return false;
    }
    else if(trigSet == 62)
    {
        //if(NM3ION < 2E12 || NM3ION > 1E13) return false;
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
    int trigSet = triggerSet();
    if(trigSet == 57 || trigSet == 59)
    {
        if(QIESum < 4E10 || QIESum > 1E12) return false;
        if(inhibitSum < 4E9 || inhibitSum > 1E11) return false;
        if(busySum < 4E9 || busySum > 1E11) return false;
    }
    else if(trigSet == 62)
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

int Spill::triggerSet()
{
    if(spillID >= SPILLID_MIN_57 && spillID <= SPILLID_MAX_57) return 57;
    if(spillID >= SPILLID_MIN_59 && spillID <= SPILLID_MAX_59) return 59;
    if(spillID >= SPILLID_MIN_62 && spillID <= SPILLID_MAX_62) return 62;
    return -1;
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
    cout << " liveG2SEM:         " << liveG2SEM << endl;
}

bool Track::goodTrack()
{
    if(nHits <= 14) return false;
    if(chisq/(nHits - 5) > 6.) return false;
    if(nHits < 18 && pz_st1 < 18.) return false;

    return true;
}

bool Track::targetTrack()
{
    if(z_vertex <= -300. || z_vertex >= 0.) return false;
    if(sqrt(x_dump*x_dump + y_dump*y_dump) - sqrt(x_target*x_target + y_target*y_target) < 
        9.4431 - 0.356141*pz_vertex + 0.00566071*pz_vertex*pz_vertex - 3.05556E-5*pz_vertex*pz_vertex*pz_vertex) return false;
    //if(sqrt(x_dump*x_dump + y_dump*y_dump) - sqrt(x_target*x_target + y_target*y_target) < 
    //  9.4431 - 0.356141*pz0 + 0.00566071*pz0*pz0 - 3.05556E-5*pz0*pz0*pz0) return false;

    return true;
}

bool Track::dumpTrack()
{
    if(z_vertex <= 0. && z_vertex >= 150.) return false;
    if(sqrt(x_target*x_target + y_target*y_target) - sqrt(x_dump*x_dump + y_dump*y_dump) <
        9.4431 - 0.356141*pz_vertex + 0.00566071*pz_vertex*pz_vertex - 3.05556E-5*pz_vertex*pz_vertex*pz_vertex) return false;

    return true;
}

