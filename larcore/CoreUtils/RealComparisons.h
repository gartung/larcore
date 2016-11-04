/**
 * @file   RealComparisons.h
 * @brief  Class for approximate comparisons
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   November 2, 2016
 * 
 * This is a header-only library.
 * 
 */
namespace lar {
  namespace util {
    
    /** ************************************************************************
     * @brief Provides simple real number checks
     * @tparam RealType type of value to operate on
     * 
     * This class provides some members to perform comparisons between real
     * numbers, allowing for some tolerance for rounding errors.
     * 
     * The tolerance parameter is fixed.
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     * lar::util::RealComparisons<float> check(1e-5);
     * 
     * double const sqrt2 = std::sqrt(2);
     * 
     * // it should print the message
     * if (check.equal(sqrt2, 1.4142)) {
     *   std::cout
     *     << "Square root of 2 is not even close to 1.4142." << std::endl;
     * }
     * 
     * // it should not print the message
     * if (check.equal(sqrt2, 1.414213)) {
     *   std::cout
     *     << "Square root of 2 is not even close to 1.414213." << std::endl;
     * }
     * 
     * // this should print the message
     * if (check.within(std::sqrt(2), 0., 1.41421)) {
     *   std::cout
     *     << "Square root of 2 is between 0 and 1.41421 (tops)." << std::endl;
     * }
     * 
     * // this will probably print the message
     * double const epsilon = 2.0 - (sqrt2 * sqrt2);
     * if (check.zero(epsilon)) {
     *   std::cout
     *     << "The square of the square root of 2 is roughly 2." << std::endl;
     * }
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     */
    template <typename RealType>
    struct RealComparisons {
      using Value_t = RealType; /// type of values being compered
      
      /// Constructor: specify the threshold
      constexpr RealComparisons(Value_t threshold): threshold(threshold) {}
      
      /// Returns whether the value is no farther from 0 than the threshold
      constexpr bool zero(Value_t value) const
        { return std::abs(value) <= threshold; }
      
      /// Returns whether the value is farther from 0 than the threshold
      constexpr bool nonZero(Value_t value) const
        { return !zero(value); }
      
      /// Returns whether a and b are no farther than the threshold
      constexpr bool equal(Value_t a, Value_t b) const
        { return zero(a - b); }
      
      /// Returns whether value is larger than zero beyond tolerance
      constexpr bool strictlyNegative(Value_t value) const
        { return value < -threshold; }
      
      /// Returns whether value is smaller than zero beyond tolerance
      constexpr bool strictlyPositive(Value_t value) const
        { return value > threshold; }
      
      /// Returns whether value is larger than or `equal()` to zero
      constexpr bool nonNegative(Value_t value) const
        { return value >= -threshold; }
      
      /// Returns whether value is smaller than or `equal()` to zero
      constexpr bool nonPositive(Value_t value) const
        { return value <= threshold; }
      
      /// Returns whether value is between the bounds (included)
      constexpr bool within(Value_t value, Value_t lower, Value_t upper) const
        { return nonNegative(value - lower) && nonPositive(value - upper); }
      
      /// Returns whether value is between bounds (included); bounds are sorted
      constexpr bool withinSorted
        (Value_t value, Value_t lower, Value_t upper) const
        {
          return (lower < upper)
            ? within(value, lower, upper)
            : within(value, upper, lower)
            ;
        } // sortedWithin
      
      Value_t threshold; /// Threshold to compare the values to
      
    }; // struct RealComparisons<>
    
    
  } // namespace util
} // namespace lar
