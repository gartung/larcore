////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterStandard.cxx
/// \brief Interface to algorithm class for sorting standard geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "Geometry/GeoObjectSorterStandard.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"

namespace geo{

  //----------------------------------------------------------------------------
  // Define sort order for cryostats in standard configuration
  static bool sortCryoStandard(const CryostatGeo* c1, const CryostatGeo* c2)
  {
    double xyz1[3] = {0.}, xyz2[3] = {0.};
    double local[3] = {0.}; 
    c1->LocalToWorld(local, xyz1);
    c2->LocalToWorld(local, xyz2);

    return xyz1[0] < xyz2[0];   
  }


  //----------------------------------------------------------------------------
  // Define sort order for tpcs in standard configuration.
  static bool sortTPCStandard(const TPCGeo* t1, const TPCGeo* t2) 
  {
    double xyz1[3] = {0.};
    double xyz2[3] = {0.};
    double local[3] = {0.};
    t1->LocalToWorld(local, xyz1);
    t2->LocalToWorld(local, xyz2);

    // sort TPCs according to x
    if(xyz1[0] < xyz2[0]) return true;

    return false;
  }


  //----------------------------------------------------------------------------
  // Define sort order for planes in standard configuration
  static bool sortPlaneStandard(const PlaneGeo* p1, const PlaneGeo* p2) 
  {
    double xyz1[3] = {0.};
    double xyz2[3] = {0.};
    double local[3] = {0.};
    p1->LocalToWorld(local, xyz1);
    p2->LocalToWorld(local, xyz2);

    // drift direction is negative, plane number increases in drift direction
    return xyz1[0] > xyz2[0];
  }


  //----------------------------------------------------------------------------
  bool sortWireStandard(WireGeo* w1, WireGeo* w2){
    double xyz1[3] = {0.};
    double xyz2[3] = {0.};

    w1->GetCenter(xyz1); w2->GetCenter(xyz2);

    if( xyz1[2] < xyz2[2] ) return true; 

    return false;
  }

  //----------------------------------------------------------------------------
  GeoObjectSorterStandard::GeoObjectSorterStandard(fhicl::ParameterSet const&)
  {
  }

  //----------------------------------------------------------------------------
  GeoObjectSorterStandard::~GeoObjectSorterStandard()
  {
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterStandard::SortCryostats(std::vector<geo::CryostatGeo*> & cgeo) const
  {
    std::sort(cgeo.begin(), cgeo.end(), sortCryoStandard);
    
    return;
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterStandard::SortTPCs(std::vector<geo::TPCGeo*>  & tgeo) const
  {
    
    std::sort(tgeo.begin(), tgeo.end(), sortTPCStandard);

    return;
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterStandard::SortPlanes(std::vector<geo::PlaneGeo*> & pgeo,
					   geo::DriftDirection_t  const& driftDir) const
  {
    // sort the planes to increase in drift direction
    // The drift direction has to be set before this method is called.  It is set when
    // the CryostatGeo objects are sorted by the CryostatGeo::SortSubVolumes method
    if     (driftDir == geo::kPosX) std::sort(pgeo.rbegin(), pgeo.rend(), sortPlaneStandard);
    else if(driftDir == geo::kNegX) std::sort(pgeo.begin(),  pgeo.end(),  sortPlaneStandard);
    else if(driftDir == geo::kUnknownDrift)
      throw cet::exception("TPCGeo") << "Drift direction is unknown, can't sort the planes\n";

    return;
  }

  //----------------------------------------------------------------------------
  void GeoObjectSorterStandard::SortWires(std::vector<geo::WireGeo*> & wgeo) const
  {
    std::sort(wgeo.begin(), wgeo.end(), sortWireStandard);

    return;
  }

}
