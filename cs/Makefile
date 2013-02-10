.PHONY: default clean

default: bin/nijikawa.exe

bin/nijikawa.exe: Core.cs Dram.cs Main.cs MemRequest.cs PriorityQueue.cs Simulator.cs TraceReader.cs TraceRecord.cs UsimmTraceReader.cs
	mkdir -p bin
	gmcs -debug -optimize -out:$@ $^

clean:
	rm -f bin/nijikawa.exe

