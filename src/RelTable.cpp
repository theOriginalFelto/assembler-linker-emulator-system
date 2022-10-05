#include "../inc/RelTable.hpp"

Symbol* RelocationTable::getSymbolFromSymbolTable(std::string key, SymbolTable& symTable) {
  return symTable.getSymbolByKey(key);
}

typedef std::multimap<std::string, RelocationEntry*>::iterator RelTableIterator;

std::ostream& RelocationTable::printSectionRelocationEntries(std::ostream& os, std::string sectionName) {
  std::pair<RelTableIterator, RelTableIterator> result = table.equal_range(sectionName);
  if (result.first != result.second) {
    os << "rel entries\n" << std::dec;
    for (RelTableIterator it = result.first; it != result.second; it++) {
      os << *(it->second);
    }
    os << "end rel entries\n";
  }
  
  return os;
}

inline std::string typeOfUseToString(TypeOfUse t) {
  switch (t) {
  case (TypeOfUse::PC_REL):
    return "PC_REL";
  case (TypeOfUse::SYMBOL):
    return "SYMBOL";
  case (TypeOfUse::SYMBOL_WORD):
    return "SYMBOL_WORD";
  default:
    break;
  }
  return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, const RelocationEntry& re) {
  os << int(re.realocationType) << ':' << re.location << ':' << re.symbolTableReference << ":\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const RelocationTable& tabela) {
  for(auto x: tabela.table) {
    os << x.first << ':' << *(x.second);
  }
  return os;
}

RelocationTable::~RelocationTable() {
  for(std::map<std::string, RelocationEntry*>::iterator itr = table.begin(); itr != table.end(); itr++){
    delete (itr->second);
  }
}