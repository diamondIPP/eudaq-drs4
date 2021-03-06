set(name "ExplorerProducer.exe")
IF(NOT IGNORE_KNOWN_ISSUES AND WIN32) # issues compiling under WINDOWS only
  IF(NOT BUILD_allproducer) # only warn if not configuring all producers at the same time (for testing)
    MESSAGE(WARNING "There are known issues compiling ${name} on your platform. Set the IGNORE_KNOWN_ISSUES option to configure this producer to be build anyway.")
  ENDIF(NOT BUILD_allproducer)
ELSE(NOT IGNORE_KNOWN_ISSUES AND WIN32)
  set(sourcefiles src/DeviceExplorer.cc src/ExplorerProducer.cxx)
  set(ext_libraries "" )
  set(ext_lib_paths "" )
  set(ext_inc_paths "include" )
  # config files to be installed
  set(ConfigFiles ni_autotrig_explorer_pedestalmeas.conf ni_coins_explorer_wpedestal.conf)

  ADD_DEFINITIONS()
  LINK_DIRECTORIES( ${ext_lib_paths} )
  INCLUDE_DIRECTORIES( ${ext_inc_paths})
  ADD_EXECUTABLE(${name} ${sourcefiles})

  TARGET_LINK_LIBRARIES(${name}   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})

  INSTALL(TARGETS ${name}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

  foreach(ConfigFile ${ConfigFiles})
    if (UNIX)
      INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/conf/${ConfigFile} ${CMAKE_INSTALL_PREFIX}/conf/${ConfigFile})")
    else()
      INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/${ConfigFile} DESTINATION ${CMAKE_INSTALL_PREFIX}/conf)
    endif()
  endforeach()
ENDIF(NOT IGNORE_KNOWN_ISSUES AND WIN32)