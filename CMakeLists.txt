cmake_minimum_required(VERSION 2.6)

PROJECT(usbmode C)
ADD_DEFINITIONS(-Os -ggdb -Wall -Werror --std=gnu99 -Wmissing-declarations)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

SET(SOURCES main.c)
SET(LIBS ubox blobmsg_json)

IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG -g3)
ENDIF()

ADD_EXECUTABLE(usbmode ${SOURCES})
TARGET_LINK_LIBRARIES(usbmode ${LIBS})

INSTALL(TARGETS usbmode
	RUNTIME DESTINATION sbin
)
