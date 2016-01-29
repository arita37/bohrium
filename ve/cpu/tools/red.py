import numpy as np

def main(nelem, niter):
    res = np.empty([nelem]*2)
    for i in xrange(niter):
        res[:] = np.add.reduce(np.ones([nelem]*3))
        #res[:] = np.add.reduce(np.ones([nelem]*3),axis=2)
    print res

if __name__ == "__main__":
    main(800, 10)
