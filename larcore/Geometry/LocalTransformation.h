/**
 * @file   LocalTransformation.h
 * @brief  Class containing local-to-world transformations
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   November 30, 2016
 *
 */

#ifndef LARCORE_GEOMETRY_LOCALTRANSFORMATION_H
#define LARCORE_GEOMETRY_LOCALTRANSFORMATION_H


// ROOT libraries
#include "TVector3.h"


// C/C++ standard libraries
#include <vector>
#include <cstdlib> // std::size_t


// forward declarations
class TGeoNode;


namespace geo {
  
  /**
   * @brief Class to transform between world and local coordinates
   * 
   * This class provides two directions of transformations (world to local and
   * the other way around), for points and for vectors.
   * The vector version of the transformation does not apply translation.
   * 
   */
  template <typename StoredMatrix>
  class LocalTransformation {
      public:
    
    /// Type of transformation matrix
    using TransformationMatrix_t = StoredMatrix;
    
    /**
     * @brief Constructor: uses the specified transformation matrix
     * @param matrix the transformation matrix to be used
     * 
     * The specified matrix is copied into a local copy.
     */
    LocalTransformation(TransformationMatrix_t const& matrix)
      : fGeoMatrix(matrix) {}
    
    /**
     * @brief Constructor: uses the specified transformation matrix
     * @param path the path of ROOT geometry nodes
     * @param depth the index in the path of the last node to be considered
     * 
     * The specified matrix is copied into a local copy.
     */
    LocalTransformation(std::vector<TGeoNode const*> const& path, size_t depth)
      : fGeoMatrix(transformationFromPath(path, depth)) {}
    
    /**
     * @brief Transforms a point from local frame to world frame
     * @param local local coordinates: [0] x, [1] y, [2] z [cm]
     * @param world (output) corresponding world coordinates [cm]
     * 
     * The full transformation is applied. Fox example:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * geo::LocalTransformation trans( ... ); // with proper initialisation
     * 
     * std::array<double, 3U> origin, center;
     * origin.fill(0.);
     * trans.LocalToWorld(origin.data(), center.data());
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * `center` will contain the world coordinates of the center of the volume,
     * which is usually represented by the origin in the local coordinates.
     */
    void LocalToWorld(double const* local, double* world) const
      { fGeoMatrix.LocalToMaster(local, world); }
    
    
    /**
     * @brief Transforms a point from local frame to world frame
     * @param local local coordinates [cm]
     * @return corresponding world coordinates [cm]
     * 
     * The full transformation is applied. Fox example:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * geo::LocalTransformation trans( ... ); // with proper initialisation
     * 
     * auto center = trans.LocalToWorld(TVector3());
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * `center` will be a TVector3 containing the world coordinates of the
     * center of the volume, which is usually represented by the origin in the
     * local coordinates (a TVector3 is by default constructed to point to the
     * origin).
     */
    TVector3 LocalToWorld(TVector3 const& local) const;
    
    
    /**
     * @brief Transforms a vector from local frame to world frame
     * @param local local coordinates: [0] x, [1] y, [2] z [cm]
     * @param world (output) corresponding world coordinates [cm]
     * 
     * The translation is not applied, since the argument is supposed to be a
     * vector, relative difference between two points.
     */
    void LocalToWorldVect(double const* local, double* world) const
      { fGeoMatrix.LocalToMasterVect(local, world); }
    
    
    /**
     * @brief Transforms a vector from local frame to world frame
     * @param local local coordinates [cm]
     * @return corresponding world coordinates [cm]
     * 
     * The translation is not applied, since the argument is supposed to be a
     * vector, relative difference between two points.
     */
    TVector3 LocalToWorldVect(TVector3 const& local) const;
    
    
    /**
     * @brief Transforms a point from world frame to local frame
     * @param world world coordinates: [0] x, [1] y, [2] z [cm]
     * @param local (output) corresponding local coordinates [cm]
     * 
     * The full transformation is applied. Fox example:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * geo::LocalTransformation trans( ... ); // with proper initialisation
     * 
     * std::array<double, 3U> world{ 4.0, 5.0, -2.5 }, local;
     * trans.WorldToLocal(world.data(), local.data());
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * `local` will contain the local coordinates of the specified point.
     */
    void WorldToLocal(double const* world, double* local) const
      { fGeoMatrix.MasterToLocal(world, local); }
    
    /**
     * @brief Transforms a point from world frame to local frame
     * @param world world coordinates [cm]
     * @return corresponding local coordinates [cm]
     * 
     * The full transformation is applied. Fox example:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * geo::LocalTransformation trans( ... ); // with proper initialisation
     * 
     * auto local = trans.WorldToLocal(TVector3(4.0, 5.0, -2.5));
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * `local` will be a TVector3 containing the local coordinates of the
     * specified point.
     */
    TVector3 WorldToLocal(TVector3 const& world) const;
    
    /**
     * @brief Transforms a vector from world frame to local frame
     * @param world world coordinates: [0] x, [1] y, [2] z [cm]
     * @param local (output) corresponding local coordinates [cm]
     * 
     * The translation is not applied, since the argument is supposed to be a
     * vector, relative difference between two points.
     */
    void WorldToLocalVect(const double* world, double* local) const
      { fGeoMatrix.MasterToLocalVect(world, local); }
    
    /**
     * @brief Transforms a vector from world frame to local frame
     * @param world coordinates [cm]
     * @return corresponding world coordinates [cm]
     * 
     * The translation is not applied, since the argument is supposed to be a
     * vector, relative difference between two points.
     */
    TVector3 WorldToLocalVect(TVector3 const& world) const;
    
    
    /// Direct access to the transformation matrix
    TransformationMatrix_t const& Matrix() const { return fGeoMatrix; }
    
    
    /// Builds a matrix to go from local to world coordinates in one step
    static TransformationMatrix_t transformationFromPath
      (std::vector<TGeoNode const*> const& path, size_t depth);
    
    
      protected:
    
    TransformationMatrix_t fGeoMatrix; ///< local to world transform
    
    
  }; // class LocalTransformation<>
  
  
} // namespace geo


//------------------------------------------------------------------------------
// template implementation

#include "LocalTransformation.txx"

//------------------------------------------------------------------------------


#endif // LARCORE_GEOMETRY_LOCALTRANSFORMATION_H
