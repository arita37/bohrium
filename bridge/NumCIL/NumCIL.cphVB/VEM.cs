﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NumCIL.Generic;
using System.Runtime.InteropServices;

namespace NumCIL.cphVB
{
    /// <summary>
    /// Basic wrapper implementation of a cphvb VEM
    /// </summary>
    public class VEM : IDisposable
    {
        /// <summary>
        /// Singleton VEM instance
        /// </summary>
        private static VEM _instance = null;

        /// <summary>
        /// Lock object for ensuring single threaded access to the VEM
        /// </summary>
        private object m_executelock = new object();

        /// <summary>
        /// Lock object for protecting the release queue
        /// </summary>
        private object m_releaselock = new object();

        /// <summary>
        /// A counter for all allocated views
        /// </summary>
        private static long m_allocatedviews = 0;

        /// <summary>
        /// Gets the number of allocated views
        /// </summary>
        public static long AllocatedViews { get { return m_allocatedviews; } }

        /// <summary>
        /// ID for the user-defined reduce operation
        /// </summary>
        private long m_reduceFunctionId = 0;

        /// <summary>
        /// ID for the user-defined random operation
        /// </summary>
        private long m_randomFunctionId = 0;

        /// <summary>
        /// Lookup table for all created userfunc structures
        /// </summary>
        private Dictionary<IntPtr, GCHandle> m_allocatedUserfuncs = new Dictionary<IntPtr, GCHandle>();

        /// <summary>
        /// Accessor for singleton VEM instance
        /// </summary>
        public static VEM Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new VEM();

                return _instance;
            }
        }

        /// <summary>
        /// A reference to the cphVB component for "self" aka the bridge
        /// </summary>
        private PInvoke.cphvb_com m_component;
        /// <summary>
        /// A reference to the chpVB VEM
        /// </summary>
        private PInvoke.cphvb_com[] m_childs;

        /// <summary>
        /// A list of cleanups not yet performed
        /// </summary>
        private List<IInstruction> m_cleanups = new List<IInstruction>();

        /// <summary>
        /// Constructs a new VEM
        /// </summary>
        public VEM()
        {
            m_component = PInvoke.cphvb_com_setup();
            PInvoke.cphvb_error e = PInvoke.cphvb_com_children(m_component, out m_childs);
            if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                throw new cphVBException(e);

            if (m_childs.Length > 1)
                throw new cphVBException(string.Format("Unexpected number of child nodes: {0}", m_childs.Length));

            e = m_childs[0].init(ref m_childs[0]);
            if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                throw new cphVBException(e);

            try
            {
                e = m_childs[0].reg_func(null, "cphvb_reduce", ref m_reduceFunctionId);
                if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                    throw new cphVBException(e);
            }
            catch
            {
                m_reduceFunctionId = 0;
            }

            try
            {
                e = m_childs[0].reg_func(null, "cphvb_random", ref m_randomFunctionId);
                if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                    throw new cphVBException(e);
            }
            catch
            {
                m_randomFunctionId = 0;
            }
        }

        /// <summary>
        /// Invokes garbage collection and flushes all pending cleanup messages
        /// </summary>
        public void Flush()
        {
            GC.Collect();
            ExecuteCleanups();
        }

        /// <summary>
        /// Executes a list of instructions
        /// </summary>
        /// <param name="insts"></param>
        public void Execute(params IInstruction[] insts)
        {
            Execute((IEnumerable<IInstruction>)insts);
        }

        /// <summary>
        /// Registers instructions for later execution, usually destroy calls
        /// </summary>
        /// <param name="insts">The instructions to queue</param>
        public void ExecuteRelease(params IInstruction[] insts)
        {
            //Lock is not really required as the GC is single threaded,
            // but user code could also call this
            lock(m_releaselock)
                m_cleanups.AddRange(insts);
        }

        /// <summary>
        /// Registers an instruction for later execution, usually destroy calls
        /// </summary>
        /// <param name="inst">The instruction to queue</param>
        public void ExecuteRelease(PInvoke.cphvb_instruction inst)
        {
            //Lock is not really required as the GC is single threaded,
            // but user code could also call this
            lock (m_releaselock)
                m_cleanups.Add(inst);
        }

        /// <summary>
        /// Executes a list of instructions
        /// </summary>
        /// <param name="inst_list">The list of instructions to execute</param>
        public void Execute(IEnumerable<IInstruction> inst_list)
        {
            lock (m_executelock)
                UnprotectedExecute(inst_list);

            ExecuteCleanups();
        }

        /// <summary>
        /// Executes all pending cleanup instructions
        /// </summary>
        private void ExecuteCleanups()
        {
            if (m_cleanups.Count > 0)
            {
                //Lock free swapping, ensures that we never block the garbage collector
                List<IInstruction> lst = m_cleanups;
                m_cleanups = new List<IInstruction>();

                lock (m_executelock)
                    UnprotectedExecute(lst);
            }
        }

        /// <summary>
        /// Internal execution handler, runs without locking of any kind
        /// </summary>
        /// <param name="inst_list">The list of instructions to execute</param>
        private void UnprotectedExecute(IEnumerable<IInstruction> inst_list)
        {
            List<GCHandle> cleanups = new List<GCHandle>();
            long destroys = 0;

            try
            {
                //We need to execute multiple times if we have more than CPHVB_MAX_NO_INST instructions
                PInvoke.cphvb_instruction[] buf = new PInvoke.cphvb_instruction[PInvoke.CPHVB_MAX_NO_INST];

                int i = 0;
                foreach (var inst in inst_list)
                {
                    buf[i] = (PInvoke.cphvb_instruction)inst;
                    if (buf[i].opcode == PInvoke.cphvb_opcode.CPHVB_DESTROY)
                        destroys++;
                    if (buf[i].userfunc != IntPtr.Zero)
                    {
                        cleanups.Add(m_allocatedUserfuncs[buf[i].userfunc]);
                        m_allocatedUserfuncs.Remove(buf[i].userfunc);
                    }
                        

                    i++;

                    if (i >= buf.Length)
                    {
                        PInvoke.cphvb_error e = m_childs[0].execute(buf.LongLength, buf);

                        if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                            throw new cphVBException(e);
                        i = 0;
                    }
                }

                if (i != 0)
                {
                    PInvoke.cphvb_error e = m_childs[0].execute(i, buf);

                    if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                        throw new cphVBException(e);
                }
            }
            finally
            {
                if (destroys > 0)
                    System.Threading.Interlocked.Add(ref m_allocatedviews, -destroys);
                
                foreach (var h in cleanups)
                    h.Free();
            }
        }

        /// <summary>
        /// Creates a cphvb descriptor for a base array, without assigning the actual data
        /// </summary>
        /// <param name="d">The array to map</param>
        /// <returns>The pointer to the base array descriptor</returns>
        public PInvoke.cphvb_array_ptr CreateArray(Array d)
        {
            return CreateArray(
                PInvoke.cphvb_array_ptr.Null,
                MapType(d.GetType().GetElementType()),
                1,
                0,
                new long[] { d.Length },
                new long[] { 1 },
                false,
                new PInvoke.cphvb_constant() { uint64 = 0 }
                );
        }

        /// <summary>
        /// Creates a base array from a scalar/initial value
        /// </summary>
        /// <typeparam name="T">The data type for the array</typeparam>
        /// <param name="data">The initial value for the base array</param>
        /// <param name="size">The size of the generated base array</param>
        /// <returns>The pointer to the base array descriptor</returns>
        public PInvoke.cphvb_array_ptr CreateArray<T>(T data, int size)
        {
            return CreateArray(
                PInvoke.cphvb_array_ptr.Null,
                MapType(typeof(T)),
                1,
                0,
                new long[] { size },
                new long[] { 1 },
                true,
                new PInvoke.cphvb_constant().Set(data)
                );
        }

        /// <summary>
        /// Creates a base array with uninitialized memory
        /// </summary>
       /// <param name="size">The size of the generated base array</param>
        /// <returns>The pointer to the base array descriptor</returns>
        public PInvoke.cphvb_array_ptr CreateArray(PInvoke.cphvb_type type, long size)
        {
            return CreateArray(
                PInvoke.cphvb_array_ptr.Null,
                type,
                1,
                0,
                new long[] { size },
                new long[] { 1 },
                false,
                new PInvoke.cphvb_constant()
                );
        }

        /// <summary>
        /// Creates a base array with uninitialized memory
        /// </summary>
        /// <typeparam name="T">The data type for the array</typeparam>
        /// <param name="size">The size of the generated base array</param>
        /// <returns>The pointer to the base array descriptor</returns>
        public PInvoke.cphvb_array_ptr CreateArray<T>(long size)
        {
            return CreateArray(MapType(typeof(T)), size);
        }

        public static PInvoke.cphvb_type MapType(Type t)
        {
            if (t == typeof(bool))
                return PInvoke.cphvb_type.CPHVB_BOOL;
            else if (t == typeof(sbyte))
                return PInvoke.cphvb_type.CPHVB_INT8;
            else if (t == typeof(short))
                return PInvoke.cphvb_type.CPHVB_INT16;
            else if (t == typeof(int))
                return PInvoke.cphvb_type.CPHVB_INT32;
            else if (t == typeof(long))
                return PInvoke.cphvb_type.CPHVB_INT64;
            else if (t == typeof(byte))
                return PInvoke.cphvb_type.CPHVB_UINT8;
            else if (t == typeof(ushort))
                return PInvoke.cphvb_type.CPHVB_UINT16;
            else if (t == typeof(uint))
                return PInvoke.cphvb_type.CPHVB_UINT32;
            else if (t == typeof(ulong))
                return PInvoke.cphvb_type.CPHVB_UINT64;
            else if (t == typeof(float))
                return PInvoke.cphvb_type.CPHVB_FLOAT32;
            else if (t == typeof(double))
                return PInvoke.cphvb_type.CPHVB_FLOAT64;
            else
                throw new cphVBException(string.Format("Unsupported data type: " + t.FullName));
        }

        /// <summary>
        /// Creates a cphvb base array or view descriptor
        /// </summary>
        /// <param name="basearray">The base array pointer if creating a view or IntPtr.Zero if the view is a base array</param>
        /// <param name="type">The cphvb type of data in the array</param>
        /// <param name="ndim">Number of dimensions</param>
        /// <param name="start">The offset into the base array</param>
        /// <param name="shape">The shape values for each dimension</param>
        /// <param name="stride">The stride values for each dimension</param>
        /// <param name="has_init_value">A value indicating if the data has a initial value</param>
        /// <param name="init_value">The initial value if any</param>
        /// <returns>The pointer to the base array descriptor</returns>
        public PInvoke.cphvb_array_ptr CreateArray(PInvoke.cphvb_array_ptr basearray, PInvoke.cphvb_type type, long ndim, long start, long[] shape, long[] stride, bool has_init_value, PInvoke.cphvb_constant init_value)
        {
            PInvoke.cphvb_error e;
            PInvoke.cphvb_array_ptr res;
            lock (m_executelock)
            {
                e = m_childs[0].create_array(basearray, type, ndim, start, shape, stride, has_init_value ? 1 : 0, init_value, out res);
            }

            if (e == PInvoke.cphvb_error.CPHVB_OUT_OF_MEMORY)
            {
                //If we get this, it can be because some of the unmanaged views are still kept in memory
                Console.WriteLine("Ouch, forcing GC, allocated views: {0}", m_allocatedviews);
                GC.Collect();
                ExecuteCleanups();

                lock (m_executelock)
                    e = m_childs[0].create_array(basearray, type, ndim, start, shape, stride, has_init_value ? 1 : 0, init_value, out res);
            }

            if (e != PInvoke.cphvb_error.CPHVB_SUCCESS)
                throw new cphVBException(e);

            System.Threading.Interlocked.Increment(ref m_allocatedviews);

            return res;
        }

        /// <summary>
        /// Releases all resources held
        /// </summary>
        public void Dispose()
        {
            ExecuteCleanups();

            //TODO: Probably not good because the call will "free" the component as well, and that is semi-managed
            lock (m_executelock)
            {
                if (m_childs != null)
                {
                    for (int i = 0; i < m_childs.Length; i++)
                        PInvoke.cphvb_com_free(ref m_childs[i]);

                    m_childs = null;
                }

                if (m_component.config != IntPtr.Zero)
                {
                    PInvoke.cphvb_com_free(ref m_component);
                    m_component.config = IntPtr.Zero;
                }
            }
        }

        /// <summary>
        /// Generates an unmanaged view pointer for the NdArray
        /// </summary>
        /// <param name="view">The NdArray to create the pointer for</param>
        /// <returns>An unmanaged view pointer</returns>
        protected PInvoke.cphvb_array_ptr CreateViewPtr<T>(NdArray<T> view)
        {
            return CreateViewPtr<T>(MapType(typeof(T)), view);
        }

        /// <summary>
        /// Generates an unmanaged view pointer for the NdArray
        /// </summary>
        /// <param name="type">The type of data</param>
        /// <param name="view">The NdArray to create the pointer for</param>
        /// <typeparam name="T">The datatype in the view</typeparam>
        /// <returns>An unmanaged view pointer</returns>
        protected PInvoke.cphvb_array_ptr CreateViewPtr<T>(PInvoke.cphvb_type type, NdArray<T> view)
        {
            PInvoke.cphvb_array_ptr basep;

            if (view.m_data is cphVBAccessor<T>)
            {
                basep = ((cphVBAccessor<T>)view.m_data).Pin();
            }
            else
            {
                if (view.m_data.Tag is ViewPtrKeeper)
                {
                    basep = ((ViewPtrKeeper)view.m_data.Tag).Pointer;
                }
                else
                {
                    GCHandle h = GCHandle.Alloc(view.m_data.Data, GCHandleType.Pinned);
                    basep = CreateArray<T>(view.m_data.Data.Length);
                    basep.Data = h.AddrOfPinnedObject();
                    view.m_data.Tag = new ViewPtrKeeper(basep, h);
                }
            }

            if (view.Tag == null || ((ViewPtrKeeper)view.Tag).Pointer != basep)
                view.Tag = new ViewPtrKeeper(CreateView(type, view.Shape, basep));

            return ((ViewPtrKeeper)view.Tag).Pointer;
        }

        /// <summary>
        /// Creates a new view of data
        /// </summary>
        /// <param name="shape">The shape to create the view for</param>
        /// <param name="baseArray">The array to set as base array</param>
        /// <typeparam name="T">The type of data in the view</typeparam>
        /// <returns>A new view</returns>
        public PInvoke.cphvb_array_ptr CreateView<T>(Shape shape, PInvoke.cphvb_array_ptr baseArray)
        {
            return CreateView(MapType(typeof(T)), shape, baseArray);
        }

        /// <summary>
        /// Creates a new view of data
        /// </summary>
        /// <param name="CPHVB_TYPE">The data type of the view</param>
        /// <param name="shape">The shape to create the view for</param>
        /// <param name="baseArray">The array to set as base array</param>
        /// <returns>A new view</returns>
        public PInvoke.cphvb_array_ptr CreateView(PInvoke.cphvb_type CPHVB_TYPE, Shape shape, PInvoke.cphvb_array_ptr baseArray)
        {
            //Unroll, to avoid creating a Linq query for basic 3d shapes
            if (shape.Dimensions.Length == 1)
            {
                return CreateArray(
                    baseArray,
                    CPHVB_TYPE,
                    shape.Dimensions.Length,
                    (int)shape.Offset,
                    new long[] { shape.Dimensions[0].Length },
                    new long[] { shape.Dimensions[0].Stride },
                    false,
                    new PInvoke.cphvb_constant()
                );
            }
            else if (shape.Dimensions.Length == 2)
            {
                return CreateArray(
                    baseArray,
                    CPHVB_TYPE,
                    shape.Dimensions.Length,
                    (int)shape.Offset,
                    new long[] { shape.Dimensions[0].Length, shape.Dimensions[1].Length },
                    new long[] { shape.Dimensions[0].Stride, shape.Dimensions[1].Stride },
                    false,
                    new PInvoke.cphvb_constant()
                );
            }
            else if (shape.Dimensions.Length == 3)
            {
                return CreateArray(
                    baseArray,
                    CPHVB_TYPE,
                    shape.Dimensions.Length,
                    (int)shape.Offset,
                    new long[] { shape.Dimensions[0].Length, shape.Dimensions[1].Length, shape.Dimensions[2].Length },
                    new long[] { shape.Dimensions[0].Stride, shape.Dimensions[1].Stride, shape.Dimensions[2].Stride },
                    false,
                    new PInvoke.cphvb_constant()
                );
            }
            else
            {
                long[] lengths = new long[shape.Dimensions.LongLength];
                long[] strides = new long[shape.Dimensions.LongLength];
                for (int i = 0; i < lengths.LongLength; i++)
                {
                    var d = shape.Dimensions[i];
                    lengths[i] = d.Length;
                    strides[i] = d.Stride;
                }

                return CreateArray(
                    baseArray,
                    CPHVB_TYPE,
                    shape.Dimensions.Length,
                    (int)shape.Offset,
                    lengths,
                    strides,
                    false,
                    new PInvoke.cphvb_constant()
                );
            }
        }

        public IInstruction CreateInstruction<T>(PInvoke.cphvb_opcode opcode, NdArray<T> operand)
        {
            return CreateInstruction<T>(MapType(typeof(T)), opcode, operand);
        }
        public IInstruction CreateInstruction<T>(PInvoke.cphvb_opcode opcode, NdArray<T> op1, NdArray<T> op2)
        {
            return CreateInstruction<T>(MapType(typeof(T)), opcode, op1, op2);
        }
        public IInstruction CreateInstruction<T>(PInvoke.cphvb_opcode opcode, NdArray<T> op1, NdArray<T> op2, NdArray<T> op3)
        {
            return CreateInstruction<T>(MapType(typeof(T)), opcode, op1, op2, op3);
        }
        public IInstruction CreateInstruction<T>(PInvoke.cphvb_opcode opcode, IEnumerable<NdArray<T>> operands)
        {
            return CreateInstruction<T>(MapType(typeof(T)), opcode, operands);
        }

        public IInstruction CreateInstruction<T>(PInvoke.cphvb_type type, PInvoke.cphvb_opcode opcode, NdArray<T> operand)
        {
            return new PInvoke.cphvb_instruction(opcode, CreateViewPtr<T>(type, operand));
        }

        public IInstruction CreateInstruction<T>(PInvoke.cphvb_type type, PInvoke.cphvb_opcode opcode, NdArray<T> op1, NdArray<T> op2)
        {
            return new PInvoke.cphvb_instruction(opcode, CreateViewPtr<T>(type, op1), CreateViewPtr<T>(type, op2));
        }

        public IInstruction CreateInstruction<T>(PInvoke.cphvb_type type, PInvoke.cphvb_opcode opcode, NdArray<T> op1, NdArray<T> op2, NdArray<T> op3)
        {
            return new PInvoke.cphvb_instruction(opcode, CreateViewPtr<T>(type, op1), CreateViewPtr<T>(type, op2), CreateViewPtr<T>(type, op3));
        }

        public IInstruction CreateInstruction<T>(PInvoke.cphvb_type type, PInvoke.cphvb_opcode opcode, IEnumerable<NdArray<T>> operands)
        {
            return new PInvoke.cphvb_instruction(opcode, operands.Select(x => CreateViewPtr<T>(type, x)));
        }

        public IInstruction CreateRandomInstruction<T>(PInvoke.cphvb_type type, NdArray<T> op1)
        {
            if (!SupportsRandom)
                throw new cphVBException("The VEM/VE setup does not support the random function");

            GCHandle gh = GCHandle.Alloc(
                new PInvoke.cphvb_userfunc_random(
                    m_randomFunctionId,
                    CreateViewPtr<T>(type, op1)
                ), 
                GCHandleType.Pinned
            );

            IntPtr adr = gh.AddrOfPinnedObject();

            m_allocatedUserfuncs.Add(adr, gh);

            return new PInvoke.cphvb_instruction(
                PInvoke.cphvb_opcode.CPHVB_USERFUNC,
                adr                    
            );
        }

        public IInstruction CreateReduceInstruction<T>(PInvoke.cphvb_type type, PInvoke.cphvb_opcode opcode, long axis, NdArray<T> op1, NdArray<T>op2)
        {
            if (!SupportsReduce)
                throw new cphVBException("The VEM/VE setup does not support the reduce function");

            GCHandle gh = GCHandle.Alloc(
                new PInvoke.cphvb_userfunc_reduce(
                    m_reduceFunctionId,
                    opcode,
                    axis,
                    CreateViewPtr<T>(type, op1),
                    CreateViewPtr<T>(type, op2)
                ), 
                GCHandleType.Pinned
            );

            IntPtr adr = gh.AddrOfPinnedObject();

            m_allocatedUserfuncs.Add(adr, gh);

            return new PInvoke.cphvb_instruction(
                PInvoke.cphvb_opcode.CPHVB_USERFUNC,
                adr
            );
        }

        /// <summary>
        /// Gets a value indicating if the Reduce operation is supported
        /// </summary>
        public bool SupportsReduce { get { return m_reduceFunctionId > 0; } }
        //public bool SupportsReduce { get { return false; } }

        /// <summary>
        /// Gets a value indicating if the Random operation is supported
        /// </summary>
        public bool SupportsRandom { get { return m_randomFunctionId > 0; } }
    }
}