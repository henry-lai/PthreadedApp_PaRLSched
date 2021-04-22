PaRLSched Library

Provided software

The source code provided is divided into two sub-directories, namely:

* src
* PaRLSched

The "PaRLSched" directory provides the source code for building the scheduler library, while the "src" directory provides an example of how the library can be linked and utilized. In particular, the provided example is "src/SchedulingExample.cpp". In this example, first, a number of parameters are initialized necessary for running the scheduler. Then, a number of parallel (POSIX) threads following a standard farm pattern are created, while the computational task assigned to each thread corresponds to the computation of combinations of numbers. In fact, the pattern over which the threads are defined and the computational tasks executed is subject to the user and does not constrain the applicability of the scheduler. Furthermore, the details and nature of these computational tasks could be different for each thread and can be defined by the user within the "src/thread_routine.h" header file which is also provided.


Installation

Currently, the provided scheduler has been developed and tested over Linux Kernel 64bit 3.13.0-43-generic. For compiling the code, gcc (version 4.9.3) compiler has been used under the C++11 standard.

Before building the library, it is necessary that the PAPI-5.4.3 library interface for performance monitoring is being downloaded and installed. Download links and instructions can be found at "http://icl.cs.utk.edu/papi/software". Furthermore, the pthread library should be installed, which is available in several Linux distributions. 

For building the example "src/SchedulingExample.cpp" and linking to the PaRLSched library, CMakeLists files are also provided. To build and run the scheduling example, simply run the following commands:

mkdir build
cd build
cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ..
make
cd ./src
./SchedulingExample

In case it is derirable to edit the "SchedulingExample.cpp" provided and/or create a new example, the following commands generate the corresponding C++ project in Eclipse IDE.

mkdir build
cd build
cmake -G"Eclipse CDT4 - Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ..
make

For example, in Eclipse IDE (Version: Mars.1 Release (4.5.1)) the project can be imported as follows: "File -> Import -> C/C++" and then select "Existing Code as Makefile Project". Then, enter the root directory of the project and select the Linux GCC toolchain. 
