#!/usr/bin/env python

import sys
import ROOT

ROOT.gSystem.Load('libAnaUtil.so')

# load the tree
saveFile = ROOT.TFile(sys.argv[1], 'READ')
saveTree = saveFile.Get('save')

# loop over all entries and apply basic analysis cuts
for entry in saveTree:

    # general cuts
	if not entry.spill.goodSpill(): continue
	if not entry.event.goodEvent(): continue
	if not entry.dimuon.goodDimuon(): continue
	if not entry.dimuon.targetDimuon(): continue
	if entry.posTrack.roadID*entry.negTrack.roadID > 0: continue
	if not (entry.posTrack.goodTrack() and entry.negTrack.goodTrack()): continue
	if not (entry.posTrack.targetTrack() and entry.negTrack.targetTrack()): continue

	# analysis specific cuts
	if entry.dimuon.mass < 4.2: continue

	print entry.dimuon.mass
