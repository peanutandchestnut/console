#-------------------------------------------------------------------
# modified from https://bitbucket.org/dark_sylinc/dergo-blender/src/bd5199ee77e5d7325c84f984a33c28f2d744c2ec/DERGO_Server/CMake/Dependencies/OGRE.cmake?at=default&fileviewer=file-view-default
#-------------------------------------------------------------------

macro( findPluginAndSetPath BUILD_TYPE CFG_VARIABLE LIBRARY_NAME )
	set( REAL_LIB_PATH ${LIBRARY_NAME} )
	if( ${BUILD_TYPE} STREQUAL "Debug" )
		set( REAL_LIB_PATH ${REAL_LIB_PATH}_d )
	endif()

	if( WIN32 )
		set( REAL_LIB_PATH "${OGRE_BINARIES}/bin/${BUILD_TYPE}/${REAL_LIB_PATH}.dll" )
	else()
		set( REAL_LIB_PATH "${OGRE_BINARIES}/lib/${REAL_LIB_PATH}.so" )
	endif()

	if( EXISTS ${REAL_LIB_PATH} )
		# DLL Exists, set the variable for Plugins.cfg
		if( ${BUILD_TYPE} STREQUAL "Debug" )
			message(STATUS "add _d for debug")
			set( ${CFG_VARIABLE} "Plugin=${LIBRARY_NAME}_d" )
		else()
			set( ${CFG_VARIABLE} "Plugin=${LIBRARY_NAME}" )
		endif()

		# On Windows, copy the DLLs to the folders.
		if( WIN32 )
			file( COPY ${REAL_LIB_PATH} DESTINATION "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/Plugins" )
		endif()
	else()
		Message(FATAL_ERROR "${REAL_LIB_PATH} not found")
	endif()
endmacro()

macro( setupPluginFileFromTemplate BUILD_TYPE )
	if( NOT UNIX )
		file( MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}/Plugins" )
	endif()

	findPluginAndSetPath( ${BUILD_TYPE} OGRE_PLUGIN_RS_GL	RenderSystem_GL )
	findPluginAndSetPath( ${BUILD_TYPE} OGRE_PLUGIN_PARTICLE_FX	Plugin_ParticleFX )

	configure_file( ${CMAKE_SOURCE_DIR}/CMake/Templates/plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cfg )

	if( WIN32 )
		# Copy standard DLLs
		if( NOT ${BUILD_TYPE} STREQUAL "Debug" )
			if( EXISTS "${OGRE_BINARIES}/bin/${BUILD_TYPE}/OgreMain.dll" )
				file( COPY "${OGRE_BINARIES}/bin/${BUILD_TYPE}/OgreMain.dll"		DESTINATION "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}" )
			endif()
		else()
			if( EXISTS "${OGRE_BINARIES}/bin/${BUILD_TYPE}/OgreMain_d.dll" )
				file( COPY "${OGRE_BINARIES}/bin/${BUILD_TYPE}/OgreMain_d.dll"		DESTINATION "${CMAKE_SOURCE_DIR}/bin/${BUILD_TYPE}" )
			endif()
		endif()
	endif()

	unset( OGRE_PLUGIN_RS_D3D11 )
	unset( OGRE_PLUGIN_RS_GL3PLUS )
endmacro()

macro( setupOgre OGRE_SOURCE, OGRE_BINARIES, OGRE_LIBRARIES, OGRE_INCLUDE_DIRS)

	# Guess the paths.
	set( OGRE_SOURCE "${CMAKE_SOURCE_DIR}/Dependencies/Ogre" CACHE STRING "Path to OGRE source code (see http://www.ogre3d.org/tikiwiki/tiki-index.php?page=CMake+Quick+Start+Guide)" )
	if( WIN32 )
		set( OGRE_BINARIES "${OGRE_SOURCE}/build" CACHE STRING "Path to OGRE's build folder generated by CMake" )
		link_directories( "${OGRE_BINARIES}/lib/$(ConfigurationName)" )
	else()
		set( OGRE_BINARIES "${OGRE_SOURCE}/build/${CMAKE_BUILD_TYPE}" CACHE STRING "Path to OGRE's build folder generated by CMake" )
		link_directories( "${OGRE_BINARIES}/lib" )
	endif()

	# Ogre config
	set(${OGRE_INCLUDE_DIRS} "${OGRE_SOURCE}/OgreMain/include;${${OGRE_INCLUDE_DIRS}}")
	set(${OGRE_INCLUDE_DIRS} "${OGRE_BINARIES}/include;${${OGRE_INCLUDE_DIRS}}")

	# Ogre includes
	#include_directories( "${OGRE_SOURCE}/Components/Hlms/Common/include" )
	#include_directories( "${OGRE_SOURCE}/Components/Hlms/Unlit/include" )
	#include_directories( "${OGRE_SOURCE}/Components/Hlms/Pbs/include" )
	#include_directories( "${OGRE_SOURCE}/Components/Overlay/include" )

	set( OGRE_LIBRARIES
		debug OgreMain_d
		optimized OgreMain
		)
	MESSAGE(STATUS "set ${OGRE_INCLUDE_DIRS}: ${${OGRE_INCLUDE_DIRS}}")
	MESSAGE(STATUS "set ${OGRE_LIBRARIES}: ${${OGRE_LIBRARIES}}")

endmacro()

macro(setupOgrePlugins)
	# Plugins.cfg
	if( UNIX )
		set( OGRE_PLUGIN_DIR "${OGRE_BINARIES}/lib" )
	else()
		set( OGRE_PLUGIN_DIR "Plugins" )
	endif()

	#message( STATUS "Copying Hlms data files from Ogre repository" )
	#file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Common"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )
	#file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Pbs"		DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )
	#file( COPY "${OGRE_SOURCE}/Samples/Media/Hlms/Unlit"	DESTINATION "${CMAKE_SOURCE_DIR}/bin/Data/Hlms" )

	message( STATUS "Copying DLLs and generating Plugins.cfg for ${CMAKE_BUILD_TYPE}" )
	setupPluginFileFromTemplate( ${CMAKE_BUILD_TYPE} )
	#message( STATUS "Copying DLLs and generating Plugins.cfg for Release" )
	#setupPluginFileFromTemplate( "Release" ) 
	#message( STATUS "Copying DLLs and generating Plugins.cfg for RelWithDebInfo" )
	#setupPluginFileFromTemplate( "RelWithDebInfo" )
	#message( STATUS "Copying DLLs and generating Plugins.cfg for MinSizeRel" )
	#setupPluginFileFromTemplate( "MinSizeRel" )
endmacro()
