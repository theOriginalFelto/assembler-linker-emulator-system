#ifndef _LINKER_
#define _LINKER_

#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <algorithm>
#include <regex>
#include "Exceptions.hpp"
#include "SymbolTable.hpp"
#include "RelTable.hpp"

using namespace std;

struct Section{
  string name;
  vector<unsigned char> code;
  int startAddress;
  vector<RelocationEntry> relocationEntries;
  vector<int> relocationEntryOffsets;
  int sectionOccurence;
  bool placed;
  Section() {name = "UND"; startAddress = 0; sectionOccurence = 0; placed = false;}
  Section(string n, int start) {name = n; startAddress = start; sectionOccurence = 1; placed = false;}
  Section(string n, vector<unsigned char> c) {name = n; code = c; startAddress = 0; sectionOccurence = 1; placed = false;}
  Section(string n, vector<unsigned char> c, int start) {name = n; code = c; startAddress = start; sectionOccurence = 1; placed = false;}
};

class Linker{
  private:
    vector<string> input;
    string output;
    vector<Section> sections;
    map<int, unsigned char> generatedCode;
    SymbolTable symbolTable;
    SymbolTable localSymbolTable;
    // used to store the section of a symbol (symbol is key, section is value)
    unordered_map<string, string> helperSectionMap;

    bool hex;
    bool relocatable;
  public:
    Linker(vector<string> i, bool hex, bool rel, string o = "linkerOutput.hex");
    ~Linker() {}

    void link();
    void parseSymbol(string line);
    void resolveSymbols(string file);
    void parseRelocationEntry(string line, int sectionIndex, int codeSize);
    void checkForUndefinedSymbols();
    void placeSections();
    void checkForOverlappingSections();
    void packCode();
    void updateSymbolAndRelocationEntryValues();
    void resolveRelocationEntries();
    
    bool sectionExists(string name);
    int getSectionIndex(string name);

    ofstream& formHexOutput(ofstream& output);
    void formHexCout();
};

#endif