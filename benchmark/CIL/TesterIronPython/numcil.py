import clr
import System

# Support for debugging
fullpath = System.IO.Path.GetFullPath(System.IO.Directory.GetCurrentDirectory())
if not System.IO.File.Exists(System.IO.Path.Combine(fullpath, "NumCIL.dll")):
    import sys
    fullpath = System.IO.Path.GetFullPath(System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(),  "..\\..\\..\\bridge\\NumCIL\\NumCIL\\bin\\Release"))
    if not System.IO.File.Exists(System.IO.Path.Combine(fullpath, "NumCIL.dll")):
        System.IO.Path.GetFullPath(System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(),  "..\\..\\..\\bridge\\NumCIL\\NumCIL\\bin\\Debug"))
        if not System.IO.File.Exists(System.IO.Path.Combine(fullpath, "NumCIL.dll")):
            raise Exception("Unable to locate NumCIL.dll")

    #We hack on the path, because the IronPython builder does not copy the reference as it should
    sys.path.insert(0, fullpath)

try:
    clr.AddReference("NumCIL")
except Exception:
    raise Exception("The NumCIL.dll binary was found but could not be loaded.\n" + 
                    "This can be caused by attempting to run a script from an untrusted location, such as a network folder.\n" + 
                    "If you are attempting to run from a network folder, you need to add a config file to the IronPython interpreter (ipy.exe or ipy64.exe):\n\n" +
                    "<?xml version=\"1.0\"?>\n" +
                    "<configuration>\n" + 
                    "  <runtime>\n" + 
                    "    <loadFromRemoteSources enabled=\"true\" />\n" +
                    "  </runtime>\n" +
                    "</configuration>\n")
import NumCIL

pi = System.Math.PI

float32 = System.Single
float64 = System.Double
double = System.Double
single = System.Single

newaxis = NumCIL.Range.NewAxis

def GetNdClass(dtype):
    if dtype==System.Single:
        return NumCIL.Float
    elif dtype==System.Double or dtype==float:
        return NumCIL.Double
    elif isinstance(dtype, NumCIL.Float.NdArray):
        return NumCIL.Float
    elif isinstance(dtype, NumCIL.Double.NdArray):
        return NumCIL.Double
    elif isinstance(dtype, ndarray):
        return dtype.cls
    else:
        raise Exception("There is only support for float and double types")

def ReshapeShape(sh):
    if (isinstance(sh, list)):
        return System.Array[System.Int64](sh)
    else:
        return sh

def SliceToRange(sl):
    if isinstance(sl, int) or isinstance(sl, long) or isinstance(sl, float) or isinstance(sl, System.Int64) or isinstance(sl, System.Int32):
        return NumCIL.Range.El(System.Int64(sl))

    start = sl.start
    stop = sl.stop
    step = sl.step

    if (start == 0 or start == None) and (stop == System.Int32.MaxValue or stop == None) and step == None:
        return NumCIL.Range.All
    elif stop == System.Int32.MaxValue:
        stop = 0

    if start == None:
        start = 0
    if stop == None:
        stop = 0

    if step == None:
        return NumCIL.Range(start, stop)
    else:
        return NumCIL.Range(start, stop, step)


def SlicesToRanges(sl):
    ranges = System.Array.CreateInstance(NumCIL.Range, len(sl))
    for i in range(0, len(sl)):
        if isinstance(sl[i], slice):
            ranges[i] = SliceToRange(sl[i])
        elif isinstance(sl[i], NumCIL.Range):
            ranges[i] = sl[i]
        elif isinstance(sl[i], int) or isinstance(sl[i], long) or isinstance(sl[i], float) or isinstance(sl[i], System.Int64) or isinstance(sl[i], System.Int32):
            ranges[i] = NumCIL.Range.El(System.Int64(sl[i]))
        else:
            raise Exception("Invalid range slice " + str(type(sl[i])))

    return ranges

class ndarray:
    parent = None
    dtype = None
    cls = None
    owncls = None
    collapsedSlicing = True

    def __init__(self, p):
        if isinstance(p, NumCIL.Float.NdArray):
            self.dtype = float32
            self.cls = NumCIL.Float
            self.parent = p
        elif isinstance(p, NumCIL.Double.NdArray):
            self.dtype = float64
            self.cls = NumCIL.Double
            self.parent = p
        elif isinstance(p, NumCIL.Generic.NdArray[float32]):
            self.dtype = float32
            self.cls = NumCIL.Float
            self.parent = NumCIL.Float.NdArray(p)
        elif isinstance(p, NumCIL.Generic.NdArray[float64]):
            self.dtype = float64
            self.cls = NumCIL.Double
            self.parent = NumCIL.Double.NdArray(p)
        elif isinstance(p, ndarray):
            self.dtype = p.dtype
            self.cls = p.cls
            self.parent = p.parent
        else:
            raise Exception("Not yet implemented " + str(type(p)))

    def sum(self):
        return self.parent.Sum()

    def max(self):
        return self.parent.Max()

    def min(self):
        return self.parent.Min()

    def repeat(self, repeats, axis = None):
        return self.owncls(self.parent.Repeat(repeats, axis))

    def getsize(self):
        return self.parent.Shape.Elements
    
    size = property(fget=getsize)

    def reshape(self, t):
        if isinstance(t, tuple):
            return self.owncls(self.parent.Reshape(NumCIL.Shape(System.Array[System.Int64](list(t)))))
        else:
            return self.owncls(self.parent.Reshape(NumCIL.Shape(t)))

    def getShapeTuple(self):
        return tuple([int(x.Length) for x in self.parent.Shape.Dimensions])

    def setShapeTuple(self, t):
        self.parent.Reshape(System.Array[System.Int64](list(t)))

    shape = property(fget = getShapeTuple, fset = setShapeTuple)

    def transpose(self):
        if self.parent.Shape.Dimensions.LongLength < 2:
            return self
        else:
            return self.owncls(self.parent.Transpose())

    T = property(fget=transpose)

    def __len__(self):
        return self.parent.Shape.Dimensions[0].Length

    def __getslice__(self, start, end):
        sl = slice(start, end)
        return self.__getitem__(sl)

    def __getitem__(self, slices):
        rval = None
        if isinstance(slices, list) or isinstance(slices, tuple):
            rval = self.owncls(self.parent.Subview(SlicesToRanges(slices), self.collapsedSlicing))
        elif isinstance(slices, slice) or isinstance(slices, int) or isinstance(slices, long) or isinstance(slices, System.Int64) or isinstance(slices, System.Int32) or isinstance(slices, float):
            rval = self.owncls(self.parent.Subview(System.Array[NumCIL.Range]([SliceToRange(slices)]), self.collapsedSlicing))
        else:
            rval = self.owncls(self.parent.Subview(slices, self.collapsedSlicing))

        #If we get a scalar as result, convert it to a python scalar
        if len(rval.shape) == 1 and rval.shape[0] == 1:
            if self.cls == NumCIL.Double or self.cls == NumCIL.Float:
                return float(rval.parent.Value[0])
            else:
                return int(rval.parent.Value[0])
        else:
            return rval

    def __setitem__(self, slices, value):
        v = value
        if isinstance(v, ndarray):
            v = v.parent
        elif isinstance(v, float) or isinstance(v, int):
            c = getattr(self.cls, "NdArray")
            v = c(self.dtype(v))
        elif isinstance(v, list) or isinstance(v, tuple):
            lst = System.Collections.Generic.List[self.dtype]()
            for a in v:
                lst.Add(self.dtype(a))

            c = getattr(self.cls, "NdArray")
            v = c(lst.ToArray())

        if isinstance(slices, list) or isinstance(slices, tuple):
            self.parent.Subview(SlicesToRanges(slices), self.collapsedSlicing).Set(v)
        elif isinstance(slices, slice) or isinstance(slices, long) or isinstance(slices, int) or isinstance(slices, System.Int64) or isinstance(slices, System.Int32):
            self.parent.Subview(System.Array[NumCIL.Range]([SliceToRange(slices)]), self.collapsedSlicing).Set(v)
        else:
            self.parent.Subview(slices, self.collapsedSlicing).Set(v)


    def __add__(self, other):
        return add(self, other)

    def __radd__(self, other):
        return add(other, self)

    def __iadd__(self, other):
        return add(self, other, self)

    def __sub__(self, other):
        return subtract(self, other)

    def __rsub__(self, other):
        return subtract(other, self)

    def __isub__(self, other):
        return subtract(self, other, self)

    def __div__(self, other):
        return divide(self, other)

    def __rdiv__(self, other):
        return divide(other, self)

    def __idiv__(self, other):
        return divide(self, other, self)

    def __mul__(self, other):
        return multiply(self, other)

    def __rmul__(self, other):
        return multiply(other, self)

    def __imul__(self, other):
        return multiply(self, other, self)

    def __mod__(self, other):
        return mod(self, other)

    def __rmod__(self, other):
        return mod(other, self)

    def __imod__(self, other):
        return mod(self, other, self)

    def __pow__(self, other, modulo = None):
        if modulo != None:
            raise Exception("Modulo version of Pow not supported")

        return pow(self, other)

    def __rpow__(self, other):
        return pow(other, self)

    def __ipow__(self, other, modulo = None):
        if modulo != None:
            raise Exception("Modulo version of Pow not supported")
        return pow(self, other, self)

    def __neg__ (self):
        return self.owncls(self.parent.Negate())

    def __abs__ (self):
        return self.owncls(self.parent.Abs())

    def __str__(self):
        return self.parent.ToString()

ndarray.owncls = ndarray

def empty(shape, dtype=float, order='C', dist=False):
    return ndarray(GetNdClass(dtype).Generate.Empty(ReshapeShape(shape)))

def ones(shape, dtype=float, order='C', dist=False):
    return ndarray(GetNdClass(dtype).Generate.Ones(ReshapeShape(shape)))

def zeroes(shape, dtype=float, order='C', dist=False):
    return ndarray(GetNdClass(dtype).Generate.Zeroes(ReshapeShape(shape)))

def zeros(shape, dtype=float, order='C', dist=False):
    return zeroes(shape, dtype, order, dist)

def arange(shape, dtype=float, order='C', dist=False):
    return ndarray(GetNdClass(dtype).Generate.Arange(ReshapeShape(shape)))

class ufunc:
    op = None
    nin = 2
    nout = 1
    nargs = 3
    name = None

    def __init__(self, op, name, nin = 2, nout = 1, nargs = 3):
        self.op = op
        self.name = name
        self.nin = nin
        self.nout = nout
        self.nargs = nargs

    def aggregate(self, a):
        if not isinstance(a, ndarray):
            raise Exception("Can only aggregate ndarrays")

        cls = a.cls
        f = getattr(cls, self.op)
        return f.Aggregate(a.parent)


    def reduce(self, a, axis=0, dtype=None, out=None, skipna=False, keepdims=False):
        if dtype != None or skipna != False or keepdims != False:
            raise Exception("Arguments dtype, skipna or keepdims are not supported")
        if not isinstance(a, ndarray):
            raise Exception("Can only reduce ndarrays")

        cls = None
        if out != None and isinstance(out, ndarray):
            cls = out.cls
        elif isinstance(a, ndarray):
            cls = a.cls

        if out != None and isinstance(out, ndarray):
            out = out.parent

        f = getattr(cls, self.op)
        return ndarray(f.Reduce(a.parent, axis, out))

    def __call__(self, a, b = None, out = None):

        if self.nin == 2 and b == None:
            raise Exception("The operation " + self.name + " requires 2 input operands")
        elif self.nin == 1 and b != None:
            raise Exception("The operation " + self.name + " accepts only 1 input operand")

        cls = None
        owncls = ndarray
        dtype = float32
        if out != None and isinstance(out, ndarray):
            cls = out.cls
            owncls = out.owncls
            dtype = out.dtype
        elif isinstance(a, ndarray):
            cls = a.cls
            owncls = a.owncls
            dtype = a.dtype
            if isinstance(b, ndarray):
                if a.owncls == matrix or b.owncls == matrix:
                    owncls = matrix
        elif isinstance(b, ndarray):
            cls = b.cls
            owncls = b.owncls
            dtype = b.dtype


        if cls == None:
            raise Exception("Apply not supported for scalars")
        else:
            f = getattr(cls, self.op)
            if isinstance(a, ndarray):
                a = a.parent
            elif isinstance(a, int) or isinstance(a, long) or isinstance(a, float):
                a = dtype(a)
            if isinstance(b, ndarray):
                b = b.parent
            elif isinstance(b, int) or isinstance(b, long) or isinstance(b, float):
                b = dtype(b)
            if out != None and isinstance(out, ndarray):
                out = out.parent

            if self.nin == 2:
                return owncls(f.Apply(a, b, out))
            else:
                return owncls(f.Apply(a, out))

add = ufunc("Add", "add")
subtract = ufunc("Sub", "subtract")
multiply = ufunc("Mul", "multiply")
divide = ufunc("Div", "divide")
mod = ufunc("Mod", "mod")
maximum = ufunc("Max", "maximum")
minimum = ufunc("Min", "minimum")
exp = ufunc("Exp", "exp", nin = 1, nargs = 2)
log = ufunc("Log", "log", nin = 1, nargs = 2)
power = ufunc("Pow", "power")

def array(p):
    return ndarray(p)

class matrix(ndarray):
    collapsedSlicing = False

    def __mul__(self, other):
        if isinstance(other, ndarray):
            return self.owncls(self.parent.MatrixMultiply(other.parent))
        else:
            return self.owncls(self.parent.MatrixMultiply(other))

    def __rmul__(self, other):
        if isinstance(other, ndarray):
            return self.owncls(other.parent.MatrixMultiply(self.parent))
        else:
            return ndarray.__rmul__(self, other)

    def __imul__(self, other):
        if isinstance(other, ndarray):
            self.parent.MatrixMultiply(other.parent, self.parent)
            return self
        else:
            self.parent.MatrixMultiply(other, self.parent)
            return self

    def transpose(self):
        if (len(self.shape) == 1):
            return self.owncls(self.parent.Subview(newaxis, 1))
        else:
            return ndarray.transpose(self)

    T = property(fget=transpose)

matrix.owncls = matrix

def size(x):
    if isinstance(x, ndarray):
        return x.getsize()
    else:
        raise Exception("Can only return size of ndarray")

def concatenate(args, axis=0):
    if args == None or len(args) == 0:
        raise Exception("Invalid args for concatenate")

    cls = None
    dtype = None
    arraycls = None
    for a in args:
        if isinstance(a, ndarray):
            cls = NumCIL.Generic.NdArray[a.dtype] 
            dtype = a.dtype
            arraycls = a.cls

    if cls == None:
        raise Exception("No elements in concatenate were ndarrays")

    arglist = System.Collections.Generic.List[cls]()
    for a in args:
        if isinstance(a, ndarray):
            arglist.Add(a.parent)
        elif isinstance(a, int) or isinstance(a, float):
            arglist.Add(cls(a))
        else:
            raise Exception("All arguments to concatenate must be ndarrays")

    rargs = arglist.ToArray()
    return ndarray(cls.Concatenate(rargs, axis))

def vstack(args):
    cls = None
    for a in args:
        if isinstance(a, ndarray):
            cls = NumCIL.Generic.NdArray[a.dtype] 

    if cls == None:
        raise Exception("No elements in vstack were ndarrays")

    rargs = []
    for a in args:
        if isinstance(a, float) or isinstance(a, int):
            rargs.append(ndarray(cls(a).Subview(NumCIL.Range.NewAxis, 0)))
        elif isinstance(a, ndarray):
            if a.parent.Shape.Dimensions.LongLength == 1:
                rargs.append(ndarray(a.parent.Subview(NumCIL.Range.NewAxis, 0)))
            else:
                rargs.append(a)
        else:
            raise Exception("Unsupporte element in vstack "  + str(type(a)))

    return concatenate(rargs, 0)

def hstack(args):
    return concatenate(args, 1)

def dstack():
    return concatenate(args, 2)

def shape(el):
    if isinstance(el, ndarray):
        return el.shape
    else:
        raise Exception("Don't know the shape of " + str(type(el)))

class random:
    @staticmethod
    def random(shape, dtype=float, order='C', cphvb=False):
        return ndarray(GetNdClass(dtype).Generate.Random(ReshapeShape(shape)))

def activate_cphVB(active = True):
    try:
        clr.AddReference("NumCIL.cphVB")
    except Exception:
        raise Exception("Unable to activate NumCIL.cphVB extensions, make sure that the NumCIL.cphVB.dll is placed in the same folder as NumCIL.dll")
    
    import NumCIL.cphVB

    if active:
        NumCIL.cphVB.Utility.Activate()
    else:
        NumCIL.cphVB.Utility.Deactivate()