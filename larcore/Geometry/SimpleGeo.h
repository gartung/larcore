/**
 * @file   SimpleGeo.h
 * @brief  Some simple functions to represent geometry entities
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * 
 * This library is simple and header-only.
 * 
 */

#ifndef LARCORE_GEOMETRY_SIMPLEGEO_H
#define LARCORE_GEOMETRY_SIMPLEGEO_H


// C/C++ standard library
#include <array>
#include <algorithm> // std::min(), std::max()
#include <stdexcept> // std::runtime_error


namespace lar {
  namespace util {

    namespace simple_geo {
      
      /// 2D point (x, y) (by default, with double precision)
      template <typename Data = double>
      struct Point2D {
        using Data_t = Data;
        Data_t x = 0.;
        Data_t y = 0.;
        
        Point2D() = default;
        Point2D(Data_t x, Data_t y): x(x), y(y) {}
        Point2D(Data_t const* p): x(p[0]), y(p[1]) {}
        
      }; // struct Point2D<>
      
      template <typename T>
      bool operator== (Point2D<T> const& a, Point2D<T> const& b)
        { return (a.x == b.x) && (a.y == b.y); }
      template <typename T>
      bool operator!= (Point2D<T> const& a, Point2D<T> const& b)
        { return (a.x != b.x) || (a.y != b.y); }
      template <typename T>
      Point2D<T> operator+ (Point2D<T> const& a, Point2D<T> const& b)
        { return { a.x + b.x, a.y + b.y }; }
      template <typename T>
      Point2D<T> operator* (Point2D<T> const& p, typename Point2D<T>::Data_t f)
        { return { p.x * f, p.y * f }; }
      template <typename T>
      Point2D<T> operator/ (Point2D<T> const& p, typename Point2D<T>::Data_t f)
        { return { p.x / f, p.y / f }; }
      template <typename Stream, typename T>
      Stream& operator<< (Stream&& out, Point2D<T> const& p)
        { out << "( " << p.x << " ; " << p.y << " )"; return out; }
      
      
      /// 3D point (x, y, z) (by default, with double precision)
      template <typename Data = double>
      struct Point3D {
        using Data_t = Data;
        Data_t x = 0.;
        Data_t y = 0.;
        Data_t z = 0.;
        
        Point3D() = default;
        Point3D(Data_t x, Data_t y, Data_t z): x(x), y(y), z(z) {}
        Point3D(Data_t const* p): x(p[0]), y(p[1]), z(p[2]) {}
      }; // struct Point3D<>
      
      template <typename T>
      bool operator== (Point3D<T> const& a, Point3D<T> const& b)
        { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }
      template <typename T>
      bool operator!= (Point3D<T> const& a, Point3D<T> const& b)
        { return (a.x != b.x) || (a.y != b.y) || (a.z != b.z); }
      template <typename T>
      Point3D<T> operator+ (Point3D<T> const& a, Point3D<T> const& b)
        { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
      template <typename T>
      Point3D<T> operator* (Point3D<T> const& p, typename Point2D<T>::Data_t f)
        { return { p.x * f, p.y * f, p.z * f }; }
      template <typename T>
      Point3D<T> operator/ (Point3D<T> const& p, typename Point2D<T>::Data_t f)
        { return { p.x / f, p.y / f, p.z / f }; }
      template <typename Stream, typename T>
      Stream& operator<< (Stream&& out, Point3D<T> const& p)
        {
          out << "( " << p.x << " ; " << p.y << " ; " << p.z << " )";
          return out;
        }
      
      
      /// Area/volume delimited by points: base/1D implementation
      template <typename Point>
      class AreaBase {
          public:
        using Point_t = Point;
        using Area_t = AreaBase<Point>;
        
        /// Exception thrown when result of intersection is null
        class NullIntersection: public std::runtime_error
          { using std::runtime_error::runtime_error; };
        
        
        AreaBase() = default;
        
        AreaBase(Point_t const& a, Point_t const& b)
          { set_sorted(min.x, max.x, a.x, b.x); }
        
        Point_t const& Min() const { return min; }
        Point_t const& Max() const { return max; }
        Point_t Center() const { return (min + max) * 0.5; }
        
        auto DeltaX() const { return Max().x - Min().x; }
        bool isEmptyX() const { return (DeltaX() == 0); }
        bool isNullX() const { return Max().x < Min().x; }
        bool isNull() const { return isNullX(); }
        unsigned int nonEmptyDims() const { return isEmptyX()? 0: 1; }
        bool isEmpty() const { return nonEmptyDims() == 0; }
        bool isLine() const { return nonEmptyDims() == 1; }
        
        auto thinnestSize() const { return DeltaX(); }
        
        /// Returns the index of the thinnest side (0 is x)
        unsigned int thinnestSide() const { return 0; }
        
        void IncludePoint(Point_t const& point)
          { set_min_max(min.x, max.x, point.x); }
        
        void Include(Area_t const& area)
          { IncludePoint(area.min); IncludePoint(area.max); }
        
        void Intersect(Area_t const& area)
          {
            set_max(min.x, area.min.x);
            set_min(max.x, area.max.x);
            if (isNullX()) throw NullIntersection("null x dimension");
          }
        
        bool operator== (Area_t const& as) const
          { return (min == as.min) && (max == as.max); }
        bool operator!= (Area_t const& as) const
          { return (min != as.min) || (max != as.max); }
        
          protected:
        using Data_t = typename Point_t::Data_t;
        
        Point_t min, max;
        
        static void set_min(Data_t& var, Data_t val)
          { if (val < var) var = val; }
        
        static void set_max(Data_t& var, Data_t val)
          { if (val > var) var = val; }
          
        static void set_min_max(Data_t& min_var, Data_t& max_var, Data_t val)
          { set_min(min_var, val); set_max(max_var, val); }
          
        static void set_sorted
          (Data_t& min_var, Data_t& max_var, Data_t a, Data_t b)
          {
            if (a > b) { min_var = b; max_var = a; }
            else       { min_var = a; max_var = b; }
          }
        
      }; // class AreaBase<>
      
      
      /// Area delimited by two points
      template <typename Point = Point2D<double>>
      class Area: public AreaBase<Point> {
        using Base_t = AreaBase<Point>;
        
          public:
        using Point_t = typename Base_t::Point_t;
        using Area_t = Area<Point_t>;
        
        Area() = default;
        
        Area(Point_t const& a, Point_t const& b)
          : Base_t(a, b)
          { Base_t::set_sorted(Base_t::min.y, Base_t::max.y, a.y, b.y); }
        
        auto DeltaY() const { return Base_t::Max().y - Base_t::Min().y; }
        bool isEmptyY() const { return DeltaY() == 0; }
        unsigned int nonEmptyDims() const
          { return Base_t::nonEmptyDims() + (isEmptyY()? 0: 1); }
        bool isNullY() const { return Base_t::Max().y < Base_t::Min().y; }
        bool isNull() const { return Base_t::isNull() || isNullY(); }
        bool isEmpty() const { return nonEmptyDims() == 0; }
        bool isLine() const { return nonEmptyDims() == 1; }
        bool isPlane() const { return nonEmptyDims() == 2; }
        
        auto thinnestSize() const
          { return std::min(DeltaY(), Base_t::thinnestSize()); }
        
        /// Returns the index of the thinnest side (0 is x, 1 is y)
        unsigned int thinnestSide() const
          {
            return
              (DeltaY() < Base_t::thinnestSize())? 1: Base_t::thinnestSide();
          }
        
        void IncludePoint(Point_t const& point)
          {
            Base_t::IncludePoint(point);
            Base_t::set_min_max(Base_t::min.y, Base_t::max.y, point.y);
          }
        
        void Include(Area_t const& area)
          { IncludePoint(area.min); IncludePoint(area.max); }
        
        void Intersect(Area_t const& area)
          {
            Base_t::Intersect(area);
            Base_t::set_max(Base_t::min.y, area.min.y);
            Base_t::set_min(Base_t::max.y, area.max.y);
            if (isNullY())
              throw typename Base_t::NullIntersection("null y dimension");
          }
        
      }; // class Area<>
      
      template <typename Stream, typename Point>
      Stream& operator<< (Stream&& out, AreaBase<Point> const& area)
        { out << area.Min() << " - " << area.Max(); return out; }
      
      
      /// Volume delimited by two points
      template <typename Point = Point3D<double>>
      class Volume: public Area<Point> {
        using Base_t = Area<Point>;
        
          public:
        using Point_t = typename Base_t::Point_t;
        using Volume_t = Volume<Point_t>;
        
        Volume() = default;
        
        Volume(Point_t const& a, Point_t const& b)
          : Base_t(a, b)
          { Base_t::set_sorted(Base_t::min.z, Base_t::max.z, a.z, b.z); }
        
        auto DeltaZ() const { return Base_t::Max().z - Base_t::Min().z; }
        bool isEmptyZ() const { return DeltaZ() == 0; }
        unsigned int nonEmptyDims() const
          { return Base_t::nonEmptyDims() + (isEmptyZ()? 0: 1); }
        bool isNullZ() const { return Base_t::Max().z < Base_t::Min().z; }
        bool isNull() const { return Base_t::isNull() || isNullZ(); }
        bool isEmpty() const { return nonEmptyDims() == 0; }
        bool isLine() const { return nonEmptyDims() == 1; }
        bool isPlane() const { return nonEmptyDims() == 2; }
        bool isVolume() const { return nonEmptyDims() == 3; }
        
        auto thinnestSize() const
          { return std::min(DeltaZ(), Base_t::thinnestSize()); }
        
        /// Returns the index of the thinnest side (0 is x, 1 is y)
        unsigned int thinnestSide() const
          {
            return
              (DeltaZ() < Base_t::thinnestSize())? 2: Base_t::thinnestSide();
          }
        
        void IncludePoint(Point_t const& point)
          {
            Base_t::IncludePoint(point);
            Base_t::set_min_max(Base_t::min.z, Base_t::max.z, point.z);
          }
        
        void Include(Volume_t const& volume)
          { IncludePoint(volume.min); IncludePoint(volume.max); }
        
        void Intersect(Volume_t const& volume)
          {
            Base_t::Intersect(volume);
            Base_t::set_max(Base_t::min.z, volume.min.z);
            Base_t::set_min(Base_t::max.z, volume.max.z);
            if (isNullZ())
              throw typename Base_t::NullIntersection("null z dimension");
          }
        
      }; // class Volume<>
      
      
      
    } // namespace simple_geo
  } // namespace util
} // namespace lar


#endif // LARCORE_GEOMETRY_SIMPLEGEO_H
