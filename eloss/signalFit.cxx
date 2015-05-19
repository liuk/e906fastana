void pdfMaker()
{
	gSystem->Load("libRooFit");
	using namespace RooFit;

	const double x1min = 0.35;
	const double x1max = 1.0;
	const double x1bin[] = {x1min, 0.55, 0.61, 0.66, 0.71, 0.79, x1max};
	const int nx1bins = 6;

	const int nTargets = 5;
	const int targets[5] = {1, 3, 5, 6, 7};

	const TString types[4] = {"DY", "JPsi", "Psip", "Comb"};

	TTree* t_dy = (TTree*)_file0->Get("save");
	TTree* t_jpsi = (TTree*)_file1->Get("save");
	TTree* t_psip = (TTree*)_file2->Get("save");
	TTree* t_comb = (TTree*)_file3->Get("save");
	TTree* tree[4] = {t_dy, t_jpsi, t_psip, t_comb};

	TH1D* hists[4][nTargets][nx1bins] = {0};
	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < nTargets; ++j)
		{
			for(int k = 0; k < nx1bins; ++k)
			{
				hists[i][j][k] = new TH1D(Form("hist_%s_%d_%.2f_%.2f", types[i].Data(), targets[j], x1bin[k], x1bin[k+1]), "", 100, 0., 10.);
			}
		}
	}

	Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
	Spill* p_spill = new Spill; Spill& spill = *p_spill;
	Event* p_event = new Event; Event& event = *p_event; 
	Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
	Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;

	for(int i = 0; i < 4; ++i)
	{
		tree[i]->SetBranchAddress("dimuon", &p_dimuon);
		tree[i]->SetBranchAddress("event", &p_event);
		tree[i]->SetBranchAddress("spill", &p_spill);
		tree[i]->SetBranchAddress("posTrack", &p_posTrack);
		tree[i]->SetBranchAddress("negTrack", &p_negTrack);

		for(int j = 0; j < tree[i]->GetEntries(); ++j)
		{
			tree[i]->GetEntry(j);

			//temporary cuts
			if(spill.spillID >= 416709 && spill.spillID <= 423255) continue;   //bad taget position
			if(spill.spillID >= 482574 && spill.spillID <= 484924) continue;   //flipped magnets
			//if(event.intensity > 10000.) continue;

			//general cuts
			if(!event.goodEvent()) continue;
			if(!dimuon.goodDimuon()) continue;
			if(!dimuon.targetDimuon()) continue;
			if(!(posTrack.goodTrack() && negTrack.goodTrack())) continue;
			if(!(posTrack.targetTrack() && negTrack.targetTrack())) continue;
			if(TMath::IsNaN(dimuon.mass)) continue;
			if(dimuon.x1 < x1min || dimuon.x1 > x1max) continue;
			if(dimuon.mass < 0.5 || dimuon.mass > 9.5) continue;

			int ibin = -1;
			for(int k = 0; k < nx1bins; ++k)
			{
				if(dimuon.x1 > x1bin[k] && dimuon.x1 <= x1bin[k+1])
				{
					ibin = k;
					break;
				}
			}

			if(i != 3)
			{
				for(int k = 0; k < 5; ++k) hists[i][k][ibin]->Fill(dimuon.mass, event.weight);
			}
			else
			{
				int itarget = -1;
				for(int k = 0; k < nTargets; ++k)
				{
					if(spill.targetPos == targets[k])
					{
						itarget = k;
						break;
					}
				}

				hists[i][itarget][ibin]->Fill(dimuon.mass);
			}
		}
	}

	TFile* saveFile = new TFile("pdfs.root", "recreate");
	RooRealVar mass("mass", "mass", 0., 10.);
	RooDataHist* data[4][nTargets][nx1bins];
	RooHistPdf* pdf[4][nTargets][nx1bins];
	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < nTargets; ++j)
		{
			for(int k = 0; k < nx1bins; ++k)
			{
				if(k == 0 || k == nx1bins-1) 
				{
					//hists[i][j][k]->Smooth(10);
				}
				else
				{
					//hists[i][j][k]->Smooth(1);
				}

				data[i][j][k] = new RooDataHist(Form("data_%s_%d_%.2f_%.2f", types[i].Data(), targets[j], x1bin[k], x1bin[k+1]), 
					Form("data_%s_%d_%.2f_%.2f", types[i].Data(), targets[j], x1bin[k], x1bin[k+1]), mass, Import(*hists[i][j][k]));
				pdf[i][j][k] = new RooHistPdf(Form("pdf_%s_%d_%.2f_%.2f", types[i].Data(), targets[j], x1bin[k], x1bin[k+1]),
					Form("pdf_%s_%d_%.2f_%.2f", types[i].Data(), targets[j], x1bin[k], x1bin[k+1]), mass, *data[i][j][k]);

				hists[i][j][k]->Write();
				pdf[i][j][k]->Write();
			}
		}
	}

	saveFile->Close();
}

void signalFit(int target, double x1_min = -1., double x1_max = 2., TString datain = "data/data_run2.root")
{
	gSystem->Load("libRooFit");
	using namespace RooFit;

	RooRealVar mass("mass", "mass", 0., 10.);
	RooDataSet data("data", "data", RooArgSet(mass));

	//Read data
	Dimuon* p_dimuon = new Dimuon; Dimuon& dimuon = *p_dimuon;
	Spill* p_spill = new Spill; Spill& spill = *p_spill;
	Event* p_event = new Event; Event& event = *p_event; 
	Track* p_posTrack = new Track; Track& posTrack = *p_posTrack;
	Track* p_negTrack = new Track; Track& negTrack = *p_negTrack;
	
	TFile* dataFile = new TFile(datain.Data(), "READ");
	TTree* dataTree = (TTree*)dataFile->Get("save");

	dataTree->SetBranchAddress("dimuon", &p_dimuon);
	dataTree->SetBranchAddress("event", &p_event);
	dataTree->SetBranchAddress("spill", &p_spill);
	dataTree->SetBranchAddress("posTrack", &p_posTrack);
	dataTree->SetBranchAddress("negTrack", &p_negTrack);

	TH1D* hist = new TH1D("hist", "hist", 100, 0., 10.);
	for(int i = 0; i < dataTree->GetEntries(); ++i)
	{
		dataTree->GetEntry(i);

		//temporary cuts
		if(spill.spillID >= 416709 && spill.spillID <= 423255) continue;   //bad taget position
		if(spill.spillID >= 482574 && spill.spillID <= 484924) continue;   //flipped magnets
		//if(event.intensity > 10000.) continue;

		//general cuts
		if(spill.targetPos != target) continue;
		if(!spill.goodSpill()) continue;
		if(!event.goodEvent()) continue;
		if(!dimuon.goodDimuon()) continue;
		if(!dimuon.targetDimuon()) continue;
		if(!(posTrack.goodTrack() && negTrack.goodTrack())) continue;
		if(!(posTrack.targetTrack() && negTrack.targetTrack())) continue;
		if(dimuon.x1 < x1_min || dimuon.x1 > x1_max) continue;
		if(TMath::IsNaN(dimuon.mass)) continue;
		if(dimuon.mass < 1.5 || dimuon.mass > 8.0) continue;
		
		mass = dimuon.mass;
		data.add(RooArgSet(mass));
		hist->Fill(dimuon.mass);
	}
	RooDataHist dataBinned("dataBinned", "dataBinned", mass, Import(*hist));

	TFile* pdfFile = new TFile("pdfs.root", "READ");
	RooAbsPdf* pdfDY = (RooAbsPdf*)pdfFile->Get(Form("pdf_DY_%d_%.2f_%.2f", target, x1_min, x1_max));
	RooAbsPdf* pdfJPsi = (RooAbsPdf*)pdfFile->Get(Form("pdf_JPsi_%d_%.2f_%.2f", target, x1_min, x1_max));
	RooAbsPdf* pdfPsip = (RooAbsPdf*)pdfFile->Get(Form("pdf_Psip_%d_%.2f_%.2f", target, x1_min, x1_max));
	RooAbsPdf* pdfComb = (RooAbsPdf*)pdfFile->Get(Form("pdf_Comb_%d_%.2f_%.2f", target, x1_min, x1_max));

	RooRealVar nDY("nDY", "Number of DY", 900., 0., 3500.);
	RooRealVar nJPsi("nJPsi", "Number of JPsi", 80., 0., 2500.);
	RooRealVar nPsip("nPsip", "Number of Psip", 120., 0., 2000.);
	RooRealVar nComb("nComb", "Number of Comb", 1000., 0., 2000.);

	RooAddPdf model("model", "model", RooArgList(*pdfDY, *pdfJPsi, *pdfPsip, *pdfComb), RooArgList(nDY, nJPsi, nPsip, nComb));
	model.fitTo(data);

	RooPlot* frame = mass.frame();
	data.plotOn(frame, Binning(40));
	model.plotOn(frame, LineColor(kBlue), LineWidth(2));
	model.plotOn(frame, Components(*pdfDY), LineColor(kMagenta), LineWidth(2));
	model.plotOn(frame, Components(*pdfJPsi), LineColor(kRed), LineWidth(2));
	model.plotOn(frame, Components(*pdfPsip), LineColor(kPink), LineWidth(2));
	model.plotOn(frame, Components(*pdfComb), LineColor(kBlack), LineWidth(2));

	frame->SetTitle(Form("TargetPos: %d, x1: %.2f - %.2f", target, x1_min, x1_max));
	frame->GetXaxis()->SetTitle("Mass (GeV)");
	frame->GetXaxis()->CenterTitle();
	frame->GetYaxis()->SetTitle("Events / 100 MeV");
	frame->GetYaxis()->CenterTitle();

	TCanvas* c1 = new TCanvas();
	frame->Draw();
	c1->SaveAs(Form("fitres_%d_x1_%.2f_%.2f.eps", target, x1_min, x1_max));
	cout << frame->chiSquare(4) << endl;

	ofstream fout(Form("fitres_%d_x1_%.2f_%.2f.txt", target, x1_min, x1_max));
	fout << "DY   " << nDY.getVal() << " +/- " << nDY.getError() << endl;
	fout << "JPsi " << nJPsi.getVal() << " +/- " << nJPsi.getError() << endl;
	fout << "Psip " << nPsip.getVal() << " +/- " << nPsip.getError() << endl;
	fout << "Comb " << nComb.getVal() << " +/- " << nComb.getError() << endl;
	fout.close();
}