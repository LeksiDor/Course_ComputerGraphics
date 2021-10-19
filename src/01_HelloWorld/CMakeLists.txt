get_filename_component( TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )

file ( GLOB SOURCE_FILES "*.cpp" )
file ( GLOB HEADER_FILES "*.h" )
file ( GLOB SHADER_FILES "shaders/*" )

add_executable ( ${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES} )

source_group ( "Sources" FILES ${HEADER_FILES} ${SOURCE_FILES} )
source_group ( "Shaders" FILES ${SHADER_FILES} )

set_target_properties ( ${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )
if ( MSVC )
set_target_properties ( ${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )
endif ( MSVC )



target_include_directories ( ${TARGET_NAME}
	PUBLIC ${Vulkan_INCLUDE_DIR}
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/glfw/include
	)


target_link_libraries( ${TARGET_NAME}
	${Vulkan_LIBRARY}
	glfw
	)


# Preprocessor definitions.
add_compile_definitions( PROJECT_NAME="${TARGET_NAME}" )
add_compile_definitions( PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )



set ( SHADER_BINARY_DIR "${BINARIES_DIRECTORY}/${TARGET_NAME}" )

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach( source IN LISTS SHADER_FILES )
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable}
      #      -MD -MF ${SHADER_BINARY_DIR}/${FILENAME}.d
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
