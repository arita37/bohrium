.. Bohrium documentation master file

Welcome!
========

Bohrium provides a runtime environment for efficiently executing vectorized applications using your favorite programming language Python/NumPy, C#, F# on Linux, Windows and MacOSX.

Forget handcrafting CUDA/OpenCL to utilize your GPU, forget threading, mutexes and locks to utilize your multi-core CPU and forget about MPI to program your cluster just Bohrium!

Features
--------

+-----------+-----+-----+-------------+-------+-----+------+
|           | Architecture Support    | Frontends          |
+-----------+-----+-----+-------------+-------+-----+------+
|           | CPU | GPU | CPU Cluster | NumPy | C++ | .NET |
+===========+=====+=====+=============+=======+=====+======+
| Linux     | X   | X   | ( X )       | X     | X   |  X   |
+-----------+-----+-----+-------------+-------+-----+------+
| MacOSX    | X   | X   |             | X     |     |  X   |
+-----------+-----+-----+-------------+-------+-----+------+
| Windows   |     |     |             |       |     |      |
+-----------+-----+-----+-------------+-------+-----+------+

Please note:
    * Bohrium is a 64bit project exclusively.
    * The Cluster implementation is very experimental thus has no Ubuntu package etc.
    * We are working on a Windows version that uses the .NET frontend and CPU backend.

Get Started!
------------

.. toctree::
   :maxdepth: 2

   installation/index

.. toctree::
   :maxdepth: 2

   users/index

.. toctree::
   :maxdepth: 1

   developers/index

* `Student Projects <https://docs.google.com/document/d/1bMxKi86q-F0kVHLZoQzzt7BhTde4GOco9cA_fhJf45c/edit?usp=sharing/>`_

* `Benchmark Suite <http://benchpress.readthedocs.org/>`_

.. toctree::
   :maxdepth: 1

   faq
   bugs
   publications
   contact
   license

