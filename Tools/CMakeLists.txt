#################
# Tools Library #
#################

#
# Find Boost
#
find_package( Boost COMPONENTS
               unit_test_framework
               system
               REQUIRED QUIET
)

#
# Find ROOT
#
find_package( ROOT QUIET )

#
# Find GSL
#
find_package( GSL COMPONENTS gsl gslcblas QUIET )

# Check if all requirements are found. Skip modules otherwise.
IF( ${ROOT_FOUND} AND ${GSL_FOUND}
    AND TARGET DataReader AND TARGET HelicityFormalism
)

FILE(GLOB lib_srcs *.cpp)

FILE(GLOB lib_headers *.hpp)

### Generator dictionaries
# Currently we do not generate dictionaries of compatibility reasons
# We have to add this directory to the include directories
# so rootcint can find the header and LinkDef files
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
#ROOT_GENERATE_DICTIONARY( G__Dict SimpleEfficiency.hpp LINKDEF libDPKinematicsLinkDef.h )
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/G__Dict.cxx.o DESTINATION lib/)
#add_library ( DPKinematics SHARED ${lib_srcs} G__Dict.cxx ${lib_headers} )

add_library ( Tools
  SHARED ${lib_srcs} ${lib_headers}
)

target_link_libraries( Tools
  Core
  DataReader
  HelicityFormalism
  ${Boost_LIBRARIES}
  ${ROOT_LIBRARIES}
  ${GSL_LIBRARIES}
)

target_include_directories( Tools
  PUBLIC ${ROOT_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${GSL_INCLUDE_DIR} )

#
# Install
#
install( FILES ${lib_headers}
    DESTINATION include/ComPWA/Tools
)

install( TARGETS Tools
    LIBRARY DESTINATION lib/ComPWA
)

#if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  add_subdirectory(PythonInterface)
#endif()

#
# TESTING
#
# Testing routines are stored in separate directory
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

# Run through each source
foreach(testSrc ${TEST_SRCS})
  # Extract the filename without an extension (NAME_WE)
  get_filename_component(fileName ${testSrc} NAME_WE)
  SET(testName "${fileName}")

  # Add compile target
  add_executable( ${testName} ${testSrc})

  # Link to Boost libraries AND your targets and dependencies
  target_link_libraries( ${testName}
    Core
	pthread
    ${Boost_LIBRARIES}
    ${ROOT_LIBRARIES}
  )

  # Move testing binaries into a testBin directory
  set_target_properties( ${testName}
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/test/
  )

  # Copy input files for test programs - we assume they have the name
  # ${testName}-input*. Multiple files can be copied.
  file(GLOB TestInput
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/${fileName}-input*
  )
  foreach( TestIn ${TestInput} )
    get_filename_component( TestInName ${TestIn} NAME )

    add_custom_command(
      TARGET ${testName} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/test/${TestInName}
      ${PROJECT_BINARY_DIR}/bin/test/${TestInName} )
  endforeach( TestIn )

  # Finally add it to test execution -
  # Notice the WORKING_DIRECTORY and COMMAND
  add_test(NAME ${testName}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin/test/
    COMMAND ${PROJECT_BINARY_DIR}/bin/test/${testName} )
endforeach(testSrc)

ELSE ()
  MESSAGE(WARNING "Requirements not found! Not building Tools module!")
ENDIF()
