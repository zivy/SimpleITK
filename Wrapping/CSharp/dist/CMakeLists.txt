#
# CSharp Packaging
#

set(_files "")
list(
  APPEND
  _files
  ${SimpleITK_DOC_FILES}
  "${CSHARP_BINARY_DIRECTORY}/SimpleITKCSharpManaged.dll"
  "$<TARGET_FILE:${SWIG_MODULE_SimpleITKCSharpNative_TARGET_NAME}>"
)

if(NOT DEFINED SimpleITK_CSHARP_ARCH)
  if(MSVC)
    if(CMAKE_CL_64)
      set(SimpleITK_CSHARP_ARCH "win64")
    else()
      set(SimpleITK_CSHARP_ARCH "win32")
    endif()
  elseif(APPLE)
    set(SimpleITK_CSHARP_ARCH "macosx")
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
      set(
        SimpleITK_CSHARP_ARCH
        "${SimpleITK_CSHARP_ARCH}-${CMAKE_OSX_DEPLOYMENT_TARGET}"
      )
    endif()
  else()
    set(SimpleITK_CSHARP_ARCH "unknown")
  endif()

  if(CSHARP_PLATFORM)
    set(SimpleITK_CSHARP_ARCH "${SimpleITK_CSHARP_ARCH}-${CSHARP_PLATFORM}")
  endif()
endif()

set(
  CSHARP_PACKAGE_STAGE_DIR
  "SimpleITK-${SimpleITK_VERSION}-CSharp-${SimpleITK_CSHARP_ARCH}"
)

add_custom_target(
  dist.CSharp
  COMMENT "Creating CSharp package ${CSHARP_PACKAGE_STAGE_DIR}.zip"
  DEPENDS
    SimpleITKCSharpManaged
)

add_custom_command(
  TARGET dist.CSharp
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E remove_directory "${CSHARP_PACKAGE_STAGE_DIR}"
  COMMAND
    ${CMAKE_COMMAND} -E make_directory "${CSHARP_PACKAGE_STAGE_DIR}"
  COMMENT "Creating CSharp staging directory..."
)

foreach(_f ${_files})
  add_custom_command(
    TARGET dist.CSharp
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different "${_f}"
      "${CSHARP_PACKAGE_STAGE_DIR}/"
    COMMENT "Copying ${_f} to CSharp stage..."
  )
endforeach()

add_custom_command(
  TARGET dist.CSharp
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E tar cf "${CSHARP_PACKAGE_STAGE_DIR}.zip" --format=zip
    "${CSHARP_PACKAGE_STAGE_DIR}"
  COMMENT "Packaging CSHARP distribution..."
)

add_dependencies(dist dist.CSharp)
