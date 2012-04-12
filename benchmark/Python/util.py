#!/usr/bin/python
#Benchmarks for DistNumPy.
#This is collection of help functions for the DistNumPy benchmarks.

import numpy as np
import cphvbnumpy
import getopt
import sys
import datetime
import time
from os import environ as env
import os
import multiprocessing
import subprocess
import pickle

class Benchmark:
    """This class should handle the presentation of benchmark results.
       A list of non-optional arguments is exposed through self.argv.
    """
    def __init__(self):
        self.batch_mode = False
        t = datetime.datetime.now()
        date = "%d:%d:%d %d/%d/%d"%(t.hour,t.minute,t.second,t.day,t.month,t.year)
        self.info = {'cphvb':False, 'date':date,'file':os.path.basename(sys.argv[0])}
        self.info['dtype'] = "float64"
        options, self.argv = getopt.gnu_getopt(sys.argv[1:], \
                'p:n:c:s:',\
                ['cphvb=','nnodes=','ncores=','size=','batch','dtype='])

        for opt, arg in options:
            if opt in ('-p', '--cphvb'):
                self.info['cphvb'] = bool(eval(arg))
            if opt in ('-n', '--nnodes'):
                self.info['nnodes'] = int(arg)
            if opt in ('-c', '--ncores'):
                self.info['ncores'] = int(arg)
            if opt in ('--batch'):
                self.batch_mode = True
            if opt in ('--size'):
                #Jobsize use the syntax: dim_size*dim_size fx. 10*20
                self.info['size'] = [int(i) for i in arg.split("*") if len(i)]
            if opt in ('--dtype'):
                self.info['dtype'] = arg

        self.info['nthd'] = multiprocessing.cpu_count()
        self.info['nblocks'] = 16
        try:
            self.info['nthd'] = int(env['CPHVB_NUM_THREADS'])
        except KeyError:
            pass
        try:
            self.info['nblocks'] = int(env['CPHVB_SCORE_NBLOCKS'])
        except KeyError:
            pass
        #Expose variables to the user.
        self.size  = self.info['size']
        self.cphvb = self.info['cphvb']
        self.dtype = eval("np.%s"%self.info['dtype'])

    def start(self):
        cphvbnumpy.flush()
        self.info['totaltime'] = time.time()

    def stop(self):
        cphvbnumpy.flush()
        self.info['totaltime'] = time.time() - self.info['totaltime']

    def pprint(self):
        if self.batch_mode:
            print "%s"%pickle.dumps(self.info)
        else:
            print "%s - cphvb: %s, nthd: %d, nblocks: %d size: %s, total time: %f"%(self.info['file'],self.info['cphvb'],self.info['nthd'],self.info['nblocks'],self.info['size'],self.info['totaltime'])


def do(nthd, nblocks, jobsize, filename, cphvb, savedir, uid):
    try:
        env = os.environ
        env['CPHVB_NUM_THREADS'] = "%d"%nthd
        env['CPHVB_SCORE_NBLOCKS'] = "%d"%nblocks

        """
        taskmask = '0'
        for i in xrange(2,nthd,2):
            taskmask += ",%d"%i
        for i in xrange(1,nthd,2):
            taskmask += ",%d"%i
        """
        p = subprocess.Popen([sys.executable,filename,"--batch","--cphvb=%s"%cphvb, "--size",jobsize],env=env,stdout=subprocess.PIPE)
        (stdoutdata, stderrdata) = p.communicate()
        err = p.wait()
        info = pickle.loads(stdoutdata)
        if not cphvb:
            print "#NumPy   ;     N/A;%10.4f; %s"%(info['totaltime'],info)
        else:
            print "%9.d;%8.d;%10.4f; %s"%(nthd,nblocks, info['totaltime'],info)
        if err:
            raise Exception(err)

        if savedir:
            savefile = os.path.join(savedir, "%s_%d.pkl"%(info['file'],uid))
            while os.path.exists(savefile):
                uid += 1;
                print "file %s exist trying %s"%(savefile,uid)
                savefile = os.path.join(savedir, "%s_%d.pkl"%(info['file'],uid))
            f = open(savefile, 'w')
            pickle.dump(info, f)
        return uid+1
    except KeyboardInterrupt:
        p.terminate()
        raise KeyboardInterrupt


if __name__ == "__main__":
    min_nblocks = 16
    max_nblocks = 16
    savedir = ''
    repeat = 1
    options, remainders = getopt.gnu_getopt(sys.argv[1:], '', ['save=','file=','thd-min=', 'thd-max=', 'jobsize=','repeat=', 'nblocks=', 'nblocks-min=', 'nblocks-max='])
    for opt, arg in options:
        if opt in ('--file'):
            filename = arg
        if opt in ('--save'):
            savedir = arg
        if opt in ('--thd-min'):
            minthd = int(arg)
        if opt in ('--thd-max'):
            maxthd = int(arg)
        if opt in ('--jobsize'):
            jobsize = arg
        if opt in ('--repeat'):
            repeat = int(arg)
        if opt in ('--nblocks'):
            min_nblocks = int(arg)
            max_nblocks = int(arg)
        if opt in ('--nblocks-min'):
            min_nblocks = int(arg)
        if opt in ('--nblocks-max'):
            max_nblocks = int(arg)

    try:
        os.mkdir(savedir)
    except:
        print "Warning the directory '%s' already exist"%savedir

    print "CPU-cores; nblocks; totaltime; info"
    uid = 1#Id
    if minthd == 1:#Lets do the NumPy run.
        for r in xrange(repeat):
            uid = do(1, 1, jobsize, filename, False, savedir,uid)

    for r in xrange(repeat):
        nthd = minthd
        while nthd <= maxthd:
            nblocks = min_nblocks
            while nblocks <= max_nblocks:
                uid = do(nthd, nblocks, jobsize, filename, True, savedir, uid)
                nblocks *= 2
            nthd *= 2
