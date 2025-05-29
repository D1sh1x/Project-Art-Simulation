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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRunSimulation();
    void onRunAnimatedSimulation();
    void updatePreviewVisualization();

private:
    QMap<QString, QDoubleSpinBox*> inputFields;
    QTextEdit *outputArea;
    QPushButton *runButton;
    QPushButton *animateButton;
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
};

#endif // MAINWINDOW_H