import numpy as np
import cphvbnumpy
import util

B = util.Benchmark()
W = B.size[0]
H = B.size[1]
iterations = B.size[2]

full = np.empty((W+2,H+2), dtype=np.double, dist=B.cphvb)
work = np.empty((W,H), dtype=np.double, dist=B.cphvb)
full[:]    = 0.0
full[:,0]  = -273.15
full[:,-1] = -273.15
full[0,:]  =  40.0
full[-1,:] = -273.13

B.start()
for i in xrange(iterations):
  work[:] = full[1:-1, 1:-1]
  work += full[1:-1, 0:-2]
  work += full[1:-1, 2:  ]
  work += full[0:-2, 1:-1]
  work += full[2:  , 1:-1]
  work *= 0.2
  full[1:-1, 1:-1] = work
B.stop()

B.pprint()