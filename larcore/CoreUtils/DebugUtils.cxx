
#include "larcore/CoreUtils/DebugUtils.h"

// C/C++ standard libraries
#include <sstream>


//----------------------------------------------------------------------------- 
//--- lar::debug::CallInfo_t
//---
bool lar::debug::CallInfo_t::ParseString(std::string const& s) {
  constexpr auto boo = std::string::npos;
  range_t libraryStr  { boo, boo };
  range_t addressStr  { boo, boo };
  range_t functionStr { boo, boo };
  range_t offsetStr   { boo, boo };
  setAll(s, addressStr, libraryStr, functionStr, offsetStr);
   
  // expected format:
  // libraryName(mangledSymbol+offset) [hexAddress]
  // (+offset optional)
  
  size_t i = s.find('(');
  if (i == boo) return false;
  libraryStr.first = 0;
  while (true) {
    // at all time, if we find a '(', we start over
    // since a '(' can only be after the library name

    libraryStr  = {  0U,   i };
    addressStr  = { boo, boo };
    functionStr = { boo, boo };
    offsetStr   = { boo, boo };
    
    functionStr.first = ++i;
    
    i = s.find_first_of("(+-)", i);
    if (i == boo) return false;
    switch (s[i]) {
      case '(': continue;
      case '+': case '-':
        functionStr.second = i;
        if (s[i] == '+') ++i;
        offsetStr.first = i;
        i = s.find_first_of("()", ++i);
        if (i == boo) return false;
        switch (s[i]) {
          case '(': continue;
          case ')':
            offsetStr.second = i;
            break;
        } // switch (inner)
        break;
      case ')':
         functionStr.second = i;
         break;
    } // switch (outer)
    
    // finish with the address
    i = s.find_first_of("([", ++i);
    if (i == boo) break;
    if (s[i] == '(') continue;
    addressStr.first = ++i;
    
    i = s.find_first_of("(]", i);
    if (s[i] == '(') continue;
    addressStr.second = i;
    
    break;
  } // while (for ever)
   
  setAll(s, addressStr, libraryStr, functionStr, offsetStr);
  return true;
} // lar::debug::CallInfo_t::ParseString()



void lar::debug::CallInfo_t::setAll(
  std::string const& s,
  range_t addressStr, range_t libraryStr,
  range_t functionStr, range_t offsetStr
  )
{
  original = s;
  
  libraryName = extract(s, libraryStr);
  
  mangledFunctionName = extract(s, functionStr);
  demangleFunction();
  
  if (!emptyRange(addressStr)) {
    std::istringstream sstr(extract(s, addressStr));
    sstr >> address;
  }
  
  if (emptyRange(offsetStr)) offset = 0;
  else {
    auto offsetRange = offsetStr;
    if (!emptyRange(offsetRange)) {
      bool neg = (s[offsetRange.first] == '-');
      std::istringstream sstr;
      if (neg || (s[offsetRange.first] == '+')) ++offsetRange.first;
      if (s.substr(offsetRange.first, 2) == "0x") {
         offsetRange.first += 2;
         sstr.setf(std::ios::hex);
      }
      sstr.str(extract(s, offsetRange));
      sstr >> offset;
      if (neg) offset = -offset;
    }
  } // if offset ... else
  
} // lar::debug::CallInfo_t::setAll()

//----------------------------------------------------------------------------- 
