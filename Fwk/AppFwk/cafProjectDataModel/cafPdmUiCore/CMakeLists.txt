cmake_minimum_required (VERSION 2.8.12)

project (cafPdmUiCore)		

# Qt
if (CAF_USE_QT5)
	find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
else()
	find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)
    include(${QT_USE_FILE})
endif(CAF_USE_QT5)


include_directories (
    ..
)

# These headers need to go through Qt's MOC compiler
set (MOC_HEADER_FILES    
    cafPdmUiEditorHandle.h
    cafPdmUiFieldEditorHandle.h
	cafPdmUiSelection3dEditorVisualizer.h
	cafQShortenedLabel.h
)

# Run MOC on the headers
add_definitions(-DCVF_USING_CMAKE)
if (CAF_USE_QT5)
	qt5_wrap_cpp(MOC_SOURCE_FILES ${MOC_HEADER_FILES} )
else()
	qt4_wrap_cpp(MOC_SOURCE_FILES ${MOC_HEADER_FILES} )
endif(CAF_USE_QT5)

set( PROJECT_FILES

    cafInternalPdmFieldTypeSpecializations.h
    cafInternalPdmUiFieldCapability.h
    cafInternalPdmUiFieldCapability.inl
    cafInternalPdmUiCommandSystemInterface.h

    cafPdmUiEditorHandle.cpp
    cafPdmUiEditorHandle.h
    cafPdmUiFieldEditorHandle.cpp
    cafPdmUiFieldEditorHandle.h
    cafPdmUiFieldHandle.cpp
    cafPdmUiFieldHandle.h
    cafPdmUiFieldSpecialization.h
    cafPdmUiItem.cpp
    cafPdmUiItem.h
    cafPdmUiModelChangeDetector.cpp
    cafPdmUiModelChangeDetector.h
    cafPdmUiObjectEditorHandle.cpp
    cafPdmUiObjectEditorHandle.h
    cafPdmUiWidgetObjectEditorHandle.cpp
    cafPdmUiWidgetObjectEditorHandle.h
    cafPdmUi3dObjectEditorHandle.cpp
    cafPdmUi3dObjectEditorHandle.h
    cafPdmUiObjectHandle.cpp
    cafPdmUiObjectHandle.h
    cafPdmUiOrdering.cpp
    cafPdmUiOrdering.h
    cafPdmUiCommandSystemProxy.cpp
    cafPdmUiCommandSystemProxy.h
    cafPdmUiTreeOrdering.cpp
    cafPdmUiTreeOrdering.h
    cafUiTreeItem.h
    cafPdmUiGroup.cpp
    cafPdmUiGroup.h

    cafSelectionManager.cpp
    cafSelectionManager.h
    cafSelectionChangedReceiver.h
    cafSelectionChangedReceiver.cpp
    cafSelectionManagerTools.h
	cafPdmUiSelection3dEditorVisualizer.h
	cafPdmUiSelection3dEditorVisualizer.cpp
	cafQShortenedLabel.cpp
	cafQShortenedLabel.h
	cafQIconProvider.cpp
	cafQIconProvider.h

)

add_library( ${PROJECT_NAME}
	${PROJECT_FILES}
	${MOC_SOURCE_FILES}
)

if (CAF_USE_QT5)
    set(QT_LIBRARIES Qt5::Core Qt5::Gui Qt5::Widgets)
endif(CAF_USE_QT5)

target_link_libraries ( ${PROJECT_NAME}
    cafPdmCore
	${QT_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (MSVC) 
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W4 /wd4100 /wd4127")
endif()

source_group("" FILES ${PROJECT_FILES})

# cotire
if (COMMAND caf_apply_cotire)
  caf_apply_cotire("${PROJECT_NAME}")
endif()
