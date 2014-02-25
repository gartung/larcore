//
// A very simple test of PhysicalConstants.h
//
#include <iostream>
#include <iomanip>
#include <sstream>
#include "SimpleTypesAndConstants/PhysicalConstants.h"

int main() {

  int nbad=0;

  std::ostringstream os1, os2, os3, os4;
  os1 << std::setprecision(8) << util::kRecombk;
  os2 << "0.0486";
  if( os1.str() != os2.str() ) {
    std::cout << "compare --" << os1.str() << "-- to --" << os2.str() << "--" << std::endl;
    nbad++;
  }
  os3 << std::fixed << std::showpoint << std::setprecision(3) << util::kModBoxA ;
  os4 << "0.930";
  if( os3.str() != os4.str() ) {
    nbad++;
    std::cout << "compare --" << os3.str() << "--" << std::endl;
    std::cout << "     to --" << os4.str() << "--" << std::endl;
  }

  return nbad;

}
