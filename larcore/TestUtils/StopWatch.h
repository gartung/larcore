/**
 * @file   StopWatch.h
 * @brief  Class to take current wall clock time differences
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   July 3, 2016
 * 
 * This is a pure header library.
 * 
 * It provides its namesake class, `testing::StopWatch`.
 */

#ifndef LARCORE_TESTUTILS_STOPWATCH_H
#define LARCORE_TESTUTILS_STOPWATCH_H

// C/C++ standard libraries
#include <cstdint> // std::intmax_t
#include <chrono>
#include <ratio>
#include <type_traits> // std::true_type, std::false_type


namespace testing {
  namespace details {
    /// Type trait containing whether Duration is std::chrono::duration
    template <typename Duration>
    struct isDuration;
  } // namespace details


  /**
   * @brief Provides time interval measurements
   * @tparam DefaultUnit unit reported by default (seconds, floating point)
   * @tparam Clock type of clock object used (default: high_resolution_clock)
   * 
   * The stopwatch keeps track of the clock and can return the time elapsed from
   * a previous time mark.
   * Example of use:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * // do initialisation of task A
   * 
   * testing::StopWatch<> timer; // starts automatically
   * 
   * // execute task A
   * 
   * timer.stop();
   * 
   * // do initialisation of task B
   * 
   * timer.resume()
   * 
   * // execute task B
   * 
   * timer.stop()
   * 
   * std::cout << "Tasks A and B took " << timer.elapsed() << " seconds"
   *   << std::endl;
   * 
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * 
   * The time from all methods returning a value are in the DefaultUnit_t unit.
   * 
   * Requirements
   * -------------
   *
   * On `DefaultUnit` type:
   * 
   * * must be a std::chrono::duration specialisation
   * 
   * On `Clock` type:
   * 
   * * must have a `now()` static function returning `std::chrono::time_point`
   * 
   */
  template <
    typename DefaultUnit = std::chrono::duration<double>, // seconds (ratio<1>)
    typename Clock = std::chrono::high_resolution_clock
    >
  class StopWatch {
    static_assert(details::isDuration<DefaultUnit>::value,
     "DefaultUnit type is not a std::chrono::duration specialization");
    
      public:
    using Clock_t = Clock; ///< type of clock used to extract current time
    using DefaultUnit_t = DefaultUnit; ///< default unit for time report
    
    /// Type representing the reported time
    using ElapsedTime_t = typename DefaultUnit_t::rep;
    
    /**
     * @brief Initializes and starts the timer
     * @param start whether to start immediately (default: true)
     */
    StopWatch(bool start = true);
    
    /**
     * @brief Initializes and starts the timer
     * @param prev time already accumulated on start
     * @param start whether to start immediately (default: true)
     */
    template <typename Unit>
    StopWatch(Unit prev, bool start = true);
    
    
    /// @{
    /// @name Watch control
    
    /// Restarts the watch; previous time is forgotten
    void restart();
    
    /// Resumes the run of the watch; previous time is preserved
    void resume();
    
    /// Pauses the watch
    void stop();
    
    /// Changes the amount of time accumulated before this run
    template <typename Unit = DefaultUnit_t>
    void setPrevious(Unit dur);
    
    /// @}
    
    /// @{
    /// @name Query
    
    /// Returns the total time spent running since the last restart
    template <typename Unit = DefaultUnit_t>
    ElapsedTime_t elapsed() const;
    
    /// Returns the time spent running since the last resume
    template <typename Unit = DefaultUnit_t>
    ElapsedTime_t partial() const;
    
    /// Returns the time accumulated before the current run
    template <typename Unit = DefaultUnit_t>
    ElapsedTime_t previous() const;
    
    /// Returns whether the watch is tracking time right now
    bool running() const;
    
    /// @}
    
    
      protected:
    using TimePoint_t = decltype(Clock_t::now()); ///< type to store start time
    
    TimePoint_t lastStart; ///< time of the last start
    DefaultUnit_t previousTime; ///< time accumulated from previous runs
    bool isRunning; ///< whether we are measuring time now
    
    /// Returns the current time point from our clock
    static TimePoint_t now();
    
    /// Returns partial time as a duration
    DefaultUnit_t partialDur() const;
    
    
    /// Trait whose type member is a std::chrono::duration type
    template <typename>
    struct makeDurationTrait;
    
    /// Type of std::chrono::duration type constructed from makeDurationTrait
    template <typename Unit>
    using makeDuration_t = typename makeDurationTrait<Unit>::type;
    
    /// Convert a duration into a unit (may be a ratio or a duration)
    template <typename Unit, typename From>
    static auto durationTo(From const& dur);
    
  }; // class StopWatch
  
  
  /// A StopWatch with default template arguments
  using StandardStopWatch = StopWatch<>;
  
} // namespace testing


//------------------------------------------------------------------------------
namespace testing {
  namespace details {
    template <typename Duration>
    struct isDuration: public std::false_type {};
    
    template <typename Rep, typename Period>
    struct isDuration<std::chrono::duration<Rep, Period>>
      : public std::true_type
      {};
    
  } // namespace details
} // namespace testing


//------------------------------------------------------------------------------
//--- StopWatch implementartion
//---
//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
testing::StopWatch<DefaultUnit, Clock>::StopWatch(bool start /* = true */)
  : lastStart{start? now(): TimePoint_t{}}
  , previousTime{}
  , isRunning{start}
{}
   

template <typename DefaultUnit, typename Clock>
template <typename Unit>
testing::StopWatch<DefaultUnit, Clock>::StopWatch
  (Unit prev, bool start /* = true */)
  : StopWatch(start)
{ 
  previousTime = prev; 
}


//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
void testing::StopWatch<DefaultUnit, Clock>::restart() {
  lastStart = now();
  isRunning = true;
  previousTime = DefaultUnit_t();
} // testing::StopWatch<>::restart()
   

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
void testing::StopWatch<DefaultUnit, Clock>::resume() {
  if (running()) return;
  lastStart = now();
  isRunning = true;
} // testing::StopWatch<>::resume()
   

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
void testing::StopWatch<DefaultUnit, Clock>::stop() {
  previousTime += partialDur();
  isRunning = false;
} // testing::StopWatch<>::stop()
   

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
template <typename Unit>
void testing::StopWatch<DefaultUnit, Clock>::setPrevious(Unit dur) {
  previousTime = std::chrono::duration_cast<DefaultUnit_t>(dur);
} // testing::StopWatch<>::setPrevious()


//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
template <typename Unit>
typename testing::StopWatch<DefaultUnit, Clock>::ElapsedTime_t
testing::StopWatch<DefaultUnit, Clock>::elapsed() const {
  auto const prev = previous<Unit>();
  return running()? (prev + partial<Unit>()): prev;
} // testing::StopWatch<>::elapsed()


//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
template <typename Unit>
typename testing::StopWatch<DefaultUnit, Clock>::ElapsedTime_t
testing::StopWatch<DefaultUnit, Clock>::partial() const {
  return running()? durationTo<Unit>(partialDur()).count(): ElapsedTime_t(0);
} // testing::StopWatch<>::partial()


//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
template <typename Unit>
typename testing::StopWatch<DefaultUnit, Clock>::ElapsedTime_t
testing::StopWatch<DefaultUnit, Clock>::previous() const {
  return durationTo<Unit>(previousTime).count();
} // testing::StopWatch<>::previous()
   

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
bool testing::StopWatch<DefaultUnit, Clock>::running() const
  { return isRunning; }
   

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
typename testing::StopWatch<DefaultUnit, Clock>::TimePoint_t
testing::StopWatch<DefaultUnit, Clock>::now()
  { return Clock_t::now(); }


//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
typename testing::StopWatch<DefaultUnit, Clock>::DefaultUnit_t
testing::StopWatch<DefaultUnit, Clock>::partialDur() const {
  return std::chrono::duration_cast<DefaultUnit_t>(now() - lastStart);
} // testing::StopWatch<>::setPrevious()


//------------------------------------------------------------------------------
// Specialisation: on std::chrono::duration (type is that very same)
namespace testing {
  template <typename DefaultUnit, typename Clock>
  template <typename Rep, typename Duration>
  struct StopWatch<DefaultUnit, Clock>::makeDurationTrait
    <std::chrono::duration<Rep, Duration>>
  {
    using type = std::chrono::duration<Rep, Duration>;
  }; // StopWatch<>::makeDurationTrait<duration>

  // Specialisation: on std::ratio (type is a duration based on that ratio)
  template <typename DefaultUnit, typename Clock>
  template <std::intmax_t Num, std::intmax_t Den>
  struct StopWatch<DefaultUnit, Clock>::makeDurationTrait<std::ratio<Num, Den>>
  {
    using type = std::chrono::duration<
      typename StopWatch<DefaultUnit, Clock>::ElapsedTime_t,
      std::ratio<Num, Den>
      >;
  }; // struct makeDurationTrait<duration>
  
} // namespace testing

//------------------------------------------------------------------------------
template <typename DefaultUnit, typename Clock>
template <typename Unit, typename From>
auto testing::StopWatch<DefaultUnit, Clock>::durationTo(From const& dur)
   { return std::chrono::duration_cast<makeDuration_t<Unit>>(dur); }


//------------------------------------------------------------------------------


#endif // LARCORE_TESTUTILS_STOPWATCH_H
