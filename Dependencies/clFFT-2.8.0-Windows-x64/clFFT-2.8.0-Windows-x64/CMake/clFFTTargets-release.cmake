#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
SET(CMAKE_IMPORT_FILE_VERSION 1)

# Compute the installation prefix relative to this file.
GET_FILENAME_COMPONENT(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
GET_FILENAME_COMPONENT(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)

# Make sure the targets which have been exported in some other 
# export set exist.

# Import target "clFFT" for configuration "Release"
SET_PROPERTY(TARGET clFFT APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(clFFT PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib64/import/clFFT.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "C:/Program Files (x86)/AMD APP SDK/2.9-1/lib/x86_64/OpenCL.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/clFFT.dll"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS clFFT )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_clFFT "${_IMPORT_PREFIX}/lib64/import/clFFT.lib" "${_IMPORT_PREFIX}/bin/clFFT.dll" )

# Make sure the targets which have been exported in some other 
# export set exist.

# Import target "StatTimer" for configuration "Release"
SET_PROPERTY(TARGET StatTimer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(StatTimer PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib64/import/StatTimer.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "C:/Program Files (x86)/AMD APP SDK/2.9-1/lib/x86_64/OpenCL.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/StatTimer.dll"
  )

LIST(APPEND _IMPORT_CHECK_TARGETS StatTimer )
LIST(APPEND _IMPORT_CHECK_FILES_FOR_StatTimer "${_IMPORT_PREFIX}/lib64/import/StatTimer.lib" "${_IMPORT_PREFIX}/bin/StatTimer.dll" )

# Loop over all imported files and verify that they actually exist
FOREACH(target ${_IMPORT_CHECK_TARGETS} )
  FOREACH(file ${_IMPORT_CHECK_FILES_FOR_${target}} )
    IF(NOT EXISTS "${file}" )
      MESSAGE(FATAL_ERROR "The imported target \"${target}\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    ENDIF()
  ENDFOREACH()
  UNSET(_IMPORT_CHECK_FILES_FOR_${target})
ENDFOREACH()
UNSET(_IMPORT_CHECK_TARGETS)

# Cleanup temporary variables.
SET(_IMPORT_PREFIX)

# Commands beyond this point should not need to know the version.
SET(CMAKE_IMPORT_FILE_VERSION)
