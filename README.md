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

All times are best of 5. All memory usage stats refer to peak resident set size
(since this is what would determine how much memory would actually be needed to
run some number of jobs simultaneously). Unless otherwise specified, all
execution time and memory usage percentages are specified relative to the
fastest build of the dedicated one-off C++ implementation.

C++11
-----

- GCC 4.7.2
- LOC: 513
- Build time:
    1. With "-O0 -g": 2.111s
    2. With "-O2": 2.013s
    3. With "-O3": 2.042s
- Execution time:
    1. With "-O0 -g": 24.393s
    2. With "-O2": 4.585
    3. With "-O3": 4.638
- Peak memory usage:
    1. With "-O0 -g": 1072 KB
    2. With "-O2": 1076 KB
    3. With "-O3": 1076 KB
- Between range-based `for`, type inference with `auto`, and `unique_ptr`,
  C++11 is a significant improvement over C++03.
- "Modules" through textual inclusion are still terrible, as are build times.
- Vim is nice and all, but it would be nice to have a cleaner, more
  tool-friendly language; there doesn't really seem to be a C++ IDE that can
  keep up with C++11, let alone the macro and template magic thrown around in
  Prismriver.

C#
--

- Mono 2.10.8.1
- LOC: 585
- Build time: 0.409s
- Execution time:
    1. Boehm GC: 16.549s (360.9%)
    2. SGen GC: 9.498s (207.2%)
- Peak memory usage:
    1. Boehm GC: 6368 KB (591.8%)
    2. SGen GC: 10808 KB (1004.5%)
- The high line count is a combination of C#'s brace placement convention
  (opening braces are placed on their own line), which results in somewhat
  sparse-looking code, and the fact that the C# standard library doesn't
  provide a priority queue class.
- C# is basically just a nicer Java.

Python
------

- Python 2.7.3 (compiled with GCC 4.7.2); PyPy 1.9.0 (based on Python 2.7.2,
  compiled with GCC 4.7.0)
- LOC: 239
- Execution time:
    1. CPython: 642.868s (13739.4%)
    2. PyPy: 45.467s (971.7%)
- Peak memory usage:
    1. CPython: 4968 KB (461.7%)
    2. PyPy: 62040 KB (5765.8%)
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

Rust
----

Rust 0.8. Does not compile.

One of Rust's major selling points is the fact that it statically checks
lifetimes, preventing dangling pointers from existing. Unfortunately, while
porting Nijikawa to Rust, I ran into a problem with the borrow checker:

The `MemRequest` type, which represents a generic memory request, carries three
pieces of data: the address that the request is to, whether the request is a
read or write, and a pointer/reference to a `MemResponseReceiver` that will
process the `MemResponse` generated when the memory request is serviced. (The
names may vary slightly from language to language, but the concept is always
the same.)

In Rust, expressing the required lifetime relationship isn't too bad:

    struct MemRequest<'self> {
        // ...
        response_receiver: &'self MemResponseReceiver
    }

meaning "a MemRequest contains a borrowed (non-owning) pointer to a
MemResponseReceiver that must live as long as the MemRequest". Fair enough.

Next, the `Request` type, which represents a request going to DRAM (it doesn't
get a more specific name because it's private to the DRAM module), contains
the DRAM geometry referred to by the address, plus an owning pointer to the
`MemRequest` that it represents. Since Rust worries about `MemRequest`'s
lifetime, it worries about `Request`'s lifetime too, and we have to assuage the
borrow checker again:

    struct Request<'self> {
        // ...
        mem_request: ~MemRequest<'self>
    }

meaning "a Request contains an owning pointer to a MemRequest that lives as
long as the Request". Ok, I guess.

The `Dram` type encompasses both an extremely simplified DRAM timing model and
an extremely simplified memory controller model. Since DRAM channels can be
scheduled independently, each DRAM channel is represented by a `Channel`
object. Each `Channel` contains a vector of owning pointers to `Requests` that
are waiting to be issued:

    struct Dram<'self> {
        // ...
        channels: ~[Channel<'self>]
    }

    struct Channel<'self> {
        // ...
        waiting_reqs: ~[~Request<'self>]
    }

This is starting to look rather dubious, and in fact the problem is right here.
We'll get back to it in a minute.

Right after all hell breaks loose:

    impl<'self> Dram<'self> {
        fn tick(&mut self) {
            for chan in self.channels.mut_iter() {
                // ...
                match (self.best_request(chan)) {
                    Some(req) => { self.issue_request(req); }
                    None => {}
                }
            }
        }
    }

(By the way, if for some reason you're actually reading the source, the reason
`best_request` is a `Dram` method and not a `Channel` one is because some
schedulers may want to choose the best request in a channel based on
information about other channels.)

This gives us lifetime errors. What happened? We promised Rust that the
contents of `Channel.waiting_reqs` will live no longer than the containing
`Channel`. This is a lie, and Rust complains when we try to take a `Request`
outside of the `Channel` in `Dram.tick()`. It's right to do so, too. When a
`Request` is inserted into `Channel.waiting_reqs`, Rust okays it on the promise
that the `Request` won't outlive `Channel`; breaking this promise on `Requests`
we take out mean that we could end up with a dangling `&MemRequestReceiver`.

So why is this Rust's problem rather than ours? Because we actually *can't*.
All components (including the `Dram`, and the `Core` that is always the
`MemResponseReceiver` in practice) live for as long as the containing
simulation (`main()`). In a perfect world, we'd be able to say something like:

    struct Channel<'self, 'sim> { // sim is the lifetime of the simulation
        waiting_reqs: ~[~Request<'sim>]
    }

Then Rust would instead check that the `Request` can't escape the lifetime of
the simulator, and everything would be fine. But we can't, and it won't.

So how do we work around this conundrum? The Rust IRC (which by the way was
incredibly helpful on both this issue and other issues that turned out to be
entirely my fault) suggested two things, and I considered a third:

- Use a managed pointer for `MemRequest.response_receiver`: managed pointers
  have completely the wrong meaning here, because a memory request does not in
  any way own the component it goes back to. This is confusing to humans. In
  the interest of trying to get a Rust implementation working, I tried this
  approach anyway, got a new torrent of incomprehensible borrowing errors, and
  gave up.
- Use an unsafe pointer for `MemRequest.response_receiver`: having to use
  `unsafe` to do something that is completely safe, in the very first
  not-completely-trivial Rust program I've ever written, leaves a very bad
  taste in my mouth. Oh, also, it doesn't work because you can't take an unsafe
  pointer to a trait.
- Go back to C++: but I'll really miss sum types, pattern matching, modules not
  based on textual inclusion, and Rust's overall incredible terseness!

Unfortunately, for all its faults, I pick option (3).

Nitpicking:

- Similarly, having "constructors" be non-special static methods is
  conceptually elegant, but I *really* miss default constructors and default
  values for structs.
- Rust combines traditional enums (types with a bounded set of named values)
  with sum types into one unified `enum` thing. This is actually kind of
  annoying because it means that in what looks like a traditional enum, each
  "value" is actually a kinda-sorta-type, so you can't use comparison (`if
  my_enum == Foo`) and must use pattern matching instead, even in cases where
  you really just want comparison (see `Dram.issue_request()`).
- Pattern matching isn't quite as nice as in Scala because patterns must be
  literals or variables in Rust, necessitating the use of pattern guards. As a
  result, `Dram::request_conflict_state()` is slightly less elegant than its
  Scala equivalent.
- `Dram::new()` and `Channel::new()` both contain the moral equivalent of
  Python's `[Something() for _ in range(n)]`, but the Rust version is tediously
  wordy. On the bright side, Rust's excellent generics meant that the tediously
  wordy expression could be factored out into a single function used for both.
- Having to cast pointer-to-object to pointer-to-trait is stupid and tiresome.
- I miss constexpr functions.

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
- Peak memory usage: 1588 KB (147.6%)
- Finding a language with longer compile times than C++ is remarkable.
- Localized implicit conversion (e.g. from a tuple to Ordered[tuple] for the
  priority queue in `Core`) is wonderful.
- Pattern matching is very nice.
- Option types are nifty.
- In a language with mutability, `map` doesn't really seem better than foreach
  loops, and `filter` isn't much better than early `break`.
- Folding is nice. `foldLeft` and pattern matching lead to a very clear
  expression of FR-FCFS in `Dram.bestRequest()`.
- Arrays of objects are initialized to null like in Java, so
  NullPointerExceptions can occur even in code exclusively using Option[] over
  null.
- Minor gripe: the conditions under which a method will be implicitly converted
  to a function to pass as a function parameter don't seem very clear, and when
  the conditions aren't met, the workaround (`_` parameter) is ugly.

License
=======

All code in this repository is released under the
[WTFPLv2](http://www.wtfpl.net/):

>                DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
>                        Version 2, December 2004
>
>     Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
>
>     Everyone is permitted to copy and distribute verbatim or modified
>     copies of this license document, and changing it is allowed as long
>     as the name is changed.
>
>                DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
>       TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
>
>      0. You just DO WHAT THE FUCK YOU WANT TO.
