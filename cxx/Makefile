CCFLAGS += -Wall -Wextra -Werror -O3
CXXFLAGS += -std=c++11

.PHONY: default clean

default: bin/nijikawa

bin/nijikawa: Makefile address.h core.cc core.h cycle.h dram.cc dram.h mem_request.h main.cc simulator.h trace_reader.h trace_record.h unique_ptr.h usimm_trace_reader.cc
	mkdir -p bin
	$(CXX) $(CCFLAGS) $(CXXFLAGS) -o bin/nijikawa core.cc dram.cc main.cc usimm_trace_reader.cc

clean:
	rm -f bin/nijikawa

