////////////////////////////////////////////////////////////////////////
/// \file RunData.h
///
/// Definition of object to store run related information
///
/// \version $Id: RunData.h,v 1.1.1.1 2011/03/03 00:19:49 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef SD_RUNDATA_H
#define SD_RUNDATA_H

#include "SimpleTypesAndConstants/geo_types.h"
#include <string>

namespace sumdata {

  class RunData{

  public:

    RunData(); // Default constructor

  private:

    geo::DetId_t fDetId;   ///< detector id
    std::string  fDetName; ///< detector name
#ifndef __GCCXML__

  public:
    explicit           RunData(geo::DetId_t detid);
    explicit           RunData(std::string detectorName);
    geo::DetId_t       DetId() const;
    std::string const& DetName() const;

#endif

  };
}

#ifndef __GCCXML__

inline geo::DetId_t       sumdata::RunData::DetId()   const { return fDetId; }
inline std::string const& sumdata::RunData::DetName() const { return fDetName; }

#endif

#endif // SD_RUNDATA_H
