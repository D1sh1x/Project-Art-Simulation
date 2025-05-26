#ifndef PARAMETERS_H
#define PARAMETERS_H

struct Parameters {
    double mass;           // масса снаряда
    double Cd;            // коэффициент сопротивления
    double air_density;   // плотность воздуха
    double radius;        // радиус снаряда
    double g;             // ускорение свободного падения
    double wind_x;        // скорость ветра по X
    double wind_z;        // скорость ветра по Z
    double angle_deg;     // угол запуска в градусах
    double initial_speed; // начальная скорость
    double azimuth_deg;   // азимут в градусах
};

#endif // PARAMETERS_H 