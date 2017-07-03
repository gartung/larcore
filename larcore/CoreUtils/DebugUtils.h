/**
 * @file   DebugUtils.h
 * @brief  Functions to help debugging by instrumenting code
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   April 8, 2016
 * @see    DebugUtils.cxx
 *
 * This library contains:
 *  - a function to return the name of the type of a variable
 *  - a function printing into a stream the current call stack
 *
 */
#ifndef LARCORE_COREUTILS_DEBUGUTILS_H
#define LARCORE_COREUTILS_DEBUGUTILS_H 1

// framework and support libraries
#include "cetlib_except/demangle.h"

// C/C++ standard libraries
#include <cstddef> // std::ptrdiff_t
#include <cstdlib> // std::free()
#include <utility> // std::pair<>
#include <vector>
#include <string>
#include <sstream>
#include <bitset>
#include <typeinfo>
#include <ostream>
#include <utility> // std::forward()

// non-standard libraries
#include <execinfo.h> // backtrace()...
// #include <experimental/filesystem> // std::experimental::filesystem::path


namespace lar {
  namespace debug {
    
    /** ***********************************************************************
     * @brief Outputs a demangled name for type T.
     * @param T type whose name must be demangled (optional)
     * @return a string with demangled name
     *
     * It relies on cetlib.
     * The type to be demangled can be specified either as template argument:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * auto name = lar::debug::demangle<std::string>();
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * or via a argument pointer:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * auto name = lar::debug::demangle(this);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * 
     */
    template <typename T>
    inline std::string demangle(T const* = nullptr)
      { return cet::demangle_symbol(typeid(std::decay_t<T>).name()); }
    
    
    /// Structure with information about a single call, parsed.
    struct CallInfo_t {
        private:
      using range_t = std::pair<size_t, size_t>;
        
        public:
      CallInfo_t(std::string const& s) { ParseString(s); }
      CallInfo_t(const char* s) { ParseString(std::string(s)); }
        
      /// Returns whether there is some information parsed.
      operator bool() const
         { return !libraryName.empty() || !mangledFunctionName.empty(); }
      /// Returns whether no information was parsed out of the original.
      bool operator! () const
         { return libraryName.empty() && mangledFunctionName.empty(); }
        
      /// Returns whether the translation was complete (offset is optional!).
      bool ParseString(std::string const& s);
       
      /// Returns the function name (mangled if nothing better).
      std::string const& function() const
        { return functionName.empty()? mangledFunctionName: functionName; }
      
      /// Returns only the library name (with suffix).
      std::string shortLibrary() const
        {
          size_t sep = libraryName.rfind('/');
          return (sep == std::string::npos)
            ? libraryName: libraryName.substr(sep + 1);
        //  return std::experimental::filesystem::path(libraryName).filename();
        }
        
      std::string original;            ///< String from the backtrace, unparsed.
      std::string libraryName;         ///< Parsed library name.
      std::string functionName;        ///< Parsed function name, demangled.
      std::string mangledFunctionName; ///< Parsed function name, unprocessed.
      void* address = nullptr;         ///< Function address.
      std::ptrdiff_t offset = 0;       ///< Instruction pointer offset.
        
         private:
  
      /// Returns whether the range is empty or invalid.
      static bool emptyRange(range_t const& r) { return r.first >= r.second; }
  
      /// Translates a range into a string.
      static std::string extract(std::string const& s, range_t const& r)
        { return emptyRange(r)? "": s.substr(r.first, r.second - r.first); }
      
      /// Runs the demangler and stores the result.
      void demangleFunction()
        { functionName = cet::demangle_symbol(mangledFunctionName); }
      
      /// Fills the information from an original string and parsed ranges.
      void setAll(
        std::string const& s,
        range_t addressStr, range_t libraryStr,
        range_t functionStr, range_t offsetStr
        );
       
    }; // CallInfo_t
     
    
    /**
     * @brief Class handling the output of information in a CallInfo_t object.
     *
     * This class has a "default" print function (also replicated as a call
     * operator), and a set of options that can be tweaked to change the amount
     * of information and format to be printed.
     *
     */
    class CallInfoPrinter {
        public:
      /// Set of options for printing.
      struct opt {
        /// List of available options.
        enum option_t {
          address,       ///< Print the instruction pointer memory address.
          demangled,     ///< Use demangled function names when possible.
          library,       ///< Print the library name the function lives in.
          shortLibrary,  ///< Print a shorter library name (base name only).
          offset,        ///< Print the offset from the beginning of function.
          NOptions       ///< Number of available options.
        }; // option_t
        
        std::bitset<NOptions> options; ///< Value of current options.
        
        /// Set one option `o` to the specified set value (true by default).
        opt& set(option_t o, bool set = true)
          { options.set(o, set); return *this; }
        
        /// Returns whether the specified option is set.
        bool has(option_t o) const { return options.test(o); }

      }; // opt
      
      opt options; ///< Set of current options.
      
      /// Default constructor: use default options.
      CallInfoPrinter() { setDefaultOptions(); }
      
      /// Constructor: use specified options.
      CallInfoPrinter(opt opts): options(opts) {}
      
      /// Override all the options.
      void setOptions(opt opts) { options = opts; }
      
      /// Print the content of info into the stream out, using the current options.
      template <typename Stream>
      void print(Stream&& out, CallInfo_t const& info) const {      
        if (!info) {
          out << info.original << " (?)";
          return;
        }
        
        if (info.mangledFunctionName.empty()) {
          if (options.has(opt::library)) {
            out << "in "
              << (options.has(opt::shortLibrary)? info.shortLibrary(): info.libraryName);
          }
          else out << "unknown";
        }
        else {
        //  auto flags = out.flags(); // not available in message facility streams
          out << info.function();
          auto offset = info.offset;
          if (offset && options.has(opt::offset)) {
            out << " [";
            if (offset > 0) out << '+';
            else {
              out << '-';
              offset = -offset;
            }
            out << std::showbase << std::hex << offset << "]";
          } // if has offset
        //  out << std::ios::setiosflags(flags);
          if (!info.libraryName.empty() && options.has(opt::library)) {
            out << " in "
              << (options.has(opt::shortLibrary)? info.shortLibrary(): info.libraryName);
          }
        }
        if (info.address && options.has(opt::address))
           out << " at " << ((void*) info.address);
        out << std::flush;
      } // print()
      
      /// Print the content of info into the stream out, using the current options
      template <typename Stream>
      void operator() (Stream&& out, CallInfo_t const& info) const
        { print(std::forward<Stream>(out), info); }     
      
      /// Sets this object to use a set of default options
      void setDefaultOptions() { options = defaultOptions(); }
      
      /// Returns a set of default options 
      static opt defaultOptions()
        {
          opt options;
          options.set(opt::demangled);
          options.set(opt::library);
          options.set(opt::shortLibrary);
          options.set(opt::address);
          return options;
        }
      
    }; // CallInfoPrinter
    
    /// Helper operator to insert a call information in a stream with default options.
    template <typename Stream>
    inline Stream& operator<< (Stream&& out, CallInfo_t const& info)
      {
        CallInfoPrinter print;
        print(std::forward<Stream>(out), info);
        return out;
      }
    
    /// Backtrace printing options
    struct BacktracePrintOptions {
      
      unsigned int maxLines = 5; ///< Total number of lines to print.
      unsigned int skipLines = 1; ///< Number of lines to skip.
      bool countOthers = true; ///< Whether to print number of omitted lines.
      std::string indent; ///< Indentation string for all lines.
      std::string firstIndent; ///< Special indentation for the first line.
      
      /// Options for each single backtrace call information line.
      CallInfoPrinter::opt callInfoOptions = CallInfoPrinter::defaultOptions();
      
      /// Sets all indentation to the same specified `uniformIndent` string.
      void setUniformIndent(std::string uniformIndent)
        { indent = firstIndent = uniformIndent; }
      
    }; // struct BacktracePrintOptions
    
    
    /**
     * @brief Prints the full backtrace into a stream.
     * @tparam Stream type of output stream
     * @param out the output stream to insert output into
     * @param options printing options (see BacktracePrintOptions)
     *
     */
    template <typename Stream>
    void printBacktrace(Stream&& out, BacktracePrintOptions options) {
      unsigned int nSkip = std::max(options.skipLines, 0U);
      std::vector<void*> buffer
        (nSkip + std::max(options.maxLines, 200U), nullptr);
      
      unsigned int const nItems
        = (unsigned int) backtrace(buffer.data(), buffer.size());
      
      // convert the calls in the buffer into a vector of strings
      char** symbols = backtrace_symbols(buffer.data(), buffer.size());
      if (!symbols) {
        out << options.firstIndent << "<failed to get the call stack>\n"
          << std::flush;
        return;
      }
      std::vector<CallInfo_t> callStack;
      for (size_t i = 0; i < buffer.size(); ++i)
        callStack.push_back((const char*) symbols[i]);
      std::free(symbols);
      
      size_t lastItem = nSkip + options.maxLines;
      if (lastItem > nItems) lastItem = nItems;
      if (lastItem >= buffer.size()) --lastItem;
      
      CallInfoPrinter print(options.callInfoOptions);
      for (size_t i = nSkip; i < lastItem; ++i) {
        out << (i == 0? options.firstIndent: options.indent);
        print(std::forward<Stream>(out), callStack[i]);
        out << "\n";
      }
      if ((lastItem < nItems) && options.countOthers) {
        out << options.indent << " ... and other " << (nItems - lastItem);
        if (nItems == buffer.size()) out << " (or more)";
        out << " levels\n";
      }
      out << std::flush;
      
    } // printBacktrace()
  
    /**
     * @brief Prints the full backtrace into a stream with default options.
     * @tparam Stream type of output stream
     * @param out the output stream to insert output into
     */
    template <typename Stream>
    void printBacktrace(Stream&& out)
      { printBacktrace(std::forward<Stream>(out), BacktracePrintOptions()); }
    
    /**
     * @brief Prints the full backtrace into a stream.
     * @tparam Stream type of output stream
     * @param out the output stream to insert output into
     * @param maxLines print at most this many lines in the output (default: 5)
     * @param indent prepend a string in front of any new line (default: "  ")
     * @param callInfoOptions use these output options (default ones if null)
     *
     * The call information output options are described in
     * `CallInfoPrinter::opt` structure.
     *
     */
    template <typename Stream>
    void printBacktrace(
      Stream&& out,
      unsigned int maxLines, std::string indent = "  ",
      CallInfoPrinter::opt const* callInfoOptions = nullptr
      )
    {
      BacktracePrintOptions options;
      options.maxLines = maxLines;
      options.indent = options.firstIndent = indent;
      if (callInfoOptions) options.callInfoOptions = *callInfoOptions;
      printBacktrace(std::forward<Stream>(out), options);
    }
    
    
  } // namespace debug
} // namespace lar


#endif // LARCORE_COREUTILS_DEBUGUTILS_H

