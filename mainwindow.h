#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include "parameters.h"

// Forward declaration for QFileDialog
class QFileDialog;
class QComboBox; // Forward declaration

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRunSimulation();
    void onRunAnimatedSimulation();
    void updatePreviewVisualization();
    void onSaveParameters();
    void onLoadParameters();
    void onPlotDependencyGraph(); // New slot for plotting
    void onBackToTrajectoryPreview(); // Slot to switch back to trajectory preview
    void onShowInstructions(); // Slot to show instructions

private:
    QMap<QString, QDoubleSpinBox*> inputFields;
    QTextEdit *outputArea;
    QPushButton *runButton;
    QPushButton *animateButton;
    QPushButton *saveParamsButton;
    QPushButton *loadParamsButton;

    // UI Elements for plotting
    QComboBox *graphTypeComboBox;
    QDoubleSpinBox *graphParamMinSpinBox;
    QDoubleSpinBox *graphParamMaxSpinBox;
    QDoubleSpinBox *graphParamStepSpinBox;
    QPushButton *plotGraphButton;
    QPushButton *backToPreviewButton; // Button to go back to trajectory preview
    QPushButton *instructionsButton; // Button to show instructions

    QGraphicsView *previewView;
    QGraphicsScene *previewScene;
    QGraphicsEllipseItem *projectileItem;
    QTimer *previewTimer;
    int previewStep;
    QList<QPointF> previewTrajectory;

    void setupUI();
    void runSimulation();
    void setupPreviewVisualization();
    void calculatePreviewTrajectory();
    // Helper function to draw the graph
    void drawDependencyGraph(const QList<QPointF>& dataPoints, const QString& xLabel, const QString& yLabel, double xMin, double xMax, double yMin, double yMax); 
    // Helper struct for simulation results
    struct SimulationResult {
        double max_height = 0.0;
        double total_distance = 0.0;
        double flight_time = 0.0;
    };
    // Helper function to run a single simulation for graph data
    SimulationResult runSingleSimulationForGraph(Parameters params);
};

#endif // MAINWINDOW_H