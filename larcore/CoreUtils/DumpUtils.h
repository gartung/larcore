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
    
    namespace details {
      
      /// Inserts `n` of elements of `a` in the specified stream.
      template <typename Stream, typename Array>
      void dumpArray(Stream&& out, Array&& a, size_t n) {
        out << "{";
        if (n == 0) { out << "}"; return; }
        out << " " << a[0];
        for (size_t i = 1; i < n; ++i) out << "; " << a[i];
        out << " }";
      } // dumpArray()
      
    } // namespace details
    
    /**
     * @brief Dumps the first N elements of an array.
     * @tparam Array the array type (supporting indexing operator)
     * @tparam NPrint the number of elements to be printed
     * 
     * The dump is producer by the call operator, in the form
     *     
     *     { a1, a2, ... aN }
     *     
     * 
     */
    template <typename Array>
    struct ArrayDumper {
      using Array_t = Array;
      using This_t = ArrayDumper<Array_t>;
      
      Array_t const& a; ///< A reference to the array to be printed.
      size_t n; ///< Number of elements to be printed.
      
      ArrayDumper(Array_t const& a, size_t n): a(a), n(n) {}
      
      // constructors ahead
      ArrayDumper(This_t const& from) = default;
      ArrayDumper(This_t&& from) = default;
      ArrayDumper& operator=(This_t const& from) = delete;
      ArrayDumper& operator=(This_t&& from) = delete;
      
      /// Inserts the content of the referenced array into the specified stream.
      template <typename Stream>
      void operator() (Stream&& out) const
        { details::dumpArray(std::forward<Stream>(out), a, n); }
      
    }; // struct ArrayDumper
    
    
    template <typename T>
    struct ArrayDumper<T*> {
      using Array_t = T*;
      using This_t = ArrayDumper<Array_t>;
      
      Array_t a; ///< A reference to the array to be printed.
      size_t n; ///< Number of elements to be printed.
      
      ArrayDumper(Array_t a, size_t n): a(a), n(n) {}
      
      /// Inserts the content of the referenced array into the specified stream.
      template <typename Stream>
      void operator() (Stream&& out) const
        { details::dumpArray(std::forward<Stream>(out), a, n); }
      
    }; // struct ArrayDumper<T*>
    
    
    /**
     * @brief Manipulator managing the dump of the vector content into a stream.
     * @tparam Vector the type of vector to be dumped
     * 
     * The manipulator is constructed with the object that needs to be dumped.
     * This implementation requires a vector of at least 3D with methods `X()`,
     * `Y()` and `Z()`.
     * The dump is producer by the call operator, in the form
     *     
     *     { vx, vy, vz }
     *     
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
      using This_t = VectorDumper<Vector_t>;
      
      Vector_t const& v; ///< A reference to the vector to be printed.
      
      explicit VectorDumper(Vector_t const& v): v(v) {}
      
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
    
    
    // Specialization for bare pointers.
    template <typename T>
    struct VectorDumper<T*>: public ArrayDumper<T const*> {
      explicit VectorDumper(T* v): ArrayDumper<T const*>(v, 3U) {}
    }; // VectorDumper<T*>
    
    // Specialization for C-style arrays.
    template <typename T>
    struct VectorDumper<T[3]>: public ArrayDumper<T const*> {
      explicit VectorDumper(T const* v): ArrayDumper<T const*>(v, 3U) {}
    }; // VectorDumper<T*>
    
    
    
    /**
     * @brief Returns a manipulator which will print the specified array.
     * @tparam NPrint the number of entries to be printed
     * @tparam Array the type of 3D vector to be printed
     * @param a the array to be printed
     * @return a manipulator
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * double const data[5] = { 1., 2., 3., 4., 6. };
     * std::cout << "Position: " << lar::dump::array<5>(data) << std::endl;
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * will produce an output like:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Position: { 1; 2; 3; 4; 6 }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     * Addition of specific classes can be achieved via specialization of
     * the class `ArrayDumper`.
     * 
     * 
     * Requirements
     * -------------
     * 
     * * the type `Array` is required to provide constant index operator
     *   returning the array element at the specified index
     * * the value type returned by the aforementioned `Array` method must have
     *   its insertion operator into a stream defined
     * * the array `a` must not fall out of scope until the insertion into
     *   the stream happens
     * 
     */
    template <size_t N, typename Array>
    auto array(Array const& a) { return ArrayDumper<Array>(a, N); }
    
    
    
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
     * @brief Dumps the array contained in the manipulator into a stream.
     * @tparam Stream the type of output stream
     * @tparam Array the type of array to be printed
     * @tparam NPrint the number of element of the array to be printed
     * @param out the output stream
     * @param manip the manipulator containing the array to be printed
     * @return a reference to the output stream
     * @see ArrayDumper
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * double const data[5] = { 1., 2., 3., 4., 6. };
     * std::cout << "Position: " << lar::dump::array<5>(data) << std::endl;
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * will produce an output like:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * Position: { 1, 2, 3, 4, 5 }
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
    template <typename Stream, typename Array>
    Stream& operator<< (Stream&& out, ArrayDumper<Array>&& manip)
      { manip(std::forward<Stream>(out)); return out; }
    
    
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
