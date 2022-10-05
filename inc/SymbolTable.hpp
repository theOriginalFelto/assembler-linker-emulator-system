#ifndef _SYMBOLTABLE_
#define _SYMBOLTABLE_

#include <map>
#include <vector>
#include <string>
#include <iostream>

enum class TypeOfUse{
  PC_REL,
  SYMBOL,
  SYMBOL_WORD,
  UNKNOWN
};

struct SymbolUsage{
  int offset;
  std::string sectionName;
  TypeOfUse typeOfUse;
  SymbolUsage() {offset = 0; sectionName = ""; typeOfUse = TypeOfUse::UNKNOWN;}
  SymbolUsage(int off, std::string secName, TypeOfUse t) {
    offset = off;
    sectionName = secName;
    typeOfUse = t;
  }
};

struct Symbol{
  static int nextNumber;
  static int currentSectionNumber;
  std::string name;
  int sectionNumber;
  int value;
  bool isGlobal;
  bool isExtern;
  int number;
  int size; //for sections
  std::vector<SymbolUsage> backpatch;
  std::string originFile; // for linker
  
  Symbol() {name = ""; sectionNumber = 0; value = 0; isGlobal = false; isExtern = false; number = 0; size = -1; originFile = "";}
  Symbol(std::string n, int val, bool isGl, bool isExt, int s, bool isDef, std::string f = "") { //isDef true for sections
    name = n;
    value = val;
    isGlobal = isGl;
    isExtern = isExt;
    number = nextNumber++;
    size = s;
    if (!isDef) sectionNumber = 0;
    else {
      if (s != -1) { //if symbol is a section
        sectionNumber = currentSectionNumber = number;
      }
      else sectionNumber = currentSectionNumber;
    }
    originFile = f;
  }
  Symbol(std::string n, int val, bool isGl, bool isExt, int s, int secNum, std::string f = "") {
    name = n;
    value = val;
    isGlobal = isGl;
    isExtern = isExt;
    number = nextNumber++;
    size = s;
    sectionNumber = secNum;
    originFile = f;
  }
  Symbol(std::string n, int val, bool isGl, bool isExt, int s, bool isDef, SymbolUsage usage, std::string f = "") { //isDef true for sections
    name = n;
    value = val;
    isGlobal = isGl;
    isExtern = isExt;
    number = nextNumber++;
    size = s;
    if (!isDef) sectionNumber = 0;
    else {
      if (s != -1) { //if symbol is a section
        sectionNumber = currentSectionNumber = number;
      }
      else sectionNumber = currentSectionNumber;
    }
    backpatch.push_back(usage);
    originFile = f;
  }
  bool isOnlyExtern() {return isExtern && !isGlobal;}
  bool isOnlyGlobal() {return !isExtern && isGlobal;}
  bool isSection() {return size != -1;}
};

class SymbolTable{
public:
  friend class RelocationTable;

  std::map<std::string, Symbol*> table;
  static std::string currentSection;

  Symbol* getSymbolByKey(std::string key);
  Symbol* getSymbolByNumber(int number);
  bool updateSectionSize(std::string key, int size);
  void addSymbolUsage(std::string key, SymbolUsage usage);
  void defineAndUpdateSymbol(std::string key, int lc);
  bool isSection(std::string key);
  void removeSymbol(std::string key);
  void emptyTable();

  friend std::ostream& operator<<(std::ostream& os, const SymbolTable& table);

  SymbolTable();
  ~SymbolTable();
};

#endif