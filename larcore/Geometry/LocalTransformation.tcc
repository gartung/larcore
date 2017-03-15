/**
 * @file   LocalTransformation.tcc
 * @brief  Class containing local-to-world transformations
 *         (template implementation)
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   November 30, 2016
 * @see    LocalTransformation.h
 *
 * This file is expected to be included directly in the header.
 *
 */

#ifndef LARCORE_GEOMETRY_LOCALTRANSFORMATION_TXX
#define LARCORE_GEOMETRY_LOCALTRANSFORMATION_TXX


// ROOT
#include "TGeoNode.h"
#include "TGeoMatrix.h"

// CLHEP
#include "CLHEP/Geometry/Transform3D.h"


//------------------------------------------------------------------------------
template <typename Matrix>
TVector3 geo::LocalTransformation<Matrix>::WorldToLocal
  (TVector3 const& world) const
{
  double const worldArray[3] = { world.X(), world.Y(), world.Z() };
  double localArray[3];
  WorldToLocal(worldArray, localArray);
  return TVector3(localArray);
} // geo::LocalTransformation::WorldToLocal()


//......................................................................
template <typename Matrix>
TVector3 geo::LocalTransformation<Matrix>::WorldToLocalVect
  (TVector3 const& world) const
{
  double const worldArray[3] = { world.X(), world.Y(), world.Z() };
  double localArray[3];
  WorldToLocalVect(worldArray, localArray);
  return TVector3(localArray);
} // geo::LocalTransformation::WorldToLocalVect()


//......................................................................
template <typename Matrix>
TVector3 geo::LocalTransformation<Matrix>::LocalToWorld
  (TVector3 const& local) const
{
  double const localArray[3] = { local.X(), local.Y(), local.Z() };
  double worldArray[3];
  LocalToWorld(localArray, worldArray);
  return TVector3(worldArray);
} // geo::LocalTransformation::LocalToWorld()

  
//......................................................................
template <typename Matrix>
TVector3 geo::LocalTransformation<Matrix>::LocalToWorldVect
  (TVector3 const& local) const
{
  double const localArray[3] = { local.X(), local.Y(), local.Z() };
  double worldArray[3];
  LocalToWorldVect(localArray, worldArray);
  return TVector3(worldArray);
} // geo::LocalTransformation::LocalToWorldVect()


//------------------------------------------------------------------------------
// specialisations (forward declarations)
// 
namespace geo {
  
  template <>
  TGeoHMatrix LocalTransformation<TGeoHMatrix>::transformationFromPath
    (std::vector<TGeoNode const*> const& path, size_t depth);

  template <>
  HepGeom::Transform3D
  LocalTransformation<HepGeom::Transform3D>::transformationFromPath
    (std::vector<TGeoNode const*> const& path, size_t depth);

} // namespace geo

//------------------------------------------------------------------------------
  
#endif // LARCORE_GEOMETRY_LOCALTRANSFORMATION_TXX
