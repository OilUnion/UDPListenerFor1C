# Ищем uchardet библиотеку
find_path(UCHARDET_INCLUDE_DIR uchardet.h)
find_library(UCHARDET_LIBRARY uchardet)

# Если библиотека найдена, указываем пути к заголовочным файлам и библиотекам
if(UCHARDET_INCLUDE_DIR AND UCHARDET_LIBRARY)
    message(STATUS "Found uchardet library")
else()
    message(FATAL_ERROR "uchardet library not found")
endif()

# Добавляем найденную библиотеку в проект
include_directories(${UCHARDET_INCLUDE_DIR})
