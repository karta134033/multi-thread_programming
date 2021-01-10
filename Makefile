all: pi
pi: omp.cpp
	g++ -O3 -march=native omp.cpp -lpthread -o pi.out
