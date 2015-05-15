/**
 * @file   GeometryCore.h
 * @brief  Access the description of detector geometry
 * @author brebel@fnal.gov
 * @see    GeometryCore.cxx
 *
 * Structure of the header:
 *     
 *     namespace geo {
 *       
 *       // forward class declarations
 *       
 *       namespace details {
 *         
 *         // geometry iterator base class
 *       
 *       }
 *       
 *       // geometry iterators declaration
 *       //  - cryostat_iterator
 *       //  - TPC_iterator
 *       //  - plane_iterator
 *       //  - wire_iterator
 *       
 *       // GeometryData_t definition (part of GeometryCore)
 *       
 *       // GeometryCore declaration
 *     
 *     }
 *     
 *
 *
 * Revised <seligman@nevis.columbia.edu> 29-Jan-2009
 *         Revise the class to make it into more of a general detector interface
 * Revised <petrillo@fnal.gov> 27-Apr-2015
 *         Factorization into a framework-independent GeometryCore.h and a
 *         art framework interface
 * Revised <petrillo@fnal.gov> 30-Apr-2015
 *         Redesign of the iterators
 */
#ifndef GEO_GEOMETRYCORE_H
#define GEO_GEOMETRYCORE_H


// LArSoft libraries
#include "SimpleTypesAndConstants/geo_types.h"
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t
#include "Geometry/ChannelMapAlg.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"

// Framework and infrastructure libraries
#include "fhiclcpp/ParameterSet.h"

// ROOT libraries
#include <TVector3.h>
// #include <Rtypes.h>

// C/C++ standard libraries
#include <cstddef> // size_t
#include <string>
#include <vector>
#include <set>
#include <memory> // std::shared_ptr<>
#include <iterator> // std::forward_iterator_tag
#include <type_traits> // std::is_base_of<>


// ROOT class prototypes
class TGeoManager;
class TGeoNode;
class TGeoMaterial;


/// Namespace collecting geometry-related classes utilities
namespace geo {
  
  
  // Forward declarations within namespace.
  class AuxDetGeo;
  class AuxDetSensitiveGeo;
  class OpDetGeo;
  class GeometryCore;
  
  
  //
  // iterators
  //
  
  namespace details {
    
    /// Base class for geometry iterators, containing some type definitions
    class geometry_iterator_types {
        public:
      
      //@{
      /// Structures to distinguish the constructors
      struct BeginPos_t {};
      struct EndPos_t {};
      struct UndefinedPos_t {};
      
      static constexpr BeginPos_t begin_pos = {};
      static constexpr EndPos_t end_pos = {};
      static constexpr UndefinedPos_t undefined_pos = {};
      //@}
      
    }; // class geometry_iterator_types
    
    /// Base class for geometry iterators (note: this is not an iterator)
    class geometry_iterator_base: public geometry_iterator_types {
        public:
      
      //@{
      /// Structures to distinguish the constructors
      struct BeginPos_t {};
      struct EndPos_t {};
      struct UndefinedPos_t {};
      
      static constexpr BeginPos_t begin_pos = {};
      static constexpr EndPos_t end_pos = {};
      static constexpr UndefinedPos_t undefined_pos = {};
      //@}
      
      /// Constructor: associates with the specified geometry
      geometry_iterator_base(geo::GeometryCore const* geom): pGeo(geom) {}
      
        protected:
      /// Returns a pointer to the geometry
      geo::GeometryCore const* geometry() const { return pGeo; }
      
      /// Default constructor; do not use a default-constructed iterator as-is!
      geometry_iterator_base() {}
      
        private:
      GeometryCore const* pGeo = nullptr; ///< pointer to the geometry
      
    }; // class geometry_iterator_base
    
    
    
    /**
     * @brief Base forward iterator browsing all cryostats in the detector
     * @tparam GEOID ID type to be used
     * 
     * This iterator assumes that GEOID is derived from geo::CryostatID.
     * Note that no polymorphic behaviour is required, or expected, from GEOID.
     * 
     * This iterator is designed to carry on, untouched, anything else that the
     * GEOID type defines beyond the required CryostatID data.
     *
     * Currently, backward iterations are not supported.
     */
    template <typename GEOID>
    class cryostat_iterator_base:
      virtual public std::forward_iterator_tag, public geometry_iterator_base
    {
      using LocalID_t = geo::CryostatID; ///< type of the ID we change
      static_assert(std::is_base_of<LocalID_t, GEOID>::value,
        "template type GEOID is not a LocalID_t");
      
      using ElementPtr_t = geo::CryostatGeo const*;

        public:
      using iterator = cryostat_iterator_base<GEOID>; ///< type of this iterator
      
      /// Default constructor; effect not defined: assign to it before using!
      cryostat_iterator_base() {}
      
      /// Constructor: points to begin
      cryostat_iterator_base(geo::GeometryCore const* geom):
        cryostat_iterator_base(geom, begin_pos) {}
      
      /// Constructor: points to the specified cryostat
      cryostat_iterator_base
        (geo::GeometryCore const* geom, GEOID const& start_from):
        cryostat_iterator_base(geom, undefined_pos)
        { id = start_from; }
      
      /// Constructor: points to begin
      cryostat_iterator_base(geo::GeometryCore const* geom, BeginPos_t):
        cryostat_iterator_base(geom, undefined_pos)
        { set_begin(); }
      
      /// Constructor: points to end
      cryostat_iterator_base(geo::GeometryCore const* geom, EndPos_t):
        cryostat_iterator_base(geom, undefined_pos)
        { set_end(); }
      
      // TODO reconsider if the additional template is indeed needed
      /// Returns true if the two iterators point to the same cryostat
      template <typename OTHERID>
      bool operator== (cryostat_iterator_base<OTHERID> const& as) const
        { return localID() == as.localID(); }
      
      /// Returns true if the two iterators point to different cryostats
      template <typename OTHERID>
      bool operator!= (cryostat_iterator_base<OTHERID> const& as) const 
        { return localID() != as.localID(); }
      
      /// Returns the ID the iterator points to
      LocalID_t const& operator* () const { return localID(); }
      
      /// Returns a pointer to the ID the iterator points to
      LocalID_t const* operator-> () const { return &(localID()); }
      
      /// Prefix increment: returns this iterator pointing to the next cryostat
      iterator& operator++ () { next(); return *this; }
      
      /// Postfix increment: returns the current iterator, then increments it
      iterator operator++ (int) { iterator old(*this); next(); return old; }
      
      /// Returns whether the iterator is pointing to a valid cryostat
      operator bool() const;
      
      /// Returns a pointer to cryostat, or nullptr if invalid
      ElementPtr_t get() const;
      
        protected:
      using GeoID_t = GEOID; ///< type of the actual ID stored in the iterator
      using ID_t = typename LocalID_t::CryostatID_t;
      
      /// Constructor: does not set the current ID
      cryostat_iterator_base(geo::GeometryCore const* geom, UndefinedPos_t):
        geometry_iterator_base(geom), id()
        { set_local_limits(); }
      
      //@{
      /// Returns the actual type of ID we store
      GeoID_t const& ID() const { return id; }
      GeoID_t& ID() { return id; }
      //@}
      
      /// Skips to the next cryostat
      void next();
      
      /// Returns whether this iterator has reached the end
      bool at_end() const { return local_index() == limit; }
      
        private:
      GeoID_t id; ///< ID of the current cryostat
      ID_t limit = LocalID_t::InvalidID; ///< maximum number of cryostats
      
      /// Sets the limit member to the past-the-end cryostat number
      void set_local_limits();
      
      /// Sets the iterator to the begin position
      void set_begin();
      
      /// Sets the iterator to the end position
      void set_end();
      
      //@{
      /// Returns the type of ID we act on
      LocalID_t const& localID() const 
        { return static_cast<LocalID_t const&>(ID()); }
      LocalID_t& localID() { return static_cast<LocalID_t&>(ID()); }
      //@}
      
      //@{
      /// Returns the index (part if the ID) this iterator runs on
      ID_t const& local_index() const { return localID().Cryostat; }
      ID_t& local_index() { return localID().Cryostat; }
      //@}
      
    }; // class cryostat_iterator_base<>
    
    
    /**
     * @brief Base forward iterator browsing all TPCs in the detector
     * @tparam GEOID ID type to be used
     * 
     * This iterator requires that GEOID is derived from geo::TPCID.
     * Note that no polymorphic behaviour is required, or expected, from GEOID.
     * 
     * This iterator is designed to carry on, untouched, anything else that the
     * GEOID type defines beyond the required TPCID data.
     * 
     * @noe A number of "local" methods are overloaded: since there is no
     * polymorphism here and they are not virtual functions, these are designed
     * not to replace the inherited methods except within the non-inherited and
     * explicitly redefined methods.
     * 
     * Currently, backward iterations are not supported.
     */
    template <typename GEOID>
    class TPC_iterator_base:
      virtual public std::forward_iterator_tag,
      protected cryostat_iterator_base<GEOID>
    {
      using LocalID_t = geo::TPCID; ///< type of the ID we change
      static_assert(std::is_base_of<LocalID_t, GEOID>::value,
        "template type GEOID is not a LocalID_t");
      
      using upper_iterator = cryostat_iterator_base<GEOID>;
      using ElementPtr_t = geo::TPCGeo const*;
      
        public:
      using iterator = TPC_iterator_base<GEOID>; ///< type of this iterator
      
      // import all the useful types from the base templated class
      using typename upper_iterator::UndefinedPos_t;
      using typename upper_iterator::BeginPos_t;
      using typename upper_iterator::EndPos_t;
      
      // import all the useful members from the base templated class
      using upper_iterator::undefined_pos;
      using upper_iterator::begin_pos;
      using upper_iterator::end_pos;
      
      /// Default constructor; effect not defined: assign to it before using!
      TPC_iterator_base() {}
      
      /// Constructor: points to begin
      TPC_iterator_base(geo::GeometryCore const* geom):
        TPC_iterator_base(geom, begin_pos) {}
      
      /// Constructor: points to the specified cryostat
      TPC_iterator_base
        (geo::GeometryCore const* geom, GEOID const& start_from):
        upper_iterator(geom, start_from)
        { set_local_limits(); }
      
      /// Constructor: points to begin
      TPC_iterator_base(geo::GeometryCore const* geom, BeginPos_t):
        upper_iterator(geom, begin_pos)
        { set_local_limits(); }
      
      /// Constructor: points to end
      TPC_iterator_base(geo::GeometryCore const* geom, EndPos_t):
        upper_iterator(geom, end_pos)
        {} // the local limit is ill-defined and left invalid
      
      // TODO reconsider if the additional template is indeed needed
      /// Returns true if the two iterators point to the same TPC
      template <typename OTHERID>
      bool operator== (TPC_iterator_base<OTHERID> const& as) const
        { return localID() == as.localID(); }
      
      /// Returns true if the two iterators point to different TPCs
      template <typename OTHERID>
      bool operator!= (TPC_iterator_base<OTHERID> const& as) const 
        { return localID() != as.localID(); }
      
      /// Returns the TPCID the iterator points to
      LocalID_t const& operator* () const { return localID(); }
      
      /// Returns the TPCID the iterator points to
      LocalID_t const* operator-> () const { return &(localID()); }
      
      /// Prefix increment: returns this iterator pointing to the next TPC
      iterator& operator++ () { next(); return *this; }
      
      /// Postfix increment: returns the current iterator, then increments it
      iterator operator++ (int) { iterator old(*this); next(); return old; }
      
      /// Returns whether the iterator is pointing to a valid TPC
      operator bool() const;
      
      /// Returns a pointer to TPC, or nullptr if invalid
      ElementPtr_t get() const;
      
        protected:
      
      using ID_t = typename LocalID_t::TPCID_t; ///< specific type for TPC ID
      
      /// Constructor: position undefined (meaning undefined local limits too)
      TPC_iterator_base(geo::GeometryCore const* geom, UndefinedPos_t):
        upper_iterator(geom, undefined_pos)
        {}
      
      using upper_iterator::ID; // to be explicit; this is NOT overloaded
      
      /// Returns the type of ID we act on
      LocalID_t const& localID() const 
        { return static_cast<LocalID_t const&>(upper_iterator::ID()); }
      
      using upper_iterator::at_end; // to be explicit; this is NOT overloaded
      
      /// Skips to the next TPC
      void next();
      
      /// Returns the index (part if the ID) this iterator runs on
      ID_t const& local_index() const { return localID().TPC; }
        
        private:
      
      /// maximum number of TPCs in the current cryostat
      ID_t limit = LocalID_t::InvalidID;
      
      /// Sets limit to the past-the-end TPC number of current croystat
      void set_local_limits();
      
      /// Returns the type of ID we act on (non-const version)
      LocalID_t& localID() { return static_cast<LocalID_t&>(ID()); }
      
      /// Returns the index (part if the ID) this iterator runs on  (non-const)
      ID_t& local_index() { return localID().TPC; }
      
    }; // class TPC_iterator_base
    
    
    /**
     * @brief Base forward iterator browsing all planes in the detector
     * @tparam GEOID ID type to be used
     * 
     * This iterator requires that GEOID is derived from geo::PlaneID.
     * Note that no polymorphic behaviour is required, or expected, from GEOID.
     * 
     * This iterator is designed to carry on, untouched, anything else that the
     * GEOID type defines beyond the required PlaneID data.
     * 
     * @noe A number of "local" methods are overloaded: since there is no
     * polymorphism here and they are not virtual functions, these are designed
     * not to replace the inherited methods except within the non-inherited and
     * explicitly redefined methods.
     * 
     * Currently, backward iterations are not supported.
     */
    template <typename GEOID>
    class plane_iterator_base:
      virtual public std::forward_iterator_tag,
      protected TPC_iterator_base<GEOID>
    {
      using LocalID_t = geo::PlaneID; ///< type of the ID we change
      static_assert(std::is_base_of<LocalID_t, GEOID>::value,
        "template type GEOID is not a LocalID_t");
      
      using upper_iterator = TPC_iterator_base<GEOID>;
      using ElementPtr_t = geo::PlaneGeo const*;
      
        public:
      using iterator = plane_iterator_base<GEOID>; ///< type of this iterator
      
      // import all the useful types from the base templated class
      using typename upper_iterator::UndefinedPos_t;
      using typename upper_iterator::BeginPos_t;
      using typename upper_iterator::EndPos_t;
      
      // import all the useful members from the base templated class
      using upper_iterator::undefined_pos;
      using upper_iterator::begin_pos;
      using upper_iterator::end_pos;
      
      /// Default constructor; effect not defined: assign to it before using!
      plane_iterator_base() {}
      
      /// Constructor: points to begin
      plane_iterator_base(geo::GeometryCore const* geom):
        plane_iterator_base(geom, begin_pos) {}
      
      /// Constructor: points to the specified cryostat
      plane_iterator_base
        (geo::GeometryCore const* geom, GEOID const& start_from):
        upper_iterator(geom, start_from)
        { set_local_limits(); }
      
      /// Constructor: points to begin
      plane_iterator_base(geo::GeometryCore const* geom, BeginPos_t):
        upper_iterator(geom, begin_pos)
        { set_local_limits(); }
      
      /// Constructor: points to end
      plane_iterator_base(geo::GeometryCore const* geom, EndPos_t):
        upper_iterator(geom, end_pos)
        {} // the local limit is ill-defined and left invalid
      
      // TODO reconsider if the additional template is indeed needed
      /// Returns true if the two iterators point to the same plane
      template <typename OTHERID>
      bool operator== (plane_iterator_base<OTHERID> const& as) const
        { return localID() == as.localID(); }
      
      /// Returns true if the two iterators point to different planes
      template <typename OTHERID>
      bool operator!= (plane_iterator_base<OTHERID> const& as) const 
        { return localID() != as.localID(); }
      
      /// Returns the PlaneID the iterator points to
      LocalID_t const& operator* () const { return localID(); }
      
      /// Returns the PlaneID the iterator points to
      LocalID_t const* operator-> () const { return &(localID()); }
      
      /// Prefix increment: returns this iterator pointing to the next plane
      iterator& operator++ () { next(); return *this; }
      
      /// Postfix increment: returns the current iterator, then increments it
      iterator operator++ (int) { iterator old(*this); next(); return old; }
      
      /// Returns whether the iterator is pointing to a valid plane
      operator bool() const;
      
      /// Returns a pointer to plane, or nullptr if invalid
      ElementPtr_t get() const;
      
        protected:
      
      using ID_t = typename LocalID_t::PlaneID_t; ///< specific type for plane ID
      
      /// Constructor: position undefined (meaning undefined local limits too)
      plane_iterator_base(geo::GeometryCore const* geom, UndefinedPos_t):
        upper_iterator(geom, undefined_pos)
        {}
      
      using upper_iterator::ID; // to be explicit; this is NOT overloaded
      
      /// Returns the type of ID we act on
      LocalID_t const& localID() const 
        { return static_cast<LocalID_t const&>(upper_iterator::ID()); }
      
      using upper_iterator::at_end; // to be explicit; this is NOT overloaded
      
      /// Skips to the next plane
      void next();
      
      /// Returns the index (part if the ID) this iterator runs on
      ID_t const& local_index() const { return localID().Plane; }
        
        private:
      
      /// maximum number of planes in the current TPC
      ID_t limit = LocalID_t::InvalidID;
      
      /// Sets limit to the past-the-end plane number of current TPC
      void set_local_limits();
      
      /// Returns the type of ID we act on (non-const version)
      LocalID_t& localID() { return static_cast<LocalID_t&>(ID()); }
      
      /// Returns the index (part if the ID) this iterator runs on  (non-const)
      ID_t& local_index() { return localID().Plane; }
      
    }; // class plane_iterator_base
    
    
    /**
     * @brief Base forward iterator browsing all wires in the detector
     * @tparam GEOID ID type to be used
     * 
     * This iterator requires that GEOID is derived from geo::WireID.
     * Note that no polymorphic behaviour is required, or expected, from GEOID.
     * 
     * This iterator is designed to carry on, untouched, anything else that the
     * GEOID type defines beyond the required WireID data.
     * 
     * @noe A number of "local" methods are overloaded: since there is no
     * polymorphism here and they are not virtual functions, these are designed
     * not to replace the inherited methods except within the non-inherited and
     * explicitly redefined methods.
     * 
     * Currently, backward iterations are not supported.
     */
    template <typename GEOID>
    class wire_iterator_base:
      virtual public std::forward_iterator_tag,
      protected plane_iterator_base<GEOID>
    {
      using LocalID_t = geo::WireID; ///< type of the ID we change
      static_assert(std::is_base_of<LocalID_t, GEOID>::value,
        "template type GEOID is not a LocalID_t");
      
      using upper_iterator = plane_iterator_base<GEOID>;
      using ElementPtr_t = geo::WireGeo const*;
      
        public:
      using iterator = wire_iterator_base<GEOID>; ///< type of this iterator
      
      // import all the useful types from the base templated class
      using typename upper_iterator::UndefinedPos_t;
      using typename upper_iterator::BeginPos_t;
      using typename upper_iterator::EndPos_t;
      
      // import all the useful members from the base templated class
      using upper_iterator::undefined_pos;
      using upper_iterator::begin_pos;
      using upper_iterator::end_pos;
      
      /// Default constructor; effect not defined: assign to it before using!
      wire_iterator_base() {}
      
      /// Constructor: points to begin
      wire_iterator_base(geo::GeometryCore const* geom):
        wire_iterator_base(geom, begin_pos) {}
      
      /// Constructor: points to the specified cryostat
      wire_iterator_base
        (geo::GeometryCore const* geom, GEOID const& start_from):
        upper_iterator(geom, start_from)
        { set_local_limits(); }
      
      /// Constructor: points to begin
      wire_iterator_base(geo::GeometryCore const* geom, BeginPos_t):
        upper_iterator(geom, begin_pos)
        { set_local_limits(); }
      
      /// Constructor: points to end
      wire_iterator_base(geo::GeometryCore const* geom, EndPos_t):
        upper_iterator(geom, end_pos)
        {} // the local limit is ill-defined and left invalid
      
      // TODO reconsider if the additional template is indeed needed
      /// Returns true if the two iterators point to the same wire
      template <typename OTHERID>
      bool operator== (wire_iterator_base<OTHERID> const& as) const
        { return localID() == as.localID(); }
      
      /// Returns true if the two iterators point to different wires
      template <typename OTHERID>
      bool operator!= (wire_iterator_base<OTHERID> const& as) const 
        { return localID() != as.localID(); }
      
      /// Returns the WireID the iterator points to
      LocalID_t const& operator* () const { return localID(); }
      
      /// Returns the WireID the iterator points to
      LocalID_t const* operator-> () const { return &(localID()); }
      
      /// Prefix increment: returns this iterator pointing to the next wire
      iterator& operator++ () { next(); return *this; }
      
      /// Postfix increment: returns the current iterator, then increments it
      iterator operator++ (int) { iterator old(*this); next(); return old; }
      
      /// Returns whether the iterator is pointing to a valid wire
      operator bool() const;
      
      /// Returns a pointer to wire, or nullptr if invalid
      ElementPtr_t get() const;
      
        protected:
      
      using ID_t = typename LocalID_t::WireID_t; ///< specific type for wire ID
      
      /// Constructor: position undefined (meaning undefined local limits too)
      wire_iterator_base(geo::GeometryCore const* geom, UndefinedPos_t):
        upper_iterator(geom, undefined_pos)
        {}
      
      using upper_iterator::ID; // to be explicit; this is NOT overloaded
      
      /// Returns the type of ID we act on
      LocalID_t const& localID() const 
        { return static_cast<LocalID_t const&>(upper_iterator::ID()); }
      
      using upper_iterator::at_end; // to be explicit; this is NOT overloaded
      
      /// Skips to the next wire
      void next();
      
      /// Returns the index (part if the ID) this iterator runs on
      ID_t const& local_index() const { return localID().Wire; }
        
        private:
      
      /// maximum number of wires in the current plane
      ID_t limit = LocalID_t::InvalidID;
      
      /// Sets limit to the past-the-end wire number of current plane
      void set_local_limits();
      
      /// Returns the type of ID we act on (non-const version)
      LocalID_t& localID() { return static_cast<LocalID_t&>(ID()); }
      
      /// Returns the index (part if the ID) this iterator runs on (non-const)
      ID_t& local_index() { return localID().Wire; }
      
    }; // class wire_iterator_base


  } // namespace details
  
  
  
  /**
   * @brief Forward iterator browsing all cryostats in the detector
   * 
   * Prefer asking the geometry object for iterators rather than constructing
   * them anew: see geo::GeometryCore::cryostat_iterator for the recommended
   * usage.
   * Stand-alone example (not recommended):
   * @code
   * geo::GeometryCore::cryostat_iterator iCryostat,
   *   cbegin(geom, geo::cryostat_iterator::begin_pos),
   *   cend(geom, geo::cryostat_iterator::end_pos);
   * for (iCryostat = cbegin; iCryostat != cend; ++iCryostat) {
   *   geo::CryostatID const& cid = *iCryostat;
   *   geo::CryostatGeo const* pCryo = iCryostat.get();
   *   std::cout << "We are at: " << cid << std::endl;
   *   // ...
   * } // for
   * @endcode
   */
  using cryostat_iterator = details::cryostat_iterator_base<geo::CryostatID>;
  
  
  /**
   * @brief Forward iterator browsing all TPCs in the detector
   * 
   * Prefer asking the geometry object for iterators rather than constructing
   * them anew: see geo::GeometryCore::TPC_iterator for the recommended usage.
   * Stand-alone example (not recommended):
   * @code
   * geo::GeometryCore::TPC_iterator iTPC,
   *   tbegin(geom, geo::TPC_iterator::begin_pos),
   *   tend(geom, geo::TPC_iterator::end_pos);
   * for (iTPC = tbegin; iTPC != tend; ++iTPC) {
   *   geo::TPCID const& tid = *iTPC;
   *   geo::TPCGeo const* pTPC = iTPC.get();
   *   std::cout << "We are at: " << tid << std::endl;
   *   // ...
   * } // for
   * @endcode
   */
  using TPC_iterator = details::TPC_iterator_base<geo::TPCID>;
  
  
  /**
   * @brief Forward iterator browsing all planes in the detector
   * 
   * Prefer asking the geometry object for iterators rather than constructing
   * them anew: see geo::GeometryCore::plane_iterator for the recommended usage.
   * Stand-alone example (not recommended):
   * @code
   * geo::GeometryCore::plane_iterator iPlane,
   *   pbegin(geom, geo::plane_iterator::begin_pos),
   *   pend(geom, geo::plane_iterator::end_pos);
   * for (iPlane = pbegin; iPlane != pend; ++iPlane) {
   *   geo::PlaneID const& pid = *iPlane;
   *   geo::PlaneGeo const* pPlane = iPlane.get();
   *   std::cout << "We are at: " << pid << std::endl;
   *   // ...
   * } // for
   * @endcode
   */
  using plane_iterator = details::plane_iterator_base<geo::PlaneID>;
  
  
  /**
   * @brief Forward iterator browsing all wires in the detector
   * 
   * Prefer asking the geometry object for iterators rather than constructing
   * them anew: see geo::GeometryCore::wire_iterator for the recommended usage.
   * Stand-alone example (not recommended):
   * @code
   * geo::GeometryCore::wire_iterator iWire,
   *   wbegin(geom, geo::wire_iterator::begin_pos),
   *   wend(geom, geo::wire_iterator::end_pos);
   * for (iWire = wbegin; iWire != wend; ++iWire) {
   *   geo::WireID const& wid = *iWire;
   *   geo::WireGeo const* pWire = iWire.get();
   *   std::cout << "We are at: " << wid << std::endl;
   *   // ...
   * } // for
   * @endcode
   */
  using wire_iterator = details::wire_iterator_base<geo::WireID>;
  
  
  
  template <
    typename Iter,
    Iter (GeometryCore::*BeginFunc)() const,
    Iter (GeometryCore::*EndFunc)() const
    >
  class IteratorBox {
      public:
    using iterator = Iter;
    
    IteratorBox(GeometryCore const* geom):
      b((geom->*BeginFunc)()), e((geom->*EndFunc)()) {}
    
    iterator begin() const { return b; }
    iterator end() const { return e; }
    
    iterator cbegin() const { return b; }
    iterator cend() const { return e; }
    
      protected:
    iterator b, e;
  }; // IteratorBox<>
  
  
  //
  // GeometryCore
  //
  
  
  /// Data in the geometry description
  struct GeometryData_t {
    
    /// Type of list of cryostats
    using CryostatList_t = std::vector<CryostatGeo*>;
    /// Type of list of auxiliary detectors
    using AuxDetList_t = std::vector<AuxDetGeo*>;
    
    CryostatList_t cryostats; ///< The detector cryostats
    AuxDetList_t   auxDets;   ///< The auxiliary detectors
    
  }; // GeometryData_t
  
  
  /** **************************************************************************
   * @brief Description of geometry of one entire detector
   * 
   * @note All lengths are specified in centimetres
   * 
   * 
   * How to correctly instantiate a GeometryCore object
   * ---------------------------------------------------
   * 
   * Instantiation is a multi-step procedure:
   * 1. construct a GeometryCore object (the "service provider"),
   *    with the full configuration; at this step, configuration is just stored
   * 2. load a geometry with GeometryCore::LoadGeometryFile();
   *    this loads the detector geometry information
   * 3. prepare a channel map algorithm object (might use for example
   *    GeometryCore::DetectorName() or the detector geometry from the
   *    newly created object, but any use of channel mapping related functions
   *    is forbidden and it would yield undefined behaviour (expected to be
   *    catastrophic)
   * 4. acquire the channel mapping algorithm with
   *    GeometryCore::ApplyChannelMap(); at this point, the ChannelMapAlg object
   *    is asked to initialize itself and to perform whatever modifications to
   *    the geometry provider is needed.
   * 
   * Step 3 (creation of the channel mapping algorithm object) can be performed
   * at any time before step 4, provided that no GeometryCore instance is needed
   * for it.
   * 
   * 
   * Configuration parameters
   * -------------------------
   * 
   * - *Name* (string; mandatory): string identifying the detector; it can be
   *   different from the base name of the file used to initialize the geometry;
   *   standard names are recommended by each experiment.
   *   This name can be used, for example, to select which channel mapping
   *   algorithm to use.
   * - *SurfaceY* (real; mandatory): depth of the detector, in centimetrs;
   *   see SurfaceY() for details
   * - *MinWireZDist* (real; default: 3)
   * - *PositionEpsilon* (real; default: 0.01%) set the default tolerance
   *   (see DefaultWiggle())
   * 
   */
  class GeometryCore {
  public:
    
    /// Type of list of cryostats
    using CryostatList_t = GeometryData_t::CryostatList_t;
    /// Type of list of auxiliary detectors
    using AuxDetList_t = GeometryData_t::AuxDetList_t;
    
    
    // import iterators
    /**
     * @brief Forward-iterator browsing all cryostats in the detector
     * 
     * Usage example with a while loop:
     * @code
     * geo::GeometryCore::cryostat_iterator iCryostat = geom->begin_cryostat(),
     *   cend = geom->end_cryostat();
     * while (iCryostat != cend) {
     *   std::cout << "Cryo: " << iCryostat->Cryostat << std::endl;
     *   const geo::CryostatGeo* pCryo = iCryostat.get();
     *   ++iCryostat;
     *   // ...
     * } // while
     * @endcode
     * The recommended way to iterate is actually to use
     * GeometryCore::IterateCryostats() in a range-for loop.
     * It is recommended to save the end iterator rather than calling
     * GeometryCore::end_cryostat() on every check.
     */
    using cryostat_iterator = geo::cryostat_iterator;
    
    /**
     * @brief Forward-iterator browsing all TPCs in the detector
     * 
     * Usage example with a while loop:
     * @code
     * geo::GeometryCore::TPC_iterator iTPC = geom->begin_TPC(),
     *   tend = geom->end_TPC();
     * while (iTPC != tend) {
     *   std::cout << "TPC: " << *iTPC << std::endl;
     *   // the TPC descriptor object
     *   const geo::TPCGeo* pTPC = iTPC.get();
     *   // the cryostat the TPC is in
     *   geo::CryostatGeo const& Cryo = geom->Cryostat(*iTPC);
     *   ++iTPC;
     *   // ...
     * } // while
     * @endcode
     * The recommended way to iterate is actually to use
     * GeometryCore::IterateTPCs() in a range-for loop.
     * It is recommended to save the end iterator rather than calling
     * GeometryCore::end_TPC() on every check.
     */
    using TPC_iterator = geo::TPC_iterator;
    
    /**
     * @brief Forward-iterator browsing all planes in the detector
     * 
     * Usage example with a while loop:
     * @code
     * geo::GeometryCore::plane_iterator iPlane = geom->begin_plane(),
     *   pend = geom->end_plane();
     * while (iPlane != pend) {
     *   std::cout << "Plane: " << *iPlane << std::endl;
     *   // the plane descriptor object
     *   const geo::PlaneGeo* pPlane = iPlane.get();
     *   // the TPC the plane is in
     *   geo::TPCGeo const& TPC = geom->TPC(*iPlane);
     *   ++iPlane;
     *   // ...
     * } // while
     * @endcode
     * The recommended way to iterate is actually to use
     * GeometryCore::IteratePlanes() in a range-for loop.
     * It is recommended to save the end iterator rather than calling
     * GeometryCore::end_plane() on every check.
     */
    using plane_iterator = geo::plane_iterator;
    /**
     * @brief Forward-iterator browsing all wires in the detector
     * 
     * Usage example with a while loop:
     * @code
     * geo::GeometryCore::wire_iterator iWire = geom->begin_wire(),
     *   wend = geom->end_wire();
     * while (iWire != wend) {
     *   std::cout << "Wire: " << *iWire << std::endl;
     *   // the wire descriptor object
     *   const geo::WireGeo* pWire = iWire.get();
     *   // the TPC the wire is in
     *   geo::TPCGeo const& TPC = geom->TPC(*iWire);
     *   ++iWire;
     *   // ...
     * } // while
     * @endcode
     * The recommended way to iterate is actually to use
     * GeometryCore::IterateWires() in a range-for loop.
     * It is recommended to save the end iterator rather than calling
     * GeometryCore::end_wire() on every check.
     */
    using wire_iterator = geo::wire_iterator;
    
    
    
    /**
     * @brief Initialize geometry from a given configuration
     * @param pset configuration parameters
     * 
     * This constructor does not load any geometry description.
     * The next step is to do exactly that, by GeometryCore::LoadGeometryFile().
     */
    GeometryCore(fhicl::ParameterSet const& pset);
    
    /// Destructor
    ~GeometryCore();
    
    
    
    /**
     * @brief Returns the tolerance used in looking for positions
     * @return the tolerance value
     * 
     * This parameter is used as tolerance ("wiggle") for methods that require
     * it (e.g. CryostatGeo::FindTPCAtPosition()).
     * Typically, it's a additional fraction of tolerance: 0 means no tolerance,
     * 0.1 means 10% tolerance.
     * 
     * @todo Confirm the definition of wiggle: this one is taken from other doc
     */
    double DefaultWiggle() const { return fPositionWiggle; }
    
    /**
     * @brief Returns the full directory path to the geometry file source
     * @return the full directory path to the geometry file source
     * 
     * This is the full path of the source of the detector geometry GeometryCore
     * relies on.
     */
    std::string ROOTFile() const { return fROOTfile; }

    /**
     * @brief Returns the full directory path to the GDML file source
     * @return the full directory path to the GDML file source
     * 
     * This is the full path of the source of the detector geometry handed to
     * the detector simulation (GEANT).
     */
    std::string GDMLFile() const { return fGDMLfile; }
    
    
    
    /// @{
    /// @name Detector information
    
    //
    // global features
    //
    /// Returns a string with the name of the detector, as configured
    std::string DetectorName() const { return fDetectorName; }
    
    
    //
    // position
    //
    
    /**
     * @brief Fills the arguments with the boundaries of the world
     * @param xlo (output) pointer to the lower x coordinate
     * @param xlo (output) pointer to the upper x coordinate
     * @param ylo (output) pointer to the lower y coordinate
     * @param ylo (output) pointer to the upper y coordinate
     * @param zlo (output) pointer to the lower z coordinate
     * @param zlo (output) pointer to the upper z coordinate
     * @throw cet::exception (`"GeometryCore"` category) if no world found
     *
     * This method fills the boundaries of the world volume, that is the one
     * known as `"volWorld"` in the geometry.
     * 
     * If a pointer is null, its coordinate is skipped.
     *
     * @todo Replace it with a TPC boundaries style thing?
     * @todo Unify the coordinates type
     */
    void WorldBox(double* xlo, double* xhi,
                  double* ylo, double* yhi,
                  double* zlo, double* zhi) const;
    
    /**
     * @brief The position of the detector respect to earth surface
     * @return typical y position at surface in units of cm
     * 
     * This is the depth (y) of the surface (where earth meets air) for this
     * detector site.
     * The number is expressed in world coordinates and in centimetres,
     * and it represents the y coordinate of earth surface.
     * A negative value means that the origin of coordinates, typically matching
     * the detector centre, is above surface.
     * 
     * @todo check that this is actually how it is used
     */
    //
    double SurfaceY() const { return fSurfaceY; }
    
    
    //
    // object description and information
    //
    
    /// Access to the ROOT geometry description manager
    TGeoManager* ROOTGeoManager() const;
    
    /// Return the name of the world volume (needed by Geant4 simulation)
    const std::string GetWorldVolumeName() const;
    
    /**
     * @brief Returns the name of the deepest volume containing specified point
     * @param point the location to query, in world coordinates
     * @return name of the volume containing the point
     * 
     * @todo Use a reference to TVector3
     * @todo Use a double[3] instead?
     * @todo declare it const
     * @todo what happens if none?
     * @todo Unify the coordinates type
     */
    const std::string VolumeName(TVector3 point);
    
    
    /**
     * @brief Name of the deepest material containing the point xyz
     * @return material of the origin by default
     * 
     * @todo make this constant
     * @todo remove return value constantness (or make it a reference)
     * @todo Unify the coordinates type
     */
    const std::string MaterialName(TVector3 point);
    
    
    /// Returns the material at the specified position
    /// @todo Unify the coordinates type
    TGeoMaterial const* Material(double x, double y, double z) const;
    
    /// Returns the total mass [kg] of the specified volume (default: world)
    /// @todo Use GetWorldVolumeName() as default instead
    double TotalMass(const char* vol = "volWorld") const;
    
    // this requires a bit more explanation about what this mass density is...
    /**
     * @brief Return the column density between two points
     * @param p1 pointer to array holding (x, y, z) of the first point
     * @param p2 pointer to array holding (x, y, z) of the second point
     * @return the mass
     * 
     * Both points are specified in world coordinates.
     * 
     * @todo Unify the coordinates type
     */
    /// Returns the mass between two coordinates
    double MassBetweenPoints(double *p1, double *p2) const;
    
    /// @}
    
    
    
    /// @{
    /// @name Cryostat access and information
    
    //
    // group features
    //
    
    //@{
    /**
     * @brief Returns the number of cryostats in the detector
     *
     * The NElements() and NSiblingElements() methods are overloaded and their
     * return depends on the type of ID.
     *
     * @todo Change return type to size_t
     */
    unsigned int Ncryostats() const { return Cryostats().size(); }
    unsigned int NElements() const { return Ncryostats(); }
    unsigned int NSiblingElements(geo::CryostatID const&) const
      { return Ncryostats(); }
    //@}
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns whether we have the specified cryostat
     *
     * The HasElement() method is overloaded and its meaning depends on the type
     * of ID.
     */
    bool HasCryostat(geo::CryostatID const& cryoid) const
      { return cryoid.Cryostat < Ncryostats(); }
    bool HasElement(geo::CryostatID const& cryoid) const
      { return HasCryostat(cryoid); }
    //@}
    
    /**
     * @brief Returns the specified cryostat
     * @param cstat number of cryostat
     * @param cryoid cryostat ID
     * @return a constant reference to the specified cryostat
     * @throws cet::exception ("GeometryCore" category) if not present
     * 
     * The GetElement() method is overloaded and its return depends on the type
     * of ID.
     *
     * @todo Make the cryostat number mandatory (as CryostatID)
     * @todo what happens if it does not exist?
     */
    CryostatGeo const& Cryostat(geo::CryostatID const& cryoid) const;
    CryostatGeo const& Cryostat(unsigned int const cstat = 0) const
      { return Cryostat(geo::CryostatID(cstat)); }
    CryostatGeo const& GetElement(geo::CryostatID const& cryoid) const
      { return Cryostat(cryoid); }
    //@}
    
    //@{
    /**
     * @brief Returns the specified cryostat
     * @param cryoid cryostat ID
     * @return a constant pointer to the specified cryostat, or nullptr if none
     * 
     * The GetElementPtr() method is overloaded and its return depends on the
     * type of ID.
     */
    CryostatGeo const* CryostatPtr(geo::CryostatID const& cryoid) const
      { return HasCryostat(cryoid)? Cryostats()[cryoid.Cryostat]: nullptr; }
    CryostatGeo const* GetElementPtr(geo::CryostatID const& cryoid) const
      { return CryostatPtr(cryoid); }
    //@}
    
    /**
     * @brief Returns the index of the cryostat at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the index of the cryostat, or UINT_MAX if no cryostat is there
     *
     * @todo replace the return value with a CryostatID
     * @todo what happens if it does not exist?
     * @todo Unify the coordinates type
     */
    unsigned int FindCryostatAtPosition(double const worldLoc[3]) const;
    
    //@{
    /**
     * @brief Returns the cryostat at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param cstat (output) number of cryostat
     * @param cid (output) cryostat ID
     * @return a constant reference to the CryostatGeo object of the cryostat
     * @throws cet::exception ("Geometry" category) if no cryostat matches
     * 
     * The tolerance used here is the one returned by DefaultWiggle().
     * 
     * @todo replace the output parameters with a cryostat ID
     */
    CryostatGeo const& PositionToCryostat
      (double const worldLoc[3], geo::CryostatID& cid) const;
    CryostatGeo const& PositionToCryostat
      (double const worldLoc[3], unsigned int &cstat) const;
    //@}
    
    //
    // iterators
    //
    
    /// Initializes the specified ID with the ID of the first cryostat
    void GetBeginID(geo::CryostatID& id) const
      { id = geo::CryostatID(0, HasCryostat(geo::CryostatID(0))); }
    
    /// Initializes the specified ID with the invalid ID after the last cryostat
    void GetEndID(geo::CryostatID& id) const
      { id = geo::CryostatID(Ncryostats(), false); }
    
    /// Returns an iterator pointing to the first cryostat
    cryostat_iterator begin_cryostat() const
      { return cryostat_iterator(this, cryostat_iterator::begin_pos); }
    
    /// Returns an iterator pointing after the last cryostat
    cryostat_iterator end_cryostat() const
      { return cryostat_iterator(this, cryostat_iterator::end_pos); }
    
    /**
     * @brief Enables ranged-for loops on all cryostats of the detector
     * @returns an object suitable for ranged-for loops on all cryostats
     * 
     * Example of usage:
     *     
     *     for (geo::CryostatID const& cID: geom->IterateCryostats()) {
     *       geo::CryostatGeo const& Cryo = geom->Cryostat(cID);
     *       
     *       // useful code here
     *       
     *     } // for all cryostats
     *     
     */
    IteratorBox<
      cryostat_iterator,
      &GeometryCore::begin_cryostat, &GeometryCore::end_cryostat
      >
    IterateCryostats() const { return { this }; }
    
    //
    // single object features
    //
    
    //@{
    /// Returns the half width of the cryostat (x direction)
    double CryostatHalfWidth(geo::CryostatID const& cid) const;
    double CryostatHalfWidth(unsigned int cstat = 0) const
      { return CryostatHalfWidth(geo::CryostatID(cstat)); }
    //@}
    
    //@{
    /// Returns the height of the cryostat (y direction)
    double CryostatHalfHeight(geo::CryostatID const& cid) const;
    double CryostatHalfHeight(unsigned int cstat = 0) const
      { return CryostatHalfHeight(geo::CryostatID(cstat)); }
    //@}
    
    //@{
    /// Returns the length of the cryostat (z direction)
    double CryostatLength(geo::CryostatID const& cid) const;
    double CryostatLength(unsigned int cstat = 0) const
      { return CryostatLength(geo::CryostatID(cstat)); }
    //@}
    
    //@{
    /**
     * @brief Returns the boundaries of the specified cryostat
     * @param boundaries (output) pointer to an area of 6 doubles for boundaries
     * @param cstat number of cryostat
     * 
     * The boundaries array is filled with:
     * [0] lower x coordinate  [1] upper x coordinate
     * [2] lower y coordinate  [3] upper y coordinate
     * [4] lower z coordinate  [5] upper z coordinate
     * 
     * @todo What happen on invalid cryostat?
     * @todo Use a CryostatID instead
     * @todo Check the implementation in TPC and use that one instead
     */
    void CryostatBoundaries
      (double* boundaries, geo::CryostatID const& cid) const;
    void CryostatBoundaries
      (double* boundaries, unsigned int cstat = 0) const
      { CryostatBoundaries(boundaries, geo::CryostatID(cstat)); }
    //@}
    
    //
    // object description
    //
    
    //@{
    /**
     * @brief Return the name of LAr TPC volume
     * @param cstat index of the cryostat
     * @return the name of the specified TPC
     * 
     * This information is used in the event display.
     * 
     * @todo Use a cryostat ID instead
     * @todo What if it does not exist?
     */
    std::string GetCryostatVolumeName(geo::CryostatID const& cid) const;
    std::string GetCryostatVolumeName(unsigned int const cstat = 0) const
      { return GetCryostatVolumeName(geo::CryostatID(cstat)); }
    //@}
    
    /// @} Cryostat access and information
    
    
    
    /// @{
    /// @name TPC access and information
    
    //
    // group features
    //
    
    /**
     * @brief Returns the total number of TPCs in the specified cryostat
     * @param cstat cryostat number
     * 
     * @todo Make the cryostat number mandatory (as CryostatID)
     * @todo Change return type to size_t
     * @todo what happens if it does not exist?
     */
    unsigned int NTPC(unsigned int cstat = 0) const
      { return NTPC(geo::CryostatID(cstat)); }
    
    /// Returns the largest number of TPCs a cryostat in the detector has
    unsigned int MaxTPCs() const;
    
    /**
     * @brief Returns the total number of TPCs in the specified cryostat
     * @param cryoid cryostat number
     * @return number of TPCs in specified cryostat, or 0 if no cryostat found
     *
     * The NElements() and NSiblingElements() methods are overloaded and their
     * return depends on the type of ID.
     * 
     * @todo Change return type to size_t
     */
    unsigned int NTPC(geo::CryostatID const& cryoid) const
      {
        CryostatGeo const* pCryo = GetElementPtr(cryoid);
        return pCryo? pCryo->NElements(): 0;
      }
    unsigned int NElements(geo::CryostatID const& cryoid) const
      { return NTPC(cryoid); }
    unsigned int NSiblingElements(geo::TPCID const& tpcid) const
      { return NTPC(tpcid); }
    //@}
    
    
    //
    // access
    //
    /// Returns whether we have the specified TPC
    bool HasTPC(geo::TPCID const& tpcid) const
      {
        CryostatGeo const* pCryo = CryostatPtr(tpcid);
        return pCryo? pCryo->HasTPC(tpcid): false;
      }
    
    /// Returns whether we have the specified TPC
    bool HasElement(geo::TPCID const& tpcid) const { return HasTPC(tpcid); }
    
    
    ///@{
    /**
     * @brief Returns the specified TPC
     * @param tpcid ID of the tpc
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return a constant reference to the specified TPC
     * 
     * The GetElement() method is overloaded and its return depends on the type
     * of ID.
     * 
     * @todo remove the version with integers
     * @todo what happens if it does not exist?
     */
    TPCGeo const& TPC
      (unsigned int const tpc   = 0, unsigned int const cstat = 0) const
      { return TPC(geo::TPCID(cstat, tpc)); }
    TPCGeo const& TPC(geo::TPCID const& tpcid) const
      { return Cryostat(tpcid).TPC(tpcid); }
    TPCGeo const& GetElement(geo::TPCID const& tpcid) const
      { return TPC(tpcid); }
    ///@}
    
    //@{
    /**
     * @brief Returns the specified TPC
     * @param tpcid TPC ID
     * @return a constant pointer to the specified TPC, or nullptr if none
     * 
     * The GetElementPtr() method is overloaded and its return depends on the
     * type of ID.
     */
    TPCGeo const* TPCPtr(geo::TPCID const& tpcid) const
      {
        CryostatGeo const* pCryo = CryostatPtr(tpcid);
        return pCryo? pCryo->TPCPtr(tpcid): nullptr;
      } // TPCPtr()
    TPCGeo const* GetElementPtr(geo::TPCID const& tpcid) const
      { return TPCPtr(tpcid); }
    //@}
    
    /**
     * @brief Returns the ID of the TPC at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the TPC ID, or an invalid one if no TPC is there
     */
    geo::TPCID FindTPCAtPosition(double const worldLoc[3]) const;
    
    
    //@{
    /**
     * @brief Returns the TPC at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param tpc (output) tpc number within the cryostat
     * @param cstat (output) number of cryostat
     * @return a constant reference to the TPCGeo object of the TPC
     * @throws cet::exception ("Geometry" category) if no TPC matches
     * 
     * @todo replace the output parameters with a TPC ID
     */
    TPCGeo const& PositionToTPC
      (double const worldLoc[3], unsigned int &tpc, unsigned int &cstat) const;
    TPCGeo const& PositionToTPC
      (double const worldLoc[3], TPCID& tpcid) const;
    //@}
    
    
    ///
    /// iterators
    ///
    
    /// Initializes the specified ID with the ID of the first TPC
    void GetBeginID(geo::TPCID& id) const
      { GetBeginID(static_cast<geo::CryostatID&>(id)); id.TPC = 0; }
    
    /// Initializes the specified ID with the invalid ID after the last TPC
    void GetEndID(geo::TPCID& id) const
      { GetEndID(static_cast<geo::CryostatID&>(id)); id.TPC = 0; }
    
    
    /// Returns an iterator pointing to the first TPC in the detector
    TPC_iterator begin_TPC() const
      { return TPC_iterator(this, TPC_iterator::begin_pos); }
    
    /// Returns an iterator pointing after the last TPC in the detector
    TPC_iterator end_TPC() const
      { return TPC_iterator(this, TPC_iterator::end_pos); }
    
    /**
     * @brief Enables ranged-for loops on all TPCs of the detector
     * @returns an object suitable for ranged-for loops on all TPCs
     * 
     * Example of usage:
     *     
     *     for (geo::TPCID const& tID: geom->IterateTPCs()) {
     *       geo::TPCGeo const& TPC = geom->TPC(tID);
     *       
     *       // useful code here
     *       
     *     } // for all TPC
     *     
     */
    IteratorBox<
      TPC_iterator,
      &GeometryCore::begin_TPC, &GeometryCore::end_TPC
      >
    IterateTPCs() const { return { this }; }
    
    
    //
    // single object features
    //
    
    //@{
    /**
     * @brief Returns the half width of the specified TPC (x direction)
     * @param tpcid ID of the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return the value of the half width of the specified TPC
     * 
     * 
     * @todo what happens if it does not exist?
     * @todo add a version with TPCID
     * @todo deprecate this function
     * @todo rename the function
     */
    double DetHalfWidth(geo::TPCID const& tpcid) const;
    double DetHalfWidth(unsigned int tpc = 0, unsigned int cstat = 0) const
      { return DetHalfWidth(geo::TPCID(cstat, tpc)); }
    //@}
    
    //@{
    /**
     * @brief Returns the half height of the specified TPC (y direction)
     * @param tpcid ID of the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return the value of the half height of the specified TPC
     * 
     * 
     * @todo what happens if it does not exist?
     * @todo add a version with TPCID
     * @todo deprecate this function
     * @todo rename the function
     */
    double DetHalfHeight(geo::TPCID const& tpcid) const;
    double DetHalfHeight(unsigned int tpc = 0, unsigned int cstat = 0) const
      { return DetHalfHeight(geo::TPCID(cstat, tpc)); }
    //@}
    
    //@{
    /**
     * @brief Returns the length of the specified TPC (z direction)
     * @param tpcid ID of the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return the value of the length of the specified TPC
     * 
     * 
     * @todo what happens if it does not exist?
     * @todo add a version with TPCID
     * @todo deprecate this function
     * @todo rename the function
     */
    double DetLength(geo::TPCID const& tpcid) const;
    double DetLength(unsigned int tpc = 0, unsigned int cstat = 0) const
      { return DetLength(geo::TPCID(cstat, tpc)); }
    //@}
    
    
    //@{
    /**
     * @brief Returns the centre of side of the detector facing the beam
     * @param tpcid ID of the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return a vector of the position of centre of TPC face toward the beam
     * 
     * As of April 2015, the origin of the co-ordinate system for ArgoNEUT and
     * MicroBooNE is for z=0 and y=0 to be at the centre of the front face of
     * the detector, but x=0 to be the edge of the TPC.
     * This is convenient for read-out, but a pain for simulation.
     * This method returns the centre of the front face of the TPC in the world
     * co-ordinate system, making it easier to write detector-independent
     * simulation code.
     * 
     * @bug Except that its implementation is not detector-independent at all,
     * not to mention that it's taking x not in the middle but at one fourth
     * of the TPC.
     * @todo Replace with a TPCID
     */
    TVector3 GetTPCFrontFaceCenter(geo::TPCID const& tpcid) const;
    TVector3 GetTPCFrontFaceCenter
      (unsigned int tpc = 0, unsigned int cstat = 0) const
      { return GetTPCFrontFaceCenter(geo::TPCID(cstat, tpc)); }
    //@}
    
    
    //
    // object description
    //
    
    //@{
    /**
     * @brief Return the name of specified LAr TPC volume
     * @param tpcid ID of the TPC
     * @param tpc index of TPC in the cryostat
     * @param cstat index of the cryostat
     * @return the name of the specified TPC
     * 
     * This information is used by Geant4 simulation
     * 
     * @todo Use a TPCID instead
     * @todo What if it does not exist?
     */
    std::string GetLArTPCVolumeName(geo::TPCID const& tpcid) const;
    std::string GetLArTPCVolumeName
      (unsigned int const tpc = 0, unsigned int const cstat = 0) const
      { return GetLArTPCVolumeName(geo::TPCID(cstat, tpc)); }
    //@}
    
    /// @} TPC access and information
    
    
    
    /// @{
    /// @name Plane access and information
    
    //
    // group features
    //
    
    /**
     * @brief Returns the total number of wire planes in the specified TPC
     * @param tpc tpc number within the cryostat
     * @param cstat cryostat number
     * 
     * @todo Make all the arguments mandatory (as TPCID)
     * @todo Change return type to size_t
     * @todo what happens if TPC does not exist?
     */
    unsigned int Nplanes(unsigned int tpc   = 0, unsigned int cstat = 0) const
      { return Nplanes(geo::TPCID(cstat, tpc)); }
    
    /// Returns the largest number of planes among all TPCs in this detector
    unsigned int MaxPlanes() const;
    
    //@{
    /**
     * @brief Returns the total number of planes in the specified TPC
     * @param tpcid TPC ID
     * @return number of planes in specified TPC, or 0 if no TPC found
     *
     * The NElements() and NSiblingElements() methods are overloaded and their
     * return depends on the type of ID.
     * 
     * @todo Change return type to size_t
     */
    unsigned int Nplanes(geo::TPCID const& tpcid) const
      {
        TPCGeo const* pTPC = GetElementPtr(tpcid);
        return pTPC? pTPC->NElements(): 0;
      }
    unsigned int NElements(geo::TPCID const& tpcid) const
      { return Nplanes(tpcid); }
    unsigned int NSiblingElements(geo::PlaneID const& planeid) const
      { return Nplanes(planeid); }
    //@}
    
    
    /**
     * @brief Returns the number of views (different wire orientations)
     * 
     * Returns the number of different views, or wire orientations, in the
     * detector.
     * 
     * The function assumes that all TPCs in all cryostats of a detector have
     * the same number of planes, which should be a safe assumption.
     * 
     * @todo Change return type to size_t
     */
    unsigned int Nviews() const;
    
    /**
     * @brief Returns a list of possible PlaneIDs in the detector
     * @return a constant reference to the set of plane IDs
     * 
     * @todo verify the implementation
     * @todo verify the use
     * @deprecated This function smells a lot... do we really need a list of 720
     * plane IDs of DUNE FD? probably better to use iterators instead
     */
    std::set<PlaneID> const& PlaneIDs() const;
    
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns whether we have the specified plane
     *
     * The HasElement() method is overloaded and its meaning depends on the type
     * of ID.
     *
     */
    bool HasPlane(geo::PlaneID const& planeid) const
      {
        geo::TPCGeo const* pTPC = TPCPtr(planeid);
        return pTPC? pTPC->HasPlane(planeid): false;
      }
    bool HasElement(geo::PlaneID const& planeid) const
      { return HasPlane(planeid); }
    //@}
    
    ///@{
    /**
     * @brief Returns the specified wire
     * @param planeid ID of the plane
     * @param p plane number within the TPC
     * @param tpc TPC number within the cryostat
     * @param cstat number of cryostat
     * @return a constant reference to the specified plane
     * 
     * The GetElement() method is overloaded and its return depends on the type
     * of ID.
     * 
     * @todo remove the version with integers
     * @todo what happens if it does not exist?
     */
    PlaneGeo const& Plane
      (unsigned int const p, unsigned int const tpc   = 0, unsigned int const cstat = 0)
      const
      { return Plane(geo::PlaneID(cstat, tpc, p)); }
    PlaneGeo const& Plane(geo::PlaneID const& planeid) const
      { return TPC(planeid).Plane(planeid); }
    PlaneGeo const& GetElement(geo::PlaneID const& planeid) const
      { return Plane(planeid); }
    ///@}
    
    //@{
    /**
     * @brief Returns the specified plane
     * @param planeid plane ID
     * @return a constant pointer to the specified plane, or nullptr if none
     * 
     * The GetElementPtr() method is overloaded and its return depends on the
     * type of ID.
     */
    PlaneGeo const* PlanePtr(geo::PlaneID const& planeid) const
      {
        geo::TPCGeo const* pTPC = TPCPtr(planeid);
        return pTPC? pTPC->PlanePtr(planeid): nullptr;
      } // PlanePtr()
    PlaneGeo const* GetElementPtr(geo::PlaneID const& planeid) const
      { return PlanePtr(planeid); }
    //@}
    
    //
    // iterators
    //
    
    /// Initializes the specified ID with the ID of the first plane
    void GetBeginID(geo::PlaneID& id) const
      { GetBeginID(static_cast<geo::TPCID&>(id)); id.Plane = 0; }
    
    /// Initializes the specified ID with the invalid ID after the last plane
    void GetEndID(geo::PlaneID& id) const
      { GetEndID(static_cast<geo::TPCID&>(id)); id.Plane = 0; }
    
    /// Returns an iterator pointing to the first plane in the detector
    plane_iterator begin_plane() const
      { return plane_iterator(this, plane_iterator::begin_pos); }
    
    /// Returns an iterator pointing after the last plane in the detector
    plane_iterator end_plane() const
      { return plane_iterator(this, plane_iterator::end_pos); }
    
    /**
     * @brief Enables ranged-for loops on all planes of the detector
     * @returns an object suitable for ranged-for loops on all planes
     * 
     * Example of usage:
     *     
     *     for (geo::PlaneID const& pID: geom->IteratePlanes()) {
     *       geo::PlaneGeo const& Plane = geom->Plane(pID);
     *       
     *       // useful code here
     *       
     *     } // for all planes
     *     
     */
    IteratorBox<
      plane_iterator,
      &GeometryCore::begin_plane, &GeometryCore::end_plane
      >
    IteratePlanes() const { return { this }; }
    
    //
    // single object features
    //
    
    //@{
    /**
     * @brief Returns the distance between two planes
     * @param p1 index of the first plane
     * @param p2 index of the second plane
     * @param tpc tpc number within the cryostat
     * @param cstat cryostat number
     * @return distance between the planes
     * 
     * @todo add a version with plane IDs
     * @todo deprecate this function
     * @todo add a default version for a given TPCID
     * @todo add a version with two plane indices for a given TPCID
     * @todo return the absolute value of the distance (makes the order unimportant)
     * @todo document what will happen (in the future methods) with planes on different TPCs
     */
    double PlanePitch(
      geo::TPCID const& tpcid,
      geo::PlaneID::PlaneID_t p1 = 0, geo::PlaneID::PlaneID_t p2 = 1
      )
      const;
    double PlanePitch(geo::PlaneID const& pid1, geo::PlaneID const& pid2) const;
    double PlanePitch(unsigned int p1 = 0,
                      unsigned int p2 = 1,
                      unsigned int tpc = 0,
                      unsigned int cstat = 0) const;
    //@}
    
    /**
     * @brief Returns the view (wire orientation) on the channels of specified TPC plane
     * @param plane TPC plane ID
     * @return the type of signal on the specified plane, or geo::kUnknown
     * 
     * @todo verify that kUnknown is returned on invalid plane
     */
    View_t View(geo::PlaneID const& pid) const;
    
    /**
     * @brief Returns the type of signal on the channels of specified TPC plane
     * @param plane TPC plane ID
     * @return the type of signal on the specified plane, or geo::kMysteryType
     * 
     * Assumes that all the channels on the plane have the same signal type.
     * 
     * @todo verify that kMysteryType is returned on invalid plane
     */
    SigType_t SignalType(geo::PlaneID const& pid) const;
    
    
    /// @} Plane access and information
    
    
    /// @{
    /// @name Wire access and information
    
    //
    // group features
    //
    
    /**
     * @brief Returns the total number of wires in the specified plane
     * @param p plane number within the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat cryostat number
     * 
     * @todo Make all the arguments mandatory (as PlaneID)
     * @todo Change return type to size_t
     * @todo what happens if it does not exist?
     */
    unsigned int Nwires
      (unsigned int p, unsigned int tpc   = 0, unsigned int cstat = 0) const
      { return Nwires(geo::PlaneID(cstat, tpc, p)); }
    
    //@{
    /**
     * @brief Returns the total number of wires in the specified plane
     * @param planeid plane ID
     * @return number of wires in specified plane, or 0 if no plane found
     *
     * The NElements() and NSiblingElements() methods are overloaded and their
     * return depends on the type of ID.
     * 
     * @todo Change return type to size_t
     */
    unsigned int Nwires(geo::PlaneID const& planeid) const
      {
        PlaneGeo const* pPlane = GetElementPtr(planeid);
        return pPlane? pPlane->NElements(): 0;
      }
    unsigned int NElements(geo::PlaneID const& planeid) const
      { return Nwires(planeid); }
    unsigned int NSiblingElements(geo::WireID const& wireid) const
      { return Nwires(wireid); }
    
    /// Returns the largest number of wires among all planes in this detector
    unsigned int MaxWires() const;
    
    //@}
    
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns whether we have the specified wire
     *
     * The HasElement() method is overloaded and its meaning depends on the type
     * of ID.
     */
    bool HasWire(geo::WireID const& wireid) const
      {
        geo::PlaneGeo const* pPlane = PlanePtr(wireid);
        return pPlane? pPlane->HasWire(wireid): false;
      }
    bool HasElement(geo::WireID const& wireid) const { return HasWire(wireid); }
    //@}
    
    //@{
    /**
     * @brief Returns the specified wire
     * @param wireid wire ID
     * @return a constant pointer to the specified wire, or nullptr if none
     * 
     * The GetElementPtr() method is overloaded and its return depends on the
     * type of ID.
     */
    WireGeo const* WirePtr(geo::WireID const& wireid) const
      {
        geo::PlaneGeo const* pPlane = PlanePtr(wireid);
        return pPlane? pPlane->WirePtr(wireid): nullptr;
      } // WirePtr()
    WireGeo const* GetElementPtr(geo::WireID const& wireid) const
      { return WirePtr(wireid); }
    //@}
    
    //@{
    /**
     * @brief Returns the specified wire
     * @param wireid ID of the wire
     * @return a constant reference to the specified wire
     * @throw cet::exception if not found
     * 
     * The GetElement() method is overloaded and its return depends on the type
     * of ID.
     */
    WireGeo const& Wire(geo::WireID const& wireid) const
      { return Plane(wireid).Wire(wireid); }
    WireGeo const& WireIDToWireGeo(geo::WireID const& wireid) const
      { return Wire(wireid); }
    WireGeo const& GetElement(geo::WireID const& wireid) const
      { return Wire(wireid); }
    //@}
    
    //
    // iterators
    //
    
    /// Initializes the specified ID with the ID of the first wire
    void GetBeginID(geo::WireID& id) const
      { GetBeginID(static_cast<geo::PlaneID&>(id)); id.Wire = 0; }
    
    /// Initializes the specified ID with the invalid ID after the last wire
    void GetEndID(geo::WireID& id) const
      { GetEndID(static_cast<geo::PlaneID&>(id)); id.Wire = 0; }
    
    /// Returns an iterator pointing to the first wire in the detector
    wire_iterator begin_wire() const
      { return wire_iterator(this, wire_iterator::begin_pos); }
    
    /// Returns an iterator pointing after the last wire in the detector
    wire_iterator end_wire() const
      { return wire_iterator(this, wire_iterator::end_pos); }
    
    /**
     * @brief Enables ranged-for loops on all wires of the detector
     * @returns an object suitable for ranged-for loops on all wires
     * 
     * Example of usage:
     *     
     *     for (geo::WireID const& wID: geom->IterateWires()) {
     *       geo::WireGeo const& Wire = geom->Wire(wID);
     *       
     *       // useful code here
     *       
     *     } // for all wires
     *     
     */
    IteratorBox<
      wire_iterator,
      &GeometryCore::begin_wire, &GeometryCore::end_wire
      >
    IterateWires() const { return { this }; }
    
    //
    // single object features
    //
    
    //@{
    /**
     * @brief Returns the distance between two wires
     * @param w1 index of the first wire (unused!)
     * @param w2 index of the second wire (unused!)
     * @param p plane number within the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat cryostat number
     * @return the distance between the two wires
     * 
     * The wires must belong to the same plane. They are assumed parallel.
     * 
     * @todo add a version with wire IDs
     * @todo deprecate this function
     * @todo add a default version for a given PlaneID
     * @todo add a version with two wire indices for a given PlaneID
     * @todo return the absolute value of the distance (makes the order unimportant)
     * @todo document what will happen (in the future methods) with wires on different planes
     * 
     */
    double WirePitch
      (geo::PlaneID const& planeid, unsigned int w1 = 0, unsigned int w2 = 1)
      const;
    double WirePitch(unsigned int w1 = 0, 
                     unsigned int w2 = 1, 
                     unsigned int plane = 0,
                     unsigned int tpc = 0,
                     unsigned int cstat = 0) const
      { return WirePitch(geo::PlaneID(cstat, tpc, plane), w1, w2); }
    //@}
    
    /**
     * @brief Returns the distance between two wires in the specified view
     * @param w1 index of the first wire
     * @param w2 index of the second wire
     * @param p plane number within the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat cryostat number
     * @return the distance between the two wires
     * 
     * This method assumes that all the wires on all the planes on the specified
     * view of all TPCs have the same pitch.
     */
    double WirePitch(geo::View_t view) const; 
    
    
    //@{
    /**
     * @brief Returns the angle of the wires in the specified view from vertical
     * @param view the view
     * @param TPC the index of the TPC in the specified cryostat
     * @param Cryo the cryostat
     * @param tpcid ID of the TPC
     * @return the angle [radians]
     * @throw cet::exception ("GeometryCore" category) if no such view
     * 
     * The angle is defined as in WireGeo::ThetaZ().
     * 
     * This method assumes all wires in the view have the same angle (it queries
     * for the first).
     *
     * @todo Use a TPCID
     * @deprecated This does not feel APA-ready
     */
    double WireAngleToVertical(geo::View_t view, geo::TPCID const& tpcid) const;
    double WireAngleToVertical(geo::View_t view, int TPC=0, int Cryo=0) const
      { return WireAngleToVertical(view, geo::TPCID(Cryo, TPC)); }
    //@}
    
    /// @} Wire access and information
    
    
    
    /// @{
    /**
     * @name Wire geometry queries
     * 
     * Please note the differences between functions:
     * ChannelsIntersect(), WireIDsIntersect() and IntersectionPoint()
     * all calculate wires intersection using the same equation.
     * ChannelsIntersect() and WireIdsIntersect() will return true
     * if the two wires cross, return false if they don't.
     * IntersectionPoint() does not check if the two wires cross.
     */
    
    //
    // simple geometry queries
    //
    
    //@{
    /**
     * @brief Fills two arrays with the coordinates of the wire end points
     * @param wireid ID of the wire
     * @param cstat cryostat number
     * @param tpc tpc number within the cryostat
     * @param plane plane number within the TPC
     * @param wire wire number within the plane
     * @param xyzStart (output) an array with the start coordinate
     * @param xyzEnd (output) an array with the end coordinate
     * 
     * The starting point is the wire end with lower z coordinate.
     * 
     * @todo use a wire ID instead
     * @todo use an array instead?
     * @todo what happens if it does not exist?
     */
    void WireEndPoints
      (geo::WireID const& wireid, double *xyzStart, double *xyzEnd) const;
    void WireEndPoints(
      unsigned int cstat, unsigned int tpc, unsigned int plane, unsigned int wire,
      double *xyzStart, double *xyzEnd
      ) const
      { WireEndPoints(geo::WireID(cstat, tpc, plane, wire), xyzStart, xyzEnd); }
    //@}
    
    //
    // closest wire
    //
    
    //@{
    /**
     * @brief Returns the ID of wire closest to position in the specified TPC
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param planeid ID of the plane
     * @param PlaneNo plane number within the TPC
     * @param TPCNo tpc number within the cryostat
     * @param cstat cryostat number
     * @return the ID of the wire, or an invalid wire ID
     *
     * The different versions allow different way to provide the position.
     *
     * @todo add a PlaneID version
     * @todo remove the integers version
     * @todo Verify the invalid wire ID part
     */
    geo::WireID NearestWireID
      (const double worldLoc[3], geo::PlaneID const& planeid) const;
    geo::WireID  NearestWireID
      (std::vector<double> const& worldLoc, geo::PlaneID const& planeid)  const;
    geo::WireID   NearestWireID
      (const TVector3& worldLoc, geo::PlaneID const& planeid) const;
    geo::WireID   NearestWireID(const double worldLoc[3],
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const
     { return NearestWireID(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    geo::WireID   NearestWireID(std::vector<double> const& worldLoc,
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const
     { return NearestWireID(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    geo::WireID   NearestWireID(const TVector3& worldLoc,
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const
     { return NearestWireID(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    //@{
    /**
     * @brief Returns the index of wire closest to position in the specified TPC
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param planeid ID of the plane
     * @param PlaneNo plane number within the TPC
     * @param TPCNo tpc number within the cryostat
     * @param cstat cryostat number
     * @return the index of the wire
     *
     * The different versions allow different way to provide the position.
     *
     * @todo add a PlaneID version
     * @todo remove the integers version
     * @todo what happens when no wire is found?
     * @todo deprecate this for NearestWireID()
     */
    unsigned int       NearestWire
      (const double worldLoc[3], geo::PlaneID const& planeid)  const;
    unsigned int       NearestWire
      (std::vector<double> const& worldLoc, geo::PlaneID const& planeid) const;
    unsigned int       NearestWire
      (const TVector3& worldLoc, geo::PlaneID const& planeid)  const;
    unsigned int       NearestWire(const double worldLoc[3],
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0)  const
      { return NearestWire(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    unsigned int       NearestWire(std::vector<double> const& worldLoc,
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0) const
      { return NearestWire(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    unsigned int       NearestWire(const TVector3& worldLoc,
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0)  const
      { return NearestWire(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    
    //@{
    /**
     * @brief Returns the index of the nearest wire to the specified position
     * @param YPos y coordinate on the wire plane
     * @param ZPos z coordinate on the wire plane
     * @param planeid ID of the plane
     * @param PlaneNo number of plane
     * @param TPCNo number of TPC
     * @param cstat number of cryostat
     * @return an index interpolation between the two nearest wires
     * @see ChannelMapAlg::WireCoordinate()
     *
     * Respect to NearestWireID(), this method returns a real number,
     * representing a continuous coordinate in the wire axis, with the round
     * values corresponding to the actual wires.
     * 
     * @todo Unify (y, z) coordinate
     * @todo use plane ID instead
     */
    double WireCoordinate
      (double YPos, double ZPos, geo::PlaneID const& planeid) const;
    double WireCoordinate(double YPos, double ZPos,
                          unsigned int PlaneNo,
                          unsigned int TPCNo,
                          unsigned int cstat) const
      { return WireCoordinate(YPos, ZPos, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    //
    // wire intersections
    //
    
    // The following functions are utilized to determine if two wires
    // in the TPC intersect or not, and if they do then
    // determine the coordinates of the intersection.
    
    /**
     * @brief Computes the intersection between two wires
     * @param wid1 ID of the first wire
     * @param wid2 ID of the other wire
     * @param widIntersect (output) the coordinate of the intersection point
     * @return whether an intersection was found
     * 
     * The "intersection" refers to the projection of the wires into the same
     * x = 0 plane.
     * Wires are assumed to have at most one intersection.
     * If wires are parallel or belong to different TPCs or to the same plane
     * (i.e. they are parallel), widIntersect is undefined and false is
     * returned.
     * 
     * @todo What if the wires intersect outside their TPC?
     */
    bool WireIDsIntersect
      (WireID const& wid1, WireID const& wid2, WireIDIntersection& widIntersect)
      const;
    
    //@{
    /**
     * @brief Returns the intersection point of two wires
     * @param wid1 ID of the first wire
     * @param wid2 ID of the other wire
     * @param wire1 wire index of the first wire
     * @param wire2 wire index of the other wire
     * @param plane1 plane index of the first wire
     * @param plane2 plane index of the other wire
     * @param tpc tpc number within the cryostat where the planes belong
     * @param cstat cryostat number
     * @param start_w1 coordinates of the start of the first wire
     * @param end_w1 coordinates of the end of the first wire
     * @param start_w2 coordinates of the start of the other wire
     * @param end_w2 coordinates of the end of the other wire
     * @param y (output) y coordinate of the intersection point
     * @param z (output) z coordinate of the intersection point
     *
     * No check is performed, not any information provided, about the validity
     * of the result.
     * 
     * @todo move to protected (or just junk it)
     * @todo make wire borders constant
     * @todo return a WireIDIntersection instead
     * @todo what if the intersection is outside the TPC?
     *
     * @deprecated For internal use only
     */
    void IntersectionPoint(geo::WireID const& wid1,
                           geo::WireID const& wid2,
                           double start_w1[3],
                           double end_w1[3],
                           double start_w2[3],
                           double end_w2[3],
                           double &y,
                           double &z);
    void IntersectionPoint(unsigned int wire1,
                           unsigned int wire2,
                           unsigned int plane1,
                           unsigned int plane2,
                           unsigned int cstat,
                           unsigned int tpc,
                           double start_w1[3],
                           double end_w1[3],
                           double start_w2[3],
                           double end_w2[3],
                           double &y,
                           double &z)
      {
        return IntersectionPoint(
          geo::WireID(cstat, tpc, plane1, wire1),
          geo::WireID(cstat, tpc, plane2, wire2),
          start_w1, end_w1, start_w2, end_w2, y, z
          );
      }
    //@}
    //@{
    /**
     * @brief Returns the intersection point of two wires
     * @param wid1 ID of the first wire
     * @param wid2 ID of the other wire
     * @param wire1 wire index of the first wire
     * @param wire2 wire index of the other wire
     * @param plane1 plane index of the first wire
     * @param plane2 plane index of the other wire
     * @param cstat cryostat number
     * @param tpc tpc number within the cryostat where the planes belong
     * @param y (output) y coordinate of the intersection point
     * @param z (output) z coordinate of the intersection point
     *
     * No check is performed, not any information provided, about the validity
     * of the result.
     * 
     * @todo use WireIDs instead? or else, use TPCID
     * @todo move to protected (or just junk it)
     * @todo return a WireIDIntersection instead
     * @todo what if the intersection is outside the TPC?
     */
    void IntersectionPoint(geo::WireID const& wid1,
                           geo::WireID const& wid2,
                           double &y,
                           double &z);
    void IntersectionPoint(unsigned int wire1,
                           unsigned int wire2,
                           unsigned int plane1,
                           unsigned int plane2,
                           unsigned int cstat,
                           unsigned int tpc,
                           double &y,
                           double &z)
      {
        return IntersectionPoint(
          geo::WireID(cstat, tpc, plane1, wire1),
          geo::WireID(cstat, tpc, plane2, wire2),
          y, z
          );
      }
    //@}
    
    //@{
    /**
     * @brief Returns the slope on the third plane, given it in the other two
     * @param plane1 plane index of the first slope
     * @param slope1 slope as seen on the first plane
     * @param plane2 plane index of the second slope
     * @param slope2 slope as seen on the second plane
     * @param tpc tpc number within the cryostat where the planes belong
     * @param cstat cryostat number
     * @return the slope on the third plane
     *
     * Given a slope as projected in two planes, returns the slope as projected
     * in the third plane.
     * The slopes are defined in dTime/dWire units, that assumes equal wire
     * pitch in all planes and a uniform drift velocity.
     * This method assumes the presence of three planes.
     * 
     * @todo Probably the math is good for any number of planes though
     * @todo Check the correctness of the definition of the slopes
     * @todo What happens if it's infinite?
     */
    double ThirdPlaneSlope(geo::PlaneID const& pid1, double slope1, 
                           geo::PlaneID const& pid2, double slope2) const;
    double ThirdPlaneSlope(geo::PlaneID::PlaneID_t plane1, double slope1, 
                           geo::PlaneID::PlaneID_t plane2, double slope2,
                           geo::TPCID const& tpcid) const
      {
        return ThirdPlaneSlope(
          geo::PlaneID(tpcid, plane1), slope1,
          geo::PlaneID(tpcid, plane2), slope2
          );
      }
    double ThirdPlaneSlope(unsigned int plane1, double slope1,
                           unsigned int plane2, double slope2,
                           unsigned int tpc, unsigned int cstat) const
      {
        return ThirdPlaneSlope
          (plane1, slope1, plane2, slope2, geo::TPCID(cstat, tpc));
      }
    //@}

    /// @} Wire geometry queries
    
    
    
    /// @{
    /// @name Optical detector access and information
    
    //
    // group features
    //
    
    /// Number of OpDets in the whole detector
    unsigned int NOpDets() const;
    
    
    //
    // access
    //
    //@{
    /// Access the OpDetGeo object by OpDet or Channel Number
    OpDetGeo const& OpDetGeoFromOpChannel(unsigned int OpChannel) const;
    OpDetGeo const& OpDetGeoFromOpDet(unsigned int OpDet) const;
    //@}
    
    /**
     * @brief Find the nearest OpChannel to some point
     * @param xyz point to be queried, in world coordinates
     * @return the nearest OpChannel to the point
     * 
     * The cryostat the channel is in is automatically discovered.
     * @todo make xyz constant; maybe an array?
     * @todo Unify the coordinates type
     */
    unsigned int GetClosestOpDet(double * xyz) const;
    
    
    //
    // object description
    //
    
    /**
     * @brief Returns gdml string which gives sensitive opdet name
     * @param c ID of the cryostat the detector is in
     * 
     * This name is defined in the geometry (GDML) description.
	  * 
     * @todo Change to use CryostatID
     */
    std::string OpDetGeoName(unsigned int c = 0) const;
    
    /// @} Optical detector access and information
    
    
    
    /// @{
    /// @name Auxiliary detectors access and information
    
    /// @todo use a AutDetID_t instead of unsigned int?
    
    //
    // group features
    //
    
    /**
     * @brief Returns the number of auxiliary detectors
     * 
     * This method returns the total number of scintillator paddles
     * (Auxiliary Detectors aka AuxDet) outside of the cryostat
     * 
     * @todo Change return type to size_t
     */
    unsigned int NAuxDets() const { return AuxDets().size(); }
    
    //
    // access
    //
    
    /// Returns the full list of pointer to the auxiliary detectors
    std::vector<AuxDetGeo*> const& AuxDetGeoVec() const { return AuxDets(); }
    
    /**
     * @brief Returns the specified auxiliary detector
     * @param ad the auxiliary detector index
     * @return a constant reference to the specified auxiliary detector
     * 
     * @todo what happens if it does not exist?
     * @todo remove the default parameter?
     */
    AuxDetGeo const& AuxDet(unsigned int const ad = 0) const;
    
    /**
     * @brief Returns the index of the auxiliary detector at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the index of the detector, or UINT_MAX if no detector is there
     * 
     * @todo replace with numeric_limits<>?
     */
    unsigned int FindAuxDetAtPosition(double const worldLoc[3]) const;
    
    /**
     * @brief Fills the indices of the sensitive auxiliary detector at location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param adg (output) auxiliary detector index
     * @param sv (output) sensitive volume index
     */
    void  FindAuxDetSensitiveAtPosition(double const worldLoc[3],
                                        size_t     & adg,
                                        size_t     & sv) const;
    
    /**
     * @brief Returns the auxiliary detector at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param ad (output) the auxiliary detector index
     * @return constant reference to AuxDetGeo object of the auxiliary detector
     * 
     * @todo what happens if it does not exist?
     */
    AuxDetGeo const& PositionToAuxDet
      (double const worldLoc[3], unsigned int &ad) const;
    
    /**
     * @brief Returns the auxiliary detector at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param ad (output) the auxiliary detector index
     * @param sv (output) the auxiliary detector sensitive volume index
     * @return reference to AuxDetSensitiveGeo object of the auxiliary detector
     * 
     * @todo what happens if it does not exist?
     */
    const AuxDetSensitiveGeo& PositionToAuxDetSensitive(double const worldLoc[3],
                                                        size_t     & ad,
                                                        size_t     & sv) const;
    
    /// @} Auxiliary detectors access and information
    
    
    
    /// @{
    /// @name TPC readout channels and views
    
    //
    // group features
    //
    
    /// Returns the number of TPC readout channels in the detector
    unsigned int Nchannels() const;
    
    /**
     * @brief Returns a list of possible views in the detector
     * @return a constant reference to the set of views
     * 
     * @todo Verify the implementation
     */
    std::set<View_t> const& Views() const;
    
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns the ID of the TPC channel connected to the specified wire
     * @param plane the number of plane
     * @param wire the number of wire
     * @param tpc the number of TPC
     * @param cryostat the number of cryostat
     * @param wireid the ID of the wire
     * @return the ID of the channel, or raw::InvalidChannelID if invalid wire
     *
     * @todo Verify the raw::InvalidChannelID part
     * @todo remove the integers version
     */
    raw::ChannelID_t  PlaneWireToChannel(WireID const& wireid) const;
    raw::ChannelID_t  PlaneWireToChannel(unsigned int const plane,
                                         unsigned int const wire,
                                         unsigned int const tpc = 0,
                                         unsigned int const cstat = 0) const
      { return PlaneWireToChannel(geo::WireID(cstat, tpc, plane, wire)); }
    //@}
    
    //
    // single object features
    //
    
    /**
     * @brief Returns the type of signal on the specified TPC channel
     * @param channel TPC channel ID
     * @return the type of signal on the specified channel, or geo::kMysteryType
     * 
     * @todo verify that kMysteryType is returned on invalid channel
     */
    SigType_t SignalType(raw::ChannelID_t const channel) const;
    
    
    /**
     * @brief Returns the view (wire orientation) on the specified TPC channel
     * @param channel TPC channel ID
     * @return the type of signal on the specified channel, or geo::kUnknown
     * 
     * @todo verify that kUnknown is returned on invalid channel
     * @todo what does this mean for APAs? is it at least well defined?
     */
    View_t View(raw::ChannelID_t const channel) const;
    
    
    /**
     * @brief Returns a list of wires connected to the specified TPC channel
     * @param channel TPC channel ID
     * @return vector containing the ID of all the connected wires
     */
    std::vector<geo::WireID> ChannelToWire
      (raw::ChannelID_t const channel) const;
    
    
    //
    // geometry queries
    //
    
    //@{
    /**
     * @brief Returns the ID of the channel nearest to the specified position
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param PlaneNo the number of plane
     * @param TPCNo the number of TPC
     * @param cstat the number of cryostat
     * @return the ID of the channel, or raw::InvalidChannelID if invalid wire
     *
     * The different versions allow different way to provide the position.
     *
     * @todo remove the integers version
     * @todo Verify the raw::InvalidChannelID part
     */
    raw::ChannelID_t  NearestChannel
      (const double worldLoc[3], geo::PlaneID const& planeid) const;
    raw::ChannelID_t  NearestChannel
      (std::vector<double> const& worldLoc, geo::PlaneID const& planeid) const;
    raw::ChannelID_t  NearestChannel
      (const TVector3& worldLoc, geo::PlaneID const& planeid) const;
    raw::ChannelID_t  NearestChannel(const double worldLoc[3],
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const
      { return NearestChannel(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    raw::ChannelID_t  NearestChannel(std::vector<double> const& worldLoc,
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const
      { return NearestChannel(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    raw::ChannelID_t  NearestChannel(const TVector3& worldLoc,
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const
      { return NearestChannel(worldLoc, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    /**
     * @brief Returns an intersection point of two channels
     * @param c1 one channel ID
     * @param c2 the other channel ID
     * @param y (output) y coordinate of the intersection
     * @param z (output) z coordinate of the intersection
     * @return whether a intersection point was found
     * 
     * @todo what happens for channels from different TPCs?
     * @todo what happens for channels with multiple intersection points?
     * 
     * @deprecated This is clearly not APA-aware
     */
    bool ChannelsIntersect
      (raw::ChannelID_t c1, raw::ChannelID_t c2, double &y, double &z);
    
    /// @} TPC readout channels
    
    
    
    /// @{
    /// @name Optical readout channels
    /// @todo add explanation of the different IDs
    
    //
    // group features
    //
    
    /// Number of electronics channels for all the optical detectors
    unsigned int NOpChannels() const;

    // Number of hardware channels for a given optical detector
    unsigned int NOpHardwareChannels(int opDet) const;
    
    
    //
    // access
    //
    
    /// Is this a valid OpChannel number?
    bool IsValidOpChannel(int opChannel) const;
    
    /// Convert detector number and hardware channel to unique channel
    unsigned int OpChannel(int detNum, int hardwareChannel) const;
    
    /// Convert unique channel to detector number
    unsigned int OpDetFromOpChannel(int opChannel) const;
    
    /// Convert unique channel to hardware channel
    unsigned int HardwareChannelFromOpChannel(int opChannel) const;
    
    /// Get unique opdet number from cryo and internal count
    unsigned int OpDetFromCryo(unsigned int o, unsigned int c) const;

    /// @} Optical readout channels
    
    
    //
    // unsorted methods
    //
    
    /**
     * @brief Returns whether a value is within the specified range
     * @param value the value to be tested
     * @param min the lower boundary
     * @param max the upper boundary
     * @return whether the value is within range
     * 
     * If min is larger than max, they are swapped.
     * A tolerance of 10^-6 (absolute) is used.
     * 
     * @todo Use wiggle instead of 10^-6
     * @todo resort source code for a bit of speed up
     */
    bool ValueInRange(double value, double min, double max) const;
    
    
    
    /// @{
    /// @name Geometry initialization
    
    /**
     * @brief Loads the geometry information from the specified files
     * @param gdmlfile path to file to be used for Geant4 simulation
     * @param rootfile path to file for internal geometry representation
     * @see ApplyChannelMap()
     *
     * Both paths must directly resolve to an available file, as no search
     * is performed for them.
     * 
     * The gdmlfile parameter does not have to necessarily be in GDML format,
     * as long as it's something supported by Geant4. This file is not used by
     * the geometry, but its path is provided on request by the simulation
     * modules (see LArSoft `LArG4` module).
     * The rootfile also does not need to be a ROOT file, but just anything
     * that TGeoManager::Import() supports. This file is parsed immediately
     * and the internal geometry representation is built out of it.
     * 
     * @note After calling this method, the detector geometry information can
     * be considered complete, but the geometry service provider is not fully
     * initialized yet, since it's still necessary to provide or update the
     * channel mapping.
     */
    void LoadGeometryFile(std::string gdmlfile, std::string rootfile);
    
    /**
     * @brief Initializes the geometry to work with this channel map
     * @param pChannelMap a pointer to the channel mapping algorithm to be used
     * @see LoadGeometryFile()
     * 
     * The specified channel mapping is used with this geometry.
     * The algorithm object is asked and allowed to make the necessary
     * modifications to the geometry description.
     * These modifications typically involve some resorting of the objects.
     * 
     * The ownership of the algorithm object is shared, usually with a calling
     * framework: we maintain it alive as long as we need it (and no other code
     * can delete it), and we delete it only if no other code is sharing the
     * ownership.
     * 
     * This method needs to be called after LoadGeometryFile() to complete the
     * geometry initialization.
     */
    void ApplyChannelMap(std::shared_ptr<geo::ChannelMapAlg> pChannelMap);
    /// @}
    
    
  protected:
    /// Sets the detector name
    void SetDetectorName(std::string new_name) { fDetectorName = new_name; }
    
    /// Sets the detector ID; this is legacy as detector ID should not be used
    void SetDetectorID(geo::DetId_t ID) { fDetId = ID; }
    
    // There are some issues that require detector-specific queries.
    /// This method returns an enumerated type that can be tested in those cases
    geo::DetId_t DetectorID() const { return fDetId; }
    
    /// Returns the object handling the channel map
    geo::ChannelMapAlg const* ChannelMap() const
      { return fChannelMapAlg.get(); }
    
    //@{
    /// Return the internal cryostat list
    CryostatList_t&       Cryostats()       { return fGeoData.cryostats; }
    CryostatList_t const& Cryostats() const { return fGeoData.cryostats; }
    //@}
    
    //@{
    /// Return the interfal auxiliary detectors list
    AuxDetList_t&       AuxDets()       { return fGeoData.auxDets; }
    AuxDetList_t const& AuxDets() const { return fGeoData.auxDets; }
    //@}
    
  private:
    
    void FindCryostat(std::vector<const TGeoNode*>& path, unsigned int depth);
    
    void MakeCryostat(std::vector<const TGeoNode*>& path, int depth);
    
    void FindAuxDet(std::vector<const TGeoNode*>& path, unsigned int depth);
    
    void MakeAuxDet(std::vector<const TGeoNode*>& path, int depth);
    
    /// Deletes the detector geometry structures
    void ClearGeometry();
    
    
    GeometryData_t fGeoData;        ///< The detector description data
    
    double         fSurfaceY;       ///< The point where air meets earth for this detector.
    std::string    fDetectorName;   ///< Name of the detector.
    std::string    fGDMLfile;       ///< path to geometry file used for Geant4 simulation
    std::string    fROOTfile;       ///< path to geometry file for geometry in GeometryCore
    double         fMinWireZDist;   ///< Minimum distance in Z from a point in which
                                    ///< to look for the closest wire
    geo::DetId_t   fDetId;          ///< Detector type (deprecated, legacy)
    double         fPositionWiggle; ///< accounting for rounding errors when testing positions
    std::shared_ptr<const geo::ChannelMapAlg>
                   fChannelMapAlg;  ///< Object containing the channel to wire mapping
  }; // class GeometryCore
  
} // namespace geo



//******************************************************************************
//***  template implementation
//***

//
// geo::details::cryostat_iterator_base<>
//
template <typename GEOID>
inline geo::details::cryostat_iterator_base<GEOID>::operator bool() const
  { return geometry() && geometry()->HasElement(localID()); }

template <typename GEOID>
inline auto geo::details::cryostat_iterator_base<GEOID>::get() const
  -> ElementPtr_t
  { return geometry()->GetElementPtr(localID()); }

template <typename GEOID>
inline void geo::details::cryostat_iterator_base<GEOID>::set_local_limits()
  { limit = geometry()->NSiblingElements(localID()); }

template <typename GEOID>
inline void geo::details::cryostat_iterator_base<GEOID>::set_begin()
  { geometry()->GetBeginID(ID()); }

template <typename GEOID>
inline void geo::details::cryostat_iterator_base<GEOID>::set_end()
  { geometry()->GetEndID(ID()); }

template <typename GEOID>
void geo::details::cryostat_iterator_base<GEOID>::next() {
  if (at_end()) return;
  if (++local_index() < limit) return;
  localID().isValid = false;
} // geo::cryostat_iterator_base<GEOID>::next()


//
// geo::details::TPC_iterator_base<>
//
template <typename GEOID>
inline geo::details::TPC_iterator_base<GEOID>::operator bool() const {
  return upper_iterator::geometry()
    && upper_iterator::geometry()->HasElement(localID());
} // geo::details::TPC_iterator_base<>::operator bool()


template <typename GEOID>
inline auto geo::details::TPC_iterator_base<GEOID>::get() const -> ElementPtr_t
  { return upper_iterator::geometry()->GetElementPtr(localID()); }

template <typename GEOID>
inline void geo::details::TPC_iterator_base<GEOID>::set_local_limits() {
  // limit is how many sibling TPCs there are
  limit = upper_iterator::geometry()->NSiblingElements(localID());
} // geo::details::TPC_iterator_base<GEOID>::set_local_limits()

template <typename GEOID>
inline void geo::details::TPC_iterator_base<GEOID>::next() {
  // if at end (checked in the inherited context), do nothing
  if (upper_iterator::at_end()) return;
  
  // if after incrementing we haven't reached the limit, we are done
  if (++local_index() < limit) return;
  
  // we reached the end of the current elements list, we need to escalate:
  // - go to the next parent; if that becomes invalid, too bad, but we go on
  upper_iterator::next();
  // - set the index to the first element of the new parent
  local_index() = 0;
  // - update how many elements there are
  //   (expect 0 if it is now at_end() -- and it does not even matter)
  set_local_limits();
} // geo::details::TPC_iterator_base<GEOID>::next()


//
// geo::details::plane_iterator_base<>
//
template <typename GEOID>
inline geo::details::plane_iterator_base<GEOID>::operator bool() const {
  return upper_iterator::geometry()
    && upper_iterator::geometry()->HasElement(localID());
} // geo::details::plane_iterator_base<>::operator bool()


template <typename GEOID>
inline auto geo::details::plane_iterator_base<GEOID>::get() const
  -> ElementPtr_t
  { return upper_iterator::geometry()->GetElementPtr(localID()); }

template <typename GEOID>
inline void geo::details::plane_iterator_base<GEOID>::set_local_limits() {
  // limit is how many sibling planes there are
  limit = upper_iterator::geometry()->NSiblingElements(localID());
} // geo::details::plane_iterator_base<GEOID>::set_local_limits()

template <typename GEOID>
inline void geo::details::plane_iterator_base<GEOID>::next() {
  // if at end (checked in the inherited context), do nothing
  if (upper_iterator::at_end()) return;
  
  // if after incrementing we haven't reached the limit, we are done
  if (++local_index() < limit) return;
  
  // we reached the end of the current elements list, we need to escalate:
  // - go to the next parent; if that becomes invalid, too bad, but we go on
  upper_iterator::next();
  // - set the index to the first element of the new parent
  local_index() = 0;
  // - update how many elements there are
  //   (expect 0 if it is now at_end() -- and it does not even matter)
  set_local_limits();
} // geo::details::plane_iterator_base<GEOID>::next()


//
// geo::details::wire_iterator_base<>
//
template <typename GEOID>
inline geo::details::wire_iterator_base<GEOID>::operator bool() const {
  return upper_iterator::geometry()
    && upper_iterator::geometry()->HasElement(localID());
} // geo::details::wire_iterator_base<>::operator bool()

template <typename GEOID>
inline auto geo::details::wire_iterator_base<GEOID>::get() const
  -> ElementPtr_t
  { return upper_iterator::geometry()->GetElementPtr(localID()); }

template <typename GEOID>
inline void geo::details::wire_iterator_base<GEOID>::set_local_limits() {
  // limit is how many sibling wires there are
  limit = upper_iterator::geometry()->NSiblingElements(localID());
} // geo::details::wire_iterator_base<>::set_local_limits()

template <typename GEOID>
inline void geo::details::wire_iterator_base<GEOID>::next() {
  // if at end (checked in the inherited context), do nothing
  if (upper_iterator::at_end()) return;
  
  // if after incrementing we haven't reached the limit, we are done
  if (++local_index() < limit) return;
  
  // we reached the end of the current elements list, we need to escalate:
  // - go to the next parent; if that becomes invalid, too bad, but we go on
  upper_iterator::next();
  // - set the index to the first element of the new parent
  local_index() = 0;
  // - update how many elements there are
  //   (expect 0 if it is now at_end() -- and it does not even matter)
  set_local_limits();
} // geo::details::wire_iterator_base<>::next()


//******************************************************************************

#endif // GEO_GEOMETRYCORE_H
