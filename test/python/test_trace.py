import numpy as np
import bohrium as bh
from numpytest import numpytest,gen_views,TYPES

class test_trace(numpytest):
    def init(self):
        for v in gen_views(4,32,6,min_ndim=2,dtype="np.uint32"):
            a = {}
            exec(v)
            yield (a,v)

    def test_trace(self,a):
        if bh.check(a[0]):
            cmd = "res = bh.trace(a[0])"
        else:
            cmd = "res = np.trace(a[0])"
        exec(cmd)
        return (res,cmd)

class test_trace_offset(numpytest):
    def init(self):
        for v in gen_views(4,16,6,min_ndim=2,dtype="np.uint32"):
            a = {}
            exec(v)

            for offset in xrange(-a[0].shape[0], a[0].shape[0]+1):
                exec(v)
                self.offset = offset
                yield (a,v)

                exec(v)
                self.offset = -offset
                yield (a,v)

    def test_trace_offset(self,a):
        if bh.check(a[0]):
            cmd = "res = bh.trace(a[0], offset=%d)" % self.offset
        else:
            cmd = "res = np.trace(a[0], offset=%d)" % self.offset
        exec(cmd)
        return (res,cmd)

class test_trace_axis(numpytest):
    def init(self):
        for v in gen_views(4,16,6,min_ndim=2,dtype="np.uint32"):
            a = {}
            self.axis1 = 0
            self.axis2 = 1
            exec(v)
            yield (a,v)

            for axis1 in xrange(a[0].ndim):
                for axis2 in xrange(a[0].ndim):
                    if axis1 == axis2:
                        continue
                    exec(v)
                    self.axis1 = axis1
                    self.axis2 = axis2
                    yield (a,v)

    def test_trace_axis(self,a):
        if bh.check(a[0]):
            cmd = "res = bh.trace(a[0], axis1=%d, axis2=%d)" % (self.axis1, self.axis2)
        else:
            cmd = "res = np.trace(a[0], axis1=%d, axis2=%d)" % (self.axis1, self.axis2)
        exec(cmd)
        return (res,cmd)

class test_trace_axis_and_offset(numpytest):
    def init(self):
        for v in gen_views(4,8,4,min_ndim=2,dtype="np.uint32"):
            a = {}
            exec(v)
            self.axis1 = 0
            self.axis2 = 1
            self.offset = 0

            for offset in xrange(-a[0].shape[0], a[0].shape[0]+1):
                self.offset = offset
                exec(v)
                yield (a,v)

            for offset in xrange(-a[0].shape[0], a[0].shape[0]+1):
                for axis1 in xrange(a[0].ndim):
                    for axis2 in xrange(a[0].ndim):
                        if axis1 == axis2:
                            continue
                        exec(v)
                        self.offset = offset
                        self.axis1 = axis1
                        self.axis2 = axis2
                        yield (a,v)

    def test_trace_axis_and_offset(self,a):
        if bh.check(a[0]):
            cmd = "res = bh.trace(a[0], offset=%d, axis1=%d, axis2=%d)" % (self.offset, self.axis1, self.axis2)
        else:
            cmd = "res = np.trace(a[0], offset=%d, axis1=%d, axis2=%d)" % (self.offset, self.axis1, self.axis2)
        exec(cmd)
        return (res,cmd)
