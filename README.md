# Traffic Master

An interactive, 3D traffic management simulator built using C++, OpenGL, and GLUT. Manage traffic flow at a busy city intersection, monitor lane occupancy, and try to keep congestion at bay as vehicles speed up over time!

---

## 🎮 Controls

- **`Spacebar`**: Toggle traffic lights (North-South vs East-West green light)
- **`R`**: Restart the simulation / reset timer and vehicle speeds
- **`Esc`**: Quit game

---

## 🚦 Key Features

### 1. Diverse Vehicle System
Rather than generic cars, the simulation spawns vehicles in a repeating sequence (`Car` ➔ `Car` ➔ `Bus` ➔ `Bike` ➔ `Truck`):
- 🟣 **Cars**: Purple body with detailed cabins.
- 🔵 **Buses**: Long blue transit vehicles with roof trims and passenger window strips.
- 🔴 **Bikes**: Compact red motorcycles with visible handlebars and fuel tanks.
- 🟡 **Trucks**: Dark yellow cargo trucks with elevated front cabs and smokestacks.
- ⚫ **Tires**: All vehicles feature black tires.

### 2. Interactive City Grid
- **Windowed Buildings**: Multi-story structures populated with procedurally generated, warm-yellow lit and dark unlit windows.
- **Playgrounds**: Added parks featuring a green grass base, wood-bordered sandboxes, swing sets with swinging chains/seats, and slides with platforms and ramps.
- **Trees & Foliage**: Abundant tree and grass patches to fill in empty zones.

### 3. Animated Pedestrians
- 🚶 **Walkers & Joggers**: Pedestrians walk or run along the sidewalks with custom arm and leg swing animations.
- **Randomized Looks**: Unique clothing colors, skin tones, heights, and speed values.
- **Jogger Bobbing**: Joggers bob up and down realistically as they run.

### 4. Gameplay Progression & UI
- **Speed Escalation**: Vehicle speeds increase by **+10% every 30 seconds** of simulation time, ramping up the difficulty.
- **Timer**: A centered session timer tracks your progress. It freezes at game over so you can view your exact survival time.
- **Game Over Screen**: Displays your survival duration, e.g., `Survived: 01:30`.
- **Occupancy Stats**: Lane capacity usage displays at the bottom-center in bright yellow.

---

## 📂 Project Structure

- `openglportable/main.cpp`: Main source file containing game logic, object drawing, physics update loops, and user interaction.
- `freeglut/`: Bundled library headers and binaries for rendering.
- `Media/`: Contains background sound tracks.
