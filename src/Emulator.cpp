#include "../inc/Emulator.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iomanip>

int getch() {
  unsigned char c;
  if (read(STDIN_FILENO, &c, sizeof(char)) > 0) return c;
  return 0;
}

Emulator::Emulator(string i) : input(i), maxAddress(0), psw(0), interruptRequests(0), terminalBreak(false) {}

void Emulator::loadMemory() {
  ifstream ulaz(input);
  string line;
  if (!ulaz.is_open()) throw UnknownFileError(input.c_str());

  int i = 0;
  while(getline(ulaz, line)) {
    stringstream sstr(line);
    string data;
    int start;
    getline(sstr, data, ':'); 
    start = stoi(data, nullptr, 16);
    getline(sstr, data, ' ');
    for(i = start; i < start + 8; i++) {
      try{
        getline(sstr, data, ' '); 
        memory[i] = (unsigned char)stoi(data, nullptr, 16); 
      } catch (const exception& e) {}
    }
  }
  maxAddress = i;
  ulaz.close();
  r[7] = getMemoryValue(0);
}

void Emulator::execute() {unsigned char ch; int terminalOut;
  while (true) {
    unsigned char opCode = memory[r[7]];
    incPC();
    if (opCode == 0x00) break; //halt instruction
    switch (opCode) {
      case 0x10:
        instructionINT();
        break;
      case 0x20:
        instructionIRET();
        break;
      case 0x30:
        instructionCALL();
        break;
      case 0x40:
        instructionRET();
        break;
      case 0x50:
        instructionJMP();
        break;
      case 0x51:
        instructionJEQ();
        break;
      case 0x52:
        instructionJNE();
        break;
      case 0x53:
        instructionJGT();
        break;
      case 0x60:
        instructionXCHG();
        break;
      case 0x70:
        instructionADD();
        break;
      case 0x71:
        instructionSUB();
        break;
      case 0x72:
        instructionMUL();
        break;
      case 0x73:
        instructionDIV();
        break;
      case 0x74:
        instructionCMP();
        break;
      case 0x80:
        instructionNOT();
        break;
      case 0x81:
        instructionAND();
        break;
      case 0x82:
        instructionOR();
        break;
      case 0x83:
        instructionXOR();
        break;
      case 0x84:
        instructionTEST();
        break;
      case 0x90:
        instructionSHL();
        break;
      case 0x91:
        instructionSHR();
        break;
      case 0xA0:
        instructionLDR();
        break;
      case 0xB0:
        instructionSTR();
        break;
      default:
        throw IllegalOperationCodeError();
        break;
    }
    terminalOut = getMemoryValue(term_out);
    if (terminalOut != 0) {
      printf("%c", (unsigned char)terminalOut);
      terminalBreak = true;
      fflush(stdout);
      setMemoryValue(term_out, 0);
    }
    if ((ch = getch()) != 0) {
      setMemoryValue(term_in, ch);
      interruptRequests |= 0x08; // terminal intr bit
    }
    checkForInterrupts();
  }
}

void Emulator::writeOutput() {
  if (terminalBreak) cout << '\n';
  cout << "------------------------------------------------\n";
  cout << "Emulated processor executed halt instruction    \n";
  cout << "Emulated processor state: psw=0b" << PSWbits() << '\n' << hex;
  for (int i = 0; i < 4; i++) {
    cout << 'r' << i << "=0x" << setfill('0') << setw(4) << r[i];
    if (i != 3) cout << "    ";
    else cout << '\n';
  }
  for (int i = 4; i < 8; i++) {
    cout << 'r' << i << "=0x" << setfill('0') << setw(4) << r[i];
    if (i != 7) cout << "    ";
    else cout << '\n';
  }
}

string Emulator::PSWbits() {
  stringstream sstr;
  unsigned int a = psw;
  for (int i = 0; i < 16; i++){
    sstr << ((a & 0x8000) >> 15);
    a <<= 1;
  }
  return sstr.str();
}

void Emulator::instructionINT() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  push(r[7]);
  push(psw);
  r[7] = getMemoryValue((r[regDescr.destination] % 8) * 2);
}

void Emulator::instructionIRET() {
  pop(psw);
  pop(r[7]);
}

void Emulator::instructionCALL() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  unsigned char addrMode = memory[r[7]];
  incPC();
  int update = (addrMode & 0xF0) >> 4;
  unsigned char dataLow, dataHigh;
  int offset, addition;
  switch (addrMode & 0xF){
    case AddressModes::IMMEDIATE:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      push(r[7]);
      r[7] = dataLow | (dataHigh << 8);
      break;
    case AddressModes::REGISTER_DIRECT:
      push(r[7]);
      r[7] = r[regDescr.source];
      break;
    case AddressModes::REGISTER_INDIRECT:
      push(r[7]);
      preUpdateSourceRegister(r[regDescr.source], update);
      r[7] = getMemoryValue(r[regDescr.source]);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::REGISTER_INDIRECT_OFFSET:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      push(r[7]);
      offset = dataLow | (dataHigh << 8);
      preUpdateSourceRegister(r[regDescr.source], update);
      r[7] = getMemoryValue(r[regDescr.source] + offset);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::MEMORY:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      push(r[7]);
      r[7] = getMemoryValue(dataLow | (dataHigh << 8));
      break;
    case AddressModes::REGISTER_DIRECT_ADDITION:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      push(r[7]);
      addition = dataLow | (dataHigh << 8);
      r[7] += addition;
      r[7] &= 0xFFFF;
      break;
    default:
      throw IllegalInstructionError("jump or call instuction with unknown address mode");
      break;
  }
}

void Emulator::instructionRET() {
  pop(r[7]);
}

void Emulator::instructionJMP() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  unsigned char addrMode = memory[r[7]];
  incPC();
  int update = (addrMode & 0xF0) >> 4;
  unsigned char dataLow, dataHigh;
  int offset, addition;
  switch (addrMode & 0xF){
    case AddressModes::IMMEDIATE:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      r[7] = dataLow | (dataHigh << 8);
      break;
    case AddressModes::REGISTER_DIRECT:
      r[7] = r[regDescr.source];
      break;
    case AddressModes::REGISTER_INDIRECT:
      preUpdateSourceRegister(r[regDescr.source], update);
      r[7] = getMemoryValue(r[regDescr.source]);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::REGISTER_INDIRECT_OFFSET:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      offset = dataLow | (dataHigh << 8);
      preUpdateSourceRegister(r[regDescr.source], update);
      r[7] = getMemoryValue(r[regDescr.source] + offset);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::MEMORY:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      r[7] = getMemoryValue(dataLow | (dataHigh << 8));
      break;
    case AddressModes::REGISTER_DIRECT_ADDITION:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      addition = dataLow | (dataHigh << 8);
      r[7] += addition;
      r[7] &= 0xFFFF;
      break;
    default:
      throw IllegalInstructionError("jump or call instuction with unknown address mode");
      break;
  }
}

void Emulator::instructionJEQ() {
  if (getZ()) instructionJMP();
  else {
    int incCnt;
    unsigned char addrMode = memory[r[7] + 1] & 0xF;
    switch (addrMode)
    {
    case IMMEDIATE:
    case REGISTER_DIRECT_ADDITION:
    case REGISTER_INDIRECT_OFFSET:
    case MEMORY:
      incCnt = 4;
      break;
    case REGISTER_DIRECT:
    case REGISTER_INDIRECT:
      incCnt = 2;
      break;
    }
    for (int i = 0; i < incCnt; i++) incPC();
  }
}

void Emulator::instructionJNE() {
  if (!getZ()) instructionJMP();
  else {
    int incCnt;
    unsigned char addrMode = memory[r[7] + 1] & 0xF;
    switch (addrMode)
    {
    case IMMEDIATE:
    case REGISTER_DIRECT_ADDITION:
    case REGISTER_INDIRECT_OFFSET:
    case MEMORY:
      incCnt = 4;
      break;
    case REGISTER_DIRECT:
    case REGISTER_INDIRECT:
      incCnt = 2;
      break;
    }
    for (int i = 0; i < incCnt; i++) incPC();
  }
}

void Emulator::instructionJGT() {
  if (!getZ() && (getN() == getO())) instructionJMP();
  else {
    int incCnt;
    unsigned char addrMode = memory[r[7] + 1] & 0xF;
    switch (addrMode)
    {
    case IMMEDIATE:
    case REGISTER_DIRECT_ADDITION:
    case REGISTER_INDIRECT_OFFSET:
    case MEMORY:
      incCnt = 4;
      break;
    case REGISTER_DIRECT:
    case REGISTER_INDIRECT:
      incCnt = 2;
      break;
    }
    for (int i = 0; i < incCnt; i++) incPC();
  }
}

void Emulator::instructionXCHG() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  int temp = r[regDescr.destination];
  r[regDescr.destination] = r[regDescr.source];
  r[regDescr.source] = temp & 0xFFFF;
}

void Emulator::instructionADD() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] += r[regDescr.source];
  r[regDescr.destination] &= 0xFFFF;
}

void Emulator::instructionSUB() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] -= r[regDescr.source];
  r[regDescr.destination] &= 0xFFFF;
}

void Emulator::instructionMUL() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] *= r[regDescr.source];
  r[regDescr.destination] &= 0xFFFF;
}

void Emulator::instructionDIV() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  if (r[regDescr.source] == 0) throw IllegalInstructionError("division by zero");
  r[regDescr.destination] /= r[regDescr.source];
  r[regDescr.destination] &= 0xFFFF;
}

void Emulator::instructionCMP() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  int temp = r[regDescr.destination] - r[regDescr.source];
  // update psw
  if (temp == 0) setZ();
  else resetZ();

  if ((temp & 0x8000) != 0) setN();
  else resetN();

  if ((temp >> 16) != 0) setC();
  else resetC();

  int destinationRegisterSign = (r[regDescr.destination] & 0x8000) >> 15;
  int sourceRegisterSign = (r[regDescr.source] & 0x8000) >> 15;
  int resultSign = (temp & 0x8000) >> 15;

  if ((destinationRegisterSign == 0 && sourceRegisterSign == 1 && resultSign == 1) ||
  (destinationRegisterSign == 1 && sourceRegisterSign == 0 && resultSign == 0)) setO();
  else resetO();
}

void Emulator::instructionNOT() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] = ~r[regDescr.destination];
}

void Emulator::instructionAND() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] &= r[regDescr.source];
}

void Emulator::instructionOR() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] |= r[regDescr.source];
}

void Emulator::instructionXOR() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  r[regDescr.destination] ^= r[regDescr.source];
}

void Emulator::instructionTEST() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  int temp = r[regDescr.destination] & r[regDescr.source];
  // update psw
  if (temp == 0) setZ();
  else resetZ();

  if ((temp & 0x8000) != 0) setN();
  else resetN();
}

void Emulator::instructionSHL() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  if ((r[regDescr.source] & 0x8000) != 0) throw IllegalInstructionError("negative shift operand");
  r[regDescr.destination] <<= r[regDescr.source];
  int result = r[regDescr.destination];
  // update psw
  if ((result & 0xFFFF) == 0) setZ();
  else resetZ();

  if ((result & 0x8000) != 0) setN();
  else resetN();

  if ((result >> 16) != 0) setC();
  else resetC();

  r[regDescr.destination] &= 0xFFFF;
}

void Emulator::instructionSHR() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  if ((r[regDescr.source] & 0x8000) != 0) throw IllegalInstructionError("negative shift operand");
  int initialValue = r[regDescr.destination];
  r[regDescr.destination] >>= r[regDescr.source];
  int result = r[regDescr.destination];
  // update psw
  if (result == 0) setZ();
  else resetZ();

  if ((result & 0x8000) != 0) setN();
  else resetN();

  bool cFlagSet = false;
  for (int i = 0; i < r[regDescr.source]; i++) {
    if ((initialValue & 1) == 0) {
      setC();
      cFlagSet = true;
      break;
    }
    initialValue >>= 1;
  }
  if (!cFlagSet) resetC();
}

void Emulator::instructionLDR() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  unsigned char addrMode = memory[r[7]];
  incPC();
  int update = (addrMode & 0xF0) >> 4;
  unsigned char dataLow, dataHigh;
  int offset, addition;
  switch (addrMode & 0xF){
    case AddressModes::IMMEDIATE:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      r[regDescr.destination] = dataLow | (dataHigh << 8);
      break;
    case AddressModes::REGISTER_DIRECT:
      r[regDescr.destination] = r[regDescr.source];
      break;
    case AddressModes::REGISTER_INDIRECT:
      if (update == UpdateModes::POST_INCREMENT & regDescr.source == 6) { //pop instruction
        pop(r[regDescr.destination]);
        return;
      }
      preUpdateSourceRegister(r[regDescr.source], update);
      r[regDescr.destination] = getMemoryValue(r[regDescr.source]);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::REGISTER_INDIRECT_OFFSET:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      offset = dataLow | (dataHigh << 8);
      preUpdateSourceRegister(r[regDescr.source], update);
      r[regDescr.destination] = getMemoryValue(r[regDescr.source] + offset);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::MEMORY:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      r[regDescr.destination] = getMemoryValue(dataLow | (dataHigh << 8));
      break;
    case AddressModes::REGISTER_DIRECT_ADDITION:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      addition = dataLow | (dataHigh << 8);
      r[regDescr.destination] = r[regDescr.source] + addition;
      r[regDescr.destination] &= 0xFFFF;
      break;
    default:
      throw IllegalInstructionError("instuction ldr with unknown address mode");
      break;
  }
}

void Emulator::instructionSTR() {
  RegisterDescription regDescr(memory[r[7]]);
  incPC();
  unsigned char addrMode = memory[r[7]];
  incPC();
  int update = (addrMode & 0xF0) >> 4;
  unsigned char dataLow, dataHigh;
  int offset, addition;
  switch (addrMode & 0xF){
    case AddressModes::IMMEDIATE:
      throw IllegalInstructionError("instruction str with immediate address mode");
      break;
    case AddressModes::REGISTER_DIRECT:
      r[regDescr.source] = r[regDescr.destination];
      break;
    case AddressModes::REGISTER_INDIRECT:
      if (update == UpdateModes::PRE_DECREMENT & regDescr.source == 6) { //push instruction
        push(r[regDescr.destination]);
        return;
      }
      preUpdateSourceRegister(r[regDescr.source], update);
      setMemoryValue(r[regDescr.source], r[regDescr.destination]);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::REGISTER_INDIRECT_OFFSET:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      offset = dataLow | (dataHigh << 8);
      preUpdateSourceRegister(r[regDescr.source], update);
      setMemoryValue(r[regDescr.source] + offset, r[regDescr.destination]);
      postUpdateSourceRegister(r[regDescr.source], update);
      break;
    case AddressModes::MEMORY:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      setMemoryValue(dataLow | (dataHigh << 8), r[regDescr.destination]);
      break;
    case AddressModes::REGISTER_DIRECT_ADDITION:
      dataLow = memory[r[7]];
      incPC();
      dataHigh = memory[r[7]];
      incPC();
      addition = dataLow | (dataHigh << 8);
      r[regDescr.source] = r[regDescr.destination] + addition;
      r[regDescr.source] &= 0xFFFF;
      break;
    default:
      throw IllegalInstructionError("instuction ldr with unknown address mode");
      break;
  }
}

void Emulator::checkForInterrupts() {
  unsigned char intr = interruptRequests;
  if (intr == 0) return;
  unsigned char mask = 2;
  bool interruptMasked = getI();
  for (int i = 1; i < 8; i++) {
    if ((intr & mask) != 0 && !interruptMasked) {
      if ((i == 2 && !getTr()) || (i == 3 && !getTl()) || (i != 2 && i != 3)) {
        push(r[7]);
        push(psw);
        setI();
        r[7] = getMemoryValue(i * 2);
        interruptRequests &= ~(1 << i);
        return;
      }
    }
    mask <<= 1;
  }
}

void Emulator::setZ() {psw |= 1;}
void Emulator::resetZ() {psw &= 0xFFFE;}

void Emulator::setO() {psw |= 2;}
void Emulator::resetO() {psw &= 0xFFFD;}

void Emulator::setC() {psw |= 4;}
void Emulator::resetC() {psw &= 0xFFFB;}

void Emulator::setN() {psw |= 8;}
void Emulator::resetN() {psw &= 0xFFF7;}

void Emulator::setTr() {psw |= 0x2000;}
void Emulator::resetTr() {psw &= 0xDFFF;}

void Emulator::setTl() {psw |= 0x4000;}
void Emulator::resetTl() {psw &= 0xBFFF;}

void Emulator::setI() {psw |= 0x8000;}
void Emulator::resetI() {psw &= 0x7FFF;}

void Emulator::incPC() {
  r[7]++;
  if ((r[7] & 0xFFFF) == 0) throw PCOutOfBoundsError();
}

void Emulator::push(int& destinationRegister) {
  r[6] -= 2;
  if (r[6] <= maxAddress) throw StackOverflow();
  setMemoryValue(r[6], destinationRegister);
}
void Emulator::pop(int& destinationRegister) {
  destinationRegister = getMemoryValue(r[6]);
  r[6] += 2;
}

void Emulator::preUpdateSourceRegister(int& sourceRegister, int update) {
  switch (update)
  {
  case UpdateModes::PRE_DECREMENT:
    sourceRegister -= 2;
    break;
  case UpdateModes::PRE_INCREMENT:
    sourceRegister += 2;
    break;
  default:
    break;
  }
}
void Emulator::postUpdateSourceRegister(int& sourceRegister, int update) {
  switch (update)
  {
  case UpdateModes::POST_DECREMENT:
    sourceRegister -= 2;
    break;
  case UpdateModes::POST_INCREMENT:
    sourceRegister += 2;
    break;
  default:
    break;
  }
}

int Emulator::getMemoryValue(int address) {
  int adr = address & 0xFFFF;
  return memory[adr] | (memory[adr + 1] << 8);
}

void Emulator::setMemoryValue(int address, int value) {
  int adr = address & 0xFFFF;
  memory[adr] = value & 0xFF;
  memory[adr + 1] = (value & 0xFF00) >> 8;
}

int main(int argc, char* argv[]) {
  // terminal configuration
  termios newt, oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~ICANON;
  newt.c_lflag &= ~ECHO;
  newt.c_cc[VTIME] = 0;
  newt.c_cc[VMIN] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  try {
    Emulator* emulator = nullptr;

    if (argc < 2) throw InvalidCmdArgs();
    emulator = new Emulator(argv[1]);

    emulator->loadMemory();
    emulator->execute();
    emulator->writeOutput();

    if (emulator != nullptr) delete emulator;
  }
  catch(const exception& e) {
    cout << e.what() << '\n';
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return 0;
}