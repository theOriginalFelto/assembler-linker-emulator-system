#include "../inc/Asembler.hpp"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <unordered_map>

int Asembler::lineCnt = 1;
int Asembler::lc = 0;
int Asembler::sectionIndexForGeneratedCode = 0;
unordered_map<Instructions, unsigned char> instructionOperationCodes = {
  {HALT, 0x00},
  {INT, 0x10},
  {IRET, 0x20},
  {CALL, 0x30},
  {RET, 0x40},
  {JMP, 0x50},
  {JEQ, 0x51},
  {JNE, 0x52},
  {JGT, 0x53},
  {PUSH, 0xb0},
  {POP, 0xa0},
  {XCHG, 0x60},
  {ADD, 0x70},
  {SUB, 0x71},
  {MUL, 0x72},
  {DIV, 0x73},
  {CMP, 0x74},
  {NOT, 0x80},
  {AND, 0x81},
  {OR, 0x82},
  {XOR, 0x83},
  {TEST, 0x84},
  {SHL, 0x90},
  {SHR, 0x91},
  {LDR, 0xa0},
  {STR, 0xb0}
};

/* friend/helper functions */
string deleteSpaces(string str){
  str.erase(remove(str.begin(), str.end(), ' '), str.end());
  return str;
}

const std::string WHITESPACE = " \n\r\t\f\v";
 
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
 
std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

vector<string> commaSeparatedListToVector(string list) {
  stringstream sstr(list);
  vector<string> vec;
  while (sstr.good()) {
    string substr;
    getline(sstr, substr, ',');
    vec.push_back(substr);
  }
  return vec;
}

Directives enumerateDirective(string dir){
  if (dir == "global") return GLOBAL;
  if (dir == "extern") return EXTERN;
  if (dir == "section") return SECTION;
  if (dir == "word") return WORD;
  if (dir == "skip") return SKIP;
  if (dir == "ascii") return ASCII;
  if (dir == "end") return END;
  return Directives::INVALID;
}

Instructions enumerateInstruction(string instr) {
  if (instr == "halt") return HALT;
  if (instr == "int") return INT;
  if (instr == "iret") return IRET;
  if (instr == "call") return CALL;
  if (instr == "ret") return RET;
  if (instr == "jmp") return JMP;
  if (instr == "jeq") return JEQ;
  if (instr == "jne") return JNE;
  if (instr == "jgt") return JGT;
  if (instr == "push") return PUSH;
  if (instr == "pop") return POP;
  if (instr == "xchg") return XCHG;
  if (instr == "add") return ADD;
  if (instr == "sub") return SUB;
  if (instr == "mul") return MUL;
  if (instr == "div") return DIV;
  if (instr == "cmp") return CMP;
  if (instr == "not") return NOT;
  if (instr == "and") return AND;
  if (instr == "or") return OR;
  if (instr == "xor") return XOR;
  if (instr == "test") return TEST;
  if (instr == "shl") return SHL;
  if (instr == "shr") return SHR;
  if (instr == "ldr") return LDR;
  if (instr == "str") return STR;
  return Instructions::ERROR;
}

unsigned char parseDestRegister(smatch match) {
  int regDest;
  string arbitrary = match[2];
  if (match[2].length() != 0) regDest = stoi(arbitrary);
  else {
    if (match[1] == "pc") regDest = 7;
    else if (match[1] == "sp") regDest = 6;
    else regDest = 8;
  }
  regDest <<= 4;
  regDest |= 0xF;
  return (unsigned char)regDest;
}

unsigned char parseRegisters(smatch match) {
  int regDescr = 0;
  int regDest, regSrc;
  string arbitrary = match[2];
  if (match[2].length() != 0) regDest = stoi(arbitrary);
  else {
    if (match[1] == "pc") regDest = 7;
    else if (match[1] == "sp") regDest = 6;
    else regDest = 8;
  }
  arbitrary = match[4];
  if (match[4].length() != 0) regSrc = stoi(arbitrary);
  else {
    if (match[3] == "pc") regSrc = 7;
    else if (match[3] == "sp") regSrc = 6;
    else regSrc = 8;
  }
  regDest <<= 4;
  regDescr |= regDest;
  regDescr |= regSrc;
  return (unsigned char)regDescr;
}

Operand parseDataOperand(string operand) {
  regex reg;
  smatch match;
  int regNumber;
  string op;

  reg = ("^\\$(\\d+|0x[0-9a-fA-F]+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::IMMEDIATE_LITERAL, match[1]);

  reg = ("^\\$(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::IMMEDIATE_SYMBOL, match[1]);

  reg = ("^%(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::PC_RELATIVE, match[1]);

  reg = ("^\\[(r([0-7])|pc|sp|psw)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT, string(""), regNumber);
    } 
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT, string(""), stoi(op));}
  }

  reg = ("^\\[(r([0-7])|pc|sp|psw) \\+ (\\d+|0x[0-9a-fA-F]+)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL, match[3], regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL, match[3], stoi(op));}
  }

  reg = ("^\\[(r([0-7])|pc|sp|psw) \\+ (\\w+)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL, match[3], regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL, match[3], stoi(op));}
  }

  reg = ("^(r([0-7])|pc|sp|psw)$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_DIRECT, string(""), regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_DIRECT, string(""), stoi(op));}
  }

  reg = ("^(\\d+|0x[0-9a-fA-F]+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::MEMORY_DIRECT_LITERAL, match[1]);

  reg = ("^(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::MEMORY_DIRECT_SYMBOL, match[1]);

  return Operand();
}

Operand parseAddressOperand(string operand) {
  regex reg;
  smatch match;
  int regNumber;
  string op;

  reg = ("^\\*(r([0-7])|pc|sp|psw)$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_DIRECT, string(""), regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_DIRECT, string(""), stoi(op));}
  }
    
  reg = ("^\\*(\\d+|0x[0-9a-fA-F]+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::MEMORY_DIRECT_LITERAL, match[1]);

  reg = ("^\\*(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::MEMORY_DIRECT_SYMBOL, match[1]);

  reg = ("^%(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::PC_RELATIVE, match[1]);

  reg = ("^\\*\\[(r([0-7])|pc|sp|psw)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT, string(""), regNumber);
    } 
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT, string(""), stoi(op));}
  }

  reg = ("^\\*\\[(r([0-7])|pc|sp|psw) \\+ (\\d+|0x[0-9a-fA-F]+)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL, match[3], regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL, match[3], stoi(op));}
  }

  reg = ("^\\*\\[(r([0-7])|pc|sp|psw) \\+ (\\w+)\\]$");
  if (regex_search(operand, match, reg)) {
    if (match[2].length() == 0) {
      if (match[1] == "pc") regNumber = 7;
      else if (match[1] == "sp") regNumber = 6;
      else regNumber = 8;
      return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL, match[3], regNumber);
    }
    else {op = match[2]; return Operand(AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL, match[3], stoi(op));}
  }

  reg = ("^(\\d+|0x[0-9a-fA-F]+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::IMMEDIATE_LITERAL, match[1]);

  reg = ("^(\\w+)$");
  if (regex_search(operand, match, reg)) return Operand(AddressModes::IMMEDIATE_SYMBOL, match[1]);

  return Operand();
}

unsigned char getAddressModeValue(AddressModes mode) {
  unsigned char ret;
  switch (mode){
    case AddressModes::IMMEDIATE_LITERAL:
    case AddressModes::IMMEDIATE_SYMBOL:
      ret = 0x00;
      break;
    case AddressModes::MEMORY_DIRECT_LITERAL:
    case AddressModes::MEMORY_DIRECT_SYMBOL:
      ret = 0x04;
      break;
    case AddressModes::PC_RELATIVE:
    case AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL:
    case AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL:
      ret = 0x03;
      break;
    case AddressModes::REGISTER_DIRECT:
      ret = 0x01;
      break;
    case AddressModes::REGISTER_INDIRECT:
      ret = 0x02;
      break;
    default:
      ret = 0;
      break;
  }
  return ret;
}

unsigned char getAddressModeValueForJumpAddress(AddressModes mode) {
  unsigned char ret;
  switch (mode){
    case AddressModes::IMMEDIATE_LITERAL:
    case AddressModes::IMMEDIATE_SYMBOL:
      ret = 0x00;
      break;
    case AddressModes::MEMORY_DIRECT_LITERAL:
    case AddressModes::MEMORY_DIRECT_SYMBOL:
      ret = 0x04;
      break;
    case AddressModes::PC_RELATIVE:
      ret = 0x05;
      break; 
    case AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL:
    case AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL:
      ret = 0x03;
      break;
    case AddressModes::REGISTER_DIRECT:
      ret = 0x01;
      break;
    case AddressModes::REGISTER_INDIRECT:
      ret = 0x02;
      break;
    default:
      ret = 0;
      break;
  }
  return ret;
}

void switchDataOperand(Operand op, Asembler* asem, unsigned char opCode, unsigned char regDest) {
  int value;
  unsigned char addrMode = getAddressModeValue(op.mode);
  unsigned char regDescr;
  Symbol* symbol;
  string currSec = string(SymbolTable::currentSection);
  switch (op.mode){
  case AddressModes::IMMEDIATE_LITERAL:
  case AddressModes::MEMORY_DIRECT_LITERAL:
    value = stoi(op.operand, nullptr, 0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDest);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value & 0xFF);
    value >>= 8;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value);
    asem->lc+=5;
    break;
  case AddressModes::IMMEDIATE_SYMBOL:
  case AddressModes::MEMORY_DIRECT_SYMBOL:
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDest);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->lc+=2;
    break;
  case AddressModes::PC_RELATIVE: // here it's like register_indirect
    regDest &= 0xF0;
    regDescr = regDest | 0x07; //pc as source register
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDescr);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::PC_REL));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::PC_REL)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFE);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFF);
    asem->lc+=2;
    break;
  case AddressModes::REGISTER_DIRECT:
  case AddressModes::REGISTER_INDIRECT:
    regDest &= 0xF0;
    regDescr = regDest | op.regNumber;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDescr);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;
    break;
  case AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL:
    value = stoi(op.operand, nullptr, 0);
    regDest &= 0xF0;
    regDescr = regDest | op.regNumber;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDescr);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value & 0xFF);
    value >>= 8;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value);
    asem->lc+=5;
    break;
  case AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL:
    regDest &= 0xF0;
    regDescr = regDest | op.regNumber;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(regDescr);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->lc+=2;
    break;
  default:
    break;
  }
}

void switchAddressOperand(Operand op, Asembler* asem, unsigned char opCode) {
  int value;
  unsigned char addrMode = getAddressModeValueForJumpAddress(op.mode);
  unsigned char regDescr;
  string currSec = string(SymbolTable::currentSection);
  Symbol* symbol;
  switch (op.mode){
  case AddressModes::IMMEDIATE_LITERAL:
  case AddressModes::MEMORY_DIRECT_LITERAL:
    value = stoi(op.operand, nullptr, 0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFF);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value & 0xFF);
    value >>= 8;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value);
    asem->lc+=5;
    break;
  case AddressModes::IMMEDIATE_SYMBOL:
  case AddressModes::MEMORY_DIRECT_SYMBOL:
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFF);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->lc+=2;
    break;
  case AddressModes::PC_RELATIVE:
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xF7); //pc register
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::PC_REL));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::PC_REL)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFE);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0xFF);
    asem->lc+=2;
    break;
  case AddressModes::REGISTER_DIRECT:
  case AddressModes::REGISTER_INDIRECT:
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(op.regNumber | 0xF0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;
    break;
  case AddressModes::REGISTER_INDIRECT_OFFSET_LITERAL:
    value = stoi(op.operand, nullptr, 0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(op.regNumber | 0xF0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value & 0xFF);
    value >>= 8;
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(value);
    asem->lc+=5;
    break;
  case AddressModes::REGISTER_INDIRECT_OFFSET_SYMBOL:
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(opCode);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(op.regNumber | 0xF0);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(addrMode);
    asem->lc+=3;

    symbol = asem->symbolTable.getSymbolByKey(op.operand);
    if (symbol != nullptr) symbol->backpatch.push_back(SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD));
    else asem->symbolTable.table.emplace(op.operand, new Symbol(op.operand, 0, false, false, -1, false, SymbolUsage(asem->lc, currSec, TypeOfUse::SYMBOL_WORD)));
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->generatedCode[asem->sectionIndexForGeneratedCode].push_back(0x00);
    asem->lc+=2;
    break;
  default:
    break;
  }
}

int switchInstruction(Instructions instr, string line, Asembler* as) {
  regex reg;
  smatch match;
  unsigned char operationCode = instructionOperationCodes.at(instr);
  unsigned char regDest;
  unsigned char regDescr;
  Operand op;

  switch (instr){
    case HALT:
    case IRET:
    case RET:
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->lc++;
      break;
    case INT:
      reg = ("^\\s*int (r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDest = parseDestRegister(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDest);
      as->lc += 2;
      break;
    case CALL:
    case JMP:
    case JEQ:
    case JNE:
    case JGT:
      reg = ("^\\s*(?:call|jmp|jeq|jne|jgt) ([ \\w$%\\[\\]\\+\\*]+)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      op = parseAddressOperand(trim(match[1]));
      if (op.mode == AddressModes::INVALID) throw SyntaxError(as->lineCnt);
      switchAddressOperand(op, as, operationCode);
      break;
    case PUSH:
      reg = ("^\\s*push (r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseDestRegister(match);
      regDescr &= 0xF0;
      regDescr |= 0x06;
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(0x12); //addrMode
      as->lc+=3;
      break;
    case POP:
      reg = ("^\\s*pop (r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseDestRegister(match);
      regDescr &= 0xF0;
      regDescr |= 0x06;
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(0x42); //addrMode
      as->lc+=3;
      break;
    case XCHG:
      reg = ("^\\s*xchg (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case ADD:
      reg = ("^\\s*add (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case SUB:
      reg = ("^\\s*sub (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case MUL:
      reg = ("^\\s*mul (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case DIV:
      reg = ("^\\s*div (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case CMP:
      reg = ("^\\s*cmp (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case NOT:
      reg = ("^\\s*not (r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDest = parseDestRegister(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDest);
      as->lc += 2;
      break;
    case AND:
      reg = ("^\\s*and (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case OR:
      reg = ("^\\s*or (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case XOR:
      reg = ("^\\s*xor (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case TEST:
      reg = ("^\\s*test (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case SHL:
      reg = ("^\\s*shl (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case SHR:
      reg = ("^\\s*shr (r([0-7])|pc|sp|psw)\\s*,\\s*(r([0-7])|pc|sp|psw)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDescr = parseRegisters(match);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(operationCode);
      as->generatedCode[as->sectionIndexForGeneratedCode].push_back(regDescr);
      as->lc += 2;
      break;
    case LDR:
      reg = ("^\\s*ldr (r([0-7])|pc|sp|psw)\\s*,\\s*([\\w $%\\[\\]\\+\\*]+)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDest = parseDestRegister(match);
      op = parseDataOperand(trim(match[3]));
      if (op.mode == AddressModes::INVALID) throw SyntaxError(as->lineCnt);
      switchDataOperand(op, as, operationCode, regDest);
      break;
    case STR:
      reg = ("^\\s*str (r([0-7])|pc|sp|psw)\\s*,\\s*([\\w $%\\[\\]\\+\\*]+)\\s*(?:#.*)?$");
      if (!regex_search(line, match, reg)) throw SyntaxError(as->lineCnt);
      regDest = parseDestRegister(match);
      op = parseDataOperand(trim(match[3]));
      if (op.mode == AddressModes::INVALID) throw SyntaxError(as->lineCnt);
      switchDataOperand(op, as, operationCode, regDest);
      break;
    case ERROR:
      break;
    default:
      break;
  }
  return int(instr);
}

int getSectionIndexOfASymbol(Asembler* as, int secNum) {
  Symbol* section = as->symbolTable.getSymbolByNumber(secNum);
  auto itr = find(as->sections.begin(), as->sections.end(), section->name);
  if (itr != as->sections.end()) {
    return (itr - as->sections.begin());
  } else return -1;
}

int getSectionIndex(Asembler* as, string secName) {
  auto itr = find(as->sections.begin(), as->sections.end(), secName);
  if (itr != as->sections.end()) {
    return (itr - as->sections.begin());
  } else return -1;
}

/* Assembler methods */
void Asembler::assemble() {
  ifstream ulaz(this->input);
  ofstream izlaz(this->output);

  string line;
  int ret;
  if (!ulaz.is_open()) throw UnknownFileError(this->input.c_str());
  if (!izlaz.is_open()) throw UnknownFileError(this->output.c_str());
  
  regex comment("^\\s*#.*\\s*$");

  while (getline(ulaz, line)) { //assembly pass
    if (line == "\n" || line.empty() || regex_search(line, comment) || line.find_first_not_of(' ') == string::npos) {
      this->lineCnt++; 
      continue;
    }

    ret = processLine(line);
    if (ret == static_cast<int>(Directives::END)) break;
    if (ret == 0) throw SyntaxError(this->lineCnt);

    this->lineCnt++;
  }
  ulaz.close();
  ret = backpatchAndGenerateRealocationEntries();
  if (ret != 0) throw SyntaxError(this->lineCnt);
  formOutput(izlaz);
  izlaz.close();
}

int Asembler::processLine(string line) {
  regex reg;
  smatch match;

  reg = ("^\\s*(\\w*):\\s*(#.*)?$");
  if (regex_search(line, match, reg)) return processLabelOnly(match[1]);

  reg = ("^\\s*\\.");
  if (regex_search(line, reg)) return processDirective(line);

  reg = ("^\\s*.*:\\s*[a-z]{3,4}");
  if (regex_search(line, reg)) return processInstructionWithLabel(line);

  reg = ("^\\s*[a-z]{3,4}");
  if (regex_search(line, reg)) return processInstruction(line);

  return 0;
}

int Asembler::processLabelOnly(string label) {
  if (Symbol::currentSectionNumber == 0) throw NoSectionError(this->lineCnt);

  Symbol* symbol = symbolTable.getSymbolByKey(label);
  if (symbol == nullptr) {
    symbolTable.table.emplace(label, new Symbol(label, lc, false, false, -1, true));
  } else {
    if (symbol->isExtern) throw DefiningExternSymbolError((symbol->name).c_str());
    symbol->value = lc;
    symbol->sectionNumber = Symbol::currentSectionNumber;
  }

  return 1;
}

int Asembler::processDirective(string line) {
  regex reg;
  smatch match;
  string literal;
  string ascii;

  reg = ("^\\s*\\.([a-z]{3,7})");
  regex_search(line, match, reg);
  Directives dir = enumerateDirective(match[1]);
  if ((dir == WORD || dir == SKIP || dir == ASCII) && Symbol::currentSectionNumber == 0) 
    throw NoSectionError(this->lineCnt);

  switch (dir){
    case GLOBAL:
      reg = ("^\\s*\\.global ((\\w*)(\\s*,\\s*\\w*)*)\\s*(#.*)?$");
      if (regex_search(line, match, reg)) {
        string symbolList = deleteSpaces(match[1]);
        vector<string> symbols = commaSeparatedListToVector(symbolList);
        for (size_t i = 0; i < symbols.size(); i++){
          Symbol* symbol = symbolTable.getSymbolByKey(symbols[i]);
          if (symbol != nullptr) {
            if (symbol->isExtern) throw GlobalExternCollision(symbol->name.c_str());
            symbol->isGlobal = true;
          } else {
            symbolTable.table.emplace(symbols[i], new Symbol(symbols[i], 0, true, false, -1, false));
          }
        }
      }
      else dir = INVALID;
      break;
    case EXTERN:
      reg = ("^\\s*\\.extern ((\\w*)(\\s*,\\s*\\w*)*)\\s*(#.*)?$");
      if (regex_search(line, match, reg)) {
        string symbolList = deleteSpaces(match[1]);
        vector<string> symbols = commaSeparatedListToVector(symbolList);
        for (size_t i = 0; i < symbols.size(); i++){
          Symbol* symbol = symbolTable.getSymbolByKey(symbols[i]);
          if (symbol != nullptr) {
            if (symbol->isGlobal) throw ExternGlobalCollision((symbol->name).c_str());
            if (symbol->sectionNumber != 0) throw ImportingDefinedSymbolError((symbol->name).c_str());
            symbol->isExtern = true;
          } else {
            symbolTable.table.emplace(symbols[i], new Symbol(symbols[i], 0, false, true, -1, false));
          }
        }
      }
      else dir = INVALID;
      break;
    case SECTION:
      reg = ("^\\s*\\.section (\\w*)\\s*(#.*)?$");
      if (regex_search(line, match, reg)) {
        if (SymbolTable::currentSection != "UND") {
          symbolTable.updateSectionSize(SymbolTable::currentSection, lc);
          sectionIndexForGeneratedCode++;
        }

        if (symbolTable.getSymbolByKey(match[1]) != nullptr) throw SectionDefinedError(this->lineCnt);
        symbolTable.table.emplace(match[1], new Symbol(match[1], 0, true, true, 0, true));
        SymbolTable::currentSection = match[1];
        
        sections.push_back(match[1]);
        generatedCode.push_back(vector<unsigned char>(0,0));
        lc = 0;
      }
      else dir = INVALID;
      break;
    case WORD:
      reg = ("^\\s*\\.word ((?:\\d+|0x[\\da-fA-F]+|\\w+)(?:\\s*,\\s*(?:\\d+|0x[\\da-fA-F]+|\\w)+)*)\\s*(?:#.*)?$");
      if (regex_search(line, match, reg)) {
        string list = deleteSpaces(match[1]);
        vector<string> init = commaSeparatedListToVector(list);
        bool stoiSuccessful;
        unsigned int literalValue;
        for(string x: init) {
          stoiSuccessful = true;
          try {
            literalValue = stoi(x);
          }
          catch(const exception& e) {
            stoiSuccessful = false;
          }
          if (stoiSuccessful) {
            if (literalValue > 65.535) throw LiteralTooBigError(this->lineCnt);
            generatedCode[sectionIndexForGeneratedCode].push_back(literalValue & 0xFF);
            literalValue >>= 8;
            generatedCode[sectionIndexForGeneratedCode].push_back(literalValue & 0xFF);
            lc += 2;
          } else {
            Symbol* symbol = symbolTable.getSymbolByKey(x);
            if (symbol == nullptr) {
              symbolTable.table.emplace(x, new Symbol(x, 0, false, false, -1, false, SymbolUsage(lc, SymbolTable::currentSection, TypeOfUse::SYMBOL_WORD)));
            } else {
              symbol->backpatch.push_back(SymbolUsage(lc, SymbolTable::currentSection, TypeOfUse::SYMBOL_WORD));
            }
            generatedCode[sectionIndexForGeneratedCode].push_back(0x00);
            generatedCode[sectionIndexForGeneratedCode].push_back(0x00);
            lc+=2;
          }
        }
      }
      else dir = INVALID;
      break;
    case ASCII:
      reg = ("^\\s*\\.ascii \"(.+)\"\\s*(?:#.*)?$");
      if (regex_search(line, match, reg)) {
        ascii = match[1];
        for (size_t i = 0; i < ascii.length(); i++){
          this->generatedCode[sectionIndexForGeneratedCode].push_back((unsigned char)ascii[i]);
        }
        lc += ascii.length();
      }
      else dir = INVALID;
      break;
    case SKIP:
      reg = ("^\\s*\\.skip (\\d*)\\s*(?:#.*)?$");
      if (regex_search(line, match, reg)) {
        literal = match[1];
        int skipCnt = stoi(literal);
        for (size_t i = 0; i < skipCnt; i++){
          this->generatedCode[sectionIndexForGeneratedCode].push_back(0x00);
        }
        lc+=skipCnt;
      }
      else dir = INVALID;
      break;
    case END:
      symbolTable.updateSectionSize(SymbolTable::currentSection, lc);
      break;
    case INVALID:
      break;
    default:
      break;
  }
  return int(dir);
}

int Asembler::processInstructionWithLabel(string line) {
  if (Symbol::currentSectionNumber == 0) throw NoSectionError(this->lineCnt);
  regex reg;
  smatch match;

  reg = ("^\\s*(.*):");
  regex_search(line, match, reg);

  Symbol* symbol = symbolTable.getSymbolByKey(match[1]);
  if (symbol == nullptr) {
    symbolTable.table.emplace(match[1], new Symbol(match[1], lc, false, false, -1, true));
  } else {
    if (symbol->isExtern) throw ImportingDefinedSymbolError((symbol->name).c_str());
    symbol->value = lc;
    symbol->sectionNumber = Symbol::currentSectionNumber;
  }

  line = regex_replace(line, regex("^\\s*.*:"), string(""));
  reg = ("^\\s*([a-z]{3,4})");
  regex_search(line, match, reg);
  Instructions instr = enumerateInstruction(match[1]);

  return switchInstruction(instr, line, this);
}

int Asembler::processInstruction(string line) {
  if (Symbol::currentSectionNumber == 0) throw NoSectionError(this->lineCnt);
  regex reg;
  smatch match;

  reg = ("^\\s*([a-z]{3,4})");
  regex_search(line, match, reg);
  Instructions instr = enumerateInstruction(match[1]);

  return switchInstruction(instr, line, this);
}

int Asembler::backpatchAndGenerateRealocationEntries() {
  int secIndex, offset, value;
  string symbolTableRef;
  for(const auto& x: symbolTable.table) {
    if (x.second->isSection()) continue;
    value = x.second->value;
    for (const auto& usage: x.second->backpatch) {
      offset = usage.offset;
      if (!x.second->isGlobal && !x.second->isExtern) {
        secIndex = getSectionIndex(this, usage.sectionName);
        int valueFromCode = (generatedCode[secIndex][offset + 1] << 8) | generatedCode[secIndex][offset];
        valueFromCode += value;
        generatedCode[secIndex][offset++] = valueFromCode & 0xFF;
        generatedCode[secIndex][offset] = ((valueFromCode & 0xFF00)>>8);
        symbolTableRef = symbolTable.getSymbolByNumber(x.second->sectionNumber)->name;
      } else symbolTableRef = x.second->name;
      //rel entry
      switch (usage.typeOfUse){
        case TypeOfUse::SYMBOL_WORD:
        case TypeOfUse::PC_REL:
          relocationTable.table.emplace(usage.sectionName, new RelocationEntry(usage.typeOfUse, usage.offset, symbolTableRef));
          break;
        case TypeOfUse::UNKNOWN:
          return -1;
          break;
        default:
          break;
      }
    }
  }
  return 0;
}

void Asembler::formOutput(ofstream& output) {
  unsigned char byte;
  output << symbolTable << hex << uppercase;

  for (int i = 0; i < sectionIndexForGeneratedCode + 1; i++){
    output << "new section\n" << sections[i] << "\n" << hex;
    for (int j = 0; j < generatedCode[i].size(); j++){
      byte = generatedCode[i][j];
      if (byte < 16 && byte >= 0) output << 0 << (int)byte << ':';
      else output << (int)byte << ':';
    }
    output << '\n';
    relocationTable.printSectionRelocationEntries(output, sections[i]);
    output << "end section\n";
  }
  output << "end file\n";
}

int main(int argc, char* argv[]) {
  try {
    const char* op = "-o";
    Asembler* asembler = nullptr;

    if (argc < 2) throw InvalidCmdArgs();
    if (argc == 2) {
      asembler = new Asembler(argv[1]);
    } else if (strcmp(argv[1], op) == 0) {
      asembler = new Asembler(argv[3], argv[2]);
    } else throw InvalidCmdArgs();

    asembler->assemble();

    if (asembler != nullptr) delete asembler;
  }
  catch(const exception& e) {
    cout << e.what() << '\n';
  }
  return 0;
}