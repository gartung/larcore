////////////////////////////////////////////////////////////////////////
/// \file  GeoObjectSorterStandard.h
/// \brief Interface to algorithm class for standard sorting of geo::XXXGeo objects
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_GEOOBJECTSORTERSTANDARD_H
#define GEO_GEOOBJECTSORTERSTANDARD_H

#include <vector>

#include "Geometry/GeoObjectSorter.h"

namespace geo{

  class GeoObjectSorterStandard : public GeoObjectSorter {

  public:

    GeoObjectSorterStandard(fhicl::ParameterSet const& p);
    ~GeoObjectSorterStandard();

    void SortCryostats(std::vector<geo::CryostatGeo*> & cgeo)     const;
    void SortTPCs     (std::vector<geo::TPCGeo*>      & tgeo)     const;
    void SortPlanes   (std::vector<geo::PlaneGeo*>    & pgeo,
		       geo::DriftDirection_t     const& driftDir) const;
    void SortWires    (std::vector<geo::WireGeo*>     & wgeo)     const;
    
  private:
    
  };

}

#endif // GEO_GEOOBJECTSORTERSTANDARD_H
