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
    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(15, 15, 15);  // Увеличиваем длину осей
    
    // Упрощенная настройка осей
    axes->AxisLabelsOff();  // Отключаем метки осей для простоты

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
    auto textMaxHeight = CreateTextActor("Max Height: " + std::to_string(max_height) + " m", 160);
    auto textFlightTime = CreateTextActor("Flight Time: " + std::to_string(flight_time) + " s", 140);
    auto textDistanceX = CreateTextActor("Distance X: " + std::to_string(distance_x) + " m", 120);
    auto textDistanceZ = CreateTextActor("Distance Z: " + std::to_string(distance_z) + " m", 100);
    auto textTotalDistance = CreateTextActor("Total Distance: " + std::to_string(total_distance) + " m", 80);
    auto textWindX = CreateTextActor("Wind X: " + std::to_string(params.wind_x) + " m/s", 60);
    auto textWindZ = CreateTextActor("Wind Z: " + std::to_string(params.wind_z) + " m/s", 40);
    auto textMass = CreateTextActor("Mass: " + std::to_string(params.mass) + " kg", 20);
    auto textRadius = CreateTextActor("Radius: " + std::to_string(params.radius) + " m", 0);

    // Настройка рендерера
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(trajectoryActor);
    renderer->AddActor(sphereActor);
    renderer->AddActor(groundActor);
    renderer->AddActor(windActor);
    renderer->AddActor(axes);
    renderer->AddActor2D(textMaxHeight);
    renderer->AddActor2D(textFlightTime);
    renderer->AddActor2D(textDistanceX);
    renderer->AddActor2D(textDistanceZ);
    renderer->AddActor2D(textTotalDistance);
    renderer->AddActor2D(textWindX);
    renderer->AddActor2D(textWindZ);
    renderer->AddActor2D(textMass);
    renderer->AddActor2D(textRadius);
    renderer->SetBackground(1.0, 1.0, 1.0); // Белый фон
    
    // Настройка окна
    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(1200, 800);
    renderWindow->SetWindowName("3D Projectile Trajectory");
    renderWindow->Render();

    // Интерактивный режим
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    interactor->Initialize();
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
    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(15, 15, 15);  // Увеличиваем длину осей
    
    // Упрощенная настройка осей
    axes->AxisLabelsOff();  // Отключаем метки осей для простоты

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
    auto textMaxHeight = CreateTextActor("Max Height: " + std::to_string(max_height) + " m", 160);
    auto textFlightTime = CreateTextActor("Flight Time: " + std::to_string(flight_time) + " s", 140);
    auto textDistanceX = CreateTextActor("Distance X: " + std::to_string(distance_x) + " m", 120);
    auto textDistanceZ = CreateTextActor("Distance Z: " + std::to_string(distance_z) + " m", 100);
    auto textTotalDistance = CreateTextActor("Total Distance: " + std::to_string(total_distance) + " m", 80);
    auto textWindX = CreateTextActor("Wind X: " + std::to_string(params.wind_x) + " m/s", 60);
    auto textWindZ = CreateTextActor("Wind Z: " + std::to_string(params.wind_z) + " m/s", 40);
    auto textMass = CreateTextActor("Mass: " + std::to_string(params.mass) + " kg", 20);
    auto textRadius = CreateTextActor("Radius: " + std::to_string(params.radius) + " m", 0);


    // Настройка рендерера
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(sphereActor);
    renderer->AddActor(groundActor);
    renderer->AddActor(windActor);
    renderer->AddActor(axes);
    renderer->AddActor2D(textMaxHeight);
    renderer->AddActor2D(textFlightTime);
    renderer->AddActor2D(textDistanceX);
    renderer->AddActor2D(textDistanceZ);
    renderer->AddActor2D(textTotalDistance);
    renderer->AddActor2D(textWindX);
    renderer->AddActor2D(textWindZ);
    renderer->AddActor2D(textMass);
    renderer->AddActor2D(textRadius);
    renderer->SetBackground(1.0, 1.0, 1.0); // Белый фон
    
    // Настройка окна
    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(1200, 800);
    renderWindow->SetWindowName("3D Projectile Trajectory Animation");
    renderWindow->Render();

    // Интерактивный режим
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);

    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    // Создаем и настраиваем обработчик анимации
    auto animationCallback = vtkSmartPointer<AnimationCallback>::New();
    animationCallback->SetSphereActor(sphereActor);
    animationCallback->SetTrajectoryPoints(states);
    animationCallback->SetRenderWindow(renderWindow);
    animationCallback->SetRenderer(renderer);
    animationCallback->SetSpeed(2.0); // Скорость анимации
    animationCallback->InitializeTrajectoryActors(); // Инициализируем акторы траектории

    // Добавляем обработчик таймера
    interactor->Initialize();
    interactor->AddObserver(vtkCommand::TimerEvent, animationCallback);
    int timerId = interactor->CreateRepeatingTimer(30); // 30 мс между кадрами (примерно 33 кадра в секунду)

    // Запускаем интерактор
    interactor->Start();
}
