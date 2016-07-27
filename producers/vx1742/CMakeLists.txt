#written by Christian Dorfer (dorfer@phys.ethz.ch)

project(VX1742Producer)
cmake_minimum_required(VERSION 2.8.12)

set(name "VX1742Producer.exe")
add_definitions(-std=c++0x)
set(sourcefiles 
src/VX1742Producer.cpp src/VX1742Interface.cpp src/VX1742Event.cpp
/ATLAS/src/RCDCmemSegment.cc 
/ATLAS/src/RCDDataRange.cc 
/ATLAS/src/RCDUtilities.cc 
/ATLAS/src/RCDVme.cc 
/ATLAS/src/RCDVmeBlockTransfer.cc 
/ATLAS/src/RCDVmeInterrupt.cc 
/ATLAS/src/RCDVmeMasterMap.cc 
/ATLAS/src/RCDVmeSlaveMap.cc)

find_library(DFDEBUG_LIB
  NAMES libDFDebug.so
  HINTS "/ATLAS/lib")

find_library(VMERCC_LIB
  NAMES libvme_rcc.so
  HINTS "/ATLAS/lib")

find_library(CMEMRCC_LIB
  NAMES libcmem_rcc.so
  HINTS "/ATLAS/lib")

find_library(RCCERROR_LIB
  NAMES librcc_error.so
  HINTS "/ATLAS/lib")

find_library(IORCC_LIB
  NAMES libio_rcc.so
  HINTS "/ATLAS/lib")

add_executable(${name} ${sourcefiles})
include_directories(/ATLAS/include/ include/)
target_link_libraries(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${DFDEBUG_LIB} ${VMERCC_LIB} ${CMEMRCC_LIB} ${RCCERROR_LIB} ${IORCC_LIB} ${ext_libs})

install(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
