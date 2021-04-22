all: seq tbb

seq:
	g++ -o matmult_seq matmult.cpp

tbb:
	g++ -D USE_TBB -o matmult_tbb matmult.cpp
