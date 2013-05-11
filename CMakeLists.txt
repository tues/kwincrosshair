project(kwin4_effect_crosshair)

cmake_minimum_required(VERSION 2.4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

find_package(X11)
find_package(OpenGL)

find_package(KDE4)
add_definitions( ${QT_DEFINITIONS} )
add_definitions( ${KDE4_DEFINITIONS} )
include(KDE4Defaults)
include(MacroLibrary)
include_directories (${KDE4_INCLUDES})

set(KWIN_NAME "kwin")

##### BEGIN kwin/effects/CMakeLists.txt #####
# Added GL to libraries
kde4_no_enable_final(kwineffects)

macro( KWIN4_ADD_EFFECT_BACKEND name )
    kde4_add_plugin( ${name} ${ARGN} )
    target_link_libraries( ${name} kwineffects ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS} ${X11_Xfixes_LIB} ${X11_Xcursor_LIB} ${X11_LIBRARIES})
endmacro( KWIN4_ADD_EFFECT_BACKEND )

# Adds effect plugin with given name. Sources are given after the name
macro( KWIN4_ADD_EFFECT name )
    if(OPENGL_FOUND OR NOT(OPENGL_FOUND AND OPENGLES_FOUND))
        # OpenGL or neither OpenGL nor OpenGL ES - default set
        KWIN4_ADD_EFFECT_BACKEND(kwin4_effect_${name} ${ARGN})
        if(OPENGL_FOUND)
            target_link_libraries(kwin4_effect_${name} kwinglutils GL)
            set_target_properties(kwin4_effect_${name} PROPERTIES COMPILE_FLAGS -DKWIN_HAVE_OPENGL)
        elseif(OPENGLES_FOUND)
            target_link_libraries(kwin4_effect_${name} kwinglesutils)
            set_target_properties(kwin4_effect_${name} PROPERTIES COMPILE_FLAGS "-DKWIN_HAVE_OPENGL -DKWIN_HAVE_OPENGLES")
        endif(OPENGL_FOUND)
        set_target_properties(kwin4_effect_${name} PROPERTIES OUTPUT_NAME ${KWIN_NAME}4_effect_${name})
        install( TARGETS kwin4_effect_${name} DESTINATION ${PLUGIN_INSTALL_DIR} )
    endif(OPENGL_FOUND OR NOT(OPENGL_FOUND AND OPENGLES_FOUND))


    if(OPENGLES_FOUND)
        KWIN4_ADD_EFFECT_BACKEND(kwin4_effect_gles_${name} ${ARGN})
        # OpenGL ES gets into a different library
        target_link_libraries(kwin4_effect_gles_${name} kwinglesutils)
        set_target_properties(kwin4_effect_gles_${name} PROPERTIES COMPILE_FLAGS "-DKWIN_HAVE_OPENGL -DKWIN_HAVE_OPENGLES")
        set_target_properties(kwin4_effect_gles_${name} PROPERTIES OUTPUT_NAME ${KWIN_NAME}4_effect_gles_${name})
        install( TARGETS kwin4_effect_gles_${name} DESTINATION ${PLUGIN_INSTALL_DIR} )
    endif(OPENGLES_FOUND)
endmacro( KWIN4_ADD_EFFECT )

macro( KWIN4_ADD_EFFECT_CONFIG name )
    set( kwin4_effect_ui ) # Initially empty
    set( kwin4_effect_src ) # Initially empty

    foreach( file ${ARGN} )
        if( file MATCHES \\.ui )
            set( kwin4_effect_ui ${kwin4_effect_ui} ${file} )
        else( file MATCHES \\.ui )
            set( kwin4_effect_src ${kwin4_effect_src} ${file} )
        endif( file MATCHES \\.ui )
    endforeach( file )

    kde4_add_ui_files( kwin4_effect_src ${kwin4_effect_ui} )
    kde4_add_plugin( kcm_kwin4_effect_${name} ${kwin4_effect_src} )
    if(OPENGLES_FOUND)
        set_target_properties(kcm_kwin4_effect_${name} PROPERTIES COMPILE_FLAGS "-DKWIN_HAVE_OPENGL -DKWIN_HAVE_OPENGLES")
    elseif(OPENGL_FOUND)
        set_target_properties(kcm_kwin4_effect_${name} PROPERTIES COMPILE_FLAGS -DKWIN_HAVE_OPENGL)
    endif(OPENGLES_FOUND)
    target_link_libraries( kcm_kwin4_effect_${name} kwineffects ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS} )
    install( TARGETS kcm_kwin4_effect_${name} DESTINATION ${PLUGIN_INSTALL_DIR} )
endmacro( KWIN4_ADD_EFFECT_CONFIG )

macro( KWIN4_EFFECT_LINK_XRENDER name )
    if( KWIN_HAVE_XRENDER_COMPOSITING )
        target_link_libraries( kwin4_effect_${name} ${X11_Xrender_LIB} )

        # if building for OpenGL and OpenGL ES we have two targets
        # TODO: if building for OpenGL ES we should not build XRender support
        if(OPENGLES_FOUND)
            target_link_libraries( kwin4_effect_gles_${name} ${X11_Xrender_LIB} )
        endif(OPENGLES_FOUND)
    endif( KWIN_HAVE_XRENDER_COMPOSITING )
endmacro( KWIN4_EFFECT_LINK_XRENDER )
##### END kwin/effects/CMakeLists.txt #####

set( kwin4_effect_crosshair_sources
    crosshair.cpp
    )

install( FILES
    crosshair.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}/kwin )

set( kwin4_effect_crosshair_config_sources
    crosshair_config.cpp
    crosshair_config.ui
    )

install( FILES
    crosshair_config.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}/kwin )

install( FILES
    data/crosshair.png
    data/crosshair_glow.png
    DESTINATION ${DATA_INSTALL_DIR}/kwin )

KWIN4_ADD_EFFECT( crosshair ${kwin4_effect_crosshair_sources} )
if( NOT KWIN_MOBILE_EFFECTS )
    KWIN4_ADD_EFFECT_CONFIG( crosshair ${kwin4_effect_crosshair_config_sources} )
endif( NOT KWIN_MOBILE_EFFECTS )
KWIN4_EFFECT_LINK_XRENDER( crosshair )
