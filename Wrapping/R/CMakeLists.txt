include("${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/sitkCMakeInit.cmake")
cmake_minimum_required(
  VERSION ${SITK_CMAKE_MINIMUM_REQUIRED_VERSION}...${SITK_CMAKE_POLICY_VERSION}
)

project(SimpleITK_R)

include(../../CMake/sitkProjectLanguageCommon.cmake NO_POLICY_SCOPE)

find_package(R REQUIRED)
include_directories(${SimpleITK_INCLUDE_DIRS})

set_source_files_properties(
  SimpleITK.i
  PROPERTIES
    CPLUSPLUS
      ON
)
set_source_files_properties(
  SimpleITK.i
  PROPERTIES
    SWIG_FLAGS
      "-namespace"
)

# Run swig
set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_GLOBAL_FLAGS})
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
set(
  SWIG_MODULE_SimpleITK_EXTRA_DEPS
  ${SWIG_EXTRA_DEPS}
  ${CMAKE_CURRENT_SOURCE_DIR}/R.i
)
swig_add_module( SimpleITK r SimpleITK.i sitkRCommand.cxx sitkRArray.cxx )
target_link_libraries(
  ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  ${SimpleITK_LIBRARIES}
)

sitk_target_link_libraries_with_dynamic_lookup( ${SWIG_MODULE_SimpleITK_TARGET_NAME} ${R_LIBRARIES})

# Workaround for SWIG 4.2.x issue with R and 64-bit integers. See GH issue swig/swig#2847 for details.
if(
  SWIG_VERSION
    VERSION_GREATER_EQUAL
    "4.2.0"
  AND
    SWIG_VERSION
      VERSION_LESS
      "4.3.0"
  AND
    SITK_ULONG_SAME_AS_UINT64
)
  target_compile_definitions(
    ${SWIG_MODULE_SimpleITK_TARGET_NAME}
    PRIVATE
      SWIGWORDSIZE64
  )
endif()

target_include_directories(
  ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  PRIVATE
    ${R_INCLUDE_DIR}
)
target_include_directories(
  ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(${SWIG_MODULE_SimpleITK_TARGET_NAME} PRIVATE -w)

# set the output directory for the R library to the binary packaging location
set_target_properties(
  ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/src/
)

sitk_strip_target( ${SWIG_MODULE_SimpleITK_TARGET_NAME} )
set(SWIG_MODULE_SimpleITKR_TARGET_NAME "${SWIG_MODULE_SimpleITK_TARGET_NAME}")

if(WIN32)
  execute_process(
    COMMAND
      "cmd" " /C date /T"
    OUTPUT_VARIABLE DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX REPLACE "(..)/(..)/(....).*" "\\3-\\2-\\1" ${DATE} DATE)
elseif(UNIX)
  execute_process(
    COMMAND
      "date" "+%Y-%m-%d"
    OUTPUT_VARIABLE DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  message(WARNING "date not implemented")
  set(DATE 00-00-0000)
endif()

# copy the R files a binary package
file(
  COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/Packaging"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
  REGEX "(.*\\.in$)|(.*\\.md5$)|(.*\\.sha512$)"
  EXCLUDE
)

set(SimpleITKR_VERSION "${SimpleITK_VERSION_MAJOR}.${SimpleITK_VERSION_MINOR}")
if(DEFINED SimpleITK_VERSION_PATCH)
  set(SimpleITKR_VERSION "${SimpleITKR_VERSION}.${SimpleITK_VERSION_PATCH}")
  if(DEFINED SimpleITK_VERSION_TWEAK)
    set(SimpleITKR_VERSION "${SimpleITKR_VERSION}.${SimpleITK_VERSION_TWEAK}")
  endif()
endif()
if(DEFINED SimpleITK_VERSION_POST)
  set(SimpleITKR_VERSION "${SimpleITKR_VERSION}-${SimpleITK_VERSION_POST}")
elseif(DEFINED SimpleITK_VERSION_DEV AND NOT SimpleITK_VERSION_DEV STREQUAL "")
  math(EXPR DEV "9000 + ${SimpleITK_VERSION_DEV}")
  set(SimpleITKR_VERSION "${SimpleITKR_VERSION}-${DEV}")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/SimpleITK/DESCRIPTION.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/DESCRIPTION"
  @ONLY
)

# Use ExternalData module to either download data to get from local
# data store.
include(sitkExternalData)

set(_algo "sha512")

file(
  GLOB_RECURSE image_links
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/SimpleITK/inst/extdata/*.${_algo}"
)

foreach(link ${image_links})
  string(REGEX REPLACE "\\.${_algo}$" "" link ${link})
  ExternalData_Expand_Arguments(
    SimpleITKRpackageData
    image_location
    DATA{${link}}
  )
  set(
    COPY_DATA_COMMAND
    ${COPY_DATA_COMMAND}
    COMMAND
    ${CMAKE_COMMAND}
    -E
    copy
    ${image_location}
    ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/inst/extdata/
  )
endforeach()

# The downloaded tar ball of documentation has been generated with
# the "Utilities/GenerateDocs/SwigDocUpdate.sh". This documentation
# should be generated for each release.
file(
  GLOB_RECURSE doc_links
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/SimpleITK/man/*.${_algo}"
)

foreach(link ${doc_links})
  string(REGEX REPLACE "\\.${_algo}$" "" link ${link})
  ExternalData_Expand_Arguments(
    SimpleITKRpackageData
    doc_location
    DATA{${link}}
  )
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/man/")
  set(
    EXTRACT_RMAN_COMMAND
    ${EXTRACT_RMAN_COMMAND}
    COMMAND
    ${CMAKE_COMMAND}
    -E
    tar
    xzf
    ${doc_location}
    WORKING_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/man/"
  )
endforeach()

if(COMMAND ExternalData_Add_Target)
  ExternalData_Add_Target(SimpleITKRpackageData)
endif()
add_dependencies(${SWIG_MODULE_SimpleITK_TARGET_NAME} SimpleITKRpackageData)

# copy sample images and extract documentation - used in vignette
add_custom_command(
  TARGET ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E make_directory
    "${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/inst/extdata/"
    ${COPY_DATA_COMMAND} ${EXTRACT_RMAN_COMMAND}
)

if(BUILD_SHARED_LIBS AND DEFINED SimpleITK_LIBRARY_OUTPUT_DIRECTORY)
  if(TARGET itkTestDriver)
    set(
      CMD_PREFIX
      itkTestDriver
      --add-before-libpath
      "${SimpleITK_LIBRARY_OUTPUT_DIRECTORY}"
    )
  else()
    message(
      WARNING
      "Unable to use \"itkTestDriver\" to set run-time library patch for R packaging."
    )
  endif()
endif()

add_custom_command(
  TARGET ${SWIG_MODULE_SimpleITK_TARGET_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/R_libs
  # install for running tests and create binary package
  COMMAND
    ${CMD_PREFIX} ${R_COMMAND} -f ${CMAKE_CURRENT_SOURCE_DIR}/processSwigCode.R
    --args ${CMAKE_CURRENT_BINARY_DIR}/SimpleITK.R
    ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/R/SimpleITK.R
    ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/man/hidden_methods.Rd
  COMMAND
    ${CMD_PREFIX} ${R_COMMAND} -f ${CMAKE_CURRENT_SOURCE_DIR}/mkNamespace.R
    --args ${CMAKE_CURRENT_BINARY_DIR}/NAMESPACE
    ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK/NAMESPACE
  COMMAND
    ${CMD_PREFIX} ${R_COMMAND} CMD INSTALL --build
    ${CMAKE_CURRENT_BINARY_DIR}/Packaging/SimpleITK
    --library=${CMAKE_CURRENT_BINARY_DIR}/R_libs --no-multiarch
  COMMENT
    "Installing R package for testing and building binary version for distribution"
)

if(NOT CMAKE_PROJECT_NAME STREQUAL "SimpleITK" AND BUILD_TESTING)
  set(WRAP_R 1)

  add_subdirectory(
    "${SimpleITK_SOURCE_DIR}/Testing/Unit/R"
    "${CMAKE_BINARY_DIR}/Testing/Unit"
  )

  if(BUILD_EXAMPLES)
    add_subdirectory(
      "${SimpleITK_SOURCE_DIR}/Examples"
      "${CMAKE_BINARY_DIR}/Examples"
    )
  endif()

  if(COMMAND ExternalData_Add_Target)
    ExternalData_Add_Target(SimpleITKData)
  endif()
endif()
