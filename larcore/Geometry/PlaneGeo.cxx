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

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// ROOT includes
#include "TMath.h"
#include "TVector3.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"

// C/C++ standard library
#include <array>


// print a TVector3, for debugging purpose
std::ostream& operator<< (std::ostream& out, TVector3 const& v) {
  out << "( " << v.X() << " ; " << v.Y() << " ; " << v.Z() << " )";
  return out;
}


namespace geo{


  //......................................................................
  PlaneGeo::PlaneGeo(std::vector<const TGeoNode*>& path, int depth)
    : fView(geo::kUnknown)
    , fOrientation(geo::kVertical)
    , fSignalType(geo::kMysteryType)
    , fWirePitch(0.)
    , fSinPhiZ(0.)
    , fCosPhiZ(0.)
    , fNormal()
    , fWireCoordDir()
  {
    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
  
    // find the wires for the plane so that you can use them later
    FindWire(path, depth);
    
    // view is now set at TPC level with SetView
    
    UpdateWirePitchSlow();
    
  } // PlaneGeo::PlaneGeo()

  //......................................................................

  PlaneGeo::~PlaneGeo()
  {
    for(unsigned int i = 0; i < fWire.size(); ++i)
      if(fWire[i]) delete fWire[i];
  
    fWire.clear();

    if(fGeoMatrix) delete fGeoMatrix;

  }

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

  void PlaneGeo::FindWire(std::vector<const TGeoNode*>& path,
			  unsigned int depth) 
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

  void PlaneGeo::MakeWire(std::vector<const TGeoNode*>& path, int depth) 
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
  TVector3 PlaneGeo::GetCenter() const {
    
    // convert the origin (default constructed TVector)
    return LocalToWorld({});
    
  } // PlaneGeo::GetCenter()
  
  
  //......................................................................
  double PlaneGeo::PlaneCoordinateFrom
    (TVector3 const& point, geo::WireGeo const& refWire) const
  {
    // a vector connecting to the point
    auto toPoint = point - refWire.GetCenter();
    
    // The distance is the projection of that vector on the plane, times the
    // sine of the angle with the wire.
    // All this machinery is realized by a "mixed" product.
    // The component of the point orthogonal to the plane is suppressed
    // (just separate point in the two components, and notice that one of the
    // two resulting mixed products has two vectors lying on the plane normal
    // and therefore vanishes).
    return refWire.Direction().Cross(toPoint).Dot(GetNormalDirection());
    
  } // PlaneGeo::PlaneCoordinateFrom()
  
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
  lar::util::simple_geo::Volume<> PlaneGeo::Coverage() const {
    
    // add both coordinates of first and last wire
    std::array<double, 3> A, B;
    
    FirstWire().GetStart(A.data());
    LastWire().GetEnd(B.data());
    
    return { A.data(), B.data() };
  } // PlaneGeo::Coverage()
  
  
  //......................................................................
  double PlaneGeo::ThetaZ() const { return FirstWire().ThetaZ(); }
  
  
  //......................................................................

  void PlaneGeo::LocalToWorld(const double* plane, double* world) const
  {
    fGeoMatrix->LocalToMaster(plane, world);
  }

  //......................................................................

  void PlaneGeo::LocalToWorldVect(const double* plane, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(plane, world);
  }

  //......................................................................

  void PlaneGeo::WorldToLocal(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocal(world, plane);
  }

  //......................................................................

  const TVector3 PlaneGeo::WorldToLocal( const TVector3& world ) const
  {
    double worldArray[4];
    double localArray[4];
    worldArray[0] = world.X();
    worldArray[1] = world.Y();
    worldArray[2] = world.Z();
    worldArray[3] = 1.; 
    fGeoMatrix->MasterToLocal(worldArray,localArray);
    return TVector3(localArray);
  }

  //......................................................................

  const TVector3 PlaneGeo::LocalToWorld( const TVector3& local ) const
  {
    double worldArray[4];
    double localArray[4];
    localArray[0] = local.X();
    localArray[1] = local.Y();
    localArray[2] = local.Z();
    localArray[3] = 1.;
    fGeoMatrix->LocalToMaster(localArray,worldArray);
    return TVector3(worldArray);
  }

  //......................................................................

  // Convert a vector from world frame to the local plane frame
  // \param world : 3-D array. Vector in world coordinates; input.
  // \param plane : 3-D array. Vector in plane coordinates; plane.
  void PlaneGeo::WorldToLocalVect(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocalVect(world,plane);
  }

  //......................................................................
  void PlaneGeo::UpdateAfterSorting
    (geo::PlaneID planeid, geo::BoxBoundedGeo const& TPCbox)
  {
    // the order here matters
    
    // reset our ID
    fID = planeid;
    
    UpdatePlaneNormal(TPCbox);
    UpdateIncreasingWireDir();
    
    // update wires
    geo::WireID::WireID_t wireNo = 0;
    for (geo::WireGeo* wire: fWire) {
      
      wire->UpdateAfterSorting(geo::WireID(fID, wireNo), shouldFlipWire(*wire));
      
      ++wireNo;
    } // for wires
    
    UpdateOrientation();
    UpdateWirePitch();
    UpdatePhiZ();
    
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
    // the normal may be not set yet because it requires external information
    // from the TPC; we can't use GetNormalDirection().
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
  
  //......................................................................
  void PlaneGeo::UpdatePlaneNormal(geo::BoxBoundedGeo const& TPCbox) {
    
    //
    // direction normal to the wire plane, points toward the center of TPC
    //
    
    // start from the axis
    fNormal = GetNormalAxis();
    
    // now evaluate where we are pointing
    TVector3 TPCcenter(TPCbox.Center().data());
    TVector3 towardCenter = TPCcenter - GetCenter();
    
    // if they are pointing in opposite directions, flip the normal
    if (fNormal.Dot(towardCenter) < 0) fNormal = -fNormal;
    roundVector(fNormal, 1e-3);
    
  } // PlaneGeo::UpdatePlaneNormal()
  
  
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
    fWireCoordDir = GetNormalDirection().Cross(WireDir).Unit();
    
    // 3) where is the next wire?
    TVector3 toNextWire
      = Wire(refWireNo + 1).GetCenter() - refWire.GetCenter();
    
    // 4) if fWireCoordDir is pointing away from the next wire, flip it
    if (fWireCoordDir.Dot(toNextWire) < 0) {
      fWireCoordDir = -fWireCoordDir;
    }
    roundVector(fWireCoordDir, 1e-3);
    
  } // PlaneGeo::UpdateIncreasingWireDir()
  
  
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
  bool PlaneGeo::shouldFlipWire(geo::WireGeo const& wire) const {
    //
    // The correct orientation is so that:
    //
    // (direction) x (wire coordinate direction) . (plane normal)
    // 
    // is positive; it it's negative, then we should flip the wire.
    
    return wire.Direction()
      .Cross(GetIncreasingWireDirection())
      .Dot(GetNormalDirection())
      < +0.5; // should be in fact exactly +1
    
  } // PlaneGeo::shouldFlipWire()
  
  //......................................................................
  
}
////////////////////////////////////////////////////////////////////////
