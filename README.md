Introduction
============

This repository contains source code for an implementation of a simple
architectural simulator consisting of an extremely simple, memory trace-driven
out-of-order core model attached to a simple DRAM model.

Program Description
===================

Each program simulates 100 million cycles of execution of the trace `comm1`
included with [USIMM v1.3](https://www.cs.utah.edu/~rajeev/jwac12/). The core
model, which is extremely simple, models the reorder buffer and cache hierarchy
(the trace is cache-prefiltered) of a 3.2GHz, 4-wide, 192-instruction window
out-of-order core. The memory model represents a 2-channel, 4-banks-per-channel
DDR3-1600 11-11-11-28 DRAM system, controlled by an FR-FCFS memory controller
with infinite queue depth and a lazy-open row policy.

Correct implementations should retire 233,452,439 instructions after 100
million cycles.

Results
=======

All implementations are run on a laptop with a Intel i7-2720QM proccessor, 16GB
DDR3-1333 DRAM, and a 512GB Crucial M4 SSD, running Xubuntu 12.10 x86-64.

All times are best of 5. Unless otherwise specified, all execution time
percentages are specified relative to the execution time of Prismriver's
release build.

1. Prismriver (reference simulator, C++11, GCC 4.7.2, commit
   c88448a7b55f0e4836e090bfb5aa218b8382eb4f):
    - Execution time:
        1. Checked build: 4.993s
        2. Release build: 4.732s
    - See comments below.
2. C++11, GCC 4.7.2:
    - LOC: 537
    - Build time:
        1. With "-O0 -g": 2.111s
        2. With "-O2": 2.013s
        3. With "-O3": 2.042s
    - Execution time:
        1. With "-O0 -g": 25.401s (536.8%)
        2. With "-O2": 4.771s (100.8%)
        3. With "-O3": 4.668s (98.6%)
    - Prismriver is slightly slower than the much simpler purpose-built
      implementation for the sake of genericity, but not by much.
    - Between range-based `for`, type inference with `auto`, and `unique_ptr`,
      C++11 is a significant improvement over C++03.
    - "Modules" through textual inclusion are still terrible, as are build
      times.
    - Vim is nice and all, but it would be nice to have a cleaner, more
      tool-friendly language; there doesn't really seem to be a C++ IDE that
      can keep up with C++11, let alone the macro and template magic thrown
      around in Prismriver.
3. Scala 2.10.0, Oracle Java HotSpot 64-bit server VM 23.7-b01 (from JDK 7u13):
    - LOC: 338
    - Build time:
        1. With "-g:vars": 6.906s
        2. With "-g:none -optimise": 7.841s
    - Execution time:
        1. With "-g:vars": 15.919s (336.4%)
        2. With "-g:none -optimise": 15.834s (334.6%)
    - Note that the Scala implementation isn't *entirely* comparable to other
      implementations in terms of LOC; in particular, some classes have public
      fields rather than setters and getters, since I'm not sure if the latter
      is idiomatic in Scala.
    - Finding a language with longer compile times than C++ is remarkable.
    - Pattern matching is wonderful.
    - In a language with mutability, `map` doesn't really seem better than
      foreach loops, and `filter` isn't much better than early `break`.
    - Folding is nice. `foldLeft` and pattern matching lead to a very clear
      expression of FR-FCF in `SimpleDram.bestRequest()`.
    - Arrays of objects are initialized to null like in Java, so
      NullPointerExceptions can occur even in code exclusively using Option[]
      over null.
    - Minor gripe: the conditions under which a method will be implicitly
      converted to a function to pass as a function parameter don't seem very
      clear, and when the conditions aren't met, the workaround (`_` parameter)
      is ugly.

