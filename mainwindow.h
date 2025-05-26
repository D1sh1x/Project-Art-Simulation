#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include "parameters.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRunSimulation();
    void onRunAnimatedSimulation();

private:
    QMap<QString, QDoubleSpinBox*> inputFields;
    QTextEdit *outputArea;
    QPushButton *runButton;
    QPushButton *animateButton;

    void setupUI();
    void runSimulation();
};

#endif // MAINWINDOW_H