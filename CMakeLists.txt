cmake_minimum_required( VERSION 3.9.6 FATAL_ERROR )

project ( CourseCG )

# Packages
find_package( Vulkan REQUIRED FATAL_ERROR )
#find_package( VulkanSDK REQUIRED FATAL_ERROR )


# Set the C/C++ specified in the projects as requirements
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_C_STANDARD_REQUIRED ON )
set( CMAKE_CXX_STANDARD 17 )


# CMake constants.
set( BINARIES_DIRECTORY ${PROJECT_SOURCE_DIR}/bin )


# Preprocessor definitions.
add_compile_definitions( NOMINMAX )
add_compile_definitions( _USE_MATH_DEFINES )
add_compile_definitions( ROOT_DIRECTORY="${PROJECT_SOURCE_DIR}" )
add_compile_definitions( BINARIES_DIRECTORY="${BINARIES_DIRECTORY}" )
if( WIN32 )
	add_compile_definitions( VK_USE_PLATFORM_WIN32_KHR )
endif( WIN32 )
if( UNIX )
	add_compile_definitions( VK_USE_PLATFORM_XCB_KHR )
	add_compile_definitions( VK_USE_PLATFORM_XLIB_KHR )
endif( UNIX )


# Compile 3rd-party
add_subdirectory( 3rdparty/glfw )

# Sub projects
add_subdirectory( src/01_HelloWorld )
