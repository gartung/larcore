#ifndef RAW_TYPES_H
#define RAW_TYPES_H

namespace raw{

  typedef enum _compress {
    kNone,       ///< no compression 
    kHuffman,    ///< Huffman Encoding
    kZeroSuppression,  ///< Zero Suppression algorithm
    kZeroHuffman,  ///< Zero Suppression followed by Huffman Encoding
    kDynamicDec  ///< Dynamic decimation
  } Compress_t;

  typedef enum _auxdettype {
    kUnknownAuxDet, ///< no idea
    kScintillator,  ///< Scintillator paddle
    kTimeOfFlight,  ///< Time of flight
    kCherenkov      ///< Cherenkov counter
  } AuxDetType_t;

}

#endif
