INCLUDE = ./src/RelTable.cpp ./src/SymbolTable.cpp
PROGRAMS = asembler linker emulator

all: clean $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

asembler: ./src/Asembler.cpp $(INCLUDE)
	g++ -o asembler ./src/Asembler.cpp $(INCLUDE)

linker: ./src/Linker.cpp $(INCLUDE)
	g++ -o linker ./src/Linker.cpp $(INCLUDE)

emulator: ./src/Emulator.cpp
	g++ -o emulator ./src/Emulator.cpp