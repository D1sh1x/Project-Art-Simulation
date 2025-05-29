#include "mainwindow.h"
#include "simulation.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainterPath>
#include <QPen>
#include <cmath>
#include <QFont>
#include <QString>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>
#include <QMessageBox>


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

    // Кнопка сохранения параметров
    saveParamsButton = new QPushButton("Сохранить параметры", this);
    connect(saveParamsButton, &QPushButton::clicked, this, &MainWindow::onSaveParameters);
    buttonLayout->addWidget(saveParamsButton);

    // Кнопка загрузки параметров
    loadParamsButton = new QPushButton("Загрузить параметры", this);
    connect(loadParamsButton, &QPushButton::clicked, this, &MainWindow::onLoadParameters);
    buttonLayout->addWidget(loadParamsButton);

    // Кнопка Инструкции
    instructionsButton = new QPushButton("Инструкция", this);
    connect(instructionsButton, &QPushButton::clicked, this, &MainWindow::onShowInstructions);
    buttonLayout->addWidget(instructionsButton);

    // Добавляем форму и кнопки в левую колонку
    leftColumnLayout->addLayout(formLayout);
    leftColumnLayout->addLayout(buttonLayout);
    
    // Добавляем секцию для построения графиков зависимостей
    QFrame *graphFrame = new QFrame(this);
    graphFrame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *graphLayout = new QVBoxLayout(graphFrame);
    
    QLabel *graphTitleLabel = new QLabel("Построение графиков зависимостей:", this);
    graphLayout->addWidget(graphTitleLabel);

    QHBoxLayout *graphTypeLayout = new QHBoxLayout();
    graphTypeLayout->addWidget(new QLabel("Тип графика:", this));
    graphTypeComboBox = new QComboBox(this);
    // Добавляем типы графиков
    graphTypeComboBox->addItem("Дальность от Начальной скорости", QVariant::fromValue(0));
    graphTypeComboBox->addItem("Высота от Начальной скорости", QVariant::fromValue(1));
    graphTypeComboBox->addItem("Дальность от Угла", QVariant::fromValue(2));
    graphTypeComboBox->addItem("Высота от Угла", QVariant::fromValue(3));
    graphTypeComboBox->addItem("Дальность от Массы", QVariant::fromValue(4));
    graphTypeComboBox->addItem("Высота от Массы", QVariant::fromValue(5));
    graphTypeComboBox->addItem("Дальность от Ветра X", QVariant::fromValue(6));
    graphTypeComboBox->addItem("Время полета от Начальной скорости", QVariant::fromValue(7));
    graphTypeLayout->addWidget(graphTypeComboBox);
    graphLayout->addLayout(graphTypeLayout);

    QHBoxLayout *graphParamMinLayout = new QHBoxLayout();
    graphParamMinLayout->addWidget(new QLabel("Мин. знач. параметра:", this));
    graphParamMinSpinBox = new QDoubleSpinBox(this);
    graphParamMinSpinBox->setRange(-1e6, 1e6);
    graphParamMinSpinBox->setDecimals(2);
    graphParamMinSpinBox->setValue(10.0); // Default
    graphParamMinLayout->addWidget(graphParamMinSpinBox);
    graphLayout->addLayout(graphParamMinLayout);

    QHBoxLayout *graphParamMaxLayout = new QHBoxLayout();
    graphParamMaxLayout->addWidget(new QLabel("Макс. знач. параметра:", this));
    graphParamMaxSpinBox = new QDoubleSpinBox(this);
    graphParamMaxSpinBox->setRange(-1e6, 1e6);
    graphParamMaxSpinBox->setDecimals(2);
    graphParamMaxSpinBox->setValue(100.0); // Default
    graphParamMaxLayout->addWidget(graphParamMaxSpinBox);
    graphLayout->addLayout(graphParamMaxLayout);

    QHBoxLayout *graphParamStepLayout = new QHBoxLayout();
    graphParamStepLayout->addWidget(new QLabel("Шаг параметра:", this));
    graphParamStepSpinBox = new QDoubleSpinBox(this);
    graphParamStepSpinBox->setRange(0.01, 1e6);
    graphParamStepSpinBox->setDecimals(2);
    graphParamStepSpinBox->setValue(5.0); // Default
    graphParamStepLayout->addWidget(graphParamStepSpinBox);
    graphLayout->addLayout(graphParamStepLayout);

    plotGraphButton = new QPushButton("Построить график", this);
    connect(plotGraphButton, &QPushButton::clicked, this, &MainWindow::onPlotDependencyGraph);
    // graphLayout->addWidget(plotGraphButton); // Will be added to a QHBoxLayout

    // Создаем QHBoxLayout для кнопок управления графиками
    QHBoxLayout *graphButtonsLayout = new QHBoxLayout();
    graphButtonsLayout->addWidget(plotGraphButton);

    backToPreviewButton = new QPushButton("К предпросмотру траектории", this);
    connect(backToPreviewButton, &QPushButton::clicked, this, &MainWindow::onBackToTrajectoryPreview);
    graphButtonsLayout->addWidget(backToPreviewButton);
    
    graphLayout->addLayout(graphButtonsLayout); // Добавляем кнопки в вертикальную компоновку панели графиков

    leftColumnLayout->addWidget(graphFrame);
    leftColumnLayout->addStretch(); // Добавляем растяжитель, чтобы панель графиков не растягивалась слишком сильно
    
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
    
    // Создаем "землю" (Ось X)
    QGraphicsLineItem *groundLine = new QGraphicsLineItem(
        0, offsetY, 
        previewView->width(), offsetY
    );
    groundLine->setPen(QPen(Qt::black, 2));
    previewScene->addItem(groundLine);
    QGraphicsTextItem *xAxisLabel = new QGraphicsTextItem("X (m)");
    xAxisLabel->setFont(QFont("Arial", 10));
    xAxisLabel->setPos(previewView->width() - xAxisLabel->boundingRect().width() - 5, offsetY + 2);
    previewScene->addItem(xAxisLabel);

    // Создаем Ось Y
    double yAxisMargin = 20; // Небольшой отступ сверху для метки Y
    QGraphicsLineItem *yAxisLine = new QGraphicsLineItem(
        offsetX, offsetY - maxY * scale - yAxisMargin, 
        offsetX, offsetY
    );
    yAxisLine->setPen(QPen(Qt::black, 2));
    previewScene->addItem(yAxisLine);
    QGraphicsTextItem *yAxisLabel = new QGraphicsTextItem("Y (m)");
    yAxisLabel->setFont(QFont("Arial", 10));
    yAxisLabel->setPos(offsetX + 2, offsetY - maxY * scale - yAxisMargin - yAxisLabel->boundingRect().height());
    previewScene->addItem(yAxisLabel);
    
    // Добавляем размерную сетку и метки
    int gridStep = 20; // Шаг сетки в единицах симуляции (метрах)
    QPen gridPen(Qt::lightGray, 0.5); 
    QFont gridLabelFont("Arial", 7);
    QColor gridLabelColor = Qt::darkGray;

    // Вертикальные линии сетки (по X)
    for (int i = 0; ; ++i) {
        double simX = i * gridStep;
        double sceneXPositive = offsetX + simX * scale;
        double sceneXNegative = offsetX - simX * scale;

        bool addedPositive = false;
        if (sceneXPositive < previewView->width()) {
            QGraphicsLineItem *line = new QGraphicsLineItem(sceneXPositive, 0, sceneXPositive, offsetY);
            line->setPen(gridPen);
            previewScene->addItem(line);
            if (simX != 0) {
                QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(simX));
                label->setFont(gridLabelFont);
                label->setDefaultTextColor(gridLabelColor);
                label->setPos(sceneXPositive + 2, offsetY - 12);
                previewScene->addItem(label);
            }
            addedPositive = true;
        }
        if (simX != 0 && sceneXNegative > 0) { // simX != 0 to avoid double label at origin
             QGraphicsLineItem *line = new QGraphicsLineItem(sceneXNegative, 0, sceneXNegative, offsetY);
            line->setPen(gridPen);
            previewScene->addItem(line);
            QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(-simX));
            label->setFont(gridLabelFont);
            label->setDefaultTextColor(gridLabelColor);
            label->setPos(sceneXNegative + 2, offsetY - 12);
            previewScene->addItem(label);
            addedPositive = true;
        }
        if (!addedPositive && i!=0) break; // Stop if lines go out of view
         if (i > 2 * (maxX / gridStep + 2) && maxX > 0) break; // Safety break
         if (i > 50 && maxX == 0) break; // Safety break for initial state
    }

    // Горизонтальные линии сетки (по Y)
    for (int i = 0; ; ++i) {
        double simY = i * gridStep;
        double sceneY = offsetY - simY * scale;
        if (sceneY < 0) break;

        QGraphicsLineItem *line = new QGraphicsLineItem(0, sceneY, previewView->width(), sceneY);
        line->setPen(gridPen);
        previewScene->addItem(line);
         if (simY != 0) {
            QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(simY));
            label->setFont(gridLabelFont);
            label->setDefaultTextColor(gridLabelColor);
            label->setPos(offsetX + 2, sceneY - 10);
            previewScene->addItem(label);
        }
        if (i > (maxY / gridStep + 2) && maxY > 0) break; // Safety break
        if (i > 50 && maxY == 0) break; // Safety break for initial state
    }
    
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
    
    // Вычисляем и выводим подробные данные траектории в outputArea
    if (!states.empty()) {
        double max_height_val = 0.0;
        for (const auto& s_val : states) {
            if (s_val.y > max_height_val) max_height_val = s_val.y;
        }
        
        double final_x_val = states.back().x;
        double final_z_val = states.back().z; // Используем Z координату для полной дальности
        double total_distance_val = std::sqrt(final_x_val * final_x_val + final_z_val * final_z_val);
        double flight_time_val = (states.size() - 1) * dt;

        QString resultsText = QString("Результаты (предпросмотр):\n") +
                              QString("-----------------------------\n") +
                              QString("Максимальная высота: %1 м\n").arg(max_height_val, 0, 'f', 2) +
                              QString("Дальность по X: %1 м\n").arg(final_x_val, 0, 'f', 2) +
                              QString("Дальность по Z: %1 м\n").arg(final_z_val, 0, 'f', 2) +
                              QString("Общая дальность: %1 м\n").arg(total_distance_val, 0, 'f', 2) +
                              QString("Время полета: %1 с").arg(flight_time_val, 0, 'f', 2);
        outputArea->setText(resultsText);
    } else {
        outputArea->setText("Нет данных для отображения.");
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

void MainWindow::onShowInstructions() {
    QString instructionText = 
        "Инструкция по использованию симулятора:\n\n" \
        "Основные элементы:\n" \
        "- Левая панель: Ввод параметров для симуляции (масса, скорость, угол и т.д.).\n" \
        "- Правая панель (верхняя часть): 2D-предпросмотр траектории полета. Обновляется автоматически при изменении параметров.\n" \
        "- Правая панель (нижняя часть): Текстовый вывод результатов 2D-предпросмотра.\n\n" \
        "Кнопки на левой панели:\n" \
        "- \"Запустить симуляцию\": Открывает окно с 3D-визуализацией конечной траектории.\n" \
        "- \"Запустить анимацию\": Открывает окно с анимированной 3D-визуализацией полета.\n" \
        "- \"Сохранить параметры\": Сохраняет текущие параметры в файл.\n" \
        "- \"Загрузить параметры\": Загружает параметры из файла.\n" \
        "- \"Инструкция\": Показывает это окно.\n\n" \
        "Секция \"Построение графиков зависимостей\":\n" \
        "- \"Тип графика\": Выбор зависимости для построения.\n" \
        "- \"Мин./Макс. знач. параметра\", \"Шаг параметра\": Настройка диапазона для графика.\n" \
        "- \"Построить график\": Строит график в области 2D-предпросмотра.\n" \
        "- \"К предпросмотру траектории\": Возвращает отображение 2D-траектории.\n\n" \
        "Окно 3D-симуляции:\n" \
        "- Управление камерой: Вращение (ЛКМ), приближение/отдаление (колесико/ПКМ), панорамирование (СКМ/Shift+ЛКМ).\n" \
        "- Отображаются оси X, Y, Z и сетка.\n" \
        "- В анимации: текущие координаты снаряда.\n" \
        "- Кнопка \"Back to Menu\": Закрывает окно 3D-симуляции.";

    QMessageBox::information(this, "Инструкция", instructionText);
}

// Реализация слота для сохранения параметров
void MainWindow::onSaveParameters() {
    QString fileName = QFileDialog::getSaveFileName(this, 
                                                    tr("Сохранить параметры"), "",
                                                    tr("Parameter Files (*.ini);;All Files (*)"));
    if (fileName.isEmpty()) {
        return;
    } else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // Можно добавить обработку ошибок, например, QMessageBox
            return;
        }

        QTextStream out(&file);
        for (auto it = inputFields.constBegin(); it != inputFields.constEnd(); ++it) {
            out << it.key() << "=" << it.value()->value() << "\n";
        }
        file.close();
    }
}

// Реализация слота для загрузки параметров
void MainWindow::onLoadParameters() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Загрузить параметры"), "",
                                                    tr("Parameter Files (*.ini);;All Files (*)"));
    if (fileName.isEmpty()) {
        return;
    } else {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Можно добавить обработку ошибок
            return;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split('=');
            if (parts.size() == 2) {
                QString key = parts[0].trimmed();
                double value = parts[1].trimmed().toDouble();
                if (inputFields.contains(key)) {
                    inputFields[key]->setValue(value);
                }
            }
        }
        file.close();
        calculatePreviewTrajectory(); // Обновляем предпросмотр после загрузки
    }
}

MainWindow::SimulationResult MainWindow::runSingleSimulationForGraph(Parameters params) {
    State state = {
        0.0, 0.0, 0.0,
        params.initial_speed * std::cos(params.angle_deg * 3.1415926535 / 180.0) * std::cos(params.azimuth_deg * 3.1415926535 / 180.0),
        params.initial_speed * std::sin(params.angle_deg * 3.1415926535 / 180.0),
        params.initial_speed * std::cos(params.angle_deg * 3.1415926535 / 180.0) * std::sin(params.azimuth_deg * 3.1415926535 / 180.0)
    };

    std::vector<State> states;
    double dt = 0.01; // Более точный dt для расчета графика
    do {
        states.push_back(state);
        state = runge_kutta_step(state, params, dt);
    } while (state.y + dt * state.vy >= 0.0 && states.size() < 10000); // Ограничиваем количество точек

    SimulationResult result;
    if (!states.empty()) {
        for (const auto& s_val : states) {
            if (s_val.y > result.max_height) result.max_height = s_val.y;
        }
        double final_x_val = states.back().x;
        double final_z_val = states.back().z;
        result.total_distance = std::sqrt(final_x_val * final_x_val + final_z_val * final_z_val);
        result.flight_time = (states.size() - 1) * dt;
    }
    return result;
}

void MainWindow::drawDependencyGraph(const QList<QPointF>& dataPoints, const QString& xLabelText, const QString& yLabelText, double xMin, double xMax, double yMin, double yMax) {
    previewScene->clear(); // Очищаем сцену перед отрисовкой графика

    if (dataPoints.isEmpty()) {
        QGraphicsTextItem *noDataText = new QGraphicsTextItem("Нет данных для построения графика.");
        noDataText->setFont(QFont("Arial", 12));
        noDataText->setPos(previewView->width()/2 - noDataText->boundingRect().width()/2, previewView->height()/2 - noDataText->boundingRect().height()/2);
        previewScene->addItem(noDataText);
        return;
    }

    // Масштабирование и отступы
    double plotWidth = previewView->width() * 0.85;
    double plotHeight = previewView->height() * 0.80;
    double H_MARGIN = previewView->width() * 0.10;
    double V_MARGIN_TOP = previewView->height() * 0.05;
    double V_MARGIN_BOTTOM = previewView->height() * 0.15;


    double effectiveXRange = (xMax - xMin == 0) ? 1 : (xMax - xMin);
    double effectiveYRange = (yMax - yMin == 0) ? 1 : (yMax - yMin);

    double scaleX = plotWidth / effectiveXRange;
    double scaleY = plotHeight / effectiveYRange;

    // Ось X
    QGraphicsLineItem *xAxis = new QGraphicsLineItem(H_MARGIN, V_MARGIN_TOP + plotHeight, H_MARGIN + plotWidth, V_MARGIN_TOP + plotHeight);
    xAxis->setPen(QPen(Qt::black, 2));
    previewScene->addItem(xAxis);
    QGraphicsTextItem *xLabel = new QGraphicsTextItem(xLabelText);
    xLabel->setFont(QFont("Arial", 10));
    xLabel->setPos(H_MARGIN + plotWidth / 2 - xLabel->boundingRect().width() / 2, V_MARGIN_TOP + plotHeight + 5);
    previewScene->addItem(xLabel);

    // Ось Y
    QGraphicsLineItem *yAxis = new QGraphicsLineItem(H_MARGIN, V_MARGIN_TOP, H_MARGIN, V_MARGIN_TOP + plotHeight);
    yAxis->setPen(QPen(Qt::black, 2));
    previewScene->addItem(yAxis);
    QGraphicsTextItem *yLabel = new QGraphicsTextItem(yLabelText);
    yLabel->setFont(QFont("Arial", 10));
    yLabel->setRotation(-90); // Поворачиваем метку для оси Y
    yLabel->setPos(H_MARGIN - yLabel->boundingRect().height() - 5, V_MARGIN_TOP + plotHeight / 2 + yLabel->boundingRect().width()/2);
    previewScene->addItem(yLabel);
    
    // Добавляем метки на осях
    int numTicks = 5; // Количество делений на осях
    QFont tickFont("Arial", 8);

    // Метки по X
    for (int i = 0; i <= numTicks; ++i) {
        double val = xMin + (effectiveXRange / numTicks) * i;
        double xPos = H_MARGIN + (val - xMin) * scaleX;
        QGraphicsLineItem *tick = new QGraphicsLineItem(xPos, V_MARGIN_TOP + plotHeight, xPos, V_MARGIN_TOP + plotHeight + 5);
        previewScene->addItem(tick);
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(val, 'f', 1));
        label->setFont(tickFont);
        label->setPos(xPos - label->boundingRect().width() / 2, V_MARGIN_TOP + plotHeight + 7);
        previewScene->addItem(label);
    }

    // Метки по Y
    for (int i = 0; i <= numTicks; ++i) {
        double val = yMin + (effectiveYRange / numTicks) * i;
        double yPos = V_MARGIN_TOP + plotHeight - (val - yMin) * scaleY;
         QGraphicsLineItem *tick = new QGraphicsLineItem(H_MARGIN - 5, yPos, H_MARGIN, yPos);
        previewScene->addItem(tick);
        QGraphicsTextItem *label = new QGraphicsTextItem(QString::number(val, 'f', 1));
        label->setFont(tickFont);
        label->setPos(H_MARGIN - label->boundingRect().width() - 7, yPos - label->boundingRect().height() / 2);
        previewScene->addItem(label);
    }


    // Рисуем график
    QPainterPath path;
    if (!dataPoints.isEmpty()) {
        QPointF firstScaledPoint(H_MARGIN + (dataPoints.first().x() - xMin) * scaleX,
                                 V_MARGIN_TOP + plotHeight - (dataPoints.first().y() - yMin) * scaleY);
        path.moveTo(firstScaledPoint);
        for (int i = 1; i < dataPoints.size(); ++i) {
            QPointF scaledPoint(H_MARGIN + (dataPoints.at(i).x() - xMin) * scaleX,
                                V_MARGIN_TOP + plotHeight - (dataPoints.at(i).y() - yMin) * scaleY);
            path.lineTo(scaledPoint);
        }
    }

    QGraphicsPathItem *pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(QPen(Qt::blue, 2));
    previewScene->addItem(pathItem);
}


void MainWindow::onPlotDependencyGraph() {
    Parameters currentParams;
    // Собираем базовые параметры из UI
    currentParams.mass = inputFields["mass"]->value();
    currentParams.Cd = inputFields["Cd"]->value();
    currentParams.air_density = inputFields["air_density"]->value();
    currentParams.radius = inputFields["radius"]->value();
    currentParams.g = inputFields["g"]->value();
    currentParams.wind_x = inputFields["wind_x"]->value();
    currentParams.wind_z = inputFields["wind_z"]->value();
    currentParams.angle_deg = inputFields["angle_deg"]->value();
    currentParams.initial_speed = inputFields["initial_speed"]->value();
    currentParams.azimuth_deg = inputFields["azimuth_deg"]->value();

    int graphTypeIndex = graphTypeComboBox->currentData().toInt();
    double paramMin = graphParamMinSpinBox->value();
    double paramMax = graphParamMaxSpinBox->value();
    double paramStep = graphParamStepSpinBox->value();

    if (paramStep <= 0) {
        outputArea->setText("Ошибка: Шаг параметра должен быть положительным.");
        return;
    }
    if (paramMin >= paramMax) {
        outputArea->setText("Ошибка: Минимальное значение параметра должно быть меньше максимального.");
        return;
    }

    QList<QPointF> dataPoints;
    QString xAxisLabel, yAxisLabel;
    
    double currentXMin = std::numeric_limits<double>::max();
    double currentXMax = std::numeric_limits<double>::lowest();
    double currentYMin = std::numeric_limits<double>::max();
    double currentYMax = std::numeric_limits<double>::lowest();


    for (double val = paramMin; val <= paramMax; val += paramStep) {
        Parameters tempParams = currentParams;
        SimulationResult result;
        double xValue = val;

        switch (graphTypeIndex) {
            case 0: // Дальность от Начальной скорости
                tempParams.initial_speed = val;
                xAxisLabel = "Начальная скорость (м/с)";
                yAxisLabel = "Дальность полета (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.total_distance));
                currentYMin = std::min(currentYMin, result.total_distance);
                currentYMax = std::max(currentYMax, result.total_distance);
                break;
            case 1: // Высота от Начальной скорости
                tempParams.initial_speed = val;
                xAxisLabel = "Начальная скорость (м/с)";
                yAxisLabel = "Макс. высота (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.max_height));
                currentYMin = std::min(currentYMin, result.max_height);
                currentYMax = std::max(currentYMax, result.max_height);
                break;
            case 2: // Дальность от Угла
                tempParams.angle_deg = val;
                xAxisLabel = "Угол запуска (град)";
                yAxisLabel = "Дальность полета (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.total_distance));
                currentYMin = std::min(currentYMin, result.total_distance);
                currentYMax = std::max(currentYMax, result.total_distance);
                break;
            case 3: // Высота от Угла
                tempParams.angle_deg = val;
                xAxisLabel = "Угол запуска (град)";
                yAxisLabel = "Макс. высота (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.max_height));
                currentYMin = std::min(currentYMin, result.max_height);
                currentYMax = std::max(currentYMax, result.max_height);
                break;
            case 4: // Дальность от Массы
                tempParams.mass = val;
                 if (val <=0) continue; // Масса должна быть > 0
                xAxisLabel = "Масса снаряда (кг)";
                yAxisLabel = "Дальность полета (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.total_distance));
                currentYMin = std::min(currentYMin, result.total_distance);
                currentYMax = std::max(currentYMax, result.total_distance);
                break;
            case 5: // Высота от Массы
                tempParams.mass = val;
                if (val <=0) continue; // Масса должна быть > 0
                xAxisLabel = "Масса снаряда (кг)";
                yAxisLabel = "Макс. высота (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.max_height));
                currentYMin = std::min(currentYMin, result.max_height);
                currentYMax = std::max(currentYMax, result.max_height);
                break;
            case 6: // Дальность от Ветра X
                tempParams.wind_x = val;
                xAxisLabel = "Скорость ветра X (м/с)";
                yAxisLabel = "Дальность полета (м)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.total_distance));
                currentYMin = std::min(currentYMin, result.total_distance);
                currentYMax = std::max(currentYMax, result.total_distance);
                break;
            case 7: // Время полета от Начальной скорости
                tempParams.initial_speed = val;
                xAxisLabel = "Начальная скорость (м/с)";
                yAxisLabel = "Время полета (с)";
                result = runSingleSimulationForGraph(tempParams);
                dataPoints.append(QPointF(xValue, result.flight_time));
                currentYMin = std::min(currentYMin, result.flight_time);
                currentYMax = std::max(currentYMax, result.flight_time);
                break;
        }
         currentXMin = std::min(currentXMin, xValue);
         currentXMax = std::max(currentXMax, xValue);
    }
    
    if (dataPoints.isEmpty()){
        currentXMin = 0; currentXMax = 1; // Default if no points
        currentYMin = 0; currentYMax = 1;
    }


    // Отключаем таймер предпросмотра, если он активен, чтобы он не мешал графику
    if (previewTimer->isActive()) {
        previewTimer->stop();
    }
    // Очищаем текстовое поле результатов, так как теперь отображается график
    outputArea->clear();

    drawDependencyGraph(dataPoints, xAxisLabel, yAxisLabel, currentXMin, currentXMax, currentYMin, currentYMax);
}

void MainWindow::onBackToTrajectoryPreview() {
    // Просто вызываем функцию, которая пересчитывает и отображает траекторию
    // Она также очистит сцену от графика
    calculatePreviewTrajectory();
    // Таймер анимации снаряда уже должен быть запущен в calculatePreviewTrajectory,
    // если он был остановлен для построения графика.
}