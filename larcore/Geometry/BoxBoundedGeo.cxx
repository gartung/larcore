/**
 * @file   BoxBoundedGeo.cxx
 * @brief  Provides a additional hit information of a line through the box
 * @author Christoph Rudolf von Rohr (crohr@fnal.gov)
 * @date   July 27th, 2015
 */

#include "larcore/Geometry/BoxBoundedGeo.h"

namespace geo
{
  std::vector<TVector3> BoxBoundedGeo::GetIntersections(TVector3 const& TrajectoryStart, TVector3 const& TrajectoryDirect) const
  {
    std::vector<TVector3> IntersectionPoints;
    std::vector<double> LineParameters;
    
    // Generate normal vectors and offsets for every plane of the box
    static std::array<TVector3,6> NormalVectors;
    static std::array<TVector3,6> NormalVectorOffsets;

  
    // All normal vectors are headed outwards
    NormalVectors.at(0) = TVector3(-1., 0., 0.); // Anode normal vector
    NormalVectors.at(1) = TVector3(1., 0., 0.);  // Cathode
    NormalVectors.at(2) = TVector3(0., -1., 0.); // Bottom
    NormalVectors.at(3) = TVector3(0., 1., 0.);  // Top
    NormalVectors.at(4) = TVector3(0., 0., -1.); // Upstram
    NormalVectors.at(5) = TVector3(0., 0., 1.);  // Downstream
    
    // Fill offset vectors  
    NormalVectorOffsets.at(0) = TVector3(c_min[0], c_min[1], c_min[2]); // Anode offset
    NormalVectorOffsets.at(1) = TVector3(c_max[0], c_min[1], c_min[2]); // Cathode
    NormalVectorOffsets.at(2) = TVector3(c_min[0], c_min[1], c_min[2]); // Bottom
    NormalVectorOffsets.at(3) = TVector3(c_min[0], c_max[1], c_min[2]); // Top
    NormalVectorOffsets.at(4) = TVector3(c_min[0], c_min[1], c_min[2]); // upstream
    NormalVectorOffsets.at(5) = TVector3(c_min[0], c_min[1], c_max[2]); // downstream

    // Loop over all surfaces of the box 
    for(unsigned int face_no = 0; face_no < NormalVectors.size(); face_no++)
    {
      // Check if trajectory and surface are not parallel
      if(NormalVectors.at(face_no).Dot(TrajectoryDirect))
      {
	// Calculate the line parameter for the intersection points
	LineParameters.push_back( NormalVectors.at(face_no).Dot(NormalVectorOffsets.at(face_no) - TrajectoryStart)
				/ NormalVectors.at(face_no).Dot(TrajectoryDirect) );
      }
      else continue;
      
      // Calculate intersection point using the line parameter
      IntersectionPoints.push_back( LineParameters.back()*TrajectoryDirect + TrajectoryStart );
      
      // Coordinate which should be ignored when checking for limits added by Christoph Rudolf von Rohr 05/21/2016
      unsigned int NoCheckCoord;
      
      // Calculate NoCheckCoord out of the face_no
      if(face_no % 2)
      {
	  // Convert odd face number to coordinate
	  NoCheckCoord = (face_no - 1)/2;
      }
      else
      {
	  // Convert even face number to coordinate
	  NoCheckCoord = face_no/2;
      }
      
      // Loop over all three space coordinates
      for(unsigned int coord = 0; coord < 3; coord++)
      {
	// Changed by Christoph Rudolf von Rohr 05/21/2016
	// Then check if point is not within the surface limits at this coordinate, without looking
	// at the plane normal vector coordinate. We can assume, that our algorithm already found this coordinate correctily.
	// In rare cases, looking for boundaries in this coordinate causes unexpected behavior due to floating point inaccuracies.
	if( coord != NoCheckCoord && (IntersectionPoints.back()[coord] > c_max[coord] || IntersectionPoints.back()[coord] < c_min[coord]) )
	{
	  // if off limits, get rid of the useless data and break the coordinate loop
	  LineParameters.pop_back();
	  IntersectionPoints.pop_back();
	  break;
	}
      }// coordinate loop
    }// Surcaces loop
    
    // sort points according to their parameter value (first is entry, second is exit)
    if(LineParameters.size() == 2 && LineParameters.front() > LineParameters.back())
    {
      std::swap(IntersectionPoints.front(),IntersectionPoints.back());
    }
    
    return IntersectionPoints;
  }
} // GetIntersections()