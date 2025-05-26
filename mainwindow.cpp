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

    // Список параметров с значениями по умолчанию
    QMap<QString, double> defaultParams = {
        {"mass", 10.0},
        {"Cd", 0.47},
        {"air_density", 1.225},
        {"radius", 0.1},
        {"g", 9.81},
        {"wind_x", 5.0},
        {"wind_z", 0.0},
        {"angle_deg", 45.0},
        {"initial_speed", 50.0},
        {"azimuth_deg", 30.0}
    };

    // Создаем поля ввода
    for (auto it = defaultParams.begin(); it != defaultParams.end(); ++it) {
        QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
        spinBox->setRange(-1e6, 1e6);
        spinBox->setDecimals(3);
        spinBox->setValue(it.value());
        inputFields[it.key()] = spinBox;
        formLayout->addRow(it.key(), spinBox);
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