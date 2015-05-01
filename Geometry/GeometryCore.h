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


// ROOT class prototypes
class TGeoManager;
class TGeoNode;
class TGeoMaterial;


/// Namespace collecting geometry-related classes utilities
namespace geo {
  
  
  // Forward declarations within namespace.
  class CryostatGeo;
  class TPCGeo;
  class PlaneGeo;
  class WireGeo;
  class AuxDetGeo;
  class AuxDetSensitiveGeo;
  class OpDetGeo;
  class GeometryCore;
  
  
  namespace details {
    
    /// Base class for geometry iterators (note: this is not an interator)
    class geometry_iterator_base {
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
      
      /// Constructor: associates with the specified geometry and sets to begin
      geometry_iterator_base(geo::GeometryCore const* geom, BeginPos_t):
        pGeo(geom) {}
      
      /// Constructor: associates with the specified geometry and sets to end
      geometry_iterator_base(geo::GeometryCore const* geom, EndPos_t):
        pGeo(geom) {}
      
        protected:
      const GeometryCore* pGeo = nullptr; ///< pointer to the geometry
      
      /// Default constructor; do not use a default-constructed iterator as-is!
      geometry_iterator_base() {}
      
    }; // class geometry_iterator_base
  } // namespace details
  
  
  
  //
  // iterators
  //
  
  
  /**
   * @brief Forward iterator browsing all cryostats in the detector
   * 
   * Usage example:
   * @code
   * geo::GeometryCore::cryostat_iterator iCryostat;
   * while (iCryostat) {
   *   std::cout << "Cryo: " << iCryostat->Cryostat << std::endl;
   *   const geo::CryostatGeo* pCryo = iCryostat.get();
   *   // ...
   *   ++iCryostat;
   * } // while
   * @endcode
   */
  class cryostat_iterator:
    public std::forward_iterator_tag, public details::geometry_iterator_base
  {
      public:
    
    /// Default constructor; effect not defined: assign to it before using!
    cryostat_iterator() {}
    
    /// Constructor: points to begin
    cryostat_iterator(geo::GeometryCore const* geom):
      cryostat_iterator(geom, begin_pos) {}
    
    /// Constructor: points to the specified cryostat
    cryostat_iterator
      (geo::GeometryCore const* geom, CryostatID const& start_from):
      cryostat_iterator(geom, undefined_pos)
      { id = start_from; }
    
    /// Constructor: points to begin
    cryostat_iterator(geo::GeometryCore const* geom, BeginPos_t):
      cryostat_iterator(geom, undefined_pos)
      { set_begin(); }
    
    /// Constructor: points to end
    cryostat_iterator(geo::GeometryCore const* geom, EndPos_t):
      cryostat_iterator(geom, undefined_pos)
      { set_end(); }
    
    bool operator== (const cryostat_iterator& as) const
      { return id == as.id; }
    bool operator!= (const cryostat_iterator& as) const 
      { return id != as.id; }
    
    /// Returns a copy of the TPCID the iterator points to
    CryostatID const& operator* () const { return id; }
    
    /// Returns a copy of the TPCID the iterator points to
    CryostatID const* operator-> () const { return &id; }
    
    /// Prefix increment: returns this iterator pointing to the next cryostat
    cryostat_iterator& operator++ () { next(); return *this; }
    
    /// Prefix decrement: returns this iterator pointing to the previous cryostat
    cryostat_iterator& operator-- () { prev(); return *this; }
    
    /// Postfix increment: returns the current iterator, then increments it
    cryostat_iterator operator++ (int)
      { cryostat_iterator old(*this); next(); return old; }
    
    /// Postfix decrement: returns the current iterator, then decrements it
    cryostat_iterator operator-- (int)
      { cryostat_iterator old(*this); prev(); return old; }
    
    /// Returns whether the iterator is pointing to a valid cryostat
    operator bool() const { return id.isValid; }
    
    /// Returns a pointer to cryostat, or nullptr if the iterator is not valid
    CryostatGeo const* get() const;
    
      protected:
    CryostatID id;     ///< current cryostat
    CryostatID limits; ///< maxima of the indices
    
    cryostat_iterator(geo::GeometryCore const* geom, UndefinedPos_t):
      details::geometry_iterator_base(geom), id()
      { set_limits(); }
    
    /// Sets the iterator to the end position
    void set_begin() { id = CryostatID(0); id.isValid = (id != limits); }
    
    /// Sets the iterator to the end position
    void set_end() { id = limits; }
    
    /// Skips to the next cryostat
    void next();
    
    /// Skips to the previous cryostat
    void prev();
    
    void set_limits();
    
  }; // class cryostat_iterator
  
  
  
  /**
   * @brief Forward iterator browsing all TPCs in the detector
   * 
   * Usage example:
   * @code
   * geo::GeometryCore::TPC_iterator iTPC;
   * while (iTPC) {
   *   std::cout << "Cryo: " << iTPC->Cryostat << " TPC: " << iTPC->TPC
   *     << std::endl;
   *   const geo::TPCGeo* pTPC = iTPC.get();
   *   // ...
   *   ++iTPC;
   * } // while
   * @endcode
   */
  class TPC_iterator:
    public std::forward_iterator_tag, protected details::geometry_iterator_base
  {
      public:
    /// Default constructor: points to the first TPC
    TPC_iterator(geo::GeometryCore const* geom):
      details::geometry_iterator_base(geom)
      { set_limits_and_validity(); }
    
    /// Constructor: points to the specified TPC
    TPC_iterator(geo::GeometryCore const* geom, TPCID start_from):
      details::geometry_iterator_base(geom), tpcid(start_from)
      { set_limits_and_validity(); }
    
    bool operator== (const TPC_iterator& as) const
      { return tpcid == as.tpcid; }
    bool operator!= (const TPC_iterator& as) const
      { return tpcid != as.tpcid; }
    
    /// Returns a copy of the TPCID the iterator points to
    const TPCID& operator* () const { return tpcid; }
    
    /// Returns a constant pointer to the TPCID the iterator points to
    const TPCID* operator-> () const { return &tpcid; }
    
    /// Prefix increment: returns this iterator pointing to the next TPC
    TPC_iterator& operator++ ();
    
    /// Postfix increment: returns the current iterator, then increments it
    TPC_iterator operator++ (int)
      { TPC_iterator old(*this); this->operator++(); return old; }
    
    /// Returns whether the iterator is pointing to a valid TPC
    operator bool() const { return tpcid.isValid; }
    
    /// Skips to the next TPC
    TPC_iterator& next() { return this->operator++(); }
    
    /// Returns a pointer to the TPC, or nullptr if the iterator is not valid
    const TPCGeo* get() const;
    
    /// Returns a pointer to the cryostat the plane belongs to
    const CryostatGeo* getCryostat() const;
    
      protected:
    TPCID tpcid = { 0, 0 }; ///< current TPC
    TPCID limits = { 0, 0 }; ///< maxima of the indices
    
    void set_limits_and_validity();
    void new_cryostat();
  }; // class TPC_iterator


  /**
   * @brief Forward iterator browsing all planes in the detector
   * 
   * Usage example:
   * @code
   * geo::GeometryCore::plane_iterator iPlane;
   * while (iPlane) {
   *   std::cout << "Cryo: " << iPlane->Cryostat << " TPC: " << iPlane->TPC
   *     << " plane: " << iPlane->Plane << std::endl;
   *   const geo::PlaneGeo* Plane = iPlane.get();
   *   // ...
   *   ++iPlane;
   * } // while
   * @endcode
   */
  class plane_iterator:
    public std::forward_iterator_tag, protected details::geometry_iterator_base
  {
      public:
    /// Default constructor: points to the first plane
    plane_iterator(geo::GeometryCore const* geom):
      details::geometry_iterator_base(geom)
      { set_limits_and_validity(); }
    
    /// Constructor: points to the specified plane
    plane_iterator(geo::GeometryCore const* geom, PlaneID start_from):
      details::geometry_iterator_base(geom), planeid(start_from)
      { set_limits_and_validity(); }
    
    bool operator== (const plane_iterator& as) const
      { return planeid == as.planeid; }
    bool operator!= (const plane_iterator& as) const
      { return planeid != as.planeid; }
    
    /// Returns a copy of the PlaneID the iterator points to
    const PlaneID& operator* () const { return planeid; }
    
    /// Returns a constant pointer to the PlaneID the iterator points to
    const PlaneID* operator-> () const { return &planeid; }
    
    /// Prefix increment: returns this iterator pointing to the next plane
    plane_iterator& operator++ ();
    
    /// Postfix increment: returns the current iterator, then increments it
    plane_iterator operator++ (int)
      { plane_iterator old(*this); this->operator++(); return old; }
    
    /// Returns whether the iterator is pointing to a valid plane
    operator bool() const { return planeid.isValid; }
    
    /// Skips to the next plane
    plane_iterator& next() { return this->operator++(); }
    
    /// Returns a pointer to plane, or nullptr if the iterator is not valid
    const PlaneGeo* get() const;
    
    /// Returns a pointer to the TPC the plane belongs to
    const TPCGeo* getTPC() const;
    
    /// Returns a pointer to the cryostat the plane belongs to
    const CryostatGeo* getCryostat() const;
    
      protected:
    PlaneID planeid = { 0, 0, 0 }; ///< current plane
    PlaneID limits = { 0, 0, 0 }; ///< maxima of the indices
    
    void set_limits_and_validity();
    void new_cryostat();
    void new_tpc();
  }; // class plane_iterator
  
  
  /**
   * @brief Forward iterator browsing all wires in the detector
   * 
   * Usage example:
   * @code
   * geo::GeometryCore::wire_iterator iWire;
   * while (iWire) {
   *   std::cout << "Cryo: " << iWire->Cryostat << " TPC: " << iWire->TPC
   *     << " plane: " << iWire->Plane << " << " wire: " << iWire->Wire
   *     << std::endl;
        *   const geo::WireGeo* Wire = iWire.get();
   *   // ...
   *   ++iWire;
   * } // while
   * @endcode
   */
  class wire_iterator:
    public std::forward_iterator_tag, protected details::geometry_iterator_base
  {
      public:
    /// Default constructor: points to the first wire
    wire_iterator(geo::GeometryCore const* geom):
      details::geometry_iterator_base(geom)
      { set_limits_and_validity(); }
    
    /// Constructor: points to the specified wire
    wire_iterator(geo::GeometryCore const* geom, WireID start_from):
      details::geometry_iterator_base(geom), wireid(start_from)
      { set_limits_and_validity(); }
    
    bool operator== (const wire_iterator& as) const
      { return wireid == as.wireid; }
    bool operator!= (const wire_iterator& as) const
      { return wireid != as.wireid; }
    
    /// Returns a copy of the WireID the iterator points to
    const WireID& operator* () const { return wireid; }
    
    /// Returns a constant pointer to the WireID the iterator points to
    const WireID* operator-> () const { return &wireid ; }
    
    /// Prefix increment: returns this iterator pointing to the next wire
    wire_iterator& operator++ ();
    
    /// Postfix increment: returns the current iterator, then increments it
    wire_iterator operator++ (int)
      { wire_iterator old(*this); this->operator++(); return old; }
    
    /// Returns whether the iterator is pointing to a valid wire
    operator bool() const { return wireid.isValid; }
    
    /// Skips to the next wire
    wire_iterator& next() { return this->operator++(); }
    
    /// Returns a pointer to wire, or nullptr if the iterator is not valid
    const WireGeo* get() const;
    
    /// Returns a pointer to the plane the wire belongs to
    const PlaneGeo* getPlane() const;
    
    /// Returns a pointer to the TPC the wire belongs to
    const TPCGeo* getTPC() const;
    
    /// Returns a pointer to the cryostat the wire belongs to
    const CryostatGeo* getCryostat() const;
    
      protected:
    WireID wireid = { 0, 0, 0, 0 }; ///< current wire
    WireID limits = { 0, 0, 0, 0 }; ///< maxima of the indices
    
    void set_limits_and_validity();
    void new_cryostat();
    void new_tpc();
    void new_plane();
  }; // class wire_iterator
  
  
  
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
    
    
    // temporary (?): import iterators
    using cryostat_iterator = geo::cryostat_iterator;
    using TPC_iterator = geo::TPC_iterator;
    using plane_iterator = geo::plane_iterator;
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
    
    /// Returns the number of cryostats in the detector
    /// @todo Change return type to size_t
    unsigned int Ncryostats() const { return Cryostats().size(); }
    
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns the specified cryostat
     * @param cstat number of cryostat
     * @param cryoid cryostat ID
     * @return a constant reference to the specified cryostat
     * 
     * @todo Make the cryostat number mandatory (as CryostatID)
     * @todo what happens if it does not exist?
     */
    CryostatGeo const& Cryostat(unsigned int const cstat = 0) const;
    CryostatGeo const& GetElement(geo::CryostatID cryoid) const
      { return Cryostat(cryoid.Cryostat); }
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
    
    /**
     * @brief Returns the cryostat at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param cstat (output) number of cryostat
     * @return a constant reference to the CryostatGeo object of the cryostat
     * @throws cet::exception ("Geometry" category) if no cryostat matches
     * 
     * The tolerance used here is the one returned by DefaultWiggle().
     * 
     * @todo replace the output parameters with a cryostat ID
     */
    CryostatGeo const& PositionToCryostat
      (double const worldLoc[3], unsigned int &cstat) const;
    
    
    //
    // iterators
    //
    
    /// Returns an iterator pointing to the first cryostat
    cryostat_iterator begin_cryostat() const
      { return { this, cryostat_iterator::begin_pos }; }
    
    /// Returns an iterator pointing after the last cryostat
    cryostat_iterator end_cryostat() const
      { return { this, cryostat_iterator::end_pos }; }
    
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
    
    /// Returns the half width of the cryostat (x direction)
    double CryostatHalfWidth(unsigned int cstat = 0) const;
    
    /// Returns the height of the cryostat (y direction)
    double CryostatHalfHeight(unsigned int cstat = 0) const;
    
    /// Returns the length of the cryostat (z direction)
    double CryostatLength(unsigned int cstat = 0) const;
    
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
      (double* boundaries, unsigned int cstat = 0) const;
    
    
    //
    // object description
    //
    
    /**
     * @brief Return the name of LAr TPC volume
     * @param cstat index of the cryostat
     * @return the name of the specified TPC
     * 
     * This information is used in the event display.
     * 
     * @todo Use a cryostat ID instead
     * @todo What if it does not exist?
     * @todo remove constantness of return type (or make it a reference)
     */
    const std::string GetCryostatVolumeName(unsigned int const cstat = 0) const;
    
    
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
    unsigned int NTPC(unsigned int cstat = 0) const;
    
    
    //
    // access
    //
    
    ///@{
    /**
     * @brief Returns the specified TPC
     * @param tpcid ID of the tpc
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return a constant reference to the specified TPC
     * 
     * @todo remove the version with integers
     * @todo what happens if it does not exist?
     */
    TPCGeo const& TPC
      (unsigned int const tpc   = 0, unsigned int const cstat = 0) const;
    TPCGeo const& TPC(geo::TPCID const& tpcid) const
      { return TPC(tpcid.TPC, tpcid.Cryostat); }
    TPCGeo const& GetElement(geo::TPCID const& tpcid) const
      { return TPC(tpcid); }
    ///@}
    
    
    /**
     * @brief Returns the ID of the TPC at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the TPC ID, or an invalid one if no TPC is there
     */
    geo::TPCID FindTPCAtPosition(double const worldLoc[3]) const;
    
    
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
    
#if 0 // not ready yet
    
    //
    // iterators
    //
    
    /// Returns an iterator pointing to the first TPC
    TPC_iterator begin_TPC() const
      { return { this, TPC_iterator::begin_pos }; }
    
    /// Returns an iterator pointing after the last TPC
    TPC_iterator end_TPC() const
      { return { this, TPC_iterator::end_pos }; }
    
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
     *     } // for all TPCs
     *     
     */
    IteratorBox<
      TPC_iterator,
      &GeometryCore::begin_TPC, &GeometryCore::end_TPC
      >
    IterateCryostats() const { return { this }; }
    
#endif // 0
    
    //
    // single object features
    //
    
    /**
     * @brief Returns the half width of the specified TPC (x direction)
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
    double DetHalfWidth(unsigned int tpc = 0, unsigned int cstat = 0) const;
    
    /**
     * @brief Returns the half height of the specified TPC (y direction)
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
    double DetHalfHeight(unsigned int tpc = 0, unsigned int cstat = 0) const;
    
    /**
     * @brief Returns the length of the specified TPC (z direction)
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
    double DetLength(unsigned int tpc = 0, unsigned int cstat = 0) const;
    
    
    /**
     * @brief Returns the centre of side of the detector facing the beam
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
     * @todo Replace with a TPCID
     */
    const TVector3 GetTPCFrontFaceCenter
      (unsigned int tpc = 0, unsigned int cstat = 0) const;
    
    
    //
    // object description
    //
    
    /**
     * @brief Return the name of specified LAr TPC volume
     * @param tpc index of TPC in the cryostat
     * @param cstat index of the cryostat
     * @return the name of the specified TPC
     * 
     * This information is used by Geant4 simulation
     * 
     * @todo Use a TPCID instead
     * @todo What if it does not exist?
     */
    const std::string GetLArTPCVolumeName
      (unsigned int const tpc = 0, unsigned int const cstat = 0) const;
    
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
    unsigned int Nplanes(unsigned int tpc   = 0, unsigned int cstat = 0) const;
    
    
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
     * @brief Returns the specified plane
     * @param planeid ID of the plane
     * @param plane plane number within the TPC
     * @param tpc tpc number within the cryostat
     * @param cstat number of cryostat
     * @return a constant reference to the specified plane
     * 
     * @todo remove the version with integers
     * @todo what happens if it does not exist?
     */
    PlaneGeo const& Plane
      (unsigned int const p, unsigned int const tpc   = 0, unsigned int const cstat = 0)
      const;
    PlaneGeo const& Plane(const geo::PlaneID& pid) const
      { return Plane(pid.Plane, pid.TPC, pid.Cryostat); }
    PlaneGeo const& GetElement(const geo::PlaneID& pid) const
      { return Plane(pid); }
    //@}
    
    
    //
    // single object features
    //
    
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
    double PlanePitch(unsigned int p1 = 0,
                      unsigned int p2 = 1,
                      unsigned int tpc = 0,
                      unsigned int cstat = 0) const;
    
    /**
     * @brief Returns the view (wire orientation) on the channels of specified TPC plane
     * @param plane TPC plane ID
     * @return the type of signal on the specified plane, or geo::kUnknown
     * 
     * @todo verify that kUnknown is returned on invalid plane
     */
    View_t View(geo::PlaneID const pid) const;
    
    /**
     * @brief Returns the type of signal on the channels of specified TPC plane
     * @param plane TPC plane ID
     * @return the type of signal on the specified plane, or geo::kMysteryType
     * 
     * Assumes that all the channels on the plane have the same signal type.
     * 
     * @todo verify that kMysteryType is returned on invalid plane
     */
    SigType_t SignalType(geo::PlaneID const pid) const;
    
    
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
      (unsigned int p, unsigned int tpc   = 0, unsigned int cstat = 0) const;
    
    
    //
    // access
    //
    
    //@{
    /**
     * @brief Returns the specified wire
     * @param wireid ID of the wire
     * @return a constant reference to the specified wire
     * 
     * @todo what happens if it does not exist?
     * @todo rename this to Wire()
     */
    WireGeo const& WireIDToWireGeo(geo::WireID const& wireid) const;
    WireGeo const& GetElement(geo::WireID const& wireid) const
      { return WireIDToWireGeo(wireid); }
    //@}
    
    
    //
    // single object features
    //
    
    /**
     * @brief Returns the distance between two wires
     * @param w1 index of the first wire
     * @param w2 index of the second wire
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
    double WirePitch(unsigned int w1 = 0, 
                     unsigned int w2 = 1, 
                     unsigned int plane = 0,
                     unsigned int tpc = 0,
                     unsigned int cstat = 0) const;
    
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
    
    
    /**
     * @brief Returns the angle of the wires in the specified view from vertical
     * @param view the view
     * @param TPC the index of the TPC in the specified cryostat
     * @param Cryo the cryostat
     * @return the angle [radians]
     * 
     * The angle is defined as in WireGeo::ThetaZ().
     * 
     * This method assumes all wires in the view have the same angle (it queries
     * for the first).
     *
     * @todo Use a TPCID
     * @deprecated This does not feel APA-ready
     */
    double WireAngleToVertical(geo::View_t view, int TPC=0, int Cryo=0) const;
    
    
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
    
    /**
     * @brief Fills two arrays with the coordinates of the wire end points
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
    void WireEndPoints(
      unsigned int cstat, unsigned int tpc, unsigned int plane, unsigned int wire,
      double *xyzStart, double *xyzEnd
      ) const;
    
    
    //
    // closest wire
    //
    
    //@{
    /**
     * @brief Returns the ID of wire closest to position in the specified TPC
     * @param worldLoc 3D coordinates of the point (world reference frame)
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
    const geo::WireID   NearestWireID(const double worldLoc[3],
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const;
    const geo::WireID   NearestWireID(std::vector<double> worldLoc,
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const;
    const geo::WireID   NearestWireID(const TVector3& worldLoc,
                                      unsigned int const PlaneNo,
                                      unsigned int const TPCNo = 0,
                                      unsigned int const cstat = 0)  const;
    //@}
    
    //@{
    /**
     * @brief Returns the index of wire closest to position in the specified TPC
     * @param worldLoc 3D coordinates of the point (world reference frame)
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
    unsigned int       NearestWire(const double worldLoc[3],
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0)  const;
    unsigned int       NearestWire(std::vector<double> worldLoc,
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0) const;
    unsigned int       NearestWire(const TVector3& worldLoc,
                                   unsigned int const PlaneNo,
                                   unsigned int const TPCNo = 0,
                                   unsigned int const cstat = 0)  const;
    //@}
    
    
   /**
    * @brief Returns the index of the nearest wire to the specified position
    * @param YPos y coordinate on the wire plane
    * @param ZPos z coordinate on the wire plane
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
    double WireCoordinate(double YPos, double ZPos,
                          unsigned int PlaneNo,
                          unsigned int TPCNo,
                          unsigned int cstat) const;
    
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
    
    /**
     * @brief Returns the intersection point of two wires
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
                           double &z);
    
    /**
     * @brief Returns the intersection point of two wires
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
    void IntersectionPoint(unsigned int wire1,
                           unsigned int wire2,
                           unsigned int plane1,
                           unsigned int plane2,
                           unsigned int cstat,
                           unsigned int tpc,
                           double &y,
                           double &z);
    
    
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
    double ThirdPlaneSlope(unsigned int plane1, double slope1, 
                           unsigned int plane2, double slope2,
                           unsigned int tpc, unsigned int cstat);

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
    
    /**
     * @brief Returns the specified auxiliary detector
     * @param ad the auxiliary detector index
     * @return a constant reference to the specified auxiliary detector
     * 
     * @todo what happens if it does not exist?
     * @todo remove the default parameter?
     */
    AuxDetGeo const& AuxDet(unsigned int const ad = 0) const;
    
    /// Returns the full list of pointer to the auxiliary detectors
    std::vector<AuxDetGeo*> const& AuxDetGeoVec() const { return fAuxDets; }
    
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
    raw::ChannelID_t  PlaneWireToChannel(unsigned int const plane,
                                         unsigned int const wire,
                                         unsigned int const tpc = 0,
                                         unsigned int const cstat = 0) const;
    raw::ChannelID_t  PlaneWireToChannel(WireID const& wireid)  const;
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
     * @todo add a PlaneID version
     * @todo remove the integers version
     * @todo Verify the raw::InvalidChannelID part
     */
    raw::ChannelID_t  NearestChannel(const double worldLoc[3],
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const;
    raw::ChannelID_t  NearestChannel(std::vector<double> const worldLoc,
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const;
    raw::ChannelID_t  NearestChannel(const TVector3& worldLoc,
                                       unsigned int const PlaneNo,
                                       unsigned int const TPCNo = 0,
                                       unsigned int const cstat = 0) const;
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
    
    
      protected:
    /// Base class for geometry iterators
    class geometry_iteratorbase {
        public:
      
      /// Default constructor: points to the first cryostat
      geometry_iteratorbase(Geometry const* geom = nullptr)
        { init_geometry(geom); }
      
        protected:
      const Geometry* pGeo; ///< pointer to the geometry
      
      void init_geometry(Geometry const* geom);
    }; // geometry_iteratorbase
    
    
    /**
     * @brief Convenience object for range-for loops
     * @tparam GeoIter a geometry iterator
     * 
     * GeoIter class must support a default constructor setting the iterator
     * to a valid begin position, and a set_end() method setting it to
     * past-the-end.
     * For the iterator to be usable in range-for loops, it also needs to define
     * a operator++(), a operator*() and operator==().
     */
    template <class GeoIter>
    class GeoIteratorBox {
        public:
      using iterator_t = GeoIter;
      
      /// Constructor: specify which geometry to use
      GeoIteratorBox(Geometry const* geom): pGeo(geom) {}
      
      iterator_t cbegin() const { return iterator_t(pGeo); }
      iterator_t begin() const { return cbegin(); }
      
      iterator_t cend() { iterator_t e(pGeo); e.set_end(); return e; }
      iterator_t end() { return cend(); }
      
        protected:
      geo::Geometry const* pGeo;
    }; // class GeoIteratorBox<>
    
    
      public:
    
    /// @{
    /// @name Iteration through geometry elements
    
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

#endif // GEO_GEOMETRYCORE_H
