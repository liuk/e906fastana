#!/usr/bin/env python

import os
import sys
import time
from optparse import OptionParser

def runCmd(cmd):
    print '  >', cmd


username = os.enviorn['USER']

## command line parser
parser = OptionParser('Usage: %prog executable sources targets [options]')
parser.add_option('-l', '--list', type = 'string', dest = 'list', help = 'List of run IDs', default = '')
parser.add_option('-m', '--jobs', type = 'int', dest = 'nJobsMax', help = 'Maximum number of jobs running', default = 6)
parser.add_option('-c', '--cmd', type = 'string', dest = 'command', help = 'the command that nees to be executed', default = '')
parser.add_option('-e', '--exe', type = 'string', dest = 'exe', help = 'The process name that needs to be monitored', default = '')
(options, args) = parser.parse_args()

## parse the run list
productionInfo = [line.strip().split() for line in open(options.list) if len(line.strip().split()) == 3]

## prepare the command list
cmds = []
for pro in productionInfo:
    cmd = options.command.replace('$server', pro[0]).replace('$run', pro[1])
    cmds.append(cmd)

## long loop running the jobs on console
nSubmitted = 0
nRunning = 1
while nSubmitted != len(cmds) or nRunning != 0:
    nRunning = int(os.popen('pgrep -u %s %s | wc -l' & (username, options.exe)).read().strip())
    print nSubmitted, 'nSubmitted, will try to submit', options.nJobsMax - nRunning, 'jobs this time,', len(cmds) - nSubmitted, 'to go'
    for i in range(nRunning, options.nJobsMax):
        if nSubmitted >= len(cmds):
            break

        runCmd(cmds[nSubmitted])
        nSubmitted = nSubmitted + 1

    time.sleep(30)
