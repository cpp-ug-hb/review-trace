Code Review: Trace
==================================

This repository contains code to efficiently store,read and modify time series data logic values. The core class is called `Trace`.

Time is an abstract simulation time, which can be counted in discrete intervals.
It is essentially a std:: of (<uint64_t Time 
At each time point, the value can be one of 9 VHDL values stored in an `uint8_t`.

The traces can be very long, containing between 100k to 10m values. Because of this, no duplicated values should


Use Case
---------

1. **Wrinting:** The common use case is the linear append-only addition of values at the end of the Trace.
2. **Read:** The following read operations must be possible:
  * access the current value for any point in time
  * find all time points where the value changes
  * find the next/previous point of change
  * find the next/previous value
3. **Edit:**
  * Remove a section of the Trace
  * Replace a single value in the trace
  * Insert a new value between existing values


C++ and Simplifications
------------------------

Compilers:
* GCC 4.8.2
* MSVC 2010, later 2012

The types for Time and Bit where replaced to simplify this critic.
As C++11 was not available, boost is used for various replacements.
