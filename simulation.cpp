#include "simulation.h"
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkAxesActor.h>
#include <vtkNamedColors.h>
#include <vtkCubeSource.h>
#include <vtkSphereSource.h>
#include <vtkArrowSource.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCommand.h>
#include <vtkCallbackCommand.h>
#include <vtkAnimationCue.h>
#include <vtkAnimationScene.h>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vtkButtonWidget.h>
#include <vtkTexturedButtonRepresentation2D.h>
#include <vtkTextWidget.h>
#include <vtkTextRepresentation.h>
#include <vtkCoordinate.h>
#include <vtkProperty2D.h>
#include <vtkCubeAxesActor.h>

// Declare a global or class member vtkTextActor for coordinates
// To be accessed by AnimationCallback
// Better approach: Pass it via a setter to AnimationCallback instance
vtkSmartPointer<vtkTextActor> g_coordinatesActor = nullptr;

// Глобальная переменная для хранения максимальных координат, чтобы vtkCubeAxesActor мог их использовать
double max_coord_x = 10.0, max_coord_y = 10.0, max_coord_z = 10.0;

State compute_derivatives(const State& state, const Parameters& params) {
    State derivatives;
    double dvx = state.vx - params.wind_x;
    double dvy = state.vy;
    double dvz = state.vz - params.wind_z;
    double speed = std::sqrt(dvx * dvx + dvy * dvy + dvz * dvz);
    double A = 3.14 * params.radius * params.radius; // Замена M_PI на 3.14
    double k = (0.5 * params.Cd * params.air_density * A) / params.mass;

    derivatives.vx = -k * dvx * speed;
    derivatives.vy = -params.g - k * dvy * speed;
    derivatives.vz = -k * dvz * speed;
    derivatives.x = state.vx;
    derivatives.y = state.vy;
    derivatives.z = state.vz;

    return derivatives;
}

State runge_kutta_step(const State& state, const Parameters& params, double dt) {
    State k1 = compute_derivatives(state, params);
    State k2_state = {
        state.x + 0.5 * dt * k1.x,
        state.y + 0.5 * dt * k1.y,
        state.z + 0.5 * dt * k1.z,
        state.vx + 0.5 * dt * k1.vx,
        state.vy + 0.5 * dt * k1.vy,
        state.vz + 0.5 * dt * k1.vz
    };
    State k2 = compute_derivatives(k2_state, params);

    State k3_state = {
        state.x + 0.5 * dt * k2.x,
        state.y + 0.5 * dt * k2.y,
        state.z + 0.5 * dt * k2.z,
        state.vx + 0.5 * dt * k2.vx,
        state.vy + 0.5 * dt * k2.vy,
        state.vz + 0.5 * dt * k2.vz
    };
    State k3 = compute_derivatives(k3_state, params);

    State k4_state = {
        state.x + dt * k3.x,
        state.y + dt * k3.y,
        state.z + dt * k3.z,
        state.vx + dt * k3.vx,
        state.vy + dt * k3.vy,
        state.vz + dt * k3.vz
    };
    State k4 = compute_derivatives(k4_state, params);

    State new_state;
    new_state.x = state.x + (dt / 6.0) * (k1.x + 2 * k2.x + 2 * k3.x + k4.x);
    new_state.y = state.y + (dt / 6.0) * (k1.y + 2 * k2.y + 2 * k3.y + k4.y);
    new_state.z = state.z + (dt / 6.0) * (k1.z + 2 * k2.z + 2 * k3.z + k4.z);
    new_state.vx = state.vx + (dt / 6.0) * (k1.vx + 2 * k2.vx + 2 * k3.vx + k4.vx);
    new_state.vy = state.vy + (dt / 6.0) * (k1.vy + 2 * k2.vy + 2 * k3.vy + k4.vy);
    new_state.vz = state.vz + (dt / 6.0) * (k1.vz + 2 * k2.vz + 2 * k3.vz + k4.vz);

    return new_state;
}

// Класс для обработки анимации
class AnimationCallback : public vtkCommand {
public:
    static AnimationCallback* New() {
        return new AnimationCallback;
    }

    void SetSphereActor(vtkActor* actor) {
        sphereActor = actor;
    }

    void SetTrajectoryPoints(const std::vector<State>& states) {
        trajectoryStates = states;
        currentPointIndex = 0;
        maxPoints = static_cast<int>(states.size());
    }

    void SetRenderWindow(vtkRenderWindow* window) {
        renderWindow = window;
    }

    void SetSpeed(double speed) {
        animationSpeed = speed;
    }

    void SetRenderer(vtkRenderer* renderer) {
        this->renderer = renderer;
    }

    void SetCoordinatesActor(vtkTextActor* actor) {
        coordinatesActor = actor;
    }

    void InitializeTrajectoryActors() {
        // Создаем вектор для хранения акторов отрезков траектории
        trajectoryActors.clear();
        points = vtkSmartPointer<vtkPoints>::New();
        
        // Добавляем начальную точку
        points->InsertNextPoint(trajectoryStates[0].x, trajectoryStates[0].y, trajectoryStates[0].z);
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override {
        if (currentPointIndex < maxPoints) {
            const State& state = trajectoryStates[currentPointIndex];
            
            // Обновляем положение снаряда
            sphereActor->SetPosition(state.x, state.y, state.z);
            
            // Обновляем текст с координатами снаряда
            if (coordinatesActor) {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) 
                   << "X: " << state.x 
                   << " Y: " << state.y 
                   << " Z: " << state.z;
                coordinatesActor->SetInput(ss.str().c_str());
            }
            
            // Добавляем новую точку в траекторию
            points->InsertNextPoint(state.x, state.y, state.z);
            
            // Создаем полигональные данные для всей траектории до текущей точки
            auto polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->SetPoints(points);
            
            // Создаем линию для всей траектории
            auto line = vtkSmartPointer<vtkPolyLine>::New();
            line->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
            for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++) {
                line->GetPointIds()->SetId(i, i);
            }
            
            auto cells = vtkSmartPointer<vtkCellArray>::New();
            cells->InsertNextCell(line);
            polyData->SetLines(cells);
            
            // Удаляем предыдущие акторы траектории, если они есть
            for (auto actor : trajectoryActors) {
                renderer->RemoveActor(actor);
            }
            trajectoryActors.clear();
            
            // Создаем новый актор для траектории
            auto lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            lineMapper->SetInputData(polyData);
            
            auto lineActor = vtkSmartPointer<vtkActor>::New();
            lineActor->SetMapper(lineMapper);
            lineActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
            lineActor->GetProperty()->SetLineWidth(3.0);
            
            renderer->AddActor(lineActor);
            trajectoryActors.push_back(lineActor);
            
            renderWindow->Render();
            
            // Увеличиваем индекс в зависимости от скорости анимации
            currentPointIndex += static_cast<int>(animationSpeed);
            if (currentPointIndex >= maxPoints) {
                currentPointIndex = maxPoints - 1;
            }
        }
    }

private:
    vtkActor* sphereActor = nullptr;
    vtkRenderWindow* renderWindow = nullptr;
    vtkRenderer* renderer = nullptr;
    std::vector<State> trajectoryStates;
    std::vector<vtkSmartPointer<vtkActor>> trajectoryActors;
    vtkSmartPointer<vtkPoints> points;
    int currentPointIndex = 0;
    int maxPoints = 0;
    double animationSpeed = 1.0;
    vtkTextActor* coordinatesActor = nullptr; // Член класса для хранения указателя на текстовый актор координат
};

// Function to close the render window
class CloseWindowCallback : public vtkCommand {
public:
    static CloseWindowCallback* New() {
        return new CloseWindowCallback;
    }
    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override {
        if (renderWindow) {
            renderWindow->Finalize(); 
            if (interactor) {
                interactor->TerminateApp(); 
            }
        }
    }
    void SetRenderWindow(vtkRenderWindow* win) {
        renderWindow = win;
    }
    void SetInteractor(vtkRenderWindowInteractor* inter) {
		interactor = inter;
	}
private:
    vtkRenderWindow* renderWindow = nullptr;
    vtkRenderWindowInteractor* interactor = nullptr;
};

void StartSimulation(Parameters params) {
    //Parameters params = {
    //    10.0,    // mass
    //    0.47,    // Cd
    //    1.225,   // air_density
    //    0.1,     // radius
    //    9.81,    // g
    //    5.0,     // wind_x
    //    0.0,     // wind_z
    //    45.0,    // angle_deg
    //    50.0,    // initial_speed
    //    30.0     // azimuth_deg
    //};

    double angle_rad = params.angle_deg * 3.14 / 180.0; // Замена M_PI на 3.14
    double azimuth_rad = params.azimuth_deg * 3.14 / 180.0; // Замена M_PI на 3.14

    State state = {
        0.0,
        0.0,
        0.0,
        params.initial_speed * std::cos(angle_rad) * std::cos(azimuth_rad),
        params.initial_speed * std::sin(angle_rad),
        params.initial_speed * std::cos(angle_rad) * std::sin(azimuth_rad)
    };

    std::vector<State> states;
    double dt = 0.01;
    do {
        states.push_back(state);
        state = runge_kutta_step(state, params, dt);
    } while (state.y + dt * state.vy >= 0.0);

    // Находим максимальные и минимальные значения координат для настройки vtkCubeAxesActor (Шаг 1.3)
    double actual_min_x = 0.0, actual_max_x = 0.0;
    double actual_min_y = 0.0, actual_max_y = 0.0; // Y всегда от 0, но найдем макс
    double actual_min_z = 0.0, actual_max_z = 0.0;

    if (!states.empty()) {
        actual_min_x = states[0].x; actual_max_x = states[0].x;
        actual_min_y = states[0].y; actual_max_y = states[0].y; // y начинается с 0 или params.initial_height
        actual_min_z = states[0].z; actual_max_z = states[0].z;

        for (const auto& s_coord : states) {
            actual_min_x = std::min(actual_min_x, s_coord.x);
            actual_max_x = std::max(actual_max_x, s_coord.x);
            actual_min_y = std::min(actual_min_y, s_coord.y); // Хотя обычно y >= 0
            actual_max_y = std::max(actual_max_y, s_coord.y);
            actual_min_z = std::min(actual_min_z, s_coord.z);
            actual_max_z = std::max(actual_max_z, s_coord.z);
        }
    }
    // Добавляем небольшой отступ (padding), чтобы траектория не прилипала к границам
    double x_range = actual_max_x - actual_min_x;
    double y_range = actual_max_y - 0; // Y ось обычно от 0
    double z_range = actual_max_z - actual_min_z;
    double padding = std::max({x_range, y_range, z_range}) * 0.1 + 1.0; // 10% от макс. диапазона + немного

    actual_min_x -= padding;
    actual_max_x += padding;
    actual_max_y += padding; // Нижняя граница Y остается 0 (или params.initial_height если > 0)
    actual_min_z -= padding;
    actual_max_z += padding;
    
    // Убедимся, что нижняя граница Y не ниже 0
    actual_min_y = std::min(0.0, actual_min_y); 

    // Создание точек траектории
    auto points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& s : states) {
        points->InsertNextPoint(s.x, s.y, s.z);
    }

    // Создание линии траектории
    auto trajectory = vtkSmartPointer<vtkPolyLine>::New();
    trajectory->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
    for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++) {
        trajectory->GetPointIds()->SetId(i, i);
    }

    // Создание полигональных данных
    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(trajectory);
    polyData->SetLines(cells);

    // Визуализация траектории
    auto trajectoryMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    trajectoryMapper->SetInputData(polyData);

    auto trajectoryActor = vtkSmartPointer<vtkActor>::New();
    trajectoryActor->SetMapper(trajectoryMapper);
    trajectoryActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    trajectoryActor->GetProperty()->SetLineWidth(3.0);

    // Создание снаряда
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetRadius(0.2);
    sphere->SetCenter(states.back().x, states.back().y, states.back().z);

    auto sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphere->GetOutputPort());

    auto sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetColor(0.0, 0.0, 1.0);

    // Создание земли
    auto ground = vtkSmartPointer<vtkCubeSource>::New();
    ground->SetXLength(100);
    ground->SetYLength(0.1);
    ground->SetZLength(100);

    auto groundMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    groundMapper->SetInputConnection(ground->GetOutputPort());

    auto groundActor = vtkSmartPointer<vtkActor>::New();
    groundActor->SetMapper(groundMapper);
    groundActor->GetProperty()->SetColor(0.5, 0.5, 0.5);

    // Визуализация ветра
    auto windArrow = vtkSmartPointer<vtkArrowSource>::New();
    windArrow->SetTipLength(0.5);
    windArrow->SetTipRadius(0.1);
    windArrow->SetShaftRadius(0.05);

    auto windMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    windMapper->SetInputConnection(windArrow->GetOutputPort());

    auto windActor = vtkSmartPointer<vtkActor>::New();
    windActor->SetMapper(windMapper);
    windActor->SetPosition(5, 0, 0); // Смещение для видимости
    windActor->SetScale(params.wind_x, 1, params.wind_z);
    windActor->SetOrientation(0, 0, 45); // Пример поворота
    windActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
    windActor->GetProperty()->SetOpacity(0.5);

    // Добавление осей координат
    // auto axes = vtkSmartPointer<vtkAxesActor>::New();
    // axes->SetTotalLength(15, 15, 15);  // Увеличиваем длину осей
    // axes->AxisLabelsOff(); // Убираем отключение меток, чтобы они отображались
    // Настройка меток осей (можно настроить цвет, шрифт и т.д. при необходимости)
    // axes->SetXAxisLabelText("X");
    // axes->SetYAxisLabelText("Y");
    // axes->SetZAxisLabelText("Z");

    // Создание текстовых меток
    auto CreateTextActor = [](const std::string& text, int y_pos) {
        auto actor = vtkSmartPointer<vtkTextActor>::New();
        actor->SetInput(text.c_str());
        actor->SetPosition(10, y_pos);
        actor->GetTextProperty()->SetFontSize(20);
        actor->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
        return actor;
    };

    double max_height = 0.0;
    for (const auto& s : states) {
        if (s.y > max_height) max_height = s.y;
    }
    double flight_time = (states.size() - 1) * dt;
    
    // Вычисляем дальность полета
    double distance_x = std::abs(states.back().x - states.front().x);
    double distance_z = std::abs(states.back().z - states.front().z);
    double total_distance = std::sqrt(distance_x * distance_x + distance_z * distance_z);

    // Поднимаем все тексты выше, чтобы они были видны


    // Настройка рендерера
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(trajectoryActor);
    renderer->AddActor(sphereActor);
    renderer->AddActor(groundActor);
    renderer->AddActor(windActor);
    // renderer->AddActor(axes); // Закомментировано
    renderer->SetBackground(1.0, 1.0, 1.0); // Белый фон

    // Создание и настройка vtkCubeAxesActor (Шаг 1.2)
    auto cubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    cubeAxesActor->SetCamera(renderer->GetActiveCamera()); // Используем камеру рендерера
    
    // Настройка внешнего вида (цвета можно взять из colors, если vtkNamedColors используется)
    // Для примера, пока зададим черным цветом
    cubeAxesActor->GetTitleTextProperty(0)->SetColor(0.0, 0.0, 0.0);
    cubeAxesActor->GetLabelTextProperty(0)->SetColor(0.0, 0.0, 0.0);
    cubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 0.0, 0.0);
    cubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 0.0, 0.0);
    cubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.0, 0.0);
    cubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.0, 0.0);

    cubeAxesActor->SetXTitle("X (m)");
    cubeAxesActor->SetYTitle("Y (m)");
    cubeAxesActor->SetZTitle("Z (m)");

    // Обновляем границы vtkCubeAxesActor (Шаг 1.3)
    cubeAxesActor->SetBounds(actual_min_x, actual_max_x, actual_min_y, actual_max_y, actual_min_z, actual_max_z); 

    // Настройка отображения сетки (Шаг 1.4)
    cubeAxesActor->SetFlyModeToOuterEdges(); // Режим отображения осей
    cubeAxesActor->DrawXGridlinesOn();
    cubeAxesActor->DrawYGridlinesOn();
    cubeAxesActor->DrawZGridlinesOn();
    // Установка цвета сетки (альтернативный способ)
    cubeAxesActor->GetXAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный
    cubeAxesActor->GetYAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный
    cubeAxesActor->GetZAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный
    // cubeAxesActor->XAxisMinorTickVisibilityOff(); // Опционально: убрать мелкие деления
    // cubeAxesActor->YAxisMinorTickVisibilityOff();
    // cubeAxesActor->ZAxisMinorTickVisibilityOff();

    renderer->AddActor(cubeAxesActor); // Добавляем cubeAxesActor к рендереру

    // Add "3D Simulation" label
    auto simulationLabel = vtkSmartPointer<vtkTextActor>::New();
    simulationLabel->SetInput("3D Simulation");
    simulationLabel->GetTextProperty()->SetFontSize(24);
    simulationLabel->GetTextProperty()->SetColor(0.0, 0.0, 0.0); // Black color
    simulationLabel->GetTextProperty()->SetJustificationToCentered();
    // Position label at the top-center of the window. Adjust X as needed.
    // Assuming window width is 1200, Y position 750 is near the top.
    simulationLabel->SetPosition(600, 750); 
    renderer->AddActor2D(simulationLabel);
    
    // Настройка окна
    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(1200, 800);
    renderWindow->SetWindowName("3D Projectile Trajectory");
    renderWindow->Render();

    // Сброс камеры для охвата всей сцены (Шаг 2)
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();

    // Интерактивный режим
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    // Create "Back to Menu" button
    auto textRepresentation = vtkSmartPointer<vtkTextRepresentation>::New();
    textRepresentation->SetText("Back to Menu");
    // Position and size the button in display coordinates
    textRepresentation->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
    textRepresentation->GetPositionCoordinate()->SetValue(20, 20); // Bottom-left corner of the button
    textRepresentation->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
    textRepresentation->GetPosition2Coordinate()->SetValue(170, 60); // Top-right corner of the button

    textRepresentation->GetTextActor()->GetTextProperty()->SetFontSize(18);
    textRepresentation->GetTextActor()->GetTextProperty()->SetColor(0.1, 0.1, 0.1); // Dark grey text
    textRepresentation->GetTextActor()->GetTextProperty()->SetJustificationToCentered();
    textRepresentation->GetTextActor()->GetTextProperty()->SetVerticalJustificationToCentered();
    textRepresentation->GetBorderProperty()->SetColor(0.2, 0.2, 0.2); // Darker grey border
    textRepresentation->SetShowBorder(true);

    auto buttonWidget = vtkSmartPointer<vtkTextWidget>::New();
    buttonWidget->SetRepresentation(textRepresentation);
    buttonWidget->SetInteractor(interactor);
    buttonWidget->SetSelectable(true); 
    
    auto closeCallback = vtkSmartPointer<CloseWindowCallback>::New();
    closeCallback->SetRenderWindow(renderWindow);
    closeCallback->SetInteractor(interactor);
    buttonWidget->AddObserver(vtkCommand::EndInteractionEvent, closeCallback); // Use EndInteractionEvent

    buttonWidget->On();

    // // Создаем и настраиваем обработчик анимации
    // auto animationCallback = vtkSmartPointer<AnimationCallback>::New();
    // animationCallback->SetSphereActor(sphereActor);
    // animationCallback->SetTrajectoryPoints(states);
    // animationCallback->SetRenderWindow(renderWindow);
    // animationCallback->SetRenderer(renderer);
    // animationCallback->SetSpeed(2.0); // Скорость анимации
    // animationCallback->InitializeTrajectoryActors(); // Инициализируем акторы траектории

    // // Добавляем обработчик таймера
    // interactor->Initialize(); // This might be needed for the button, let's test. If button fails, uncomment this and comment out the next two.
    // interactor->AddObserver(vtkCommand::TimerEvent, animationCallback);
    // int timerId = interactor->CreateRepeatingTimer(30); // 30 мс между кадрами (примерно 33 кадра в секунду)

    // Запускаем интерактор (должен быть после инициализации, если она нужна)
    interactor->Initialize(); // Ensure interactor is initialized for the button to work.
    interactor->Start();
}
 
void StartAnimatedSimulation(Parameters params) {
    double angle_rad = params.angle_deg * 3.14 / 180.0;
    double azimuth_rad = params.azimuth_deg * 3.14 / 180.0;

    State state = {
        0.0,
        0.0,
        0.0,
        params.initial_speed * std::cos(angle_rad) * std::cos(azimuth_rad),
        params.initial_speed * std::sin(angle_rad),
        params.initial_speed * std::cos(angle_rad) * std::sin(azimuth_rad)
    };

    std::vector<State> states;
    double dt = 0.01;
    do {
        states.push_back(state);
        state = runge_kutta_step(state, params, dt);
    } while (state.y + dt * state.vy >= 0.0);

    // Находим максимальные и минимальные значения координат для настройки vtkCubeAxesActor (Шаг 2.2)
    double anim_min_x = 0.0, anim_max_x = 0.0;
    double anim_min_y = 0.0, anim_max_y = 0.0; 
    double anim_min_z = 0.0, anim_max_z = 0.0;

    if (!states.empty()) {
        anim_min_x = states[0].x; anim_max_x = states[0].x;
        anim_min_y = states[0].y; anim_max_y = states[0].y;
        anim_min_z = states[0].z; anim_max_z = states[0].z;

        for (const auto& s_coord : states) {
            anim_min_x = std::min(anim_min_x, s_coord.x);
            anim_max_x = std::max(anim_max_x, s_coord.x);
            anim_min_y = std::min(anim_min_y, s_coord.y);
            anim_max_y = std::max(anim_max_y, s_coord.y);
            anim_min_z = std::min(anim_min_z, s_coord.z);
            anim_max_z = std::max(anim_max_z, s_coord.z);
        }
    }
    double x_range_anim = anim_max_x - anim_min_x;
    double y_range_anim = anim_max_y - 0; 
    double z_range_anim = anim_max_z - anim_min_z;
    double padding_anim = std::max({x_range_anim, y_range_anim, z_range_anim}) * 0.1 + 1.0; 

    anim_min_x -= padding_anim;
    anim_max_x += padding_anim;
    anim_max_y += padding_anim; 
    anim_min_z -= padding_anim;
    anim_max_z += padding_anim;
    anim_min_y = std::min(0.0, anim_min_y);

    // Создание снаряда
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetRadius(0.2);
    sphere->SetCenter(states[0].x, states[0].y, states[0].z); // Начальная позиция

    auto sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphere->GetOutputPort());

    auto sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper(sphereMapper);
    sphereActor->GetProperty()->SetColor(0.0, 0.0, 1.0);

    // Создание земли
    auto ground = vtkSmartPointer<vtkCubeSource>::New();
    ground->SetXLength(100);
    ground->SetYLength(0.1);
    ground->SetZLength(100);

    auto groundMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    groundMapper->SetInputConnection(ground->GetOutputPort());

    auto groundActor = vtkSmartPointer<vtkActor>::New();
    groundActor->SetMapper(groundMapper);
    groundActor->GetProperty()->SetColor(0.5, 0.5, 0.5);

    // Визуализация ветра
    auto windArrow = vtkSmartPointer<vtkArrowSource>::New();
    windArrow->SetTipLength(0.5);
    windArrow->SetTipRadius(0.1);
    windArrow->SetShaftRadius(0.05);

    auto windMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    windMapper->SetInputConnection(windArrow->GetOutputPort());

    auto windActor = vtkSmartPointer<vtkActor>::New();
    windActor->SetMapper(windMapper);
    windActor->SetPosition(5, 0, 0); // Смещение для видимости
    windActor->SetScale(params.wind_x, 1, params.wind_z);
    windActor->SetOrientation(0, 0, 45); // Пример поворота
    windActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
    windActor->GetProperty()->SetOpacity(0.5);

    // Добавление осей координат
    // auto axes = vtkSmartPointer<vtkAxesActor>::New();
    // axes->SetTotalLength(15, 15, 15);  // Увеличиваем длину осей
    
    // Настройка меток осей
    // axes->AxisLabelsOn(); 
    // axes->SetXAxisLabelText("X");
    // axes->SetYAxisLabelText("Y");
    // axes->SetZAxisLabelText("Z");

    // Создание текстовых меток
    auto CreateTextActor = [](const std::string& text, int y_pos) {
        auto actor = vtkSmartPointer<vtkTextActor>::New();
        actor->SetInput(text.c_str());
        actor->SetPosition(10, y_pos);
        actor->GetTextProperty()->SetFontSize(20);
        actor->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
        return actor;
    };

    double max_height = 0.0;
    for (const auto& s : states) {
        if (s.y > max_height) max_height = s.y;
    }
    double flight_time = (states.size() - 1) * dt;
    
    // Вычисляем дальность полета
    double distance_x = std::abs(states.back().x - states.front().x);
    double distance_z = std::abs(states.back().z - states.front().z);
    double total_distance = std::sqrt(distance_x * distance_x + distance_z * distance_z);

    // Поднимаем все тексты выше, чтобы они были видны


    // Создание текстового актора для координат снаряда (для анимации)
    g_coordinatesActor = vtkSmartPointer<vtkTextActor>::New();
    g_coordinatesActor->GetTextProperty()->SetFontSize(18);
    g_coordinatesActor->GetTextProperty()->SetColor(0.0, 0.0, 0.0); // Черный цвет
    g_coordinatesActor->SetPosition(20, 80); // Позиция в левом нижнем углу, выше кнопки "Back to menu" и ниже других текстов
    g_coordinatesActor->SetInput("X: 0.00 Y: 0.00 Z: 0.00"); // Начальный текст

    // Настройка рендерера
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(sphereActor);
    renderer->AddActor(groundActor);
    renderer->AddActor(windActor);
    // renderer->AddActor(axes); // Закомментировано (или удалить, если точно не нужен)
    renderer->SetBackground(1.0, 1.0, 1.0); // Белый фон

    // Удаляем некорректно добавленный ранее cubeAxesActor, чтобы избежать дублирования
    // Этот блок нужно будет найти и удалить или изменить, если он был создан ранее в этой функции
    // ОБРАТИТЕ ВНИМАНИЕ: если предыдущий шаг добавил cubeAxesActor в StartAnimatedSimulation,
    // этот код нужно адаптировать, чтобы не создавать его дважды, а обновить существующий.
    // На данном этапе предполагается, что мы его создаем заново здесь корректно.

    // Создание и настройка vtkCubeAxesActor для анимированной симуляции (Шаг 2.2)
    auto animCubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
    animCubeAxesActor->SetCamera(renderer->GetActiveCamera());
    
    animCubeAxesActor->GetTitleTextProperty(0)->SetColor(0.0, 0.0, 0.0);
    animCubeAxesActor->GetLabelTextProperty(0)->SetColor(0.0, 0.0, 0.0);
    animCubeAxesActor->GetTitleTextProperty(1)->SetColor(0.0, 0.0, 0.0);
    animCubeAxesActor->GetLabelTextProperty(1)->SetColor(0.0, 0.0, 0.0);
    animCubeAxesActor->GetTitleTextProperty(2)->SetColor(0.0, 0.0, 0.0);
    animCubeAxesActor->GetLabelTextProperty(2)->SetColor(0.0, 0.0, 0.0);

    animCubeAxesActor->SetXTitle("X (m)");
    animCubeAxesActor->SetYTitle("Y (m)");
    animCubeAxesActor->SetZTitle("Z (m)");

    animCubeAxesActor->SetBounds(anim_min_x, anim_max_x, anim_min_y, anim_max_y, anim_min_z, anim_max_z);
    
    animCubeAxesActor->SetFlyModeToOuterEdges();
    animCubeAxesActor->DrawXGridlinesOn();
    animCubeAxesActor->DrawYGridlinesOn();
    animCubeAxesActor->DrawZGridlinesOn();
    // Установка цвета сетки (альтернативный способ)
    animCubeAxesActor->GetXAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный
    animCubeAxesActor->GetYAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный
    animCubeAxesActor->GetZAxesGridlinesProperty()->SetColor(0.0, 0.0, 0.0); // Черный

    renderer->AddActor(animCubeAxesActor); // Добавляем настроенный cubeAxesActor
    renderer->AddActor2D(g_coordinatesActor); // Убедимся, что это добавлено ПОСЛЕ настройки renderer

    // Add "3D Simulation" label
    auto simulationLabel = vtkSmartPointer<vtkTextActor>::New();
    simulationLabel->SetInput("3D Simulation");
    simulationLabel->GetTextProperty()->SetFontSize(24);
    simulationLabel->GetTextProperty()->SetColor(0.0, 0.0, 0.0); // Black color
    simulationLabel->GetTextProperty()->SetJustificationToCentered();
    // Position label at the top-center of the window. Adjust X as needed.
    simulationLabel->SetPosition(600, 750);
    renderer->AddActor2D(simulationLabel);
    
    // Настройка окна
    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(1200, 800);
    renderWindow->SetWindowName("3D Projectile Trajectory Animation");
    renderWindow->Render();

    // Сброс камеры для охвата всей сцены (Шаг 2)
    renderer->ResetCamera();
    renderer->ResetCameraClippingRange();

    // Интерактивный режим
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    // Create "Back to Menu" button
    auto textRepresentation = vtkSmartPointer<vtkTextRepresentation>::New();
    textRepresentation->SetText("Back to Menu");
    // Position and size the button in display coordinates
    textRepresentation->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
    textRepresentation->GetPositionCoordinate()->SetValue(20, 20); // Bottom-left corner of the button
    textRepresentation->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
    textRepresentation->GetPosition2Coordinate()->SetValue(170, 60); // Top-right corner of the button

    textRepresentation->GetTextActor()->GetTextProperty()->SetFontSize(18);
    textRepresentation->GetTextActor()->GetTextProperty()->SetColor(0.1, 0.1, 0.1); // Dark grey text
    textRepresentation->GetTextActor()->GetTextProperty()->SetJustificationToCentered();
    textRepresentation->GetTextActor()->GetTextProperty()->SetVerticalJustificationToCentered();
    textRepresentation->GetBorderProperty()->SetColor(0.2, 0.2, 0.2); // Darker grey border
    textRepresentation->SetShowBorder(true);

    auto buttonWidget = vtkSmartPointer<vtkTextWidget>::New();
    buttonWidget->SetRepresentation(textRepresentation);
    buttonWidget->SetInteractor(interactor);
    buttonWidget->SetSelectable(true); 

    auto closeCallback = vtkSmartPointer<CloseWindowCallback>::New();
    closeCallback->SetRenderWindow(renderWindow);
    closeCallback->SetInteractor(interactor);
    buttonWidget->AddObserver(vtkCommand::EndInteractionEvent, closeCallback); // Use EndInteractionEvent

    buttonWidget->On();

    // Создаем и настраиваем обработчик анимации
    auto animationCallback = vtkSmartPointer<AnimationCallback>::New();
    animationCallback->SetSphereActor(sphereActor);
    animationCallback->SetTrajectoryPoints(states);
    animationCallback->SetRenderWindow(renderWindow);
    animationCallback->SetRenderer(renderer);
    animationCallback->SetSpeed(2.0); // Скорость анимации
    animationCallback->InitializeTrajectoryActors(); // Инициализируем акторы траектории
    animationCallback->SetCoordinatesActor(g_coordinatesActor); // Передаем актор координат в callback

    // Добавляем обработчик таймера
    interactor->Initialize();
    interactor->AddObserver(vtkCommand::TimerEvent, animationCallback);
    int timerId = interactor->CreateRepeatingTimer(30); // 30 мс между кадрами (примерно 33 кадра в секунду)

    // Запускаем интерактор
    interactor->Start();
}
