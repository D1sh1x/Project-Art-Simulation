#include "mainwindow.h"
#include "simulation.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

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

    // Вывод результатов
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);

    // Компоновка
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(outputArea);

    centralWidget->setLayout(mainLayout);
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