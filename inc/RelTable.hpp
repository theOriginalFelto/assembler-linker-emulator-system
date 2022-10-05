#ifndef _RELTABLE_
#define _RELTABLE_

#include "SymbolTable.hpp"

struct RelocationEntry{
  TypeOfUse realocationType;
  int location;
  std::string symbolTableReference;
  RelocationEntry() {realocationType = TypeOfUse::UNKNOWN; location = 0; symbolTableReference = "";}
  RelocationEntry(TypeOfUse type, int loc, std::string symbolTableReference) {realocationType = type; location = loc; this->symbolTableReference = symbolTableReference;}
  friend std::ostream& operator<<(std::ostream& os, const RelocationEntry& re);
};

class RelocationTable{
public:
  friend class SymbolTable;

  std::multimap<std::string, RelocationEntry*> table;

  Symbol* getSymbolFromSymbolTable(std::string key, SymbolTable& symTable);
  std::ostream& printSectionRelocationEntries(std::ostream& os, std::string sectionName);

  friend std::ostream& operator<<(std::ostream& os, const RelocationTable& table);


  RelocationTable() {}
  ~RelocationTable();
};

#endif