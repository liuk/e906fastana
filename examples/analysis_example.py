#!/usr/bin/env python

import sys
import ROOT

ROOT.gSystem.Load('libAnaUtil.so')

# load the tree
saveFile = ROOT.TFile(sys.argv[1], 'READ')
saveTree = saveFile.Get('save')

# loop over all entries and apply basic analysis cuts
for entry in saveTree:

	# temporary cuts
	if entry.spill.spillID >= 416709 and entry.spill.spillID <= 423255: continue    
	if entry.spill.spillID >= 482574 and entry.spill.spillID <= 484924: continue

    # general cuts
	if not entry.spill.goodSpill(): continue
	if not entry.event.goodEvent(): continue
	if not entry.dimuon.goodDimuon(): continue
	if not entry.dimuon.targetDimuon(): continue
	if not (entry.posTrack.goodTrack() and entry.negTrack.goodTrack()): continue
	if not (entry.posTrack.targetTrack() and entry.negTrack.targetTrack()): continue

	# analysis specific cuts
	if entry.dimuon.mass < 4.2: continue

	print entry.dimuon.mass