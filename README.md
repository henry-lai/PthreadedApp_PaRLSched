# PthreadedApp_PaRLSched
A Pthread application developed with hooks to the PaRLSched for performance evaluation purposes


Requirements:
- Native installation of Linux - I used Ubuntu 18.04 GNU Linux 64bit 4.15.0-118-generic.
- PAPI library - At the time, I used version 6.0.0
- gcc/g++
- CMake
- libhwloc-dev, libnuma-dev, boost-dev, boost-thread-dev etc (for simplicity I just installed libboost-all-dev)

- Install the above libraries/programs FIRST before attempting the installation of scheduler.

Installation of Scheduler
- cd into Scheduler
- mkdir build (if build dir already exist, simply delete that existing build dir and make a new one.)
- cd build and run ' cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release .. '
- run ' make '
- cd examples to find directories of executable examples. i.e. examples/charmap_pthread/, examples/blackscholes etc

