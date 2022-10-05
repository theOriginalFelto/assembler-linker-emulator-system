#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#define term_out 0xFF00
#define term_in 0xFF02
#define tim_cfg 0xFF10

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include "Exceptions.hpp"

using namespace std;

enum UpdateModes{
  NO_UPDATE,
  PRE_DECREMENT,
  PRE_INCREMENT,
  POST_DECREMENT,
  POST_INCREMENT
};

enum AddressModes{
  IMMEDIATE,
  REGISTER_DIRECT,
  REGISTER_INDIRECT,
  REGISTER_INDIRECT_OFFSET,
  MEMORY,
  REGISTER_DIRECT_ADDITION
};

struct RegisterDescription{
  short destination;
  short source;
  RegisterDescription() {destination = 0; source = 0;}
  RegisterDescription(unsigned char regDescr) {destination = (regDescr & 0xF0) >> 4; source = regDescr & 0xF;}
};

class Emulator{
private:
  int r[8] = {0}; // r[7] = pc, r[6] = sp
  int psw;
  unsigned char memory[65536] = {0};
  unsigned char interruptRequests; // zeroth bit reset, first error, second timer and third terminal interrupts

  string input;
  int maxAddress; // top address of code
  bool terminalBreak; // indicates if there was any output from terminal

  void instructionINT();
  void instructionIRET();
  void instructionCALL();
  void instructionRET();
  void instructionJMP();
  void instructionJEQ();
  void instructionJNE();
  void instructionJGT();
  void instructionXCHG();
  void instructionADD();
  void instructionSUB();
  void instructionMUL();
  void instructionDIV();
  void instructionCMP();
  void instructionNOT();
  void instructionAND();
  void instructionOR();
  void instructionXOR();
  void instructionTEST();
  void instructionSHL();
  void instructionSHR();
  void instructionLDR();
  void instructionSTR();

  void checkForInterrupts();

  void setZ();
  void resetZ();
  bool getZ() const {return (psw & 1) != 0;}

  void setO();
  void resetO();
  bool getO() const {return (psw & 2) != 0;}

  void setC();
  void resetC();
  bool getC() const {return (psw & 4) != 0;}

  void setN();
  void resetN();
  bool getN() const {return (psw & 8) != 0;}

  void setTr();
  void resetTr();
  bool getTr() const {return (psw & 8192) != 0;}

  void setTl();
  void resetTl();
  bool getTl() const {return (psw & 16384) != 0;}

  void setI();
  void resetI();
  bool getI() const {return (psw & 32768) != 0;}

  void incPC();
  void push(int& destinationRegister);
  void pop(int& destinationRegister);
  string PSWbits();

  void preUpdateSourceRegister(int& sourceRegister, int update);
  void postUpdateSourceRegister(int& sourceRegister, int update);

  int getMemoryValue(int address);
  void setMemoryValue(int address, int value);
public:
  void loadMemory();
  void execute();
  void writeOutput();

  Emulator(string i);
  ~Emulator() {}
};

#endif