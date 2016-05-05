/**
 * @file   UncopiableAndUnmoveableClass.h
 * @brief  Defines a class that can't be copied nor moved.
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * 
 * This library is currently a pure header.
 * 
 */

#ifndef LARCORE_COREUTILS_UNCOPIABLEANDUNMOVEABLECLASS_H
#define LARCORE_COREUTILS_UNCOPIABLEANDUNMOVEABLECLASS_H 1


namespace lar {
  
  /** **************************************************************************
   * @brief An empty class that can't be copied nor moved
   * 
   * A class derived from this one can still be copied (or moved)
   * with an explicit effort. For example, to enable copy construction:
   *     
   *     struct CopiableClass: protected UncopiableAndUnmoveableClass {
   *       CopiableClass(CopiableClass const& from)
   *         : UncopiableAndUnmoveableClass() // , ...
   *         {
   *           // ...
   *         }
   *     };
   *     
   * the default constructor of the base class can be called explicitly instead
   * of the copy constructor. To provide an assignment operation, 
   *     
   *     struct MoveAssignableClass: protected UncopiableAndUnmoveableClass {
   *       MoveAssignableClass& operator= (MoveAssignableClass&& from)
   *         {
   *           // ...
   *           return *this;
   *         }
   *     };
   *     
   * 
   */
  struct UncopiableAndUnmoveableClass {
    
    /// Default constructor
    UncopiableAndUnmoveableClass() = default;
    
    // @{
    /// Deleted copy and move constructors and assignments
    UncopiableAndUnmoveableClass(UncopiableAndUnmoveableClass const&) = delete;
    UncopiableAndUnmoveableClass(UncopiableAndUnmoveableClass&&) = delete;
    
    UncopiableAndUnmoveableClass& operator=
      (UncopiableAndUnmoveableClass const&) = delete;
    UncopiableAndUnmoveableClass& operator=
      (UncopiableAndUnmoveableClass&&) = delete;
    // @}
    
    /// Default destructor
    ~UncopiableAndUnmoveableClass() = default;
    
  }; // UncopiableAndUnmoveableClass
  
  
} // namespace lar

#endif // LARCORE_COREUTILS_UNCOPIABLEANDUNMOVEABLECLASS_H
