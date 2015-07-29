/**
 * @file   BoxBoundedGeo.cxx
 * @brief  Provides a additional hit information of a line through the box
 * @author Christoph Rudolf von Rohr (crohr@fnal.gov)
 * @date   July 27th, 2015
 */

#include "BoxBoundedGeo.h"

// This algorithm calculates
TVector3 GetEntryPoint(TVector3 TrackOffset, TVector3 TrackDirect)
{
  // Generate Normal vectors and offsets for every plane of the box
  std::array<TVector3,3> NormalVectors;
  std::array<TVector3,6> NormalVectorOffsets;
  
  // There are only 3 plane normal vectors
  NormalVectors.at(0) = (1., 0., 0.); // Cathode and anode
  NormalVectors.at(1) = (0., 1., 0.); // Top and bottom
  NormalVectors.at(2) = (0., 0., 1.); // Upstram and Downstream
  
  NormalVectorOffsets.at(0) = (x_min, y_min, z_min); // Anode offset
  NormalVectorOffsets.at(1) = (x_max, y_min, z_min); // Cathode
  NormalVectorOffsets.at(2) = (x_min, y_max, z_min); // Top
  NormalVectorOffsets.at(3) = (x_min, y_min, z_min); // Bottom
  NormalVectorOffsets.at(4) = (x_min, y_min, z_min); // upstream
  NormalVectorOffsets.at(5) = (x_min, y_min, z_max); // downstream
  
  
  
}
