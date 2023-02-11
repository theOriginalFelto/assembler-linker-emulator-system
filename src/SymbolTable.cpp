#include "../inc/SymbolTable.hpp"

int Symbol::nextNumber = 0;
int Symbol::currentSectionNumber = 0;
std::string SymbolTable::currentSection = "UND";

SymbolTable::SymbolTable(){
  table.emplace("UND", new Symbol("UND", 0, false, false, 0, false));
}

Symbol* SymbolTable::getSymbolByKey(std::string key) {
  std::map<std::string, Symbol*>::iterator it = table.find(key);
  if (it != table.end()) {
    return it->second;
  } else return nullptr;
}

Symbol* SymbolTable::getSymbolByNumber(int num) {
  for (auto const& x: table) {
    if (x.second->number == num) return (x.second);
  }
  return nullptr;
}

bool SymbolTable::updateSectionSize(std::string key, int size) {
  std::map<std::string, Symbol*>::iterator it = table.find(key);
  if (it != table.end()) {
    Symbol* symbol = it->second;
    symbol->size = size;
    return true;
  } else return false;
}

void SymbolTable::addSymbolUsage(std::string key, SymbolUsage usage) {
  std::map<std::string, Symbol*>::iterator it = table.find(key);
  if (it != table.end()) {
    Symbol* symbol = it->second;
    symbol->backpatch.push_back(usage);
  }
}

void SymbolTable::defineAndUpdateSymbol(std::string key, int lc) {
  std::map<std::string, Symbol*>::iterator it = table.find(key);
  if (it != table.end()) {
    Symbol* symbol = it->second;
    symbol->value = lc;
    symbol->sectionNumber = Symbol::currentSectionNumber; 
  }
}

inline std::string boolToString(bool b) {return b ? "true" : "false";}

std::ostream& operator<<(std::ostream& os, const SymbolTable& tabela) {
  Symbol* sym;
  for(auto x: tabela.table) {
    sym = x.second;
    os << sym->name << ':' << sym->sectionNumber << ':' << sym->value << ':' << boolToString(sym->isGlobal) << ':'
     << boolToString(sym->isExtern) << ':' << sym->number << ':' << sym->size << ":\n";
  }
  os << "end symbol table\n";
  return os;
}

bool SymbolTable::isSection(std::string key) {
  Symbol* symbol = getSymbolByKey(key);
  if (symbol == nullptr) return false;
  return symbol->size != -1;
}

void SymbolTable::removeSymbol(std::string key) {
  std::map<std::string, Symbol*>::iterator itr = table.find(key);
  if (itr != table.end()) {
    delete (itr->second);
    table.erase(itr);
  }
}

void SymbolTable::emptyTable() {
  for(std::map<std::string, Symbol*>::iterator itr = table.begin(); itr != table.end(); itr++){
    delete (itr->second);
  }
  table.clear();
}

SymbolTable::~SymbolTable(){
  emptyTable();
}