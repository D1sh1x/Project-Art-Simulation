# Симуляция траектории артиллерийского снаряда

Проект для моделирования и визуализации траектории полета артиллерийского снаряда с учетом сопротивления воздуха и ветра.

## Инструкция по сборке проекта со статическими библиотеками

Для сборки проекта с минимальным количеством DLL-зависимостей, следуйте этим инструкциям:

### Предварительные требования

1. CMake 3.10 или выше
2. Компилятор C++ с поддержкой C++11 (MSVC, GCC, Clang)
3. Qt 5.12 или выше
4. VTK 9.0 или выше

### Шаги по сборке со статическими библиотеками

#### 1. Сборка статической версии Qt

1. Скачайте исходный код Qt с [официального сайта](https://www.qt.io/download)
2. Распакуйте архив и перейдите в директорию с исходным кодом
3. Настройте сборку статической версии Qt:

```bash
# Windows
configure -static -release -prefix C:\Qt\static -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -opengl desktop -no-openssl -opensource -confirm-license -make libs -nomake tools -nomake examples -nomake tests

# Linux/macOS
./configure -static -release -prefix /opt/Qt/static -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -no-openssl -opensource -confirm-license -make libs -nomake tools -nomake examples -nomake tests
```

4. Выполните сборку:

```bash
# Windows
nmake
nmake install

# Linux/macOS
make -j$(nproc)
make install
```

#### 2. Сборка статической версии VTK

1. Скачайте исходный код VTK с [официального сайта](https://vtk.org/download/)
2. Создайте директорию для сборки и перейдите в нее:

```bash
mkdir vtk-build
cd vtk-build
```

3. Настройте сборку статической версии VTK с помощью CMake:

```bash
# Windows
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DVTK_GROUP_ENABLE_Qt=YES -DVTK_QT_VERSION=5 -DQt5_DIR=C:\Qt\static\lib\cmake\Qt5 -DCMAKE_INSTALL_PREFIX=C:\VTK\static ..\VTK-9.x.x

# Linux/macOS
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DVTK_GROUP_ENABLE_Qt=YES -DVTK_QT_VERSION=5 -DQt5_DIR=/opt/Qt/static/lib/cmake/Qt5 -DCMAKE_INSTALL_PREFIX=/opt/VTK/static ../VTK-9.x.x
```

4. Выполните сборку:

```bash
# Windows
cmake --build . --config Release --target INSTALL

# Linux/macOS
make -j$(nproc)
make install
```

#### 3. Сборка проекта симуляции

1. Создайте файл CMakeLists.txt в корневой директории проекта:

```cmake
cmake_minimum_required(VERSION 3.10)
project(ArtillerySimulation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Статическая сборка с MSVC
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

# Пути к статическим библиотекам Qt и VTK
# Windows
if(WIN32)
    set(Qt5_DIR "C:/Qt/static/lib/cmake/Qt5")
    set(VTK_DIR "C:/VTK/static/lib/cmake/vtk-9.x")
else()
    # Linux/macOS
    set(Qt5_DIR "/opt/Qt/static/lib/cmake/Qt5")
    set(VTK_DIR "/opt/VTK/static/lib/cmake/vtk-9.x")
endif()

# Найти пакеты
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(VTK REQUIRED)

# Включить автоматическую обработку Qt (moc, uic, rcc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Исходные файлы проекта
set(SOURCES
    main.cpp
    mainwindow.cpp
    simulation.cpp
)

set(HEADERS
    mainwindow.h
    simulation.h
)

# Создание исполняемого файла
add_executable(ArtillerySimulation ${SOURCES} ${HEADERS})

# Связывание с библиотеками
target_link_libraries(ArtillerySimulation PRIVATE Qt5::Core Qt5::Widgets ${VTK_LIBRARIES})

# Включить модули VTK
vtk_module_autoinit(
    TARGETS ArtillerySimulation
    MODULES ${VTK_MODULES_USED}
)

# Настройка для создания автономного исполняемого файла
if(WIN32)
    # Для Windows используем статическую линковку рантайма
    set_target_properties(ArtillerySimulation PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Копирование необходимых DLL файлов (если остались зависимости)
if(WIN32)
    # Настройка установки
    install(TARGETS ArtillerySimulation DESTINATION .)
    
    # Использование windeployqt для сборки зависимостей
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ArtillerySimulation POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler "$<TARGET_FILE:ArtillerySimulation>"
            COMMENT "Running windeployqt..."
        )
    endif()
endif()
```

2. Создайте директорию для сборки и перейдите в нее:

```bash
mkdir build
cd build
```

3. Настройте сборку проекта:

```bash
# Windows
cmake -DCMAKE_BUILD_TYPE=Release ..

# Linux/macOS
cmake -DCMAKE_BUILD_TYPE=Release ..
```

4. Выполните сборку:

```bash
# Windows
cmake --build . --config Release

# Linux/macOS
make -j$(nproc)
```

### Альтернативный подход: использование статической сборки с MinGW

Для Windows можно использовать MinGW для создания полностью статического исполняемого файла:

1. Установите MinGW-w64
2. Соберите Qt и VTK с MinGW
3. Добавьте в CMakeLists.txt:

```cmake
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -s")
endif()
```

### Проверка зависимостей

После сборки проекта вы можете проверить зависимости исполняемого файла:

- Windows: используйте утилиту Dependency Walker
- Linux: `ldd ./ArtillerySimulation`
- macOS: `otool -L ./ArtillerySimulation`

## Минимизация зависимостей от DLL

Если полностью статическая сборка невозможна, вы можете минимизировать количество DLL-файлов:

1. Используйте статическую линковку рантайма C++ (опция `/MT` для MSVC)
2. Отключите ненужные функции Qt и VTK при конфигурации
3. Используйте инструменты для анализа и уменьшения зависимостей:
   - UPX для сжатия исполняемого файла
   - Dependency Walker для анализа зависимостей

## Распространение приложения

Для распространения приложения с минимальным количеством DLL:

1. Используйте инструмент windeployqt для Qt-приложений
2. Создайте установщик с NSIS или WiX, который включит только необходимые DLL
3. Рассмотрите возможность использования AppImage (Linux) или создания DMG (macOS)

## Дополнительные советы

- Отключите отладочную информацию в релизной сборке
- Используйте оптимизации компилятора для уменьшения размера
- Рассмотрите возможность использования Qt Quick/QML вместо Qt Widgets для уменьшения зависимостей
- Для VTK отключите ненужные модули, чтобы уменьшить количество зависимостей 