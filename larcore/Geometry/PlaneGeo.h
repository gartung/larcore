////////////////////////////////////////////////////////////////////////
/// \file  PlaneGeo.h
/// \brief Encapsulate the construction of a single detector plane
///
/// \version $Id: PlaneGeo.h,v 1.7 2009/12/01 21:07:51 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_PLANEGEO_H
#define GEO_PLANEGEO_H

// LArSoft libraries
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcore/CoreUtils/DereferenceIterator.h"
#include "larcore/Geometry/GeoObjectSorter.h"
#include "larcore/Geometry/SimpleGeo.h"
#include "larcore/Geometry/LocalTransformation.h" // geo::LocalTransformationFromPath
#include "larcore/Geometry/BoxBoundedGeo.h"
#include "larcore/Geometry/Decomposer.h"
#include "larcore/Geometry/WireGeo.h"

// ROOT libraries
#include "Math/GenVector/Cartesian2D.h"
#include "Math/GenVector/PositionVector2D.h"
#include "Math/GenVector/DisplacementVector2D.h"


// C/C++ standard libraries
#include <cmath> // std::atan2()
#include <vector>
#include <string>

class TGeoNode;
class TGeoHMatrix;
class TVector3;

namespace geo {

  //......................................................................
  
  /**
   * @brief Geometry information for a single wire plane.
   * 
   * The plane is represented in the geometry by a solid which contains wires.
   * Currently, only box solids are well supported.
   * The box which is representation of the plane has some thickness, and it
   * should not be assumed that the wires are in the median section of it,
   * that is, the center of the box may not lie on the plane defined by the
   * wires.
   * 
   * The plane defines two local reference frames.
   * The first, depending on wire directions and therefore called "wire base",
   * is defined by the normal to the plane (pointing toward the center of the
   * TPC), the direction of the wires, and the direction that the wires measure.
   * This is a positive orthogonal base.
   * Note that for this base to be correctly defined, the Geometry service has
   * to provide external information (for example, where the center of the
   * TPC is).
   * 
   * The second, depending only on the shape of the plane and called "frame
   * base", is defined by the normal (the same as for the previous one), and two
   * orthogonal axes, "width" and "depth", aligned with the sides of the plane.
   * If the plane has not the shape of a box, this reference frame is not
   * available. This coordinate system is also positive defined.
   * These components are all measured in centimeters.
   * 
   */
  // Note: SignalType() and SetSignalType() have been removed.
  //       Use `geo::GeometryCore::SignalType` instead.
  //       (see LArSoft issue #14365 at https://cdcvs.fnal.gov/redmine/issues/14365 )
  class PlaneGeo {
  public:
    
    using GeoNodePath_t = std::vector<TGeoNode const*>;
    
    /// Tag for wire base vectors.
    struct WireCoordinateReferenceTag {};
    
    /// Tag for plane frame base vectors.
    struct WidthDepthReferenceTag {};
    
    /// Type for projections in the wire base representation.
    using WireCoordProjection_t = ROOT::Math::DisplacementVector2D
      <ROOT::Math::Cartesian2D<double>, WireCoordinateReferenceTag>;
    
    /// Type for projections in the plane frame base representation.
    using WidthDepthProjection_t = ROOT::Math::DisplacementVector2D
      <ROOT::Math::Cartesian2D<double>, WidthDepthReferenceTag>;
    
    /// Type used for plane decompositions on wire base.
    using WireDecomposer_t = geo::Decomposer
      <geo::vect::Vector_t, geo::vect::Point_t, WireCoordProjection_t>;
    
    /// Type used for plane decompositions on plane frame (width/depth).
    using WidthDepthDecomposer_t = geo::Decomposer
      <geo::vect::Vector_t, geo::vect::Point_t, WidthDepthProjection_t>;
    
    /// Type describing a 3D point or vector decomposed on a plane on wire base.
    using WireDecomposedVector_t = WireDecomposer_t::DecomposedVector_t;
    
    /// Type describing a 3D point or vector decomposed on a plane
    /// with plane frame base (width and depth).
    using WDDecomposedVector_t = WidthDepthDecomposer_t::DecomposedVector_t;
      
    
    /// Construct a representation of a single plane of the detector
    PlaneGeo(GeoNodePath_t& path, size_t depth);
    ~PlaneGeo();

    
    /// @{
    /// @name Plane properties
    
    /// Which coordinate does this plane measure
    View_t View()                                             const { return fView;          }
    
    /// What is the orientation of the plane
    Orient_t Orientation()                                    const { return fOrientation;   }

    /// Angle of the wires from positive z axis; @f$ \theta_{z} \in [ 0, \pi ]@f$.
    double ThetaZ()                                           const;
    
    /// Angle from positive z axis of the wire coordinate axis, in radians
    double PhiZ()                                             const
      { return std::atan2(fSinPhiZ, fCosPhiZ); }
    
    /// Sine of PhiZ()
    double SinPhiZ()                                          const { return fSinPhiZ; }
    
    /// Cosine of PhiZ()
    double CosPhiZ()                                          const { return fCosPhiZ; }
    
    /// Returns the identifier of this plane
    geo::PlaneID const& ID() const { return fID; }
    
    
    /// @{
    /// @name Plane size and coordinates
    
    /**
     * @brief Return the direction of plane width.
     * 
     * The precise definition of the sides is arbitrary, but they are defined
     * to lie on the wire plane and so that WidthDir(), DepthDir() and
     * GetNormalDirection() make a orthonormal base.
     * That base (width, depth, normal) is guaranteed to be positive defined.
     */
    TVector3 const& WidthDir() const { return fDecompFrame.MainDir(); }
    
    /**
     * @brief Return the direction of plane depth.
     * 
     * The precise definition of the sides is arbitrary, but they are defined
     * to lie on the wire plane and so that WidthDir(), DepthDir() and
     * GetNormalDirection() make a orthonormal base.
     * That base (width, depth, normal) is guaranteed to be positive defined.
     */
    TVector3 const& DepthDir() const { return fDecompFrame.SecondaryDir(); }
    
    /**
     * @brief Return the width of the plane.
     * @see Depth(), WidthDir(), DepthDir()
     * 
     * The precise definition is arbitrary (see `WidthDir()`).
     */
    double Width() const { return fFrameSize.Width(); }
    
    /**
     * @brief Return the depth of the plane.
     * @see Width(), WidthDir(), DepthDir()
     * 
     * The precise definition is arbitrary (see `DepthDir()`).
     */
    double Depth() const { return fFrameSize.Depth(); }

    
    /// Returns the world coordinates of the box containing the plane.
    /// @see GetBoxCenter()
    geo::BoxBoundedGeo BoundingBox() const;
    
    /// @}
    
    
    /// @{
    /// @name Wire access
    
    //@{
    /// Number of wires in this plane
    unsigned int Nwires()                                     const { return fWire.size();   }
    unsigned int NElements()                                  const { return Nwires();       }
    //@}
    
    //@{
    /**
     * @brief Returns whether a wire with index iwire is present in this plane.
     * @param iwire index of wire in this plane
     * @return whether the wire with index iwire is present in this plane
     */
    bool HasWire(unsigned int iwire) const { return iwire < Nwires(); }
    bool HasElement(unsigned int iwire) const { return HasWire(iwire); }
    //@}
    
    //@{
    /**
     * @brief Returns whether the wire in wireid is present in this plane.
     * @param wireid full wire ID
     * @return whether the wire in wireid is present in this plane
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    bool HasWire(geo::WireID const& wireid) const
      { return HasWire(wireid.Wire); }
    bool HasElement(geo::WireID const& wireid) const
      { return HasWire(wireid); }
    //@}
    
    /// Return the iwire'th wire in the plane.
    /// @throws cet::exception (category "WireOutOfRange") if no such wire
    /// @note In the past, no check was performed.
    WireGeo const& Wire(unsigned int iwire) const;
    
    //@{
    /**
     * @brief Returns the wire in wireid from this plane.
     * @param wireid full wire ID
     * @return a constant reference to the wire in wireid
     * @throws cet::exception (category "WireOutOfRange") if no such wire
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    WireGeo const& Wire(WireID const& wireid) const
      { return Wire(wireid.Wire); }
    WireGeo const& GetElement(WireID const& wireid) const
      { return Wire(wireid); }
    //@}
    
    /**
     * @brief Returns the wire number iwire from this plane.
     * @param iwire the number of local wire
     * @return a constant pointer to the wire, or nullptr if it does not exist
     */
    WireGeo const* WirePtr(unsigned int iwire) const
      { return HasWire(iwire)? fWire[iwire]: nullptr; }
    
    //@{
    /**
     * @brief Returns the wire in wireid from this plane.
     * @param wireid full wire ID
     * @return a constant pointer to the wire, or nullptr if it does not exist
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    WireGeo const* WirePtr(WireID const& wireid) const
      { return WirePtr(wireid.Wire); }
    WireGeo const* GetElementPtr(WireID const& wireid) const
      { return WirePtr(wireid); }
    //@}
    
    
    /// Return the first wire in the plane.
    const WireGeo& FirstWire()                                const { return Wire(0);        }
    
    /// Return the middle wire in the plane.
    const WireGeo& MiddleWire()                               const { return Wire(Nwires()/2); }
    
    /// Return the last wire in the plane.
    const WireGeo& LastWire()                                 const { return Wire(Nwires()-1); }
    
    /**
     * @brief Allows range-for iteration on all wires in this plane.
     * @return an object suitable for range-for iteration on all wires
     * 
     * This example uses geometry to iterate on all planes in an outer loop,
     * and then iterates on all wires in the plane in the inner loop:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     * auto geom = lar::providerFrom<geo::Geometry>();
     * for (geo::PlaneGeo const& plane: geom->IteratePlanes()) {
     *   
     *   // collect plane information
     *   
     *   for (geo::WireGeo const& wire: plane.IterateWires()) {
     *     
     *     // do something with each single wire
     *     
     *   }
     * } // for planes
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * (note that all data types here can be replaced with `auto`).
     * 
     */
    auto IterateWires() const
      { return lar::util::dereferenceConstIteratorLoop(fWire); }
    
    /// @}
    
    
    /// @{
    /// @name Plane geometry properties
    
    /// Return the wire pitch (in centimeters). It is assumed constant.
    double WirePitch() const { return fWirePitch; }
    
    /**
     * @brief Returns whether the higher z wires have higher wire ID.
     * @return whether the higher z wires have higher wire ID
     * @see GetIncreasingWireDirection()
     * 
     * This method is related to GetIncreasingWireDirection()
     * (it might be expressed as "GetIncreasingWireDirection()[2] > 0"),
     * but it is implemented in a faster and independent way.
     */
    bool WireIDincreasesWithZ() const;
    
    /**
     * @brief Returns the direction normal to the plane.
     * @return a TVector3 versor with a direction normal to the plane
     *
     * The versor is orthogonal to the plane.
     * The direction is defined so that the semi-space pointed to contains
     * the TPC center.
     * 
     * @note Each decomposition base (wire-based and frame-based) has its own
     *       normal, defined solely from its two decomposition plane axes.
     *       The wire-based frame is nevertheless required to have a normal
     *       matching this one, while the frame-based normal might happen to be
     *       in the opposite direction depending on the original geometry
     *       description.
     */
    TVector3 const& GetNormalDirection() const { return fNormal; }

    /**
     * @brief Returns the direction of increasing wires.
     * @return a TVector3 versor with the direction of increasing wires
     *
     * The versor is orthogonal to the wires (assumed parallel),
     * lies on the plane and its direction goes toward increasing wire IDs.
     */
    TVector3 const& GetIncreasingWireDirection() const
      { return fDecompWire.SecondaryDir(); }
    
    
    /**
     * @brief Returns the centre of the wire plane in world coordinates [cm]
     * @see GetBoxCenter()
     * 
     * The center of the plane is defined so that it has width and depth
     * coordinates in the middle of the plane box (that is, the geometrical
     * representation of the plane in the geometry description), and the other
     * coordinate set at drift distance 0.
     * 
     * Note that this does not necessarily match the center of the box, if the
     * geometry does not place the wires, which define the drift distance, in
     * the plane in the middle of the box.
     */
    TVector3 GetCenter() const
      { return fCenter; }
    
    /**
     * @brief Returns the centre of the box representing the plane.
     * @return the centre of the box, in world coordinates [cm]
     * @see GetCenter()
     * 
     * This is the centre of the box representing the plane in the geometry
     * description, in world coordinates.
     * This is rarely of any use, as most of the times `GetCenter()` delivers
     * the proper information, e.g. for simulation and reconstruction.
     */
    TVector3 GetBoxCenter() const;
    
    /**
     * @brief Returns the direction of the wires.
     * @return a unit vector following the direction of the wires
     * 
     * All wires in the plane are assumed parallel.
     */
    TVector3 const& GetWireDirection() const { return fDecompWire.MainDir(); }
    
    
    /**
     * @brief Returns the ID of wire closest to the specified position.
     * @param pos world coordinates of the point [cm]
     * @return the ID of the wire closest to the projection of pos on the plane
     * @throw InvalidWireError (category: `"Geometry"`) if out of range
     *
     * The position is projected on the wire plane, and the ID of the nearest
     * wire to the projected point is returned.
     * 
     * If the wire does not exist, an exception is thrown that contains both the
     * wire that would be the closest one (`badWireID()`), and also the wire
     * that is actually the closest one (`betterWireID()`). When this happens,
     * the specified position was outside the wire plane.
     * 
     * Note that the caller should check for containment: this function may or
     * may not report the position being outside the plane, depending on where
     * it is. In the current implementation, the wires are considered infinitely
     * long, and if the position projection is closer than half the wire pitch
     * from any of these extrapolated wires, the method will not report error.
     * 
     */
    geo::WireID NearestWireID(TVector3 const& pos) const;
    
    
    /**
     * @brief Returns the distance of the specified point from the wire plane.
     * @param point a point in world coordinates [cm]
     * @return the signed distance from the wire plane
     * 
     * The distance is defined positive if the point lies in the side the normal
     * vector (GetNormalDirection()) points to.
     * 
     * The distance is defined from the geometric plane where the wires lie, and
     * it may not match the distance from the center of the geometry box
     * representing the plane.
     * It should always match the drift distance from this wire plane, and the
     * result of `DriftPoint(point, DistanceFromPlane(point))` will bring the
     * point to the plane.
     */
    double DistanceFromPlane(TVector3 const& point) const
      { return fDecompWire.PointNormalComponent(point); }
    
    
    /**
     * @brief Shifts the position of an electron drifted by a distance.
     * @param position _(modified)_ the position of the electron
     * @param drift drift distance to shift the electron by [cm]
     * 
     * This is a pure geometry computation: the position is shifted by the drift
     * distance in the direction opposite to the normal to the plane (as
     * returned by `GetNormalDirection()`), no matter where the position is
     * relative to the plane.
     * The wording about "electron position" is just meant to remind that the
     * drift shift is taken with opposite sign: since the point is assumed to be
     * an electron, a positive drift normally moves its position toward the wire
     * plane.
     */
    void DriftPoint(TVector3& position, double distance) const
      { position -= distance * GetNormalDirection(); }
    
    
    /// Returns a volume including all the wires in the plane
    lar::util::simple_geo::Volume<> Coverage() const;
    
    /**
     * @brief Prints information about this plane.
     * @tparam Stream type of output stream to use
     * @param out stream to send the information to
     * @param indent prepend each line with this string
     * @param verbosity amount of information printed
     * 
     * Information on single wires is not printed.
     * Note that the first line out the output is _not_ indented.
     * 
     * Verbosity levels
     * -----------------
     * 
     * * 0: only plane ID
     * * 1 _(default)_: also center and wire angle
     * * 2: also information about wires
     * * 3: also information about normal and increasing coordinate direction
     * * 4: also information about wire direction, width and depth
     * * 5: also coverage
     * * 6: also bounding box
     * 
     */
    template <typename Stream>
    void PrintPlaneInfo
      (Stream&& out, std::string indent = "", unsigned int verbosity = 1) const;
    
    /// @}
    
    
    /// @{
    /// @name Projections on wire length/wire coordinate direction base
    /// 
    /// These methods deal with projection of points and vectors on the plane,
    /// using a geometric reference base which is dependent on the wire
    /// direction. This is useful for plane reconstruction.
    /// 
    
    /**
     * @brief Returns the coordinate of point on the plane respect to a wire.
     * @param point world coordinate of the point to get the coordinate of [cm]
     * @param refWire reference wire
     * @return the coordinate of the point [cm]
     * @see WireCoordinate()
     * 
     * The method returns the coordinate of the point in the direction measured
     * by the wires on this plane starting from the specified reference wire,
     * in world units (that is, centimeters).
     *  
     * The point does not need to be on the plane, and the projection of the
     * point to the plane is considered.
     * The reference wire, instead, must belong to this plane. This assumption
     * is not checked, and if violated the results are undefined (in the current
     * implementation, they are just wrong).
     */
    double PlaneCoordinateFrom
      (TVector3 const& point, geo::WireGeo const& refWire) const
      {
        return
          fDecompWire.VectorSecondaryComponent(point - refWire.GetCenter());
      }
    
    
    /**
     * @brief Returns the coordinate of the point on the plane.
     * @param point world coordinate of the point to get the coordinate of [cm]
     * @return the coordinate of the point [cm]
     * @see PlaneCoordinateFrom(TVector3 const&, geo::Wire const&)
     * 
     * The method returns the coordinate of the point in the direction measured
     * by the wires on this plane starting on the first wire, in world units
     * (that is, centimeters). A point on the first wire will have coordinate
     * 0.0, one on the next wire will have coordinate equal to a single wire
     * pitch, etc.
     *  
     * The point does not need to be on the plane, and the projection of the
     * point to the plane is considered.
     */
    double PlaneCoordinate(TVector3 const& point) const
      { return fDecompWire.PointSecondaryComponent(point); }
    
    
    /**
     * @brief Returns the coordinate of the point on the plane, in wire units.
     * @param point world coordinate of the point to get the coordinate of
     * @return the coordinate of the point, in wire pitch units
     * @see CoordinateFrom(TVector3 const&, geo::Wire const&)
     * 
     * The method returns the coordinate of the point in the direction measured
     * by the wires on this plane starting on the first wire, in wire units
     * (that is, wire pitches). A point on the first wire will have coordinate
     * 0.0, one on the next wire will have coordinate 1.0, etc.
     *  
     * The point does not need to be on the plane, and the projection of the
     * point to the plane is considered.
     */
    double WireCoordinate(TVector3 const& point) const
      { return PlaneCoordinate(point) / WirePitch(); }
    
    
    /**
     * @brief Decomposes a 3D point in two components.
     * @param point the point to be decomposed
     * @return the two components of point, on the plane and orthogonal to it
     * 
     * The point is decomposed in:
     * 
     * 1. a component orthogonal to the plane, expressed as a signed real number
     * 2. a component lying on the plane, expressed as a 2D vector
     * 
     * The distance is obtained as by DistanceFromPlane().
     * The projection on the plane is obtained following the same convention
     * as PointProjection().
     */
    WireDecomposedVector_t DecomposePoint(TVector3 const& point) const
      { return fDecompWire.DecomposePoint(point); }
    
    /**
     * @brief Returns the reference point used by `PointProjection()`.
     * 
     * The returned point is such that its decomposition results in a null
     * projection and a 0 distance from the plane.
     */
    TVector3 ProjectionReferencePoint() const
      { return fDecompWire.ReferencePoint(); }
    
    /**
     * @brief Returns the projection of the specified point on the plane.
     * @param point the 3D point to be projected, in world coordinates
     * @return a 2D vector representing the projection of point on the plane
     * 
     * The returned vector is a 2D vector expressing the projection of the point
     * (from world coordinates) on the wire plane.
     * The vector is expressed as @f$ ( \ell, w ) @f$. The component
     * @f$ \ell @f$ is measured on the direction of the first wire (see
     * `WireGeo::Direction()`), using its center (see `WireGeo::GetCenter()`) as
     * reference point. The component @f$ w @f$ is defined on the wire
     * coordinate direction (see `GetIncreasingWireDirection()`), relative to
     * the first wire, as it is returned by PlaneCoordinate(). 
     * 
     * The reference point is also returned by ProjectionReferencePoint().
     */
    WireCoordProjection_t PointProjection(TVector3 const& point) const
      { return fDecompWire.ProjectPointOnPlane(point); }
    
    /**
     * @brief Returns the projection of the specified vector on the plane.
     * @param v the 3D vector to be projected, in world units
     * @return a 2D vector representing the projection of v on the plane
     * 
     * The returned vector is a 2D vector expressing the projection of the
     * vector (from world units) on the wire plane.
     * The vector is expressed as @f$ ( \ell, w ) @f$. The component
     * @f$ \ell @f$ is measured on the direction of the first wire (see
     * `WireGeo::Direction()`). The component @f$ w @f$ is defined on the wire
     * coordinate direction (see `GetIncreasingWireDirection()`). 
     */
    WireCoordProjection_t VectorProjection(TVector3 const& v) const
      { return fDecompWire.ProjectVectorOnPlane(v); }
    
    /**
     * @brief Returns the 3D vector from composition of projection and distance.
     * @param decomp decomposed point
     * @return the 3D vector from composition of projection and distance
     * @see DecomposePoint(), ComposePoint(double, WireCoordProjection_t const&)
     * 
     * See `ComposePoint(double, WireCoordProjection_t const&)` for details.
     */
    TVector3 ComposePoint(WireDecomposedVector_t const& decomp) const
      { return fDecompWire.ComposePoint(decomp); }
    
    /**
     * @brief Returns the 3D point from composition of projection and distance.
     * @param distance distance of the target point from the wire plane
     * @param proj projection of the target point on the wire plane
     * @return the 3D vector from composition of projection and distance
     * @see DecomposePoint()
     * 
     * The returned point is the reference point of the frame system (that is,
     * the plane center), translated by two 3D vectors:
     * 
     * 1. a vector parallel to the plane normal, with norm the input distance
     * 2. a vector lying on the plane, whose projection via `PointProjection()`
     *    gives the input projection
     * 
     * The choice of the projection reference point embodies the same convention
     * used in `PointProjection()` and `DecomposePoint()`.
     * In fact, the strict definition of the result of this method is a 3D point
     * whose decomposition on the plane frame base matches the method arguments.
     * 
     * Note that currently no equivalent facility is available to compose
     * vectors instead of points, that is, entities ignoring the reference
     * point.
     */
    TVector3 ComposePoint
      (double distance, WireCoordProjection_t const& proj) const
      { return fDecompWire.ComposePoint(distance, proj); }
    
    
    /// @}
    
    
    /// @{
    /// @name Projection on width/depth plane
    /// 
    /// These methods deal with projection of points and vectors on the plane,
    /// using a geometric reference base which is not dependent on the wire
    /// direction. This is more useful when comparing with the TPC or other
    /// planes.
    /// 
    
    /**
     * @brief Decomposes a 3D point in two components.
     * @param point the point to be decomposed
     * @return the two components of point, on the plane and orthogonal to it
     * 
     * The point is decomposed in:
     * 
     * 1. a component orthogonal to the plane, expressed as a signed real number
     * 2. a component lying on the plane, expressed as a 2D vector
     * 
     * The distance is obtained as by DistanceFromPlane().
     * The projection on the plane is obtained following the same convention
     * as PointWidthDepthProjection().
     */
    WDDecomposedVector_t DecomposePointWidthDepth(TVector3 const& point) const
      { return fDecompFrame.DecomposePoint(point); }
    
    /**
     * @brief Returns the projection of the specified point on the plane.
     * @param point the 3D point to be projected, in world coordinates
     * @return a 2D vector representing the projection of point on the plane
     * 
     * The returned vector is a 2D vector expressing the projection of the point
     * (from world coordinates) on the wire plane.
     * The vector is expressed as @f$ ( w, d ) @f$, components following the
     * width direction (`WidthDir()`) and the depth direction (`DepthDir()`)
     * respectively. The origin point is the center of the plane.
     */
    WidthDepthProjection_t PointWidthDepthProjection
      (TVector3 const& point) const
      { return fDecompFrame.ProjectPointOnPlane(point); }
    
    /**
     * @brief Returns the projection of the specified vector on the plane.
     * @param v the 3D vector to be projected, in world units
     * @return a 2D vector representing the projection of v on the plane
     * 
     * The returned vector is a 2D vector expressing the projection of the
     * vector (from world units) on the wire plane.
     * The vector is expressed as @f$ ( w, d ) @f$, components following the
     * width direction (`WidthDir()`) and the depth direction (`DepthDir()`)
     * respectively.
     */
    WidthDepthProjection_t VectorWidthDepthProjection
      (TVector3 const& v) const
      { return fDecompFrame.ProjectVectorOnPlane(v); }
    
    /**
     * @brief Returns if the projection of specified point is within the plane.
     * @param point world coordinate of the point to test [cm]
     * @return whether the projection of specified point is within the plane
     * @see PointWidthDepthProjection(), Width(), Height()
     * 
     * The method extracts the projection of the specified point on the plane,
     * as in `PointWidthDepthProjection()`, and then verifies that the
     * projection falls within the wire plane area, as defined by the dimensions
     * from the geometry description.
     */
    bool isProjectionOnPlane(TVector3 const& point) const;
    
    /**
     * @brief Returns a projection vector that, added to the argument, gives a
     *        projection inside (or at the border of) the plane.
     * @param proj starting projection
     * @return a projection displacement
     * 
     * If the projection is already on the plane, the returned displacement is
     * null.
     */
    WidthDepthProjection_t DeltaFromPlane
      (WidthDepthProjection_t const& proj) const;
    
    /**
     * @brief Returns the projection, moved onto the plane if necessary.
     * @param proj projection to be checked and moved
     * @return the new value of the projection
     * @see isProjectionOnPlane(), Width(), Height()
     * 
     * The projection proj is defined as in the output of
     * `PointWidthDepthProjection()`.
     * The method caps width and depth of the projection so that it stays on
     * the plane. A new capped value is returned.
     * Since the reference point of the frame is defined as the center of the
     * plane, this action is equivalent to force the width component in
     * @f$ \left[ -\frac{w}{2}, \frac{w}{2} \right] @f$ range and the depth
     * component into @f$ \left[ -\frac{d}{2}, \frac{d}{2} \right] @f$, with
     * @f$ w @f$ and @f$ d @f$ the width and depth of the wire plane.
     */
    WidthDepthProjection_t MoveProjectionToPlane
      (WidthDepthProjection_t const& proj) const;
    
    /**
     * @brief Returns the point, moved so that its projection is over the plane.
     * @param point point to be checked and moved
     * @return the new value of the point
     * @see isProjectionOnPlane(), MoveProjectionToPlane(), Width(), Height()
     * 
     * If the projection of the point on the plane falls outside it, the
     * returned point is translated so that its projection is now on the border
     * of the plane. The translation happens along the directions of the plane
     * frame, as described in MoveProjectionToPlane().
     */
    TVector3 MovePointOverPlane(TVector3 const& point) const;
    
    /**
     * @brief Returns the 3D vector from composition of projection and distance.
     * @param decomp decomposed point
     * @return the 3D vector from composition of projection and distance
     * @see DecomposePointWidthDepth(),
     *      ComposePointWidthDepth(double, DecomposedVector_t::Projection_t const&)
     * 
     * See
     * `ComposePointWidthDepth(double, DecomposedVector_t::Projection_t const&)`
     * for details.
     */
    TVector3 ComposePoint(WDDecomposedVector_t const& decomp) const
      { return fDecompFrame.ComposePoint(decomp); }
    
    /**
     * @brief Returns the 3D point from composition of projection and distance.
     * @param distance distance of the target point from the wire plane
     * @param proj projection of the target point on the wire plane
     * @return the 3D vector from composition of projection and distance
     * @see DecomposePointWidthDepth()
     * 
     * The returned vector is the sum of two 3D vectors:
     * 
     * 1. a vector parallel to the plane normal, with norm the input distance
     * 2. a vector lying on the plane, whose projection via
     *    `PointWidthDepthProjection()` gives the input projection
     * 
     * Given the arbitrary definition of the projection reference, it is assumed
     * that the same convention is used as in PointWidthDepthProjection() and
     * DecomposePointWidthDepth().
     * 
     */
    TVector3 ComposePoint
      (double distance, WidthDepthProjection_t const& proj) const
      { return fDecompFrame.ComposePoint(distance, proj); }
    
    
    /// @}
    
    
    /// @{
    /// @name Coordinate transformation
    
    /// Transform point from local plane frame to world frame.
    void LocalToWorld(const double* plane, double* world) const
      { fTrans.LocalToWorld(plane, world); }
      
    /// Transform point from local plane frame to world frame.
    TVector3 LocalToWorld(const TVector3& local) const
      { return fTrans.LocalToWorld(local); }
    
    /// Transform direction vector from local to world.
    void LocalToWorldVect(const double* plane, double* world) const
      { fTrans.LocalToWorldVect(plane, world); }
    
    /// Transform direction vector from local to world.
    TVector3 LocalToWorldVect(const TVector3& local) const
      { return fTrans.LocalToWorldVect(local); }
    
    /// Transform point from world frame to local plane frame.
    void WorldToLocal(const double* world, double* plane) const
      { fTrans.WorldToLocal(world, plane); }
    
    /// Transform point from world frame to local plane frame.
    TVector3 WorldToLocal(TVector3 const& world) const
      { return fTrans.WorldToLocal(world); }
    
    /// Transform direction vector from world to local.
    void WorldToLocalVect(const double* world, double* plane) const
      { fTrans.WorldToLocalVect(world, plane); }
    
    /// Transform direction vector from world to local.
    TVector3 WorldToLocalVect(TVector3 const& world) const
      { return fTrans.WorldToLocalVect(world); }
    
    /// @}
    
    
    
    /// @{
    /// @name Setters
    
    /// Set the signal view (for TPCGeo).
    void SetView(geo::View_t view)                                  { fView = view; }
    
    /// @}
    
    /// Apply sorting to WireGeo objects.
    void SortWires(geo::GeoObjectSorter const& sorter);
    
    /// Performs all needed updates after the TPC has sorted the planes.
    void UpdateAfterSorting
      (geo::PlaneID planeid, geo::BoxBoundedGeo const& TPCbox);
    
    /// Returns the name of the specified view.
    static std::string ViewName(geo::View_t view);
    
    /// Returns the name of the specified orientation.
    static std::string OrientationName(geo::Orient_t orientation);
    
    
    /// Returns value, but rounds it to 0, -1 or +1 if value is closer than tol.
    template <typename T>
    static T roundUnit(T value, T tol)
      {
        if (std::abs(value) < tol) return 0.;
        if (std::abs(std::abs(value) - 1.) < tol) return (value > 0.)? 1.: -1.;
        return value;
      } // roundUnit()
    
    /// Rounds in place all components of a vector using
    /// `geo::vect::RoundValue01()`.
    static void roundVector(TVector3& v, double tol)
      { geo::vect::Round01(v, tol); }
    
    /// Returns a vector with all components rounded using `roundUnit()`.
    static TVector3 roundedVector(TVector3 const& v, double tol)
      { return geo::vect::Rounded01(v, tol); }
    
  private:
    
    void FindWire(GeoNodePath_t& path, size_t depth);
    void MakeWire(GeoNodePath_t& path, size_t depth);
    
    /// Sets the geometry directions.
    void DetectGeometryDirections();
    
    /// Returns a direction normal to the plane (pointing is not defined).
    TVector3 GetNormalAxis() const;
    
    /// Updates the cached normal to plane versor; needs the TPC box coordinates.
    void UpdatePlaneNormal(geo::BoxBoundedGeo const& TPCbox);
    
    /// Updates the cached depth and width direction.
    void UpdateWidthDepthDir();
    
    /// Updates the cached direction to increasing wires.
    void UpdateIncreasingWireDir();
    
    /// Updates the cached direction to wire.
    void UpdateWireDir();
    
    /// Updates plane orientation.
    void UpdateOrientation();
    
    /// Updates the stored wire pitch.
    void UpdateWirePitch();
    
    /// Updates the stored wire plane center.
    void UpdateWirePlaneCenter();
    
    /// Updates the stored @f$ \phi_{z} @f$.
    void UpdatePhiZ();
    
<<<<<<< HEAD
    /// Updates the stored view
    void UpdateView();

    /// Performs all the updates needed after mapping changes
    void UpdateFromMapping();
=======
    /// Updates the stored wire pitch with a slower, more robust algorithm.
    void UpdateWirePitchSlow();
    
    /// Updates the position of the wire coordinate decomposition.
    void UpdateDecompWireOrigin();
    
    /// Whether the specified wire should have start and end swapped.
    bool shouldFlipWire(geo::WireGeo const& wire) const;

>>>>>>> master
    
  private:
    using LocalTransformation_t = geo::LocalTransformation<TGeoHMatrix>;
    
    struct RectSpecs {
      double halfWidth;
      double halfDepth;
      
      double HalfWidth() const { return halfWidth; }
      double HalfDepth() const { return halfDepth; }
      double Width() const { return 2.0 * HalfWidth(); }
      double Depth() const { return 2.0 * HalfDepth(); }
    }; // RectSpecs
    
    LocalTransformation_t fTrans;       ///< Plane to world transform.
    TGeoVolume const*     fVolume;      ///< Plane volume description.
    View_t                fView;        ///< Does this plane measure U, V, or W?
    Orient_t              fOrientation; ///< Is the plane vertical or horizontal?
    std::vector<WireGeo*> fWire;        ///< List of wires in this plane.
    double                fWirePitch;   ///< Pitch of wires in this plane.
    double                fSinPhiZ;     ///< Sine of @f$ \phi_{z} @f$.
    double                fCosPhiZ;     ///< Cosine of @f$ \phi_{z} @f$.
    
    TVector3              fNormal;      ///< Normal to the plane, inward in TPC.
    /// Decomposition on wire coordinates; the main direction is along the wire,
    /// the secondary one is the one measured by the wire, the normal matches
    /// the plane's normal.
    WireDecomposer_t      fDecompWire;
    /// Decomposition on frame coordinates; the main direction is a "width",
    /// the secondary one is just orthogonal to it ("depth").
    /// Normal can differ in sign from the plane one.
    WidthDepthDecomposer_t fDecompFrame;
    RectSpecs             fFrameSize;   ///< Size of the frame of the plane.
    /// Center of the plane, lying on the wire plane.
    TVector3              fCenter;

    geo::PlaneID          fID;          ///< ID of this plane.

  };
}


//------------------------------------------------------------------------------
//--- template implementation
//---
template <typename Stream>
void geo::PlaneGeo::PrintPlaneInfo(
  Stream&& out,
  std::string indent /* = "" */,
  unsigned int verbosity /* = 1 */
) const {
  
  //----------------------------------------------------------------------------
  out << "plane " << std::string(ID());
  
  if (--verbosity <= 0) return; // 0
  
  //----------------------------------------------------------------------------
  decltype(auto) center = GetCenter();
  
  out
    << " at (" << center.X() << ", " << center.Y() << ", " << center.Z()
      << ") cm"
    << ", theta: " << ThetaZ() << " rad";
  
  if (--verbosity <= 0) return; // 1
  
  //----------------------------------------------------------------------------
  unsigned int const nWires = Nwires();
  
  out << "\n" << indent
    << "normal to wire: " << PhiZ() << " rad"
      << ", with orientation " << OrientationName(Orientation())
      << ", has " << nWires << " wires measuring " << ViewName(View())
      << " with a wire pitch of " << WirePitch() << " cm"
    ;
  
  if (--verbosity <= 0) return; // 2
  
  //----------------------------------------------------------------------------
  auto normal = GetNormalDirection();
  auto incrZdir = GetIncreasingWireDirection();
  decltype(auto) wireNormalDir = fDecompWire.NormalDir();
  out << "\n" << indent
    << "normal to plane: ("
      << normal.X() << ", " << normal.Y() << ", " << normal.Z() << ")"
    << ", direction of increasing wire number: ("
      << incrZdir.X() << ", " << incrZdir.Y() << ", " << incrZdir.Z() << ")"
    << " [wire frame normal: (" << wireNormalDir.X()
      << ", " << wireNormalDir.Y() << ", " << wireNormalDir.Z() << ")]"
    << " (" << (WireIDincreasesWithZ()? "increases": "decreases") << " with z)";
    
  if (--verbosity <= 0) return; // 3
  
  //----------------------------------------------------------------------------
  
  decltype(auto) wireDir = GetWireDirection();
  decltype(auto) widthDir = WidthDir();
  decltype(auto) depthDir = DepthDir();
  decltype(auto) frameNormalDir = fDecompFrame.NormalDir();
  
  out << "\n" << indent
    << "wire direction: ("
      << wireDir.X() << ", " << wireDir.Y() << ", " << wireDir.Z() << ")"
    << "; width " << Width() << " cm in direction: ("
      << widthDir.X() << ", " << widthDir.Y() << ", " << widthDir.Z() << ")"
    << ", depth " << Depth() << " cm in direction: ("
      << depthDir.X() << ", " << depthDir.Y() << ", " << depthDir.Z() << ")"
    << " [normal: (" << frameNormalDir.X()
      << ", " << frameNormalDir.Y() << ", " << frameNormalDir.Z() << ")]"
    ;
    
  if (--verbosity <= 0) return; // 4
  
  //----------------------------------------------------------------------------
  // get the area spanned by the wires
  auto plane_area = Coverage();

  out << "\n" << indent << "wires cover ";
  bool bPrint2D = false;
  if (plane_area.isPlane()) {
    switch (Orientation()) {
      case geo::kVertical:
        out << plane_area.DeltaY() << " x " << plane_area.DeltaZ() << " cm";
        bPrint2D = true;
        break;
      case geo::kHorizontal:
        out << plane_area.DeltaX() << " x " << plane_area.DeltaZ() << " cm";
        bPrint2D = true;
        break;
      default: break;
    } // switch
  }
  if (!bPrint2D) {
    out << "between " << plane_area.Min() << " and " << plane_area.Max()
      << " (cm)";
  }
  out << " around " << plane_area.Center();
  if (--verbosity <= 0) return; // 5
  
  //----------------------------------------------------------------------------
  // print also the containing box
  auto const box = BoundingBox();
  out << "\n" << indent
    << "bounding box: ( "
      << box.MinX() << ", " << box.MinY() << ", " << box.MinZ()
    << " ) -- ( "
      << box.MaxX() << ", " << box.MaxY() << ", " << box.MaxZ()
    << " )";
  
  
  //----------------------------------------------------------------------------
} // geo::PlaneGeo::PrintPlaneInfo()


#endif
////////////////////////////////////////////////////////////////////////
