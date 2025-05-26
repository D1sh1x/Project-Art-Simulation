cmake_minimum_required(VERSION 3.15)
project(ProjectileTrajectory)

# Настройка C++ стандарта
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # Для Qt MOC
set(CMAKE_AUTORCC ON)  # Для Qt ресурсов
set(CMAKE_AUTOUIC ON)  # Для Qt UI файлов

# Поиск Qt
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
)

# Поиск VTK
find_package(VTK REQUIRED
    COMPONENTS
        CommonCore
        CommonDataModel
        CommonColor
        FiltersSources
        RenderingCore
        RenderingOpenGL2
        RenderingFreeType
        RenderingContextOpenGL2
        RenderingGL2PSOpenGL2
        InteractionStyle
        IOXML
        RenderingAnnotation
)

# Добавление исполняемого файла
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    simulation.cpp
    simulation.h
    parameters.h
)

# Линковка с Qt
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Линковка с VTK
target_link_libraries(${PROJECT_NAME} PRIVATE
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::CommonColor
    VTK::FiltersSources
    VTK::RenderingCore
    VTK::RenderingOpenGL2
    VTK::RenderingContextOpenGL2
    VTK::RenderingGL2PSOpenGL2
    VTK::RenderingFreeType
    VTK::InteractionStyle
    VTK::IOXML
    VTK::RenderingAnnotation
)

# Автоинициализация VTK модулей
vtk_module_autoinit(
    TARGETS ${PROJECT_NAME}
    MODULES ${VTK_LIBRARIES}
)
