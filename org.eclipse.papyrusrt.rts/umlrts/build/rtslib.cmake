# rts library

# RTS value must match LIBRARY value in library's CMakelists.txt file
set(RTS rts)
set(NANO nanomsg)
set(FLAT flatbuffers)
set(FBSCHEMAS fbschemas)

set(CMAKE_CXX_STANDARD 11)

# building library
if (RTS STREQUAL LIBRARY)

  # library sources
  set(SRCS)
  if (NOT OS_FILES_SDIR)
    set(OS_FILES_SDIR ${TARGETOS})
  endif ()
  set(FILES_CONFIG_DIR ${UMLRTS_ROOT}/build/os)
  include(${FILES_CONFIG_DIR}/files.cmake)
  set(FILES_CONFIG_DIR ${UMLRTS_ROOT}/build/os/${OS_FILES_SDIR})
  include(${FILES_CONFIG_DIR}/files.cmake)
else ()

  # model project depends

  # add external project support
  include(ExternalProject)

  ExternalProject_Add(${NANO}
    SOURCE_DIR ${UMLRTS_ROOT}/${NANO}
    DOWNLOAD_COMMAND ""
    BUILD_COMMAND ""
    UPDATE_COMMAND ""
    CMAKE_ARGS "${CMAKE_ARGS};-DCMAKE_INSTALL_LIBDIR=${UMLRTS_ROOT}/lib/${TARGETOS}/${BUILD_TOOLS}/${BUILD_CONFIG}/;-DCMAKE_INSTALL_INCLUDEDIR=${UMLRTS_ROOT}/include"
    INSTALL_COMMAND
            ${CMAKE_COMMAND}
            --build .
            --target install
            --config ${configuration}
    )

  ExternalProject_Add(${FLAT}
    SOURCE_DIR ${UMLRTS_ROOT}/flatbuffers
    DOWNLOAD_COMMAND ""
    BUILD_COMMAND ""
    UPDATE_COMMAND ""
    CMAKE_ARGS "${CMAKE_ARGS};-DCMAKE_INSTALL_LIBDIR=${UMLRTS_ROOT}/lib/${TARGETOS}/${BUILD_TOOLS}/${BUILD_CONFIG}/;-DCMAKE_INSTALL_INCLUDEDIR=${UMLRTS_ROOT}/include;-DCMAKE_INSTALL_BINDIR=${UMLRTS_ROOT}/bin"
    INSTALL_COMMAND
            ${CMAKE_COMMAND}
            --build .
            --target install
            --config ${configuration}
    )

  # RTS build/install
  ExternalProject_Add(${RTS}
    SOURCE_DIR ${UMLRTS_ROOT}
    DOWNLOAD_COMMAND ""
    BUILD_COMMAND ""
    UPDATE_COMMAND ""
    INSTALL_COMMAND
            ${CMAKE_COMMAND}
            --build .
            --target install
            --config ${configuration}
    )

  add_custom_command(
    OUTPUT  fbschemasc.stamp
    DEPENDS ${FLAT}
    COMMAND ${UMLRTS_ROOT}/bin/flatc -o ${UMLRTS_ROOT}/include/fbschemas  -c --include-prefix fbschemas ${UMLRTS_ROOT}/fbschemas/*.fbs
    COMMAND ${CMAKE_COMMAND} -E touch fbschemasc.stamp
  )

  add_custom_target(${FBSCHEMAS} DEPENDS fbschemasc.stamp)
  add_dependencies(${RTS} ${NANO} ${FLAT} ${FBSCHEMAS})
  add_dependencies(${TARGET} ${RTS})

  # Destination directory for the RTS services library.
  set(RTS_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${RTS}${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(RTS_LIB ${UMLRTS_ROOT}/lib/${TARGETOS}/${BUILD_TOOLS}/${BUILD_CONFIG}/${RTS_NAME})

  # Destination directory for the nanomsg library.
  set(NANO_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${NANO}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(NANO_LIB ${UMLRTS_ROOT}/lib/${TARGETOS}/${BUILD_TOOLS}/${BUILD_CONFIG}/${NANO_NAME})

  # Destination directory for the faltbuffers library.
  set(FLAT_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${FLAT}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(FLAT_LIB ${UMLRTS_ROOT}/lib/${TARGETOS}/${BUILD_TOOLS}/${BUILD_CONFIG}/${FLAT_NAME})

  set(INCS
    ${INCS}
    ${UMLRTS_ROOT}/include
    )
  set(LIBS
    ${LIBS}
    ${RTS_LIB}
    ${NANO_LIB}
    ${FLAT_LIB}
    )

  # install flatbuffer schemas
  file(COPY ${UMLRTS_ROOT}/fbschemas DESTINATION ${CMAKE_BINARY_DIR})

endif ()

# reorder lib list placing object files upfront
set(OLIST)
set(SLIST)
foreach(ITEM  ${LIBS})
  if (${ITEM} MATCHES "\\.[ao]$")
    list(APPEND OLIST ${ITEM})
  else ()
    list(APPEND SLIST ${ITEM})
  endif ()
endforeach (ITEM)
set(LIBS ${OLIST} ${SLIST})
set(OLIST)
set(SLIST)
