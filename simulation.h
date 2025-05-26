#ifndef SIMULATION_H
#define SIMULATION_H

#include "parameters.h"
#include <vector>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

struct State {
    double x, y, z;
    double vx, vy, vz;
};

// Объявления функций
State compute_derivatives(const State& state, const Parameters& params);
State runge_kutta_step(const State& state, const Parameters& params, double dt);
void StartSimulation(Parameters params);

// Новые функции для анимации
void StartAnimatedSimulation(Parameters params);
class AnimationCallback;

#endif // SIMULATION_H 