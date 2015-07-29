/**
 * @file   BoxBoundedGeo.h
 * @brief  Provides a base class aware of world box coordinates
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   April 9th, 2015
 */

#ifndef GEO_BOXBOUNDEDGEO_H
#define GEO_BOXBOUNDEDGEO_H

/// C/C++ standard library
#include <array>

/// ROOT library
#include "TVector3.h"


namespace geo {
  /**
   * @brief A base class aware of world box coordinates
   * 
   * An object describing a simple shape can inherit from this one to gain
   * access to a common boundary checking interface.
   * 
   * The boundary is a simple box with axes parallel to the coordinate system.
   */
  class BoxBoundedGeo {
      public:
    using Coord_t = double; ///< type of the coordinate
    using Coords_t = std::array<Coord_t, 3>; ///< type of the coordinate triplet
    
    /**
     * @brief Default constructor: sets an empty volume
     * @see SetBoundaries
     * 
     * We allow for a default construction since the derived object might need
     * some specific action before being aware of its boundaries.
     * In that case, SetBoundaries() will set the boundaries.
     */
    BoxBoundedGeo()
      { c_min.fill(0); c_max.fill(0); }
    
    //@{
    /**
     * @brief Constructor: sets the boundaries in world coordinates as specified
     * @param x_min lower x coordinate
     * @param x_max upper x coordinate
     * @param y_min lower y coordinate
     * @param y_max upper y coordinate
     * @param z_min lower z coordinate
     * @param z_max upper z coordinate
     * @param lower lower coordinates (x, y, z)
     * @param upper upper coordinates (x, y, z)
     * @see SetBoundaries
     * 
     * Note that is assumed that each minimum is larger than its maximum,
     * and no check is performed.
     */
    BoxBoundedGeo(
      Coord_t x_min, Coord_t x_max,
      Coord_t y_min, Coord_t y_max,
      Coord_t z_min, Coord_t z_max
      ):
      c_min{ x_min, y_min, z_min }, c_max{ x_max, y_max, z_max }
      {}
    BoxBoundedGeo(Coords_t lower, Coords_t upper):
      c_min(lower), c_max(upper)
      {}
    //@}
    
    
    /// @name Dimension queries
    /// @{
    /// Returns the world x coordinate of the start of the box
    double MinX() const { return c_min[0]; }
    
    /// Returns the world x coordinate of the end of the box
    double MaxX() const { return c_max[0]; }
    
    /// Returns the world x coordinate of the center of the box
    double CenterX() const { return (MinX() + MaxX()) / 2.; }
    
    /// Returns the world y coordinate of the start of the box
    double MinY() const { return c_min[1]; }
    
    /// Returns the world y coordinate of the end of the box
    double MaxY() const { return c_max[1]; }
    
    /// Returns the world y coordinate of the center of the box
    double CenterY() const { return (MinY() + MaxY()) / 2.; }
    
    /// Returns the world z coordinate of the start of the box
    double MinZ() const { return c_min[2]; }
    
    /// Returns the world z coordinate of the end of the box
    double MaxZ() const { return c_max[2]; }
    
    /// Returns the world z coordinate of the center of the box
    double CenterZ() const { return (MinZ() + MaxZ()) / 2.; }
    
    /// @}
    
    
    
    /// @name Containment in the full volume
    /// @{
    /**
     * @brief Returns whether this TPC contains the specified world x coordinate
     * @param x the absolute ("world") coordinate x
     * @param wiggle expansion factor for the range (see ContainsPosition())
     * @return whether the specified coordinate is in this TPC
     * @see ContainsPosition()
     * 
     * Note that x is by definition the drift direction, and a reconstructed x
     * typically depends on an assumption respect to the event time.
     */
    bool ContainsX(double x, double const wiggle = 1) const
      { return CoordinateContained(x, MinX(), MaxX(), wiggle); }
    
    /**
     * @brief Returns whether this TPC contains the specified world y coordinate
     * @param y the absolute ("world") coordinate y
     * @param wiggle expansion factor for the range (see ContainsPosition())
     * @return whether the specified coordinate is in this TPC
     * @see ContainsPosition()
     */
    bool ContainsY(double y, double const wiggle = 1) const
      { return CoordinateContained(y, MinY(), MaxY(), wiggle); }
    
    /**
     * @brief Returns whether this TPC contains the specified world z coordinate
     * @param z the absolute ("world") coordinate z
     * @param wiggle expansion factor for the range (see ContainsPosition())
     * @return whether the specified coordinate is in this TPC
     * @see ContainsPosition()
     */
    bool ContainsZ(double z, double const wiggle = 1) const
      { return CoordinateContained(z, MinZ(), MaxZ(), wiggle); }
    
    /**
     * @brief Returns if TPC contains the specified world y and z coordinates
     * @param y the absolute ("world") coordinate y
     * @param z the absolute ("world") coordinate z
     * @param wiggle expansion factor for the range (see ContainsPosition())
     * @return whether the specified coordinate is in this TPC
     * @see ContainsPosition()
     */
    bool ContainsYZ(double y, double z, double const wiggle = 1) const
      { return ContainsY(y, wiggle) && ContainsZ(z, wiggle); }
    
    /**
     * @brief Returns whether this TPC contains the specified world coordinate
     * @param worldLoc the absolute ("world") coordinate (x, y, z)
     * @param wiggle expansion factor for the range
     * @return whether the specified coordinate is in this TPC
     *
     * If the wiggle is larger than 1, each size of the TPC is expanded by the
     * wiggle factor.
     * If the wiggle is less than 1, each size is shrinked.
     */
    bool ContainsPosition
      (double const worldLoc[3], double const wiggle = 1) const
      {
        return ContainsX(worldLoc[0], wiggle)
          && ContainsYZ(worldLoc[1], worldLoc[2], wiggle);
      } // ContainsPosition()
    ///@}
    
    
    /// @name Containment in a fiducial volume
    /// @{
    /**
     * @brief Returns whether TPC fiducial volume contains world x coordinate
     * @param x the absolute ("world") coordinate x
     * @param neg_margin how far within the TPC the fiducial region starts
     * @param pos_margin how far before the TPC the fiducial region ends
     * @return whether the specified coordinate is in this TPC fiducial volume
     * 
     * The fiducial volume is defined by the specified margins, that denote how
     * much of the coordinate is not fiducial. For example,
     *     
     *     bool bWithin = tpc->InFiducialX(x, 5., 8.);
     *     
     * on a TPC with x from 0 to 100 (cm, presumably) will return true for all
     * `x` between 5 and 92.
     * If positive margin is not specified, it's assumed to be the same as the
     * negative one.
     * Specifying a negative mergin effectively extends the TPC volume.
     * Note that x is by definition the drift direction, and a reconstructed x
     * typically depends on an assumption respect to the event time.
     */
    bool InFiducialX(double x, double neg_margin, double pos_margin) const
      {
        return CoordinateContained(x, MinX() + neg_margin, MaxX() - pos_margin);
      }
    bool InFiducialX(double x, double margin) const
      { return InFiducialX(x, margin, margin); }
    
    /**
     * @brief Returns whether TPC fiducial volume contains world y coordinate
     * @param y the absolute ("world") coordinate y
     * @param neg_margin how far within the TPC the fiducial region starts
     * @param pos_margin how far before the TPC the fiducial region ends
     * @return whether the specified coordinate is in this TPC fiducial volume
     * 
     * The fiducial volume is defined by the specified margins, that denote how
     * much of the coordinate is not fiducial. For example,
     *     
     *     bool bWithin = tpc->InFiducialY(y, 5., 8.);
     *     
     * on a TPC with y from 0 to 100 (cm, presumably) will return true for all
     * `y` between 5 and 92.
     * If positive margin is not specified, it's assumed to be the same as the
     * negative one.
     * Specifying a negative mergin effectively extends the TPC volume.
     */
    bool InFiducialY(double y, double neg_margin, double pos_margin) const
      {
        return CoordinateContained(y, MinY() + neg_margin, MaxY() - pos_margin);
      }
    bool InFiducialY(double y, double margin) const
      { return InFiducialY(y, margin, margin); }
    
    /**
     * @brief Returns whether TPC fiducial volume contains world z coordinate
     * @param z the absolute ("world") coordinate y
     * @param neg_margin how far within the TPC the fiducial region starts
     * @param pos_margin how far before the TPC the fiducial region ends
     * @return whether the specified coordinate is in this TPC fiducial volume
     * 
     * The fiducial volume is defined by the specified margins, that denote how
     * much of the coordinate is not fiducial. For example,
     *     
     *     bool bWithin = tpc->InFiducialZ(z, 5., 8.);
     *     
     * on a TPC with z from 0 to 100 (cm, presumably) will return true for all
     * `z` between 5 and 92.
     * If positive margin is not specified, it's assumed to be the same as the
     * negative one.
     * Specifying a negative mergin effectively extends the TPC volume.
     */
    bool InFiducialZ(double z, double neg_margin, double pos_margin) const
      {
        return CoordinateContained(z, MinZ() + neg_margin, MaxZ() - pos_margin);
      }
    bool InFiducialZ(double z, double margin) const
      { return InFiducialZ(z, margin, margin); }
    
    /// @}
    
    
    /**
     * @brief Returns whether the specified coordinate is in a range
     * @param c the coordinate
     * @param min lower boundary of the range
     * @param max upper boundary of the range
     * @param wiggle expansion factor for the range
     * @return whether the specified coordinate is in a range
     *
     * If the wiggle is larger than 1, the range is expanded by the wiggle factor.
     * If the wiggle is less than 1, the range is shrinked.
     */
    static bool CoordinateContained
      (double c, double min, double max, double wiggle = 1.)
      {
        return (c >= (min > 0? min / wiggle: min * wiggle))
          && (c <= (max < 0? max / wiggle: max * wiggle));
      } // CoordinateContained()
    
    static bool CoordinateContained
      (double c, double const* range, double wiggle = 1.)
      { return CoordinateContained(c, range[0], range[1], wiggle); }
    
    
      protected:
    
    //@{
    /**
     * @brief Sets the boundaries in world coordinates as specified.
     * @param x_min lower x coordinate
     * @param x_max upper x coordinate
     * @param y_min lower y coordinate
     * @param y_max upper y coordinate
     * @param z_min lower z coordinate
     * @param z_max upper z coordinate
     * @param lower lower coordinates (x, y, z)
     * @param upper upper coordinates (x, y, z)
     * 
     * This class is protected, since we don't want users to mess up with the
     * boundaries.
     * This may become a limit if an object wants to contain a BoxBoundedGeo
     * (as opposed to derive from it) and needs to defer the construction.
     * In that case, a pointer needs to be used.
     */
    void SetBoundaries(
      Coord_t x_min, Coord_t x_max,
      Coord_t y_min, Coord_t y_max,
      Coord_t z_min, Coord_t z_max
      )
      {
        c_min = { x_min, y_min, z_min };
        c_max = { x_max, y_max, z_max };
      }
    void SetBoundaries(Coords_t lower, Coords_t upper)
      { c_min = lower; c_max = upper; }
    //@}
    
    
      private:
    // we don't alow the derived classes to mess with the boundaries
    Coords_t c_min; ///< minimum coordinates (x, y, z)
    Coords_t c_max; ///< maximum coordinates (x, y, z)
    
      public:
    //@{
    /**
     * @brief Calculates of a entry point on the box surface
     * @author Christoph Rudolf von Rohr (crohr@fnal.gov)
     * @date July 27th, 2015
     * @param TrackOffset position of the track source
     * @param TrackDirect direction vector of the track
     * @param TrackAngles is a doublet containing the angles theta and phi of the track
     *
     * This member is public since it just gives an output and does not change any member.
     * The algorithm works only for a box shaped active volume with parallel facing walls.
     */
    TVector3 GetEntryPoint(TVector3 TrackOffset, TVector3 TrackDirect);
//     TVector3 GetEntryPoint(TVector3 TrackOffset, std::array<Coord_t> TrackAngles);
    //@}
  }; // class BoxBoundedGeo
  
} // namespace geo



#endif // GEO_BOXBOUNDEDGEO
