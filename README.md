# Projectile Trajectory Simulation

This project simulates the trajectory of a projectile using a graphical user interface (GUI) built with Qt and 3D visualization powered by VTK. Users can input various parameters to see how they affect the projectile's path.

## Features
- **Parameter Input**: Users can input parameters such as mass, drag coefficient, air density, radius, gravity, wind speed, launch angle, initial speed, and azimuth.
- **3D Visualization**: The trajectory is visualized in a 3D environment, allowing users to see the path from different angles.
- **Interactive GUI**: Built with Qt, the GUI is user-friendly and allows for easy input and simulation control.

## Components
- **Qt**: Used for building the GUI, handling user input, and managing the application window.
- **VTK**: Used for rendering the 3D trajectory and visual elements like the projectile, ground, and wind.

## Code Structure
- **main.cpp**: Initializes the application and opens the main window.
- **mainwindow.h/cpp**: Defines the `MainWindow` class, which sets up the UI and handles user interactions.
  - **setupUI()**: Configures the input fields and buttons.
  - **onRunSimulation()**: Collects input data and starts the simulation.
- **simulation.h/cpp**: Contains the logic for simulating the projectile's trajectory.
  - **State**: A struct representing the projectile's state (position and velocity).
  - **Parameters**: A struct holding all input parameters for the simulation.
  - **compute_derivatives()**: Calculates the derivatives of the state based on current conditions.
  - **runge_kutta_step()**: Performs a Runge-Kutta integration step to update the state.
  - **StartSimulation()**: Runs the simulation using the provided parameters and visualizes the results.
- **parameters.h**: Defines the `Parameters` struct used to store simulation parameters.

## Requirements
- **CMake**: Version 3.15 or higher is required to build the project.
- **Qt6**: The project uses Qt6 for the GUI, requiring components like Core, Gui, and Widgets.
- **VTK**: The visualization relies on VTK, with components such as CommonCore, RenderingCore, and more.

## Build Instructions
1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   ```
2. **Navigate to the project directory**:
   ```bash
   cd ProjectArt
   ```
3. **Create a build directory and navigate into it**:
   ```bash
   mkdir build && cd build
   ```
4. **Run CMake to configure the project**:
   ```bash
   cmake ..
   ```
5. **Build the project**:
   ```bash
   cmake --build .
   ```

## Usage
- **Run the executable**: After building, run the executable found in the build directory.
- **Input parameters**: Use the GUI to input desired parameters.
- **Start simulation**: Click "Запустить симуляцию" to visualize the trajectory.

## License
This project is licensed under the MIT License - see the LICENSE file for details. 