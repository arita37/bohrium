import numpy as np
import cphvbnp as cnp
import linalg as cla
import numpy.linalg as nla

def jacobi(A, b, tol=0.0005):
    # solve Ax=b via the Jacobi method
    x = cnp.ones(np.shape(b), dtype=b.dtype, cphvb=b.cphvb)
    D = 1/cnp.diag(A)
    R = cnp.diag(cnp.diag(A)) - A
    T = D[:,np.newaxis]*R
    C = D*b
    error = tol + 1
    while error > tol:
        xo = x
        x = np.add.reduce(T*x,-1) + C
        error = nla.norm(x-xo)
    return x
