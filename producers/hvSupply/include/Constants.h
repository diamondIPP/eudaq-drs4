/** ---------------------------------------------------------------------------------
 *  Constants for HV supplies
 *
 *  Date: April 23rd 2018
 *  Author: Michael Reichmann (remichae@phys.ethz.ch)
 *  ------------------------------------------------------------------------------------*/

#ifndef _HVDEFS_H
#define _HVDEFS_H

#define BOLDRED "\33[1m\033[31m"
#define BOLDGREEN "\33[1m\033[32m"
#define CLEAR "\033[0m\n"
#define ON "ON"
#define OFF "OFF"
#define EARLY "EARLY"
#define LATE "LATE"
#define NEVER "NEVER"
#define FRONT "FRONT"
#define REAR "REAR"
#define REPEAT "REP"
#define MOVING "MOV"
#define FIXED "FIX"
#define SWEEP "SWE"
#define INVALID 1e6

#include <map>
#include <string>
#include <vector>

namespace hvc{

  const std::map<std::string, uint16_t> MaxVoltages = { {"Keithley 2410", 1100},
                                                        {"Keithley 2400", 200} };

  const std::vector<std::string> AbortLevels = {"EARLY", "LATE", "NEVER"};

}
#endif