Introduction
============

This repository contains source code for an implementation of a simple
architectural simulator consisting of an extremely simple, memory trace-driven
out-of-order core model attached to a simple DRAM model.

Program Description
===================

Each program simulates 100 million cycles of execution of the trace "comm1"
included with [USIMM v1.3](https://www.cs.utah.edu/~rajeev/jwac12/). The core
model, which is extremely simple, models the reorder buffer and cache hierarchy
(the trace is cache-prefiltered) of a 3.2GHz, 4-wide, 192-instruction window
out-of-order core. The memory model represents a 2-channel, 4-banks-per-channel
DDR3-1600 11-11-11-28 DRAM system, controlled by an FR-FCFS memory controller
with infinite queue depth and a lazy-open row policy.

Correct implementations should retire 243,887,974 instructions after 100
million cycles.

Results
=======

All implementations are run on a laptop with a Intel i7-2720QM proccessor, 16GB
DDR3-1333 DRAM, and a 512GB Crucial M4 SSD, running Xubuntu 12.10 x86-64.

All times are best of 5. Unless otherwise specified, all execution time
percentages are specified relative to the execution time of Prismriver's
release build.

Prismriver (C++11, reference simulator)
---------------------------------------

- Commit c88448a7b55f0e4836e090bfb5aa218b8382eb4f
- GCC 4.7.2
- Execution time:
    1. Checked build: 4.858s
    2. Release build: 4.679s
- See comments below.

C++11
-----

- GCC 4.7.2
- LOC: 510
- Build time:
    1. With "-O0 -g": 2.111s
    2. With "-O2": 2.013s
    3. With "-O3": 2.042s
- Execution time:
    1. With "-O0 -g": 24.393s (521.3%)
    2. With "-O2": 4.585 (98.0%)
    3. With "-O3": 4.638 (99.1%)
- Prismriver is slightly slower than the much simpler purpose-built
  implementation for the sake of genericity, but not by much.
- Between range-based `for`, type inference with `auto`, and `unique_ptr`,
  C++11 is a significant improvement over C++03.
- "Modules" through textual inclusion are still terrible, as are build times.
- Vim is nice and all, but it would be nice to have a cleaner, more
  tool-friendly language; there doesn't really seem to be a C++ IDE that can
  keep up with C++11, let alone the macro and template magic thrown around in
  Prismriver.

Python
------

- Python 2.7.3 (compiled with GCC 4.7.2); PyPy 1.9.0 (based on Python 2.7.2,
  compiled with GCC 4.7.0)
- LOC: 239
- Execution time:
    1. CPython: 642.868s (13739.4%)
    2. PyPy: 45.467s (971.7%)
- PyPy is the best thing to happen to Python since NumPy and co. Now if only if
  they were compatible, and PyPy supported Py3K.
- For all the flak Python gets for having meaningful whitespace, it's visually
  much cleaner than braces.
- Conceptually, explicit `self` is clean and obvious. In practice, it gets
  tedious rather quickly.
- Reference semantics mean that `[MyClass()] * n` creates an array containing
  `n` references to the *same object*. Python has the same problem as Java in
  this regard (primitive types have value semantics, everything else has
  reference semantics).

Scala
-----

- Scala 2.10.0, Oracle Java HotSpot 64-bit server VM 23.7-b01 (from JDK 7u13)
- LOC: 324
- Build time:
    1. With "-g:vars": 6.906s
    2. With "-g:none -optimise": 7.841s
- Execution time:
    1. With "-g:vars": 15.276 (326.5%)
    2. With "-g:none -optimise": 15.184 (324.5%)
- Finding a language with longer compile times than C++ is remarkable.
- Localized implicit conversion (e.g. from a tuple to Ordered[tuple] for the
  priority queue in `Core`) is wonderful.
- Pattern matching is very nice.
- Option types are nifty.
- In a language with mutability, `map` doesn't really seem better than foreach
  loops, and `filter` isn't much better than early `break`.
- Folding is nice. `foldLeft` and pattern matching lead to a very clear
  expression of FR-FCFS in `SimpleDram.bestRequest()`.
- Arrays of objects are initialized to null like in Java, so
  NullPointerExceptions can occur even in code exclusively using Option[] over
  null.
- Minor gripe: the conditions under which a method will be implicitly converted
  to a function to pass as a function parameter don't seem very clear, and when
  the conditions aren't met, the workaround (`_` parameter) is ugly.

