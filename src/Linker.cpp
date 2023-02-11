#include "../inc/Linker.hpp"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <iomanip>

unordered_map<string, int> placeOptionMap;


/* friend/helper functions */
string deleteSpaces(string str) {
  str.erase(remove(str.begin(), str.end(), ' '), str.end());
  return str;
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

vector<unsigned char> parseSectionCode(string sectionCode) {
  stringstream sstr(sectionCode);
  vector<unsigned char> vec;
  string substr;
  while (sstr.good()) {
    getline(sstr, substr, ':');
    if (!substr.empty())
      vec.push_back((unsigned char)stoi(substr, nullptr, 16));
  }
  return vec;
}

/* Linker methods */
Linker::Linker(vector<string> i, bool hex, bool rel, string o) {
  if (hex == rel) throw InvalidCmdArgs();
  input = i;
  output = o;
  this->hex = hex;
  this->relocatable = rel;
}

void Linker::link() {
  ifstream ulaz;
  string line;

  for (string x : input) { // parsing symbol tables
    ulaz = ifstream(x);
    if (!ulaz.is_open())
      throw UnknownFileError(x.c_str());
    getline(ulaz, line); //"UND" section
    while (getline(ulaz, line)) {
      if (line == "end symbol table")
        break;
      parseSymbol(line);
    }
    resolveSymbols(x);
    ulaz.close();
  }

  if (this->hex) checkForUndefinedSymbols();

  for (string x : input) {
    ulaz = ifstream(x);
    if (!ulaz.is_open())
      throw UnknownFileError(x.c_str());
    while (getline(ulaz, line)) { // getting to sections
      if (line == "end symbol table")
        break;
    }

    while (getline(ulaz, line)) { //"new section" or "end file"
      if (line == "end file")
        break;
      getline(ulaz, line); // section name
      string secName = line;
      getline(ulaz, line); // section code
      vector<unsigned char> c = parseSectionCode(line);
      int ind;
      
      if ((ind = getSectionIndex(secName)) == -1)
        sections.push_back(Section(secName, c));
      else {
        Symbol* thisSection = symbolTable.getSymbolByKey(secName);
        for(auto& sym: symbolTable.table) {
          if (sym.second->name != secName && sym.second->sectionNumber == thisSection->sectionNumber &&
          sym.second->originFile == x) sym.second->value += sections[ind].code.size();
        }
        sections[ind].code.insert(sections[ind].code.end(), c.begin(), c.end());
        sections[ind].sectionOccurence++;
      }

      getline(ulaz, line); // "rel entries" or "end section"
      if (line == "rel entries") {
        ind = getSectionIndex(secName);
        while (getline(ulaz, line)) {
          if (line == "end rel entries")
            break;
          parseRelocationEntry(line, ind, sections[ind].code.size() - c.size());
        }
        getline(ulaz, line); // to get "end section"
      }
    }
    ulaz.close();
  }
  
  // check when relocatable==true
  if (!placeOptionMap.empty()) {
    placeSections();
    checkForOverlappingSections();
  }
  packCode();
  updateSymbolAndRelocationEntryValues();
  resolveRelocationEntries();

  ofstream izlaz(this->output);
  if (this->hex)
    formHexOutput(izlaz);
  izlaz.close();
}

bool Linker::sectionExists(string name) {
  return (getSectionIndex(name) != -1);
}
int Linker::getSectionIndex(string name) {
  for (int i = 0; i < sections.size(); i++) {
    if (sections[i].name == name)
      return i;
  }
  return -1;
}

void Linker::parseSymbol(string line) {
  stringstream sstr(line);
  string data;
  Symbol symbol;

  getline(sstr, data, ':'); symbol.name = data;
  getline(sstr, data, ':'); symbol.sectionNumber = stoi(data);
  getline(sstr, data, ':'); symbol.value = stoi(data);
  getline(sstr, data, ':'); symbol.isGlobal = ((data == "true") ? true : false);
  getline(sstr, data, ':'); symbol.isExtern = ((data == "true") ? true : false);
  getline(sstr, data, ':'); symbol.number = stoi(data);
  getline(sstr, data, ':'); symbol.size = stoi(data);

  localSymbolTable.table.emplace(symbol.name, new Symbol(symbol));
}

void Linker::resolveSymbols(string file) {
  for(auto& x: localSymbolTable.table) {
    Symbol* symbol = x.second;
    Symbol *symbolFromTable = symbolTable.getSymbolByKey(symbol->name);
    if (symbolFromTable == nullptr) {
      if (symbol->isExtern || symbol->isGlobal) { // no local symbols
        symbolTable.table.emplace(symbol->name, new Symbol(symbol->name, symbol->value, symbol->isGlobal,
        symbol->isExtern, symbol->size, symbol->isGlobal, file));
      }
    }
    else {
      if (symbolFromTable->isOnlyExtern() && symbol->isOnlyGlobal()) { // extern symbolFromTable
        symbolFromTable->isExtern = false;
        symbolFromTable->isGlobal = true;
        symbolFromTable->value = symbol->value;
        symbolFromTable->sectionNumber = Symbol::currentSectionNumber;
        symbolFromTable->originFile = file;
      }
      else if (symbolFromTable->isOnlyGlobal() && symbol->isOnlyGlobal()) { // both global
        throw MultipleSymbolDefinitionError(symbol->name.c_str());
      }
      else if ((symbolFromTable->isSection() && !symbol->isSection()) || (!symbolFromTable->isSection() && symbol->isSection())) { // one is section the other is label
        throw LabelSectionCollisionError(symbol->name.c_str());
      } // else if (symbolFromTable->isOnlyGlobal() && symbol.isOnlyExtern()) <--- check this
    }
    if (!symbol->isSection() && (symbol->isExtern || symbol->isGlobal) && symbol->sectionNumber != 0) {
      helperSectionMap.insert({symbol->name, localSymbolTable.getSymbolByNumber(symbol->sectionNumber)->name});
    }
  }
  localSymbolTable.emptyTable();
}

void Linker::checkForUndefinedSymbols() {
  for(auto& x: symbolTable.table) {
    Symbol* symbol = x.second;
    if ((symbol->isOnlyExtern() || symbol->sectionNumber == 0) && symbol->name != "UND") 
      throw UnresolvedSymbolError(symbol->name.c_str());
  }
}

void Linker::parseRelocationEntry(string line, int sectionIndex, int codeSize) {
  stringstream sstr(line);
  string data;
  RelocationEntry entry;

  getline(sstr, data, ':'); entry.realocationType = (TypeOfUse)stoi(data);
  getline(sstr, data, ':'); entry.location = stoi(data);
  getline(sstr, data, ':'); entry.symbolTableReference = data;

  sections[sectionIndex].relocationEntries.push_back(entry);
  if (sections[sectionIndex].sectionOccurence > 1) 
    sections[sectionIndex].relocationEntryOffsets.push_back(codeSize);
}

inline bool inRange(int num, int low, int high) {
  return num > low && num < high;
}

void Linker::placeSections() {
  for(auto& x: placeOptionMap) {
    int ind = getSectionIndex(x.first);
    if (ind == -1) continue;
    sections[ind].startAddress = x.second;
    sections[ind].placed = true;
  }
}

void Linker::checkForOverlappingSections() {
  for(Section& s1: sections) {
    if (s1.placed) {
      for(Section& s2: sections) {
        if (s2.placed && s1.name != s2.name) {
          if (inRange(s2.startAddress + s2.code.size(), s1.startAddress, s1.startAddress + s1.code.size()) ||
          inRange(s2.startAddress, s1.startAddress, s1.startAddress + s1.code.size()))
            throw OverlappingSectionsError(s1.name.c_str(), s2.name.c_str());
        }
      }
    }
  }
}

void Linker::packCode() {
  int currentAddress = 0;
  for(Section& section: sections) { // putting sections determined with place option first
    if (section.placed) {
      if (currentAddress < section.startAddress || currentAddress == 0) 
        currentAddress = section.startAddress + section.code.size();
      for (int i = 0; i < section.code.size(); i++) {
        generatedCode[section.startAddress + i] = section.code[i];
      }
    }
  }
  for(Section& section: sections) { // other sections afterwards
    if (!section.placed) {
      section.startAddress = currentAddress;
      currentAddress += section.code.size();
      for (int i = 0; i < section.code.size(); i++) {
        generatedCode[section.startAddress + i] = section.code[i];
      }
    }
  }
}

void Linker::updateSymbolAndRelocationEntryValues() {
  for (auto& x: symbolTable.table) {
    Symbol* symbol = x.second;
    if (!symbol->isSection()) {
      Symbol* section = symbolTable.getSymbolByKey(helperSectionMap.at(symbol->name));
      int sectionIndex = getSectionIndex(section->name);
      symbol->value += sections[sectionIndex].startAddress;
    }
  }
  for(Section& s: sections) {
    if (s.sectionOccurence > 1) {
      for (int i = 1; i < s.relocationEntries.size(); i++){
        s.relocationEntries[i].location += s.relocationEntryOffsets[i - 1];
      }
    }
  }
}

void Linker::resolveRelocationEntries() {
  int addition, offset, valueFromCode;
  Symbol* symbol;
  for(Section& section: sections) {
    for(RelocationEntry& entry: section.relocationEntries) {
      symbol = symbolTable.getSymbolByKey(entry.symbolTableReference);
      if (symbol != nullptr) {
        if (symbol->isSection()) addition = sections[getSectionIndex(symbol->name)].startAddress;
        else addition = symbol->value;
      } else continue;
      if (entry.realocationType == TypeOfUse::PC_REL) {
        addition -= (entry.location + section.startAddress);
      }
      offset = section.startAddress + entry.location;
      if (addition != 0) {
        valueFromCode = generatedCode[offset] | (generatedCode[offset + 1] << 8);
        valueFromCode += addition;
        generatedCode[offset] = valueFromCode & 0xFF;
        generatedCode[offset + 1] = ((valueFromCode & 0xFF00)>>8);
      }
    }
  }
}

ofstream& Linker::formHexOutput(ofstream &output) {
  unsigned char byte;
  int prevAddr = -1;
  int gap, spacesLeft;
  bool addrSet = false;
  output << std::hex << uppercase << setfill('0');
  for (auto& x: generatedCode) {
    int addr = x.first;
    if (addr != prevAddr + 1 && prevAddr != -1) { // testing in relation to previous address
      gap = addr - prevAddr;
      spacesLeft = 7 - prevAddr % 8;
      if (spacesLeft < gap) { // if the code cannot fit in one line, break
        if (spacesLeft != 0) {
          output << '\n';
          addrSet = false;
        }
      } else { // else fill the gap with 0x00
        for (int i = 0; i < gap; i++) {
          output << setw(2) << 0x00 << ' ';
        }
      }
    }
    int fillCnt;
    if ((fillCnt = addr % 8) != 0 && !addrSet) { // see if the address doesn't align with 8
      output << setw(4) << addr - fillCnt << ": ";
      for (int i = 0; i < fillCnt; i++) { // fill the gap before the actual data with 0x00
        output << setw(2) << 0x00 << ' ';
      }
      addrSet = true;
    }
    else if (fillCnt == 0 && !addrSet) { // else set the address
      output << setw(4) << addr << ": ";
      addrSet = true;
    }
    byte = x.second;
    output << setw(2) << (int)byte;
    if ((addr + 1) % 8 == 0) {
      output << '\n';
      addrSet = false;
    }
    else output << ' ';
    prevAddr = addr;
  }
  return output;
}

void Linker::formHexCout() {
  unsigned char byte;
  int prevAddr = -1;
  int gap, spacesLeft;
  bool addrSet = false;
  cout << std::hex << uppercase << setfill('0');
  for (auto& x: generatedCode) {
    int addr = x.first;
    if (addr != prevAddr + 1 && prevAddr != -1) { // testing in relation to previous address
      gap = addr - prevAddr;
      spacesLeft = 7 - prevAddr % 8;
      if (spacesLeft < gap) { // if the code cannot fit in one line, break
        if (spacesLeft != 0) {
          cout << '\n';
          addrSet = false;
        }
      } else { // else fill the gap with 0x00
        for (int i = 0; i < gap; i++) {
          cout << setw(2) << 0x00 << ' ';
        }
      }
    }
    int fillCnt;
    if ((fillCnt = addr % 8) != 0 && !addrSet) { // see if the address doesn't align with 8
      cout << setw(4) << addr - fillCnt << ": ";
      for (int i = 0; i < fillCnt; i++) { // fill the gap before the actual data with 0x00
        cout << setw(2) << 0x00 << ' ';
      }
      addrSet = true;
    }
    else if (fillCnt == 0 && !addrSet) { // else set the address
      cout << setw(4) << addr << ": ";
      addrSet = true;
    }
    byte = x.second;
    cout << setw(2) << (int)byte;
    if ((addr + 1) % 8 == 0) {
      cout << '\n';
      addrSet = false;
    }
    else cout << ' ';
    prevAddr = addr;
  }
}

int main(int argc, char* argv[]) {
  try {
    const char* outputOption = "-o";

    const char* hexOption = "-hex";
    bool hex = false;

    const char* relOption = "-relocatable";
    bool rel = false;

    regex placeRegex("^-place=(\\w+)@(\\d+|0x[\\da-fA-F]+)$");
    string placeOption;
    smatch match;

    string output = "linkerOutput.hex";
    string inputFile;
    regex inputRegex("^.*\\.o$");
    vector<string> input;

    Linker* linker = nullptr;

    int ind = 1;
    while (ind < argc) {
      if (strcmp(hexOption, argv[ind]) == 0) {
        if (rel) throw InvalidCmdArgs();
        hex = true;
        ind++;
        continue;
      }
      if (strcmp(relOption, argv[ind]) == 0) {
        if (hex) throw InvalidCmdArgs();
        rel = true;
        ind++;
        continue;
      }
      if (strcmp(outputOption, argv[ind]) == 0) {
        output = argv[ind + 1];
        ind += 2;
        continue;
      }
      placeOption = argv[ind];
      if (regex_search(placeOption, match, placeRegex)) {
        string addr = match[2];
        string sec = match[1];
        placeOptionMap.emplace(sec, stoi(addr.c_str(), nullptr, 0));
        ind++;
        continue;
      }
      inputFile = argv[ind];
      while (regex_search(inputFile, inputRegex)) {
        input.push_back(inputFile);
        if (ind + 1 == argc) {
          ind++;
          break;
        }
        inputFile = argv[++ind];
      }
      if (ind != argc) throw InvalidCmdArgs();
    }

    linker = new Linker(input, hex, rel, output);

    linker->link();

    if (linker != nullptr) delete linker;
  }
  catch(const exception& e) {
    cout << e.what() << '\n';
  }
  return 0;
}