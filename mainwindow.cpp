#include "mainwindow.h"
#include "simulation.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainterPath>
#include <QPen>
#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    setupPreviewVisualization();
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Создаем компоновку с двумя колонками
    QHBoxLayout *mainHLayout = new QHBoxLayout();
    
    // Левая колонка с формой параметров
    QVBoxLayout *leftColumnLayout = new QVBoxLayout();
    QFormLayout *formLayout = new QFormLayout();

    // Список параметров с значениями по умолчанию и русскими названиями
    struct ParamInfo {
        QString key;
        QString russianName;
        double defaultValue;
    };
    
    QList<ParamInfo> paramInfoList = {
        {"mass", "Масса снаряда (кг)", 10.0},
        {"Cd", "Коэффициент сопротивления", 0.47},
        {"air_density", "Плотность воздуха (кг/м³)", 1.225},
        {"radius", "Радиус снаряда (м)", 0.1},
        {"g", "Ускорение свободного падения (м/с²)", 9.81},
        {"wind_x", "Скорость ветра по X (м/с)", 5.0},
        {"wind_z", "Скорость ветра по Z (м/с)", 0.0},
        {"angle_deg", "Угол запуска (градусы)", 45.0},
        {"initial_speed", "Начальная скорость (м/с)", 50.0},
        {"azimuth_deg", "Азимут (градусы)", 30.0}
    };

    // Создаем поля ввода
    for (const auto& paramInfo : paramInfoList) {
        QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
        spinBox->setRange(-1e6, 1e6);
        spinBox->setDecimals(3);
        spinBox->setValue(paramInfo.defaultValue);
        inputFields[paramInfo.key] = spinBox;
        formLayout->addRow(paramInfo.russianName, spinBox);
        
        // Подключаем сигнал изменения значения к обновлению предпросмотра
        connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                [this]() { calculatePreviewTrajectory(); });
    }

    // Создаем кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // Кнопка запуска обычной симуляции
    runButton = new QPushButton("Запустить симуляцию", this);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onRunSimulation);
    buttonLayout->addWidget(runButton);
    
    // Кнопка запуска анимированной симуляции
    animateButton = new QPushButton("Запустить анимацию", this);
    connect(animateButton, &QPushButton::clicked, this, &MainWindow::onRunAnimatedSimulation);
    buttonLayout->addWidget(animateButton);

    // Добавляем форму и кнопки в левую колонку
    leftColumnLayout->addLayout(formLayout);
    leftColumnLayout->addLayout(buttonLayout);
    
    // Правая колонка с 2D визуализацией и выводом результатов
    QVBoxLayout *rightColumnLayout = new QVBoxLayout();
    
    // 2D предпросмотр траектории
    QLabel *previewLabel = new QLabel("Предпросмотр траектории:", this);
    rightColumnLayout->addWidget(previewLabel);
    
    previewView = new QGraphicsView(this);
    previewView->setMinimumSize(400, 300);
    previewView->setRenderHint(QPainter::Antialiasing);
    previewView->setBackgroundBrush(QBrush(Qt::white));
    rightColumnLayout->addWidget(previewView);
    
    // Вывод результатов
    QLabel *resultsLabel = new QLabel("Результаты симуляции:", this);
    rightColumnLayout->addWidget(resultsLabel);
    
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    outputArea->setMinimumHeight(150);
    rightColumnLayout->addWidget(outputArea);

    // Добавляем колонки в главную компоновку
    mainHLayout->addLayout(leftColumnLayout, 1);
    mainHLayout->addLayout(rightColumnLayout, 1);

    // Устанавливаем главную компоновку
    centralWidget->setLayout(mainHLayout);
}

void MainWindow::setupPreviewVisualization() {
    previewScene = new QGraphicsScene(this);
    previewView->setScene(previewScene);
    
    // Создаем объект для снаряда
    projectileItem = new QGraphicsEllipseItem(0, 0, 10, 10);
    projectileItem->setBrush(QBrush(Qt::blue));
    projectileItem->setPos(-5, -5); // Центрируем круг относительно его позиции
    previewScene->addItem(projectileItem);
    
    // Инициализируем таймер для анимации
    previewTimer = new QTimer(this);
    connect(previewTimer, &QTimer::timeout, this, &MainWindow::updatePreviewVisualization);
    
    // Рассчитываем начальную траекторию
    calculatePreviewTrajectory();
}

void MainWindow::calculatePreviewTrajectory() {
    // Получаем текущие параметры
    Parameters params;
    params.mass = inputFields["mass"]->value();
    params.Cd = inputFields["Cd"]->value();
    params.air_density = inputFields["air_density"]->value();
    params.radius = inputFields["radius"]->value();
    params.g = inputFields["g"]->value();
    params.wind_x = inputFields["wind_x"]->value();
    params.wind_z = inputFields["wind_z"]->value();
    params.angle_deg = inputFields["angle_deg"]->value();
    params.initial_speed = inputFields["initial_speed"]->value();
    params.azimuth_deg = inputFields["azimuth_deg"]->value();
    
    // Переводим углы в радианы
    double angle_rad = params.angle_deg * 3.14 / 180.0;
    double azimuth_rad = params.azimuth_deg * 3.14 / 180.0;
    
    // Начальное состояние
    State state = {
        0.0,
        0.0,
        0.0,
        params.initial_speed * std::cos(angle_rad) * std::cos(azimuth_rad),
        params.initial_speed * std::sin(angle_rad),
        params.initial_speed * std::cos(angle_rad) * std::sin(azimuth_rad)
    };
    
    // Рассчитываем траекторию
    std::vector<State> states;
    double dt = 0.05; // Увеличенный шаг для предпросмотра (меньше точек)
    do {
        states.push_back(state);
        state = runge_kutta_step(state, params, dt);
    } while (state.y + dt * state.vy >= 0.0 && states.size() < 200); // Ограничиваем количество точек
    
    // Очищаем предыдущую траекторию
    previewScene->clear();
    previewTrajectory.clear();
    
    // Находим максимальные значения для масштабирования
    double maxX = 0, maxY = 0;
    for (const auto& s : states) {
        maxX = std::max(maxX, std::abs(s.x));
        maxY = std::max(maxY, s.y);
    }
    
    // Масштабирование (используем только 80% размера вида для отступов)
    double scaleX = previewView->width() * 0.8 / (maxX * 2 + 1);
    double scaleY = previewView->height() * 0.8 / (maxY + 1);
    double scale = std::min(scaleX, scaleY);
    
    // Центрирование
    double offsetX = previewView->width() / 2;
    double offsetY = previewView->height() * 0.9; // Земля внизу
    
    // Создаем "землю"
    QGraphicsLineItem *groundLine = new QGraphicsLineItem(
        0, offsetY, 
        previewView->width(), offsetY
    );
    groundLine->setPen(QPen(Qt::black, 2));
    previewScene->addItem(groundLine);
    
    // Создаем траекторию
    QPainterPath path;
    bool firstPoint = true;
    
    for (const auto& s : states) {
        // Преобразуем координаты в координаты сцены
        double x = offsetX + s.x * scale;
        double y = offsetY - s.y * scale; // Инвертируем Y, так как в Qt ось Y направлена вниз
        
        QPointF point(x, y);
        previewTrajectory.append(point);
        
        if (firstPoint) {
            path.moveTo(point);
            firstPoint = false;
        } else {
            path.lineTo(point);
        }
    }
    
    // Добавляем путь на сцену
    QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(QPen(Qt::red, 2));
    previewScene->addItem(pathItem);
    
    // Добавляем снаряд снова (он был удален при очистке)
    projectileItem = new QGraphicsEllipseItem(0, 0, 10, 10);
    projectileItem->setBrush(QBrush(Qt::blue));
    projectileItem->setPos(previewTrajectory.first().x() - 5, previewTrajectory.first().y() - 5);
    previewScene->addItem(projectileItem);
    
    // Добавляем информацию о траектории
    if (!states.empty()) {
        double max_height = 0.0;
        for (const auto& s : states) {
            if (s.y > max_height) max_height = s.y;
        }
        
        // Информация о дальности и высоте
        QString infoText = QString("Макс. высота: %1 м\nДальность: %2 м")
                          .arg(max_height, 0, 'f', 1)
                          .arg(states.back().x, 0, 'f', 1);
        
        QGraphicsTextItem *infoItem = new QGraphicsTextItem(infoText);
        infoItem->setPos(10, 10);
        previewScene->addItem(infoItem);
    }
    
    // Перезапускаем анимацию
    previewStep = 0;
    if (!previewTimer->isActive()) {
        previewTimer->start(50); // 20 кадров в секунду
    }
}

void MainWindow::updatePreviewVisualization() {
    if (previewTrajectory.isEmpty()) {
        previewTimer->stop();
        return;
    }
    
    // Обновляем позицию снаряда
    if (previewStep < previewTrajectory.size()) {
        QPointF pos = previewTrajectory.at(previewStep);
        projectileItem->setPos(pos.x() - 5, pos.y() - 5); // -5 для центрирования
        previewStep++;
    } else {
        // Запускаем анимацию заново
        previewStep = 0;
    }
}

void MainWindow::onRunSimulation() {
    // Создаем и заполняем структуру Parameters
    Parameters params;
    
    // Получаем значения из полей ввода
    params.mass = inputFields["mass"]->value();
    params.Cd = inputFields["Cd"]->value();
    params.air_density = inputFields["air_density"]->value();
    params.radius = inputFields["radius"]->value();
    params.g = inputFields["g"]->value();
    params.wind_x = inputFields["wind_x"]->value();
    params.wind_z = inputFields["wind_z"]->value();
    params.angle_deg = inputFields["angle_deg"]->value();
    params.initial_speed = inputFields["initial_speed"]->value();
    params.azimuth_deg = inputFields["azimuth_deg"]->value();

    // Запускаем симуляцию с заполненными параметрами
    StartSimulation(params);
}

void MainWindow::onRunAnimatedSimulation() {
    // Создаем и заполняем структуру Parameters
    Parameters params;
    
    // Получаем значения из полей ввода
    params.mass = inputFields["mass"]->value();
    params.Cd = inputFields["Cd"]->value();
    params.air_density = inputFields["air_density"]->value();
    params.radius = inputFields["radius"]->value();
    params.g = inputFields["g"]->value();
    params.wind_x = inputFields["wind_x"]->value();
    params.wind_z = inputFields["wind_z"]->value();
    params.angle_deg = inputFields["angle_deg"]->value();
    params.initial_speed = inputFields["initial_speed"]->value();
    params.azimuth_deg = inputFields["azimuth_deg"]->value();

    // Запускаем анимированную симуляцию с заполненными параметрами
    StartAnimatedSimulation(params);
}