#ifndef _greske_h
#define _greske_h_

#include <exception>
#include "math.h"
#include "stdlib.h"
#include <cstring>

class UnknownFileError : public std::exception {
private:
  const char* symbol;
  const char* text = "File error: file \"%s\" does not exist! Please try again.";
  char* ret;
public:
  UnknownFileError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~UnknownFileError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class InvalidCmdArgs : public std::exception {
public:
	virtual const char* what() const throw() {
    return "Invalid command line arguments!";
  }
};

class SyntaxError : public std::exception {
private:
  int lineNum;
  const char* text = "Syntax error at line number: %d.";
  char* ret;
public:
  SyntaxError(int l) : lineNum(l) {
    ret = (char*)malloc((int)((ceil(log10(lineNum))-1+strlen(text))*sizeof(char)));
    sprintf(ret, text, lineNum);
  }
  ~SyntaxError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class NoSectionError : public std::exception {
private:
  int lineNum;
  const char* text = "Symbol defined at line %d belongs to no section. Add a .section directive beforehand.";
  char* ret;
public:
  NoSectionError(int l) : lineNum(l) {
    ret = (char*)malloc((int)((ceil(log10(lineNum))-1+strlen(text))*sizeof(char)));
    sprintf(ret, text, lineNum);
  }
  ~NoSectionError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class LiteralTooBigError : public std::exception {
private:
  int lineNum;
  const char* text = "Literal value at line %d is too big.";
  char* ret;
public:
  LiteralTooBigError(int l) : lineNum(l) {
    ret = (char*)malloc((int)((ceil(log10(lineNum))-1+strlen(text))*sizeof(char)));
    sprintf(ret, text, lineNum);
  }
  ~LiteralTooBigError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class GlobalExternCollision : public std::exception {
private:
  const char* symbol;
  const char* text = "Symbol \"%s\" already declared as extern.";
  char* ret;
public:
  GlobalExternCollision(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~GlobalExternCollision() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class ExternGlobalCollision : public std::exception {
private:
  const char* symbol;
  const char* text = "Symbol \"%s\" already declared as global.";
  char* ret;
public:
  ExternGlobalCollision(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~ExternGlobalCollision() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class SectionDefinedError : public std::exception {
private:
  int lineNum;
  const char* text = "Section defined at line %d has already been defined.";
  char* ret;
public:
  SectionDefinedError(int l) : lineNum(l) {
    ret = (char*)malloc((int)((ceil(log10(lineNum))-1+strlen(text))*sizeof(char)));
    sprintf(ret, text, lineNum);
  }
  ~SectionDefinedError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class ImportingDefinedSymbolError : public std::exception {
private:
  const char* symbol;
  const char* text = "Symbol \"%s\" is defined and cannot be imported using .extern.";
  char* ret;
public:
  ImportingDefinedSymbolError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~ImportingDefinedSymbolError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class DefiningExternSymbolError : public std::exception {
private:
  const char* symbol;
  const char* text = "Cannot define symbol \"%s\" as it is declared as extern.";
  char* ret;
public:
  DefiningExternSymbolError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~DefiningExternSymbolError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class MultipleSymbolDefinitionError : public std::exception {
private:
  const char* symbol;
  const char* text = "Error: Symbol \"%s\" is defined multiple times.";
  char* ret;
public:
  MultipleSymbolDefinitionError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~MultipleSymbolDefinitionError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class LabelSectionCollisionError : public std::exception {
private:
  const char* symbol;
  const char* text = "Error: Symbol \"%s\" is defined both as a section and a label.";
  char* ret;
public:
  LabelSectionCollisionError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~LabelSectionCollisionError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class UnresolvedSymbolError : public std::exception {
private:
  const char* symbol;
  const char* text = "Error: Symbol \"%s\" cannot be resolved.";
  char* ret;
public:
  UnresolvedSymbolError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~UnresolvedSymbolError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class OverlappingSectionsError : public std::exception {
private:
  const char* section1;
  const char* section2;
  const char* text = "Error: Sections \"%s\" and \"%s\" are overlapping due to improper use of -place option.";
  char* ret;
public:
  OverlappingSectionsError(const char* l, const char* k) : section1(l), section2(k) {
    ret = (char*)malloc((int)((strlen(section1)+strlen(section2)+strlen(text) - 1)*sizeof(char))); //-4 for the %s characters
    sprintf(ret, text, section1, section2);
  }
  ~OverlappingSectionsError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

class PCOutOfBoundsError : public std::exception {
public:
	virtual const char* what() const throw() {
    return "Program counter has gotten out of bounds!";
  }
};

class StackOverflow : public std::exception {
public:
	virtual const char* what() const throw() {
    return "Program has stopped because stack overflow was about to happen.";
  }
};

class IllegalOperationCodeError : public std::exception {
public:
	virtual const char* what() const throw() {
    return "Emulator has detected an unknown operation code. Emulation stopped.";
  }
};

class IllegalInstructionError : public std::exception {
private:
  const char* symbol;
  const char* text = "Illegal instruction was about to be executed. Reason: \"%s\".";
  char* ret;
public:
  IllegalInstructionError(const char* l) : symbol(l) {
    ret = (char*)malloc((int)((strlen(symbol)+strlen(text))*sizeof(char))); //-2 for the %s character
    sprintf(ret, text, symbol);
  }
  ~IllegalInstructionError() {
    free(ret);
  }
	virtual const char* what() const throw() {
    return ret;
  }
};

#endif