# build from tests
# g++ -o ../asembler ../src/Asembler.cpp ../src/RelTable.cpp ../src/SymbolTable.cpp
# g++ -o ../linker ../src/Linker.cpp ../src/RelTable.cpp ../src/SymbolTable.cpp
# g++ -o ../emulator ../src/Emulator.cpp

# build from resenje
# g++ -o asembler ./src/Asembler.cpp ./src/RelTable.cpp ./src/SymbolTable.cpp
# g++ -o linker ./src/Linker.cpp ./src/RelTable.cpp ./src/SymbolTable.cpp
# g++ -o emulator ./src/Emulator.cpp


ASSEMBLER=../asembler
LINKER=../linker
EMULATOR=../emulator

${ASSEMBLER} -o main.o main.s
${ASSEMBLER} -o math.o math.s
${ASSEMBLER} -o ivt.o ivt.s
${ASSEMBLER} -o isr_reset.o isr_reset.s
${ASSEMBLER} -o isr_terminal.o isr_terminal.s
${ASSEMBLER} -o isr_timer.o isr_timer.s
${ASSEMBLER} -o isr_user0.o isr_user0.s
${LINKER} -hex -o program.hex -place=my_code@0x207 -place=ivt@0x0 math.o main.o isr_reset.o isr_terminal.o isr_timer.o isr_user0.o ivt.o
${EMULATOR} program.hex