#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cctype>
#include <stdio.h>
#include <unistd.h>

// for cross-platform sleep:
#include <chrono>
#include <thread>

#if EUDAQ_PLATFORM_IS(WIN32)
#ifndef __CINT__
#define WIN32_LEAN_AND_MEAN // causes some rarely used includes to be ignored
#define _WINSOCKAPI_
#define _WINSOCK2API_
#include <cstdio>  // HK
#include <cstdlib>  // HK
#endif
#else
# include <unistd.h>
#endif

using namespace std;

namespace eudaq {
    void PressEnterToContinue()
      {
      std::cout << "Press ENTER to continue... " << std::flush;
      std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
      }

  std::string ucase(const std::string & str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::toupper(result[i]);
    }
    return result;
  }

  std::string lcase(const std::string & str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::tolower(result[i]);
    }
    return result;
  }

  /** Trims the leading and trainling white space from a string
   */
  std::string trim(const std::string & s, std::string trim_characters) {
//    static const std::string spaces = " \t\n\r\v";
    size_t b = s.find_first_not_of(trim_characters);
    size_t e = s.find_last_not_of(trim_characters);
    if (b == std::string::npos || e == std::string::npos) {
      return "";
    }
    return std::string(s, b, e - b + 1);
  }

  std::string escape(const std::string & s) {
    std::ostringstream ret;
    ret << std::setfill('0') << std::hex;
    for (size_t i = 0; i < s.length(); ++i) {
      if (s[i] == '\\')
        ret << "\\\\";
      else if (s[i] < 32)
        ret << "\\x" << std::setw(2) << int(s[i]);
      else
        ret << s[i];
    }
    return ret.str();
  }

  std::string firstline(const std::string & s) {
    size_t i = s.find('\n');
    return s.substr(0, i);
  }

  std::vector<std::string> split(const std::string & str, const std::string & delim) {
    return split(str, delim, false);
  }

  std::vector<std::string> split(const std::string & str, const std::string & delim, bool dotrim) {
    std::string s(str);
    std::vector<std::string> result;
    if (str == "") return result;
    size_t i;
    while ((i = s.find_first_of(delim)) != std::string::npos) {
      result.push_back(dotrim ? trim(s.substr(0, i)) : s.substr(0, i));
      s = s.substr(i + 1);
    }
    result.push_back(s);
    return result;
  }

  void mSleep(unsigned ms) {
    // use c++11 std sleep routine
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  template<>
  int64_t from_string(const std::string & x, const int64_t & def) {
      if (x == "") return def;
      const char * start = x.c_str();
      size_t end = 0;
      int base = 10;
      std::string bases("box");
      if (x.length() > 2 && x[0] == '0' && bases.find(x[1]) != std::string::npos) {
        if (x[1] == 'b') base = 2;
        else if (x[1] == 'o') base = 8;
        else if (x[1] == 'x') base = 16;
        start += 2;
      }
      int64_t result = static_cast<int64_t>(std::stoll(start, &end, base));
      if (!x.substr(end).empty()) throw std::invalid_argument("Invalid argument: " + x);
      return result;
    }

  template<>
    uint64_t from_string(const std::string & x, const uint64_t & def) {
      if (x == "") return def;
      const char * start = x.c_str();
      size_t end = 0;
      int base = 10;
      std::string bases("box");
      if (x.length() > 2 && x[0] == '0' && bases.find(x[1]) != std::string::npos) {
        if (x[1] == 'b') base = 2;
        else if (x[1] == 'o') base = 8;
        else if (x[1] == 'x') base = 16;
        start += 2;
      }
      uint64_t result = static_cast<uint64_t>(std::stoull(start, &end, base));
      if (!x.substr(end).empty()) throw std::invalid_argument("Invalid argument: " + x);
      return result;
    }

  void WriteStringToFile(const std::string & fname, const std::string & val) {
    std::ofstream file(fname.c_str());
    if (!file.is_open()) EUDAQ_THROW("Unable to open file " + fname + " for writing");
    file << val << std::endl;
    if (file.fail()) EUDAQ_THROW("Error writing to file " + fname);
  }

  std::string ReadLineFromFile(const std::string & fname) {
    std::ifstream file(fname.c_str());
    std::string result;
    if (file.is_open()) {
      std::getline(file, result);
      if (file.fail()) {
        EUDAQ_THROW("Error reading from file " + fname);
      }
    }
    return result;
  }

  void bool2uchar(const bool* inBegin, const bool* inEnd, std::vector<unsigned char>& out)
  {
    int j = 0;
    unsigned char dummy = 0;
    //bool* d1=&in[0];
    size_t size = (inEnd - inBegin);
    if (size % 8)
    {
      size += 8;
    }
    size /= 8;
    out.reserve(size);
    for (auto i = inBegin; i < inEnd; ++i)
    {
      dummy += (unsigned char)(*i) << (j % 8);

      if ((j % 8) == 7)
      {
        out.push_back(dummy);
        dummy = 0;
      }
      ++j;
    }
  }

  void DLLEXPORT uchar2bool(const unsigned char* inBegin, const unsigned char* inEnd, std::vector<bool>& out)
  {
    auto distance = inEnd - inBegin;
    out.reserve(distance * 8);
    for (auto i = inBegin; i != inEnd; ++i)
    {
      for (int j = 0; j < 8; ++j){
        out.push_back((*i)&(1 << j));
      }
    }

  }

  void DLLEXPORT print_banner(std::string message, const char seperator, uint16_t max_lenght) {
    uint16_t mes_length = 0;
    std::vector<std::string> split_msg = split(message, "\n");;
    std::stringstream ss;
    for (std::vector<std::string>::iterator it = split_msg.begin(); it != split_msg.end(); it++)
      if (it->size() > mes_length) mes_length = uint16_t(it->size());
    if (mes_length > max_lenght) mes_length = max_lenght;
    for (std::vector<std::string>::iterator it = split_msg.begin(); it != split_msg.end(); it++){
      it->resize(mes_length);
      ss << it->c_str() << "\n";
    }
    std::string send_msg = trim(ss.str(), "\n");
    std::string banner = std::string(mes_length, seperator);
    std::cout << "\n" << banner << "\n" << send_msg << "\n" << banner << "\n" << std::endl;
  }

    ProgressBar::ProgressBar(uint32_t n_events, bool use_ETA, uint16_t update): nEvents(n_events), currentEvent(0), useETA(use_ETA), updateFrequency(update), lastTime(clock()),
                              nCycles(0), timePerCycle(0) {
      ioctl(0, TIOCGWINSZ, &w);
      uint8_t diff = not use_ETA ? uint8_t(26) : uint8_t(37);
      barLength = uint8_t(w.ws_col - diff);
    }

    void ProgressBar::update(uint32_t event) {

      if (!currentEvent) cout << endl;

      if (!event) currentEvent++;
      else currentEvent = event;

      if (currentEvent % updateFrequency == 0 && currentEvent){
        stringstream ss;
        ss << "\rEvent: "  << setw(7) << setfill('0') << fixed << currentEvent;
        uint8_t current_bar_length = uint8_t(float(currentEvent) / nEvents * barLength);
        ss << " [" << string(current_bar_length, '=') << ">" << string(barLength - current_bar_length, ' ') << "] ";
        ss << setprecision(2) << setw(6) << setfill('0') << fixed << float(currentEvent) / nEvents * 100 << "%";
        if (useETA) {
          averageTime();
          float tot = (nEvents - currentEvent) / updateFrequency * timePerCycle;
          ss << setprecision(0) << " ETA: " << setw(2) << setfill('0') << tot / 60 << ":" << setw(2) << setfill('0') << tot - int(tot) / 60 * 60;
        }

        cout << ss.str() << flush;


//        if ( stopAt - ievent < 10 ) cout << "|" <<string( 50 , '=') << ">";
//        else cout << "|" <<string(int(float(ievent) / stopAt * 100) / 2, '=') << ">";
//        if ( stopAt - ievent < 10 ) cout << "| 100%    ";
//        else cout << string(50 - int(float(ievent) / stopAt * 100) / 2, ' ') << "| 100%    " << endl;
//        float all_seconds = (stopAt - ievent) / speed;
//        uint16_t minutes = all_seconds / 60;
//        uint16_t seconds = all_seconds - int(all_seconds) / 60 * 60;
//        uint16_t miliseconds =  (all_seconds - int(all_seconds)) * 1000;
////        if (speed) cout << "time left:\t\t" << setprecision(2) << fixed << (nEntries - ievent) / speed <<  " seconds     " << minutes;
//        if (speed) {
//          cout << "time left:\t\t" << setw(2) << setfill('0') << minutes;
//          cout << ":" << setw(2) << setfill('0') << seconds;
//          cout << ":" << setw(3) << setfill('0') << miliseconds << "      ";
//        }
//        //else cout << "time left: ???";// Don't know why it is persistent. Commented it :P
      }
      if (currentEvent == nEvents) {
        cout << "\rEvent: "  << setw(7) << setfill('0') << fixed << nEvents;
        cout << " [" << string(barLength, '=') << ">" << "] 100.00%" << endl;
      }

    }
    float ProgressBar::getTime() {
      clock_t now = clock();
      float ret = float(now - lastTime) / CLOCKS_PER_SEC;
      lastTime = now;
      return ret;
    }

    void ProgressBar::averageTime() {
      if (nCycles < 10)
        timePerCycle = (timePerCycle * nCycles + getTime()) / (nCycles + 1);
      else
        timePerCycle = float(.98) * timePerCycle + float(.02) * getTime();
      nCycles++;
    }

    string join(string s1, string s2) {

      return s1 + '/' + s2;
    }
    std::vector<ssize_t> range(ssize_t begin, ssize_t end, int32_t step){

      end = end == SIZE_MAX ? begin : end;
      begin = end != begin ? begin : 0;
      std::vector<ssize_t> tmp;
      for (ssize_t i(begin); i < end; i+=step)
        tmp.push_back(i);
      return tmp;
    }
}
