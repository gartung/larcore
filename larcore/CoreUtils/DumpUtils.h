/**
 * @file   DumpUtils.h
 * @brief  Utilities to dump objects into a stream.
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   February 28, 2017
 *
 * The library `DumpUtils.h` is currently header-only.
 *
 */

#ifndef LARCORE_COREUTILS_DUMPUTILS_H
#define LARCORE_COREUTILS_DUMPUTILS_H


namespace lar {
  
  
  /// Namespace for LArSoft dumping utilities.
  namespace dump {
    
    /**
     * @brief Manipulator managing the dump of the vector content into a stream.
     * @tparam Vector the type of vector to be dumped
     * 
     * The manipulator is constructed with the object that needs to be dumped.
     * This implementation requires a vector of at least 3D with methods `X()`,
     * `Y()` and `Z()`.
     * 
     * An overloaded insertion operator will call the `operator()` method of
     * the manipulator to perform the insertion.
     * 
     * This class can be specialised to manage different vector types. 
     * For example:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * #include <array>
     * 
     * namespace lar {
     *   namespace dump {
     *     
     *     // specialization for 3D std::array<>
     *     template <typename T>
     *     struct VectorDumper<std::array<T, 3U>> {
     *       
     *       using Vector_t = std::array<T, 3U>;
     *       
     *       Vector_t const& a;
     *       
     *       VectorDumper(Vector_t const& a): a(a) {}
     *       
     *       template <typename Stream>
     *       void operator() (Stream&& out) const
     *         { out << "{ " << a[0] << "; " << a[1] << "; " << a[2] << " }"; }
     *       
     *     }; // struct VectorDumper<array>
     *     
     *     
     *   } // namespace dump
     * } // namespace lar
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     */
    template <typename Vector>
    struct VectorDumper {
      using Vector_t = Vector;
      using This_t = VectorDumper<Vector>;
      
      Vector_t const& v; ///< A reference to the vector to be printed.
      
      VectorDumper(Vector_t const& v): v(v) {}
      
      // constructors ahead
      VectorDumper(This_t const& from) = default;
      VectorDumper(This_t&& from) = default;
      VectorDumper& operator=(This_t const& from) = delete;
      VectorDumper& operator=(This_t&& from) = delete;
      
      /// Inserts the content of the stored vector into the specified stream.
      template <typename Stream>
      void operator() (Stream&& out) const
        { out << "{ " << v.X() << "; " << v.Y() << "; " << v.Z() << " }"; }
      
    }; // struct VectorDumper<>
    
    
    
    /**
     * @brief Returns a manipulator which will print the specified vector.
     * @tparam Vector3D the type of 3D vector to be printed
     * @param v the vector to be printed
     * @return a manipulator
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * TVector3 pos;
     * std::cout << "Position: " << lar::dump::vector3D(pos) << std::endl;
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * will produce an output like:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Position: { 0; 0; 0 }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     * Addition of specific classes can be achieved via specialization of
     * the class `VectorDumper`.
     * 
     * 
     * Requirements
     * -------------
     * 
     * * the type `Vector3D` is required to provide constant accessor methods
     *   returning the vector components, called `X()`, `Y()` and `Z()`
     * * the values returned by the aforementioned `Vector3D` methods must have
     *   their insertion operator into a stream defined
     * * the vector `v` must not fall out of scope until the insertion into
     *   the stream happens
     * 
     */
    template <typename Vector3D>
    auto vector3D(Vector3D const& v) { return VectorDumper<Vector3D>(v); }
    
    
    /**
     * @brief Dumps the vector contained in the manipulator into a stream.
     * @tparam Stream the type of output stream
     * @tparam Vector3D the type of 3D vector to be printed
     * @param out the output stream
     * @param manip the manipulator containing the vector to be printed
     * @return a reference to the output stream
     * @see VectorDumper
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * TVector3 pos;
     * std::cout << "Position: " << lar::dump::vector3D(pos) << std::endl;
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * will produce an output like:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Position: { 0; 0; 0 }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     * 
     * Requirements
     * -------------
     * 
     * * the type `Vector3D` is required to provide constant accessor methods
     *   returning the vector components, called `X()`, `Y()` and `Z()`
     * * the values returned by the aforementioned `Vector3D` methods must have
     *   their insertion operator into Stream defined
     * 
     */
    template <typename Stream, typename Vector>
    Stream& operator<< (Stream&& out, VectorDumper<Vector>&& manip)
      { manip(std::forward<Stream>(out)); return out; }
    
  } // namespace dump
  
  
} // namespace lar


#endif // LARCORE_COREUTILS_DUMPUTILS_H
