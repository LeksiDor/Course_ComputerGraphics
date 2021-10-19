
# Function to compile GLSL source files to Spir-V
#
# add_spv_compilation(
#   TARGET_NAME MyProject
#   SHADER_FILES foo.vert foo.frag
#   OUT_SPV_DIR ./shaders
# )
function ( add_spv_compilation )
  #set(oneValueArgs DST VULKAN_TARGET HEADER DEPENDENCY FLAGS)
  #set(multiValueArgs SOURCE_FILES HEADER_FILES)
  #cmake_parse_arguments(COMPILE  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  
  # Parse arguments.
  set ( oneValueArgs TARGET_NAME OUT_SPV_DIR )
  set ( multiValueArgs SHADER_FILES )
  cmake_parse_arguments( COMPILE  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  
  # Sanity check.
  if ( NOT glslc_executable )
    message ( ERROR ": glslc_executable was not specified.")
    return()
  endif()
  
  if ( NOT DEFINED COMPILE_TARGET_NAME )
    message ( ERROR ": TARGET_NAME was not defined." )
    return()
  endif()
  
  if ( NOT DEFINED COMPILE_OUT_SPV_DIR )
    message ( ERROR ": OUT_SPV_DIR was not defined." )
    return()
  endif()
  
  # Create output directory.
  add_custom_command(
    TARGET ${COMPILE_TARGET_NAME}
    PRE_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E make_directory ${COMPILE_OUT_SPV_DIR}
    COMMENT "Creating ${COMPILE_OUT_SPV_DIR}"
  )

  # Compile each shader into the target directory.
  foreach( source IN LISTS SHADER_FILES )
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
      TARGET ${COMPILE_TARGET_NAME}
      PRE_BUILD
      COMMAND
        ${glslc_executable}
        #      -MD -MF ${COMPILE_OUT_SPV_DIR}/${FILENAME}.d
        -o ${COMPILE_OUT_SPV_DIR}/${FILENAME}.spv
        ${source}
      DEPENDS ${source} ${COMPILE_OUT_SPV_DIR}
      COMMENT "Compiling ${FILENAME}"
    )
    list(APPEND SPV_SHADERS ${COMPILE_OUT_SPV_DIR}/${FILENAME}.spv)
  endforeach()

endfunction()