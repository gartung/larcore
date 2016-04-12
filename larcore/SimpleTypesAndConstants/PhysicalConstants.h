////////////////////////////////////////////////////////////////////////
/// \file  PhysicalConstants.h
/// \brief Collection of Physical constants used in LArSoft
///
///
/// \author brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef UTIL_PHYSICALCONSTANTS_H
#define UTIL_PHYSICALCONSTANTS_H

namespace util {

  // Recombination factor coefficients come from Nucl.Instrum.Meth.A523:275-286,2004
  // R = A/(1 + (dE/dx)*k/Efield)
  // dE/dx is given by the voxel energy deposition, but have to convert it to MeV/cm
  // from GeV/voxel width
  // A = 0.800 +/- 0.003
  // k = 0.0486           needs to be scaled with Electric field.
  // Efield in kV/cm
  constexpr double kRecombA        = 0.8;      ///< see Nucl.Instrum.Meth.A523:275-286,2004
  constexpr double kRecombk        = 0.0486;   ///< in g/(MeVcm^{2})*kV/cm
  constexpr double kModBoxA        = 0.930;     // Modified Box Alpha (ArgoNeuT JINST)
  constexpr double kModBoxB        = 0.212;     // in g/(MeVcm^{2})*kV/cm
 // static constexpr double kWion   = 23.6e-6;  ///< ionization potenial in LAr, 23.6 eV = 1e, Wion in MeV/e

  // Conversion for energy deposited in GeV to number of ionization electrons produced
  constexpr double kGeVToElectrons = 4.237e7; ///< 23.6eV per ion pair, 1e9 eV/GeV

  // More constants
  //static double kc    = 2.99792458e10;   ///< cm/s
  //static double khbar = 6.58211899e-22;  ///< MeVs

  // Conversion factors
  constexpr double kMeterToCentimeter = 1.e2;                  ///< 1 m = 100 cm
  constexpr double kCentimeterToMeter = 1./kMeterToCentimeter;
  constexpr double kMeterToKilometer  = 1.e-3;                 ///< 1000 m = 1 km
  constexpr double kKilometerToMeter  = 1./kMeterToKilometer;

  constexpr double keVToMeV           = 1.e-6;                 ///< 1e6 eV = 1 MeV
  constexpr double kMeVToeV           = 1./keVToMeV;

  constexpr double kBogusD            = -999.;                 ///< obviously bogus double value
  constexpr int    kBogusI            = -999;                  ///< obviously bogus integer value
  constexpr float  kBogusF            = -999.;                 ///< obviously bogus float value

  constexpr double quietCompiler = kBogusD*kBogusI*kBogusF*kRecombA*kRecombk*kGeVToElectrons;
  
  
  /// Returns the constant pi (up to 35 decimal digits of precision)
  template <typename T = double>
  inline constexpr T pi() { return 3.14159265358979323846264338327950288L; }
  
  /// Converts the argument angle from degrees into radians
  template <typename T>
  inline constexpr T DegreesToRadians(T angle) { return angle / 180 * pi<T>(); }
  
  /// Converts the argument angle from radians into degrees (@f$ \pi \rightarrow 180 @f$)
  template <typename T>
  inline constexpr T RadiansToDegrees(T angle) { return angle / pi<T>() * 180; }
  
  
  
} // namespace util

#endif //UTIL_PHYSICALCONSTANTS_H
