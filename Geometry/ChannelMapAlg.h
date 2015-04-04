////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapAlg.h
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_CHANNELMAPALG_H
#define GEO_CHANNELMAPALG_H

#include <vector>
#include <set>

#include "cetlib/exception.h"
#include "SimpleTypesAndConstants/geo_types.h"
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t
#include "Geometry/AuxDetGeo.h"
#include "Geometry/CryostatGeo.h"

#include "TVector3.h"

namespace geo{

  /// Exception thrown on invalid wire number (e.g. NearestWireID())
  class InvalidWireIDError: public cet::exception {
      public:
    InvalidWireIDError(std::string cat): cet::exception(cat) {}
    
    InvalidWireIDError(std::string cat, int bad_wire, int better_wire = -1):
      cet::exception(cat),
      wire_number(bad_wire), better_wire_number(better_wire)
      {}
    
    int wire_number = -1; ///< the invalid wire number
    int better_wire_number = -1; ///< a suggestion for a good wire number
  }; // class InvalidWireIDError
  
  
 class ChannelMapAlg{

 public:

   ChannelMapAlg();
   virtual ~ChannelMapAlg();

   virtual void                     Initialize( std::vector<geo::CryostatGeo*> & cgeo,
						std::vector<geo::AuxDetGeo*>   & adgeo ) = 0;
   virtual void                	    Uninitialize() = 0;				   
   virtual std::vector<WireID> 	    ChannelToWire(raw::ChannelID_t channel)   const = 0;
   virtual unsigned int        	    Nchannels()                               const = 0;
   //virtual unsigned int             NOpChans()                                const = 0;

   /**
    * @brief Returns the index of the wire nearset to the specified position
    * @param YPos y coordinate on the wire plane
    * @param ZPos z coordinate on the wire plane
    * @param TPCNo number of TPC
    * @param cstat number of cryostat
    * @return an index interpolation between the two nearest wires
    * @see NearestWireID()
    *
    * Respect to NearestWireID(), this method returns a real number,
    * representing a continuous coordinate in the wire axis, with the round
    * values corresponding to the actual wires.
    */
   virtual double WireCoordinate(double YPos,
				 double ZPos,
                                 unsigned int PlaneNo,
                                 unsigned int TPCNo,
                                 unsigned int cstat) const = 0;

   virtual WireID              	    NearestWireID(const TVector3& worldPos,		   
						  unsigned int    PlaneNo,
						  unsigned int    TPCNo,
						  unsigned int    cstat)   const = 0;
   virtual raw::ChannelID_t    	    PlaneWireToChannel(unsigned int plane,		   
                               	                       unsigned int wire,		   
                               	                       unsigned int tpc,		   
                               	                       unsigned int cstat)    const = 0;
   virtual View_t	       	    View( raw::ChannelID_t const channel ) 	      const = 0;
   virtual SigType_t     	    SignalType( raw::ChannelID_t const channel )      const = 0;
   virtual std::set<View_t>  const& Views()                                   const = 0;
   virtual std::set<PlaneID> const& PlaneIDs()                                const = 0;
   unsigned int                     NearestWire(const TVector3& worldPos,
					   unsigned int    PlaneNo,
					   unsigned int    TPCNo,
					   unsigned int    cstat)        const;

   virtual unsigned int OpChanUniqueID(int detNum, int channel) const = 0;
   virtual unsigned int OpDetFromUniqueChanID(int uniqueChannel) const = 0;
   virtual unsigned int OpDetChannelFromUniqueChanID(int uniqueChannel) const = 0;

   // These methods retrieve the private fFirstChannel*
   // vectors for testing.
   const std::vector<std::vector<std::vector<raw::ChannelID_t>>> FirstChannelInNextPlane() const
   { return fFirstChannelInThisPlane; }

   const std::vector<std::vector<std::vector<raw::ChannelID_t>>> FirstChannelInThisPlane() const
   { return fFirstChannelInNextPlane; }

 protected:

   // These 3D vectors are used in initializing the Channel map.
   // Only a 1D vector is really needed so far, but these are more general.
   std::vector< std::vector<std::vector<raw::ChannelID_t> > > fFirstChannelInThisPlane;
   std::vector< std::vector<std::vector<raw::ChannelID_t> > > fFirstChannelInNextPlane;

 };
}
#endif // GEO_CHANNELMAPALG_H

