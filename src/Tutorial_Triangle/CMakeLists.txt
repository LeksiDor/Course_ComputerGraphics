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
	PUBLIC ../Utilities
	PUBLIC ${Vulkan_INCLUDE_DIR}
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/glfw/include
	)

add_dependencies( ${TARGET_NAME} Utilities )

target_link_libraries( ${TARGET_NAME}
	${Vulkan_LIBRARY}
	glfw
	Utilities
	)


# Preprocessor definitions.
add_compile_definitions( PROJECT_NAME="${TARGET_NAME}" )
add_compile_definitions( PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" )

add_spv_compilation(
  TARGET_NAME ${TARGET_NAME}
  SHADER_FILES ${SHADER_FILES}
  OUT_SPV_DIR "${BINARIES_DIRECTORY}/${TARGET_NAME}"
  )
