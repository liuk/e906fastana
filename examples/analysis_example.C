void analysis_example()
{
    gSystem->Load("libAnaUtil.so");

    // define the output file structure
    Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
    Spill* p_spill = new Spill; Spill& spill = *p_spill;
    Event* p_event = new Event; Event& event = *p_event;
    Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
    Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

    TFile* dataFile = new TFile("data.root", "READ");
    TTree* dataTree = (TTree*)dataFile->Get("save");

    dataTree->SetBranchAddress("dimuon", &p_dimuon);
    dataTree->SetBranchAddress("event", &p_event);
    dataTree->SetBranchAddress("spill", &p_spill);
    dataTree->SetBranchAddress("posTrack", &p_posTrack);
    dataTree->SetBranchAddress("negTrack", &p_negTrack);

    //loop over all the events
    for(int i = 0; i < dataTree->GetEntries(); ++i)
    {
        dataTree->GetEntry(i);

        //temporary cuts
        if(spill.spillID >= 416709 && spill.spillID <= 424180) continue;   //bad taget position
        if(spill.spillID >= 482574 && spill.spillID <= 484924) continue;   //flipped magnets

        //general cuts
        if(!spill.goodSpill()) continue;
        if(!event.goodEvent()) continue;
        if(!dimuon.goodDimuon()) continue;
        if(!dimuon.targetDimuon()) continue;
        if(!(posTrack.goodTrack() && negTrack.goodTrack())) continue;
        if(!(posTrack.targetTrack() && negTrack.targetTrack())) continue;

        //analysis specific cuts
        if(dimuon.mass < 4.2) continue;

        //real analysis here

    }
}
