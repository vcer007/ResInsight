cmake_minimum_required (VERSION 2.8.12)

project ( cafPdmXml_UnitTests )

find_package(Qt5Core CONFIG QUIET)
if (Qt5Core_FOUND)
	find_package(Qt5 CONFIG REQUIRED Core Xml)
else()
	find_package(Qt4 COMPONENTS QtCore QtXml REQUIRED)
	include(${QT_USE_FILE})
endif(Qt5Core_FOUND)

if (MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.11))
  # VS 2017 : Disable warnings from from gtest code, using deprecated code related to TR1 
  add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
  message(STATUS "Add flag to disable warings from gtest -  _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
endif()

include_directories (
	${CMAKE_CURRENT_SOURCE_DIR} # required for gtest-all.cpp
)

# add the executable 
add_executable (${PROJECT_NAME}
	cafPdmXml_UnitTests.cpp
	gtest/gtest-all.cpp

	cafPdmXmlBasicTest.cpp
	cafPdmAdvancedTemplateTest.cpp
	cafPdmXmlNumberTest.cpp
)

if (Qt5Core_FOUND)
    set(QT_LIBRARIES Qt5::Core Qt5::Xml)
endif()

target_link_libraries ( ${PROJECT_NAME}
	cafPdmXml
    ${QT_LIBRARIES}
    ${THREAD_LIBRARY}
)

source_group("" FILES ${PROJECT_FILES})

# Copy Qt Dlls
if (Qt5Core_FOUND)
    foreach (qtlib ${QT_LIBRARIES})
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${qtlib}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
	endforeach(qtlib)
	# Copy Qt Dlls	
else()
	# Copy Qt Dlls
	if (MSVC)
	    set (QTLIBLIST QtCore)
	    foreach (qtlib ${QTLIBLIST})
	        
	        # Debug
	        execute_process(COMMAND cmake -E copy_if_different ${QT_BINARY_DIR}/${qtlib}d4.dll ${CMAKE_BINARY_DIR}/Debug/${qtlib}d4.dll)
	        
	        # Release
	        execute_process(COMMAND cmake -E copy_if_different ${QT_BINARY_DIR}/${qtlib}4.dll ${CMAKE_BINARY_DIR}/Release/${qtlib}4.dll)
	    endforeach( qtlib )
	endif(MSVC)
endif(Qt5Core_FOUND)