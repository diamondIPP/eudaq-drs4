#ifndef EUDAQ_INCLUDED_Utils
#define EUDAQ_INCLUDED_Utils

/**
 * \file Utils.hh
 * Contains generally useful utility functions.
 */

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <fstream>
#include <sys/types.h>
#include "eudaq/Platform.hh"
#include <map>
#include <sys/ioctl.h>
#include <numeric>
#include <cmath>
#include <deque>

#if ((defined WIN32) && (defined __CINT__))
typedef unsigned long long uint64_t
typedef long long int64_t
typedef unsigned int uint32_t
typedef int int32_t
#else
#include <cstdint>
#endif

namespace eudaq {
  void DLLEXPORT PressEnterToContinue();
  std::string DLLEXPORT ucase(const std::string &);
  std::string DLLEXPORT lcase(const std::string &);
//  std::string DLLEXPORT trim(const std::string & s);
  std::string DLLEXPORT trim(const std::string & s, std::string trim_characters="\t\n\r\v ");
  std::string DLLEXPORT firstline(const std::string & s);
  std::string DLLEXPORT escape(const std::string &);
  std::vector<std::string> DLLEXPORT split(const std::string & str, const std::string & delim = "\t");
  std::vector<std::string> DLLEXPORT split(const std::string & str, const std::string & delim, bool dotrim);

  void DLLEXPORT bool2uchar(const bool* inBegin, const bool* inEnd, std::vector<unsigned char>& out);
  void DLLEXPORT uchar2bool(const unsigned char* inBegin, const unsigned char* inEnd, std::vector<bool>& out);
  void DLLEXPORT print_banner(std::string message, char seperator = '=', uint16_t max_lenght = 100);
  inline float DLLEXPORT elapsed_time(clock_t t) { return float(std::clock() - t) / CLOCKS_PER_SEC; }

  /** Sleep for a specified number of milliseconds.
   * \param ms The number of milliseconds
   */
  void DLLEXPORT mSleep(unsigned ms);

  /** Converts any type to a string.
   * There must be a compatible streamer defined, which this function will make use of.
   * \param x The value to be converted.
   * \return A string representing the passed in parameter.
   */
  template <typename T>
    inline std::string to_string(const T & x, int digits = 0, uint8_t precision = 10) {
      std::ostringstream s;
      s << std::setfill('0') << std::setw(digits) << std::setprecision(precision) << x;
      return s.str();
    }

    inline std::string to_string(signed char & x) {
      return std::to_string(x * 1);
    }

    inline std::string to_string(std::vector<signed char> & x) {
        std::ostringstream os;
        for (auto i: x)
            os << "," << to_string(i);
        return os.str();
    }

  template <typename T, typename Q>
      inline std::string to_string(const std::pair<T,Q> & x) {
        std::ostringstream s;
        s << "["<<to_string(x.first)<<","<<to_string(x.second)<<"]";
        return s.str();
      }

  template <typename T>
    inline std::string to_string(const std::vector<T> & x, const std::string & sep, int digits = 0, uint8_t precision = 10) {
      std::ostringstream s;
      if (x.size() > 0) s << to_string(x[0], digits, precision);
      for (size_t i = 1; i < x.size(); ++i) {
        s << sep << to_string(x[i], digits, precision);
      }
      return s.str();
    }

  template <typename T>
    inline std::string to_string(const std::vector<T> & x, int digits = 0, uint8_t precision = 10) {
      return to_string(x, ",", digits, precision);
    }

  template <typename T, typename Q>
    inline std::string to_string(const std::map<Q, std::vector<T> > & x, const std::string & sep1 = "; ", const std::string & sep2 = "/ ", int digits = 0) {
        if (sep1 == ", " || sep2 == ", " || sep1 == sep2) throw "wrong separator";
        std::ostringstream os;
        if (x.size()) os << to_string(x.begin()->first) << sep2 << to_string(x.at(0), digits);
        typename std::map<Q, std::vector<T> >::const_iterator it = x.begin();
        it++;
        for (it; it != x.end(); it++) os << sep1 << to_string(it->first, digits) << sep2 << to_string(it->second, digits);
        return os.str();
    }

  inline std::string to_string(const std::string & x, int /*digits*/ = 0) {
    return x;
  }
  inline std::string to_string(const char * x, int /*digits*/ = 0) {
    return x;
  }


  /** Converts any type that has an ostream streamer to a string in hexadecimal.
   * \param x The value to be converted.
   * \param digits The minimum number of digits, shorter numbers are padded with zeroes.
   * \return A string representing the passed in parameter in hex.
   */
  template <typename T>
    inline std::string to_hex(const T & x, int digits = 0) {
      std::ostringstream s;
      s << std::hex << std::setfill('0') << std::setw(digits) << x;
      return s.str();
    }

  template<>
    inline std::string to_hex(const unsigned char & x, int digits) {
      return to_hex((int)x, digits);
    }

  template<>
    inline std::string to_hex(const signed char & x, int digits) {
      return to_hex((int)x, digits);
    }

  template<>
    inline std::string to_hex(const char & x, int digits) {
      return to_hex((unsigned char)x, digits);
    }

  /** Converts a string to any type.
   * \param x The string to be converted.
   * \param def The default value to be used in case of an invalid string,
   *            this can also be useful to select the correct template type
   *            without having to specify it explicitly.
   * \return An object of type T with the value represented in x, or if
   *         that is not valid then the value of def.
   */
  template <typename T>
    inline T DLLEXPORT from_string(const std::string & x, const T & def = 0) {
      if (x == "") return def;
      T ret = def;
      std::istringstream s(x);
      s >> ret;
      char remain = '\0';
      s >> remain;
      if (remain) throw std::invalid_argument("Invalid argument: " + x);
      return ret;
    }

  template<>
    inline std::string DLLEXPORT from_string(const std::string & x, const std::string & def) {
      return x == "" ? def : x;
    }

  template<>
    inline signed char DLLEXPORT from_string(const std::string & x, const signed char & def) {
        return x == "" ? def : static_cast<signed char>(atoi(x.c_str()));
    }

  template<>
    int64_t DLLEXPORT from_string(const std::string & x, const int64_t & def);
  template<>
    uint64_t DLLEXPORT from_string(const std::string & x, const uint64_t & def);
  template<>
    inline int32_t DLLEXPORT from_string(const std::string & x, const int32_t & def) {
      return static_cast<int32_t>(from_string(x, (int64_t)def));
    }
  template<>
    inline uint32_t from_string(const std::string & x, const uint32_t & def) {
      return static_cast<uint32_t>(from_string(x, (uint64_t)def));
    }

  template<typename P,typename Q>
  std::pair<P,Q> DLLEXPORT from_string(const  std::string & x, const std::pair<P,Q>  & def) {
      std::string trimmed_string = trim(x," \t\n\r\v");
      trimmed_string = trim(trimmed_string,"{}[]()<>");
      std::vector<std::string> splitted_string = split(trimmed_string,",");
      P first = from_string(splitted_string.at(0),(P)def.first);
      Q second = from_string(splitted_string.at(1),(Q)def.second);
      return std::make_pair((Q)first,(P)second);
  }

  template<typename P>
  std::vector<P> DLLEXPORT from_string(const  std::string & x, const std::vector<P> & def) {
      std::string trimmed_string = trim(x," \t\n\r\v");
      trimmed_string = trim(trimmed_string,"{}[]()<>");
      std::vector<std::string> split_string = split(trimmed_string,",");
      std::vector<P> vec;
      if (split_string.size() > def.size())
          return def;
      for (auto i: split_string) {
          vec.push_back(from_string(i, def.at(0)));
      }
//      std::cout<<"Converted \""<<x<<"\" to "<<to_string(vec)<<std::endl;
      return vec;
  }

  template<typename T, typename Q>
  std::map<Q, std::vector<T> > DLLEXPORT from_string(const std::string & x, const std::map<Q, std::vector<T> > & def) {
      std::string trimmed_string = trim(x, " \t\n\r\v");
      trimmed_string = trim(trimmed_string, "{}[]()<>");
      std::vector<std::string> split_string1 = split(trimmed_string, ";");
      std::map<Q, std::vector<T> > ret_val;
      for (uint8_t i = 0; i < split_string1.size(); i++) {
          std::vector<std::string> split_string2 = split(split_string1.at(i), "/");
          ret_val[from_string(split_string2.at(0), def.begin()->first)] = from_string(split_string2.at(1), def.begin()->second);
      }
      return ret_val;
  }

  template <typename T>
    struct Holder {
      Holder(T val) : m_val(val) {}
      T m_val;
    };

  template <typename T>
    struct hexdec_t {
      enum { DIGITS = 2 * sizeof (T) };
      hexdec_t(T val, unsigned hexdigits) : m_val(val), m_dig(hexdigits) {}
      T m_val;
      unsigned m_dig;
    };

  template <typename T>
    inline hexdec_t<T> hexdec(T val, unsigned hexdigits = hexdec_t<T>::DIGITS) {
      return hexdec_t<T>(val, hexdigits);
    }

  template <typename T>
    inline std::ostream & operator << (std::ostream & os, const hexdec_t<T> & h) {
      return os << "0x" << to_hex(h.m_val, h.m_dig) << " (" << h.m_val << ")";
    }

  template <>
    inline std::ostream & operator << (std::ostream & os, const hexdec_t<unsigned char> & h) {
      return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
    }

  template <>
    inline std::ostream & operator << (std::ostream & os, const hexdec_t<signed char> & h) {
      return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
    }

  template <>
    inline std::ostream & operator << (std::ostream & os, const hexdec_t<char> & h) {
      return os << (int)(unsigned char)h.m_val
        << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
    }

  template <typename T> unsigned char * uchar_cast(T * x) {
    return reinterpret_cast<unsigned char *>(x);
  }

  template <typename T> unsigned char * uchar_cast(std::vector<T> & x) {
    return uchar_cast(&x[0]);
  }

  template <typename T> const unsigned char * constuchar_cast(const T * x) {
    return reinterpret_cast<const unsigned char *>(x);
  }

  template <typename T> const unsigned char * constuchar_cast(const std::vector<T> & x) {
    return constuchar_cast(&x[0]);
  }

  template <typename T>
    inline T getbigendian(const unsigned char * ptr) {
#if (defined(       __BYTE_ORDER) &&        __BYTE_ORDER ==        __BIG_ENDIAN) || \
      (defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN)
      return *reinterpret_cast<const T *>(ptr);
#else
      T result = 0;
      for (size_t i = 0; i < sizeof (T); ++i) {
        result <<= 8;
        result += *ptr++;
      }
      return result;
#endif
    }

  template <typename T>
    inline T getlittleendian(const unsigned char * ptr) {
#if (defined(       __BYTE_ORDER) &&        __BYTE_ORDER ==        __LITTLE_ENDIAN) || \
      (defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
      return *reinterpret_cast<const T *>(ptr);
#else
      T result = 0;
      for (size_t i = 0; i < sizeof (T); ++i) {
        result += *ptr++ << (8*i);
      }
      return result;
#endif
    }

  template <typename T>
    inline void setbigendian(unsigned char * ptr, const T & val) {
#if (defined(       __BYTE_ORDER) &&        __BYTE_ORDER ==        __BIG_ENDIAN) || \
      (defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN)
      *reinterpret_cast<T *>(ptr) = val;
#else
      T tmp = val;
      ptr += sizeof (T);
      for (size_t i = 0; i < sizeof (T); ++i) {
        *--ptr = tmp & 0xff;
        tmp >>= 8;
      }
#endif
    }

  template <typename T>
    inline void setlittleendian(unsigned char * ptr, const T & val) {
#if (defined(       __BYTE_ORDER) &&        __BYTE_ORDER ==        __LITTLE_ENDIAN) || \
      (defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
      *reinterpret_cast<T *>(ptr) = val;
#else
      T tmp = val;
      for (size_t i = 0; i < sizeof (T); ++i) {
        *ptr++ = tmp & 0xff;
        tmp >>= 8;
      }
#endif
    }

  std::string DLLEXPORT ReadLineFromFile(const std::string & fname);

  template <typename T>
    inline T ReadFromFile(const std::string & fname, const T & def = 0) {
      return from_string(ReadLineFromFile(fname), def);
    }

  void DLLEXPORT WriteStringToFile(const std::string & fname, const std::string & val);

  template <typename T>
    inline void WriteToFile(const std::string & fname, const T & val) {
      WriteStringToFile(fname, to_string(val));
    }

  template <typename T>
    std::string DLLEXPORT append_spaces(const uint16_t max, const T str, const bool app_str = true) {
      std::string ret = app_str ? to_string(str) : "";
      size_t spaces = (int(max) - int(to_string(str).size())) > 0 ? max - to_string(str).size() : 0;
      return ret + std::string(spaces, ' ');
    }

  template <typename T>
  std::pair<float, float> calc_mean(std::vector<T> vec){

    float mean = std::accumulate(vec.begin(), vec.end(), 0.0) / float(vec.size());
    float sq_sum = std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
    float stdev = std::sqrt(sq_sum / vec.size() - mean * mean);
    return std::make_pair(mean, stdev);
  }

  template <typename T>
  std::pair<float, float> calc_mean(std::deque<T> deq){

    float mean = std::accumulate(deq.begin(), deq.end(), 0.0) / float(deq.size());
    float sq_sum = std::inner_product(deq.begin(), deq.end(), deq.begin(), 0.0);
    float stdev = std::sqrt(sq_sum / deq.size() - mean * mean);
    return std::make_pair(mean, stdev);
  }

  template<typename T>
  bool in(T num, std::vector<T> ids){ return find(ids.begin(), ids.end(), num ) != ids.end(); }

  template<typename Q>
  std::vector<Q> stovec(std::string name, Q def){
    std::vector<Q> tmp;
    for (const auto &str: split(trim(name, " []"), ","))
      tmp.push_back(stoi(str));
    return tmp;
  }

  std::vector<size_t > range(size_t begin, size_t end=SIZE_MAX, int32_t step=1);

  inline float get_p1(float x0, float x1, float y0, float y1) { return x0 != x1 ? (y0 - y1) / (x0 - x1) : 0; }
  inline float get_p0(float x, float y, float p1) { return y - x * p1; }
  float interpolate_x(float x0, float x1, float y0, float y1, float y=0);

  class ProgressBar {

  private:
    uint32_t nEvents;
    uint32_t currentEvent;
    bool useETA;
    struct winsize w;
    uint8_t barLength;
    uint16_t updateFrequency;
    clock_t lastTime;
    uint8_t nCycles;
    float timePerCycle;

  public:
    ProgressBar(uint32_t, bool use_ETA=true, uint16_t update=100);
    ~ProgressBar() { };
    void update(uint32_t=0);
    void averageTime();
    float getTime();
  };

  std::string DLLEXPORT join(std::string s1, std::string s2);

}

#endif // EUDAQ_INCLUDED_Utils
