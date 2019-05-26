CXXFLAGS=-g -Wall -Wpedantic -std=c++14 -pthread -O3
default: cache_measure
cache_measure: cache_measure.cc Makefile
	$(CXX) $(CXXFLAGS) -o $@ cache_measure.cc
