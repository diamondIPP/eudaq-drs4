set(name "DRSProducer.exe")

FIND_PACKAGE(DRS REQUIRED)

IF (NOT DRS_FOUND)
  MESSAGE(FATAL_ERROR "Could not find DRS! Make sure to set the environment variable $DRS_PATH to the path of your pxar installation!")
ENDIF()

SET(sourcefiles src/DRS4Producer.cxx )

INCLUDE_DIRECTORIES(include ${DRS_INCLUDE_DIRS})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name} EUDAQ ${EUDAQ_THREADS_LIB} ${DRS_LIBRARY})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
