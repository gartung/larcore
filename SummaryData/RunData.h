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

namespace sumdata {

  class RunData{

  public:

    RunData(); // Default constructor
    explicit RunData(geo::DetId_t detid);

  private:

    geo::DetId_t fDetId;  ///< detector id

#ifndef __GCCXML__

  public:
    geo::DetId_t DetId() const;

#endif

  };
}

#ifndef __GCCXML__

inline geo::DetId_t sumdata::RunData::DetId() const { return fDetId; }

#endif

#endif // SD_RUNDATA_H
