data: input data for use case
doc: documentation
erdm_offline: original source code (Eclipse project, should be compiled with gcc -O3)
erdm_cpu: cpu-based parallel version (CMake-based, fastflow is expected to be in the same directory as erdm_cpu)
erdm_gpu: gpu-based parallel version (CMake-based, fastflow is expected to be in the same directory as erdm_gpu)

program call: erdm_offline <path to data/settings.json> <path to data/rawSamples[_xx].dat>
		erdm_cpu <path to data/settings_new.json> <path to data/rawSamples[_xx].dat>
		erdm_gpu <path to data/settings_new.json> <path to data/rawSamples[_xx].dat>