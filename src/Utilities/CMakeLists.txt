get_filename_component( TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )

file ( GLOB SOURCE_FILES *.cpp )
file ( GLOB HEADER_FILES *.h )

add_library ( ${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES} )

source_group ( "Sources"  FILES ${HEADER_FILES} ${SOURCE_FILES} )

set_target_properties ( ${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )


target_include_directories ( ${TARGET_NAME}
	PUBLIC ${Vulkan_INCLUDE_DIR}
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/glfw/include
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/stb
	)
