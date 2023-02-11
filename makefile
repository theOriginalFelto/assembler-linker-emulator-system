INCLUDE = ./src/RelTable.cpp ./src/SymbolTable.cpp
METAFILES = ./b_tests/*.o ./b_tests/*.hex ./a_tests/*.o ./a_tests/*.hex
PROGRAMS = asembler linker emulator

all: clean $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) $(METAFILES)

asembler: ./src/Asembler.cpp $(INCLUDE)
	g++ -o asembler ./src/Asembler.cpp $(INCLUDE)

linker: ./src/Linker.cpp $(INCLUDE)
	g++ -o linker ./src/Linker.cpp $(INCLUDE)

emulator: ./src/Emulator.cpp
	g++ -o emulator ./src/Emulator.cpp