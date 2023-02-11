#ifndef _ASEMBLER_
#define _ASEMBLER_

#include <regex>
#include <vector>
#include <string>
#include <iostream>
#include "Exceptions.hpp"
#include "SymbolTable.hpp"
#include "RelTable.hpp"

using namespace std;

enum class AddressModes{
  IMMEDIATE_LITERAL,
  IMMEDIATE_SYMBOL,
  MEMORY_DIRECT_LITERAL,
  MEMORY_DIRECT_SYMBOL,
  PC_RELATIVE,
  REGISTER_DIRECT,
  REGISTER_INDIRECT,
  REGISTER_INDIRECT_OFFSET_LITERAL,
  REGISTER_INDIRECT_OFFSET_SYMBOL,
  INVALID
};

struct Operand{
  AddressModes mode;
  string operand;
  int regNumber;
  Operand() {mode = AddressModes::INVALID, operand = ""; regNumber = 0;}
  Operand(AddressModes m, string op, int num = 0) {mode = m; operand = op; regNumber = num;}
};

enum Directives{
  INVALID = 0,
  GLOBAL,
  EXTERN,
  SECTION,
  WORD,
  SKIP,
  ASCII,
  END
};

enum Instructions{
  ERROR = 0,
  HALT = 8,
  INT, IRET, CALL, RET,
  JMP, JEQ, JNE, JGT,
  PUSH, POP,
  XCHG,
  ADD, SUB, MUL, DIV, CMP,
  NOT, AND, OR, XOR, TEST, SHL, SHR,
  LDR, STR
};

class Asembler{
  private:
    string input;
    string output;
    static int lineCnt;
    vector<vector<unsigned char>> generatedCode;  //
    vector<string> sections;                      // --> generated code is split between sections
    static int sectionIndexForGeneratedCode;      //
    SymbolTable symbolTable;
    RelocationTable relocationTable;

    static int lc;
  public:
    Asembler(string i, string o = "asemblerOutput.o") : input(i), output(o) {}
    ~Asembler() {}


    void assemble();

    int processLine(string line);
    int processDirective(string line);
    int processInstruction(string line);
    int processInstructionWithLabel(string line);
    int processLabelOnly(string label);

    int backpatchAndGenerateRealocationEntries();
    void formOutput(ofstream& output);

    friend int switchInstruction(Instructions instr, string line, Asembler* as);
    friend void switchDataOperand(Operand op, Asembler* asem, unsigned char opCode, unsigned char regDest);
    friend void switchAddressOperand(Operand op, Asembler* asem, unsigned char opCode);
    friend int getSectionIndexOfASymbol(Asembler* as, int secNum);
    friend int getSectionIndex(Asembler* as, string secName);
};

#endif