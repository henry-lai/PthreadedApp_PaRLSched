FLAGS = -std=c++11 -I ./include-rev491/
LFLAGS = -lpthread

ifeq ($(NUMA), 1)
LFLAGS     += -lnuma
FLAGS      += -DUSE_NUMA_H
endif

# compile for debug or release
ifeq ($(DEBUG), 1)
OPTIMIZE_FLAGS		= -O0 -g #-DTRACE_FASTFLOW
else
OPTIMIZE_FLAGS		= -O3 -finline-functions #-DTRACE_FASTFLOW
endif

SEQ_LFLAGS = 

all: fffarm

fffarm:
	$(CXX) $(FLAGS) $(INCLUDE) ant_farm_nm.cpp -o nm_ant_farm $(LFLAGS) $(OPTIMIZE_FLAGS)
#seq:
#	$(CXX) $(FLAGS) $(INCLUDE) ant_seq.cpp -o ant_seq $(SEQ_LFLAGS)
#farm:
#	$(CXX) $(FLAGS) $(INCLUDE) ant_farm.cpp -o ant_farm $(LFLAGS)
#update_farm:
#	$(CXX) $(FLAGS) $(INCLUDE) ant_update_farm.cpp -o ant_update_farm $(LFLAGS)

clean:
	rm -f *.o *~ nm_ant_farm


