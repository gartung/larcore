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
#include "larcore/Geometry/BoxBoundedGeo.h"

// C/C++ standard libraries
#include <cmath> // std::atan2()
#include <vector>
#include <array>
#include <string>

class TGeoNode;
class TGeoHMatrix;
class TVector3;

namespace geo {
  class WireGeo;

  //......................................................................
  
  /// Geometry information for a single readout plane
  // Note: SignalType() and SetSignalType() have been removed.
  //       Use `geo::GeometryCore::SignalType` instead.
  //       (see LArSoft issue #14365 at https://cdcvs.fnal.gov/redmine/issues/14365 )
  class PlaneGeo {
  public:
    
    /// Construct a representation of a single plane of the detector
    PlaneGeo(std::vector<const TGeoNode*>& path, int depth);
    ~PlaneGeo();

    
    /// @{
    /// @name Plane properties
    
    /// Which coordinate does this plane measure
    View_t View()                                             const { return fView;          }
    
    /// What is the orienation of the plane
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
     * @brief Returns whether a wire with index iwire is present in this plane
     * @param iwire index of wire in this plane
     * @return whether the wire with index iwire is present in this plane
     */
    bool HasWire(unsigned int iwire) const { return iwire < Nwires(); }
    bool HasElement(unsigned int iwire) const { return HasWire(iwire); }
    //@}
    
    //@{
    /**
     * @brief Returns whether the wire in wireid is present in this plane
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
     * @brief Returns the wire in wireid from this plane
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
     * @brief Returns the wire number iwire from this plane
     * @param iwire the number of local wire
     * @return a constant pointer to the wire, or nullptr if it does not exist
     */
    WireGeo const* WirePtr(unsigned int iwire) const
      { return HasWire(iwire)? fWire[iwire]: nullptr; }
    
    //@{
    /**
     * @brief Returns the wire in wireid from this plane
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
     * @brief Allows range-for iteration on all wires in this plane
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
    
    double WirePitch()                                        const { return fWirePitch; }
    
    /**
     * @brief Returns whether the higher z wires have higher wire ID
     * @return whether the higher z wires have higher wire ID
     * @see GetIncreasingWireDirection()
     * 
     * This method is related to GetIncreasingWireDirection()
     * (it might be expressed as "GetIncreasingWireDirection()[2] > 0"),
     * but it is implemented in a faster and independent way.
     */
    bool WireIDincreasesWithZ() const;
    
    /**
     * @brief Returns the direction normal to the plane
     * @return a TVector3 versor with a direction normal to the plane
     *
     * The versor is orthogonal to the plane.
     * The direction is defined so that the semi-space pointed to contains
     * the TPC center.
     */
    TVector3 const& GetNormalDirection() const { return fNormal; }

    /**
     * @brief Returns the direction of increasing wires
     * @return a TVector3 versor with the direction of increasing wires
     *
     * The versor is orthogonal to the wires (assumed parallel),
     * lies on the plane and its direction goes toward increasing wire IDs.
     */
    TVector3 const& GetIncreasingWireDirection() const { return fWireCoordDir; }
    
    
    /// Returns the centre of the plane in world coordinates [cm]
    TVector3 GetCenter() const;
    
    
    /**
     * @brief Returns the distance of the specified point from the plane
     * @param point a point in world coordinates [cm]
     * @return the signed distance from the plane
     * 
     * The distance is defined positive if the point lies in the side the normal
     * vector (GetNormalDirection()) points to.
     * 
     * It should match the drift distance from this plane.
     */
    double DistanceFromPlane(TVector3 const& point) const
      { return GetNormalDirection().Dot(point - GetCenter()); }
    
    
    /**
     * @brief Returns the coordinate of the point on the plane respect to a wire
     * @param point world coordinate of the point to get the coordinate of [cm]
     * @param refWire reference wire
     * @return the coordinate of the point [cm]
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
     * 
     * @note This method needs to be validated.
     */
    double WireCoordinateFrom
      (TVector3 const& point, geo::WireGeo const& refWire) const;
    
    /**
     * @brief Returns the coordinate of the point on the plane
     * @param point world coordinate of the point to get the coordinate of [cm]
     * @return the coordinate of the point [cm]
     * @see CoordinateFrom(TVector3 const&, geo::Wire const&)
     * 
     * The method returns the coordinate of the point in the direction measured
     * by the wires on this plane starting on the first wire, in world units
     * (that is, centimeters).
     *  
     * The point does not need to be on the plane, and the projection of the
     * point to the plane is considered.
     * 
     * @note This method needs to be validated.
     */
    double WireCoordinate(TVector3 const& point) const
      { return WireCoordinateFrom(point, FirstWire()); }
    
    
    
    /// Returns a volume including all the wires in the plane
    lar::util::simple_geo::Volume<> Coverage() const;
    
    /**
     * @brief Prints information about this plane
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
     * * 1 _(default)_: also wire angle
     * * 2: also information about position and wires
     * * 3: also information about increasing coordinate direction
     * * 4: also coverage
     * 
     */
    template <typename Stream>
    void printPlaneInfo
      (Stream&& out, std::string indent = "", unsigned int verbosity = 1) const;
    
    /// @}
    
    
    /// @{
    /// @name Coordinate transformation
    
    /// Transform point from local plane frame to world frame
    void LocalToWorld(const double* plane, double* world)     const;
    
    /// Transform direction vector from local to world
    void LocalToWorldVect(const double* plane, double* world) const;

    // Again, with TVectors
    const TVector3 LocalToWorld( const TVector3& local )      const;

    /// Transform point from world frame to local plane frame
    void WorldToLocal(const double* world, double* plane)     const;

    /// Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* plane) const;
    
    // Again, with TVectors
    const TVector3 WorldToLocal( const TVector3& world )      const;
    
    /// @}
    
    
    /// @{
    /// @name Setters
    
    /// Set the signal view (for TPCGeo)
    void SetView(geo::View_t view)                                  { fView = view; }
    
    /// @}
    
    /// Apply sorting to WireGeo objects
    void SortWires(geo::GeoObjectSorter const& sorter);
    
    /// Performs all needed updates after the TPC has sorted the planes
    void UpdateAfterSorting
      (geo::PlaneID planeid, geo::BoxBoundedGeo const& TPCbox);
    
    /// Returns the name of the specified view
    static std::string ViewName(geo::View_t view);
    
    /// Returns the name of the specified orientation
    static std::string OrientationName(geo::Orient_t orientation);
    
    
    /// Returns value, but rounds it to 0, -1 or +1 if value is closer than tol
    template <typename T>
    static T roundUnit(T value, T tol)
      {
        if (std::abs(value) < tol) return 0.;
        if (std::abs(std::abs(value) - 1.) < tol) return (value > 0.)? 1.: -1.;
        return value;
      } // roundUnit()
    
    /// Rounds in place all components of a vector using roundUnit()
    static void roundVector(TVector3& v, double tol)
      {
        v.SetX(roundUnit(v.X(), tol)); 
        v.SetY(roundUnit(v.Y(), tol));
        v.SetZ(roundUnit(v.Z(), tol));
      } // roundVector()
    
  private:
    
    void FindWire(std::vector<const TGeoNode*>& path,
		  unsigned int depth);
    void MakeWire(std::vector<const TGeoNode*>& path, 
		  int depth);
    
    /// Returns a direction normal to the plane (pointing is not defined)
    TVector3 GetNormalAxis() const;
    
    /// Updates the cached normal to plane versor; needs the TPC box coordinates
    void UpdatePlaneNormal(geo::BoxBoundedGeo const& TPCbox);
    
    /// Updates the cached direction to increasing wires
    void UpdateIncreasingWireDir();
    
    /// Updates plane orientation
    void UpdateOrientation();
    
    /// Updates the stored wire pitch
    void UpdateWirePitch();
    
    /// Updates the stored phi_z
    void UpdatePhiZ();
    
    /// Updates the stored wire pitch with a slower, more robust algorithm
    void UpdateWirePitchSlow();
    
    /// Whether the specified wire should have start and end swapped
    bool shouldFlipWire(geo::WireGeo const& wire) const;

    
  private:
    TGeoHMatrix*          fGeoMatrix;   ///< Plane to world transform
    View_t                fView;        ///< Does this plane measure U,V, or W?
    Orient_t              fOrientation; ///< Is the plane vertical or horizontal?
    SigType_t             fSignalType;  ///< Is the plane induction or collection?
    std::vector<WireGeo*> fWire;        ///< List of wires in this plane
    double                fWirePitch;   ///< pitch of wires in this plane
    double                fSinPhiZ;     ///< sine of phiZ
    double                fCosPhiZ;     ///< cosine of phiZ
    
    TVector3 fNormal; ///< normal to the plane, points to TPC center
    TVector3 fWireCoordDir; ///< direction measured by the wires
    
    geo::PlaneID          fID;          ///< ID of this plane

  };
}


//------------------------------------------------------------------------------
//--- template implementation
//---
template <typename Stream>
void geo::PlaneGeo::printPlaneInfo(
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
  out << "\n" << indent
    << "normal to plane: ("
      << normal.X() << ", " << normal.Y() << ", " << normal.Z() << ")"
    << ", direction of increasing wire number: ("
      << incrZdir.X() << ", " << incrZdir.Y() << ", " << incrZdir.Z() << ")"
    << " (" << (WireIDincreasesWithZ()? "increases": "decreases") << " with z)";
    
  if (--verbosity <= 0) return; // 3
  
  //----------------------------------------------------------------------------
  // get the area spanned by the wires
  auto plane_area = Coverage();

  out << "\n" << indent << "its wires cover ";
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
  
  //----------------------------------------------------------------------------
} // geo::PlaneGeo::printPlaneInfo()


#endif
////////////////////////////////////////////////////////////////////////
