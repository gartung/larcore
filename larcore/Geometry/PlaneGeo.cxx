////////////////////////////////////////////////////////////////////////
/// \file PlaneGeo.cxx
///
/// \version $Id: PlaneGeo.cxx,v 1.12 2010/03/05 19:47:51 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////



// LArSoft includes
#include "larcore/Geometry/Exceptions.h" // geo::InvalidWireError
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"
#include "larcore/CoreUtils/RealComparisons.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// ROOT includes
#include "TMath.h"
#include "TVector3.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoBBox.h"
#include "TGeoMatrix.h"
#include "TClass.h"

// C/C++ standard library
#include <array>
#include <cassert>


// print a TVector3, for debugging purpose
std::ostream& operator<< (std::ostream& out, TVector3 const& v) {
  out << "( " << v.X() << " ; " << v.Y() << " ; " << v.Z() << " )";
  return out;
}

namespace {
  
  /// Returns the offset to apply to value to move it inside [ -limit, +limit ].
  template <typename T>
  T symmetricCapDelta(T value, T limit) {
    
    return (value < -limit)
      ? -limit - value
      : (value > +limit)
        ? +limit - value
        : 0.0
      ;
    
  } // symmetricCapDelta()
  
  
  /// Returns a value shifted to fall into [ -limit; +limit ] interval.
  template <typename T>
  T symmetricCap(T value, T limit) {
    
    return value + symmetricCapDelta(value, limit);
    
  } // symmetricCap()
  
  
} // local namespace

namespace geo{


  //......................................................................
  PlaneGeo::PlaneGeo(GeoNodePath_t& path, size_t depth)
    : fTrans(path, depth)
    , fVolume(path[depth]->GetVolume())
    , fView(geo::kUnknown)
    , fOrientation(geo::kVertical)
    , fWirePitch(0.)
    , fSinPhiZ(0.)
    , fCosPhiZ(0.)
    , fDecompWire()
    , fDecompFrame()
    , fCenter()
  {
    assert(depth < path.size());
    
    if (!fVolume) {
      TGeoNode const* pNode = path[depth];
      throw cet::exception("PlaneGeo")
        << "Plane geometry node " << pNode->IsA()->GetName()
        << "[" << pNode->GetName() << ", #" << pNode->GetNumber()
        << "] has no volume!\n";
    }
    
    // find the wires for the plane so that you can use them later
    FindWire(path, depth);
    
    // view is now set at TPC level with SetView
    
    DetectGeometryDirections();
    UpdateWirePitchSlow();
    
  } // PlaneGeo::PlaneGeo()

  //......................................................................

  PlaneGeo::~PlaneGeo()
  {
    for(unsigned int i = 0; i < fWire.size(); ++i)
      if(fWire[i]) delete fWire[i];
  
    fWire.clear();

  }

  //......................................................................
  geo::BoxBoundedGeo PlaneGeo::BoundingBox() const {
    
    //
    // The algorithm is not very refined...
    //
    
    TGeoBBox const* pShape = dynamic_cast<TGeoBBox const*>(fVolume->GetShape());
    if (!pShape) {
      throw cet::exception("PlaneGeo")
        << "BoundingBox(): volume " << fVolume->IsA()->GetName()
        << "['" << fVolume->GetName() << "'] has a shape which is a "
        << pShape->IsA()->GetName()
        << ", not a TGeoBBox!";
    }
    
    geo::BoxBoundedGeo box;
    unsigned int points = 0;
    for (double dx: { -(pShape->GetDX()), +(pShape->GetDX()) }) {
      for (double dy: { -(pShape->GetDY()), +(pShape->GetDY()) }) {
        for (double dz: { -(pShape->GetDZ()), +(pShape->GetDZ()) }) {
          
          auto const p = LocalToWorld({ dx, dy, dz });
          
          if (points++ == 0)
            box.SetBoundaries(p.X(), p.X(), p.Y(), p.Y(), p.Z(), p.Z());
          else
            box.ExtendToInclude(p.X(), p.Y(), p.Z());
          
        } // for z
      } // for y
    } // for x
    return box;
    
  } // PlaneGeo::BoundingBox()
  
  //......................................................................
  
  geo::WireGeo const& PlaneGeo::Wire(unsigned int iwire) const {
    geo::WireGeo const* pWire = WirePtr(iwire);
    if (!pWire) {
      throw cet::exception("WireOutOfRange")
        << "Request for non-existant wire " << iwire << "\n";
    }
    return *pWire;
  } // PlaneGeo::Wire(int)
  
  //......................................................................

  void PlaneGeo::FindWire(GeoNodePath_t& path, size_t depth) 
  {
    // Check if the current node is a wire
    const char* wireLabel = "volTPCWire";
    auto const labelLength = strlen(wireLabel);
    if(strncmp(path[depth]->GetName(), wireLabel, labelLength) == 0){
      MakeWire(path, depth);
      return;
    }
  
    // Explore the next layer down
    unsigned int deeper = depth+1;
    if (deeper>=path.size()) {
      throw cet::exception("ExceededMaxDepth") << "Exceeded maximum depth\n";
    }
    const TGeoVolume* v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for (int i=0; i<nd; ++i) {
      path[deeper] = v->GetNode(i);
      FindWire(path, deeper);
    }
  }

  //......................................................................

  void PlaneGeo::MakeWire(GeoNodePath_t& path, size_t depth) 
  {
    fWire.push_back(new WireGeo(path, depth));
  }

  //......................................................................

  // sort the WireGeo objects
  void PlaneGeo::SortWires(geo::GeoObjectSorter const& sorter )
  {
    sorter.SortWires(fWire);
  }
  
  
  //......................................................................
  bool PlaneGeo::WireIDincreasesWithZ() const {
    return GetIncreasingWireDirection().Z() > 0.;
  } // PlaneGeo::WireIDincreasesWithZ()
  
  
  //......................................................................
  TVector3 PlaneGeo::GetBoxCenter() const {
    
    // convert the origin (default constructed TVector)
    return LocalToWorld({});
    
  } // PlaneGeo::GetBoxCenter()
  
  
  //......................................................................
  lar::util::simple_geo::Volume<> PlaneGeo::Coverage() const {
    
    // add both coordinates of first and last wire
    std::array<double, 3> A, B;
    
    FirstWire().GetStart(A.data());
    LastWire().GetEnd(B.data());
    
    return { A.data(), B.data() };
  } // PlaneGeo::Coverage()
  
  
  //......................................................................
  PlaneGeo::WidthDepthProjection_t PlaneGeo::DeltaFromPlane
    (WidthDepthProjection_t const& proj) const
  {
    
    return {
      symmetricCapDelta(proj.X(), fFrameSize.HalfWidth()),
      symmetricCapDelta(proj.Y(), fFrameSize.HalfDepth())
    };
    
  } // PlaneGeo::DeltaFromPlane()
  
  
  //......................................................................
  bool PlaneGeo::isProjectionOnPlane(TVector3 const& point) const {
    
    auto const deltaProj
      = DeltaFromPlane(PointWidthDepthProjection(point));
    
    return (deltaProj.X() == 0.) && (deltaProj.Y() == 0.);
    
  } // PlaneGeo::isProjectionOnPlane()
  
  
  //......................................................................
  PlaneGeo::WidthDepthProjection_t PlaneGeo::MoveProjectionToPlane
    (WidthDepthProjection_t const& proj) const
  {
    
    return proj + DeltaFromPlane(proj);
    
  } // PlaneGeo::MoveProjectionToPlane()
  
  
  //......................................................................
  TVector3 PlaneGeo::MovePointOverPlane(TVector3 const& point) const {
    
    auto const deltaProj
      = DeltaFromPlane(PointWidthDepthProjection(point));
    
    return point + deltaProj.X() * WidthDir() + deltaProj.Y() * DepthDir();
    
  } // PlaneGeo::MovePointOverPlane()
  
  
  //......................................................................
  geo::WireID PlaneGeo::NearestWireID(TVector3 const& pos) const {
    
    //
    // 1) compute the wire coordinate of the point
    // 2) get the closest wire number
    // 3) check if the wire does exist
    // 4) build and return the wire ID
    //
    
    // this line merges parts (1) and (2); add 0.5 to have the correct rounding:
    int nearestWireNo = int(0.5 + WireCoordinate(pos));
    
    // if we are outside of the wireplane range, throw an exception
    if ((nearestWireNo < 0) || ((unsigned int) nearestWireNo >= Nwires())) {
      
      auto wireNo = nearestWireNo; // save for the output
      
      if (nearestWireNo < 0 ) wireNo = 0;
      else                    wireNo = Nwires() - 1;
      
      throw InvalidWireError("Geometry", ID(), nearestWireNo, wireNo)
        << "Can't find nearest wire for position " << pos
        << " in plane " << std::string(ID()) << " approx wire number # "
        << wireNo << " (capped from " << nearestWireNo << ")\n";
    } // if invalid

    return { ID(), (geo::WireID::WireID_t) nearestWireNo };
    
  } // PlaneGeo::NearestWireID()
  
  
  //......................................................................
  double PlaneGeo::ThetaZ() const { return FirstWire().ThetaZ(); }
  
  
  //......................................................................
  void PlaneGeo::UpdateAfterSorting
    (geo::PlaneID planeid, geo::BoxBoundedGeo const& TPCbox)
  {
    // the order here matters
    
    // reset our ID
    fID = planeid;
    
    UpdatePlaneNormal(TPCbox);
    UpdateWidthDepthDir();
    UpdateIncreasingWireDir();
    
    // update wires
    geo::WireID::WireID_t wireNo = 0;
    for (geo::WireGeo* wire: fWire) {
      
      wire->UpdateAfterSorting(geo::WireID(fID, wireNo), shouldFlipWire(*wire));
      
      ++wireNo;
    } // for wires
    
    UpdateDecompWireOrigin();
    UpdateWireDir();
    UpdateWirePlaneCenter();
    UpdateOrientation();
    UpdateWirePitch();
    UpdatePhiZ();
    UpdateView();
    
  } // PlaneGeo::UpdateAfterSorting()
  
  //......................................................................
  std::string PlaneGeo::ViewName(geo::View_t view) {
    switch (view) {
      case geo::kU:       return "U";
      case geo::kV:       return "V";
      case geo::kZ:       return "Z";
      case geo::k3D:      return "3D";
      case geo::kUnknown: return "?";
      default:            return "<UNSUPPORTED>";
    } // switch
  } // PlaneGeo::ViewName()
  
  //......................................................................
  std::string PlaneGeo::OrientationName(geo::Orient_t orientation) {
    switch (orientation) {
      case geo::kHorizontal: return "horizontal"; break;
      case geo::kVertical:   return "vertical"; break;
      default:               return "unexpected"; break;
    } // switch
  } // PlaneGeo::OrientationName()
  
  
  //......................................................................
  void PlaneGeo::DetectGeometryDirections() {
    
    //
    // We need to identify which are the "long" directions of the plane.
    // We assume it is a box, and the shortest side is excluded.
    // The first direction ("width") is given by preference to z.
    // If z is the direction of the normal to the plane... oh well.
    // Let's say privilege to the one which comes from local z, then y.
    // That means: undefined.
    // 
    // Requirements:
    //  - ROOT geometry information (shapes and transformations)
    //  - the shape must be a box (an error is PRINTED if not)
    //  - center of the wire plane (not just the center of the plane box)
    //
    
    //
    // how do they look like in the world?
    //
    TGeoBBox const* pShape = dynamic_cast<TGeoBBox const*>(fVolume->GetShape());
    if (!pShape) {
      mf::LogError("BoxInfo")
        << "Volume " << fVolume->IsA()->GetName() << "['" << fVolume->GetName()
        << "'] has a shape which is a " << pShape->IsA()->GetName()
        << ", not a TGeoBBox! Dimensions won't be available.";
      // set it invalid
      fDecompFrame.SetOrigin({ 0., 0., 0. });
      fDecompFrame.SetMainDir({ 0., 0., 0. });
      fDecompFrame.SetSecondaryDir({ 0., 0., 0. });
      fFrameSize = { 0.0, 0.0 };
      return;
    }
    
    TVector3 sides[3];
    size_t iSmallest = 3;
    {
      
      size_t iSide = 0;
      TVector3 dir;
      
      sides[iSide] = LocalToWorldVect({ pShape->GetDX(), 0.0, 0.0 });
      iSmallest = iSide;
      ++iSide;
      
      sides[iSide] = LocalToWorldVect({ 0.0, pShape->GetDY(), 0.0 });
      if (sides[iSide].Mag2() < sides[iSmallest].Mag2()) iSmallest = iSide;
      ++iSide;
      
      sides[iSide] = LocalToWorldVect({ 0.0, 0.0, pShape->GetDZ() });
      if (sides[iSide].Mag2() < sides[iSmallest].Mag2()) iSmallest = iSide;
      ++iSide;
      
    }
    
    //
    // which are the largest ones?
    //
    size_t kept[2];
    {
      size_t iKept = 0;
      for (size_t i = 0; i < 3; ++i) if (i != iSmallest) kept[iKept++] = i;
    }
    
    //
    // which is which?
    // 
    // Pick width as the most z-like.
    //
    size_t const iiWidth =
      std::abs(sides[kept[0]].Unit().Z()) > std::abs(sides[kept[1]].Unit().Z())
      ? 0: 1;
    size_t const iWidth = kept[iiWidth];
    size_t const iDepth = kept[1 - iiWidth]; // the other
    
    fDecompFrame.SetMainDir(roundedVector(sides[iWidth].Unit(), 1e-4));
    fDecompFrame.SetSecondaryDir(roundedVector(sides[iDepth].Unit(), 1e-4));
    fFrameSize.halfWidth = sides[iWidth].Mag();
    fFrameSize.halfDepth = sides[iDepth].Mag();
    
  } // PlaneGeo::DetectGeometryDirections()

  //......................................................................
  TVector3 PlaneGeo::GetNormalAxis() const {
    const unsigned int NWires = Nwires();
    if (NWires < 2) return TVector3(); // why are we even here?
    
    // 1) get the direction of the middle wire
    TVector3 WireDir = Wire(NWires / 2).Direction();
    
    // 2) get the direction between the middle wire and the next one
    double MiddleWireCenter[3], NextToMiddleWireCenter[3];
    Wire(NWires / 2).GetCenter(MiddleWireCenter);
    Wire(NWires / 2 + 1).GetCenter(NextToMiddleWireCenter);
    TVector3 ToNextWire(NextToMiddleWireCenter);
    ToNextWire -= TVector3(MiddleWireCenter);
    
    // 3) get the direction perpendicular to the plane
    TVector3 PlaneNorm = WireDir.Cross(ToNextWire);
    
    // 4) round it
    for (int i = 0; i < 3; ++i)
      if (std::abs(PlaneNorm[i]) < 1e-4) PlaneNorm[i] = 0.;
    
    // 5) return its norm
    return PlaneNorm.Unit();
  } // GeometryTest::GetNormalAxis()
  
  
  //......................................................................
  void PlaneGeo::UpdateOrientation() {
    
    //
    // this algorithm needs to know about the axis;
    // the normal is expected to be already updated.
    //
    
    // sanity check
    if (fWire.size() < 2) {
      // this likely means construction is not complete yet
      throw cet::exception("NoWireInPlane")
        << "PlaneGeo::UpdateOrientation(): only " << fWire.size()
        << " wires!\n";
    } // if
    
    auto normal = GetNormalDirection();
    
    if (std::abs(std::abs(normal.X()) - 1.) < 1e-3)
      fOrientation = kVertical;
    else if (std::abs(std::abs(normal.Y()) - 1.) < 1e-3)
      fOrientation = kHorizontal;
    else {
      // at this point, the only problem is the lack of a label for this
      // orientation; probably introducing a geo::kOtherOrientation would
      // suffice
      throw cet::exception("Geometry")
        << "Plane with unsupported orientation (normal: { "
        << normal.X() << " ; " << normal.Y() << " ; " << normal.Z() << " })\n";
    }
    
  } // PlaneGeo::UpdateOrientation()

  //......................................................................
  void PlaneGeo::UpdateWirePitch() {
    fWirePitch = geo::WireGeo::WirePitch(Wire(0), Wire(1));
  } // PlaneGeo::UpdateWirePitch()
  
  //......................................................................
  void PlaneGeo::UpdatePhiZ() {
    TVector3 const& wire_coord_dir = GetIncreasingWireDirection();
  /*
    TVector3 const& normal = GetNormalDirection();
    TVector3 z(0., 0., 1.);
    
    // being defined in absolute terms as angle respect to z axis,
    // we take the z component as cosine, and all the rest as sine
    fCosPhiZ = wire_coord_dir.Dot(z);
    fSinPhiZ = wire_coord_dir.Cross(z).Dot(normal);
  */  
    fCosPhiZ = wire_coord_dir.Z();
    fSinPhiZ = wire_coord_dir.Y();
  } // PlaneGeo::UpdatePhiZ()
    
  void PlaneGeo::UpdateView()
  {
      auto cos_thetaz = std::cos(ThetaZ());
      
      if( std::abs(cos_thetaz - 1.0) < std::numeric_limits<double>::epsilon() ||
          std::abs(cos_thetaz + 1.0) < std::numeric_limits<double>::epsilon())
          SetView(geo::View_t::kY);
      else if( std::abs(cos_thetaz - 0.0) < std::numeric_limits<double>::epsilon())
          SetView(geo::View_t::kZ);
      else if( cos_thetaz < 0.0 )
          SetView(geo::View_t::kV);
      else if( cos_thetaz > 0.0 )
          SetView(geo::View_t::kU);
      else
          SetView(geo::View_t::kUnknown);
      
      return;
  }
  
  //......................................................................
  void PlaneGeo::UpdatePlaneNormal(geo::BoxBoundedGeo const& TPCbox) {
    
    //
    // direction normal to the wire plane, points toward the center of TPC
    //
    
    // start from the axis
    fNormal = GetNormalAxis();
    
    // now evaluate where we are pointing
    TVector3 TPCcenter(TPCbox.Center().data());
    TVector3 towardCenter = TPCcenter - GetBoxCenter();
    
    // if they are pointing in opposite directions, flip the normal
    if (fNormal.Dot(towardCenter) < 0) fNormal = -fNormal;
    roundVector(fNormal, 1e-3);
    
  } // PlaneGeo::UpdatePlaneNormal()
  
  
  //......................................................................
  void PlaneGeo::UpdateWidthDepthDir() {
    
    //
    // fix the positiveness of the width/depth/normal frame
    //
    
    // The basis is already set and orthonormal, with only the width
    // and depth directions arbitrary.
    // We choose the direction of the secondary axis ("depth")
    // so that the frame normal is oriented in the general direction of the
    // plane normal (the latter is computed independently).
    if (WidthDir().Cross(DepthDir()).Dot(GetNormalDirection()) < 0.0) {
      fDecompFrame.SetSecondaryDir
        (roundedVector(-fDecompFrame.SecondaryDir(), 1e-4));
    }
    
  } // PlaneGeo::UpdateWidthDepthDir()
  
  
  //......................................................................
  void PlaneGeo::UpdateIncreasingWireDir() {
    
    //
    // Direction measured by the wires, pointing toward increasing wire number;
    // requires:
    // - the normal to the plane to be correct
    // - wires to be sorted
    //
    
    // 1) get the direction of the middle wire
    auto refWireNo = Nwires() / 2;
    if (refWireNo == Nwires() - 1) --refWireNo;
    auto const& refWire = Wire(refWireNo);
    TVector3 WireDir = refWire.Direction(); // we only rely on the axis
    
    
    // 2) get the axis perpendicular to it on the wire plane
    //    (arbitrary direction)
    auto wireCoordDir = GetNormalDirection().Cross(WireDir).Unit();
    
    // 3) where is the next wire?
    TVector3 toNextWire
      = Wire(refWireNo + 1).GetCenter() - refWire.GetCenter();
    
    // 4) if wireCoordDir is pointing away from the next wire, flip it
    if (wireCoordDir.Dot(toNextWire) < 0) {
      wireCoordDir = -wireCoordDir;
    }
    fDecompWire.SetSecondaryDir(roundedVector(wireCoordDir, 1e-4));
    
  } // PlaneGeo::UpdateIncreasingWireDir()
  
  
  //......................................................................
  void PlaneGeo::UpdateWireDir() {
    
    fDecompWire.SetMainDir(roundedVector(FirstWire().Direction(), 1e-4));
    
    //
    // check that the resulting normal matches the plane one
    //
    assert(lar::util::makeVector3DComparison(1e-5)
      .equal(fDecompWire.NormalDir(), GetNormalDirection()));
    
  } // PlaneGeo::UpdateWireDir()
  
  
  //......................................................................
  void PlaneGeo::UpdateWirePitchSlow() {
    
    // 
    // Compare one wire (the first one, for convenience) with all other wires;
    // the wire pitch is the smallest distance we find.
    // 
    // This algorithm assumes wire pitch is constant, but it does not assume
    // wire ordering (which UpdateWirePitch() does).
    // 
    auto firstWire = fWire.cbegin(), wire = firstWire, wend = fWire.cend();
    fWirePitch = geo::WireGeo::WirePitch(**firstWire, **(++wire));
    
    while (++wire != wend) {
      auto wirePitch = geo::WireGeo::WirePitch(**firstWire, **wire);
      if (wirePitch < 1e-4) continue; // it's 0!
      if (wirePitch < fWirePitch) fWirePitch = wirePitch;
    } // while
    
  } // PlaneGeo::UpdateWirePitchSlow()
  
  
  //......................................................................
  void PlaneGeo::UpdateDecompWireOrigin() {
    
    //
    // update the origin of the reference frame (the middle of the first wire)
    //
    fDecompWire.SetOrigin(FirstWire().GetCenter());
    
  } // PlaneGeo::UpdateDecompWireOrigin()
  
  //......................................................................
  void PlaneGeo::UpdateWirePlaneCenter() {
    
    //
    // The center of the wire plane is defined as the center of the plane box,
    // translated to the plane the wires lie on.
    // This assumes that the thickness direction of the box is aligned with
    // the drift direction, so that the translated point is still in the middle
    // of width and depth dimensions.
    // It is possible to remove that assumption by translating the center of the
    // box along the thickness direction enough to bring it to the wire plane.
    // The math is just a bit less straightforward, so we don't bother yet.
    //
    // Requirements:
    //  * the wire decomposition frame must be set up (at least its origin and
    //    normal direction)
    //
    
    fCenter = GetBoxCenter();
    
    DriftPoint(fCenter, DistanceFromPlane(fCenter));
    
    fDecompFrame.SetOrigin(fCenter); // equivalent to GetCenter() now
    
  } // PlaneGeo::UpdateWirePlaneCenter()
  
  
  //......................................................................
  bool PlaneGeo::shouldFlipWire(geo::WireGeo const& wire) const {
    //
    // The correct orientation is so that:
    //
    // (direction) x (wire coordinate direction) . (plane normal)
    // 
    // is positive; it it's negative, then we should flip the wire.
    // 
    // Note that the increasing wire direction comes from the wire frame, while
    // the normal direction is computed independently by geometry.
    // The resulting normal in the wire frame is expected to be the same as the
    // plane normal from GetNormalDirection(); if this is not the case, flipping
    // the wire direction should restore it.
    // 
    
    return wire.Direction()
      .Cross(GetIncreasingWireDirection())
      .Dot(GetNormalDirection())
      < +0.5; // should be in fact exactly +1
    
  } // PlaneGeo::shouldFlipWire()
  
  //......................................................................
  
}
////////////////////////////////////////////////////////////////////////
