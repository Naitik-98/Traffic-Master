#include <GL/glut.h>
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>

void drawBox(float sx, float sy, float sz);
void drawBuildings();
void drawHUD();
void resetSimulation();
void keyboard(unsigned char key, int x, int y);

void drawTree(float x, float z);
void drawGrassPatch(float x, float z);

int windowWidth = 1280;
int windowHeight = 720;

// vehicleType: 0=car, 1=car(2nd), 2=bus, 3=bike, 4=truck
struct Car
{
    float x;
    float y;
    float z;
    float speed;
    int direction;
    bool active;
    float r;
    float g;
    float b;
    int vehicleType;
};

const int MAX_CARS = 12;
Car cars[MAX_CARS];
int nextVehicleType = 0; // Cycles: 0=car,1=car,2=bus,3=bike,4=truck
int lastUpdateTime = 0;
int lastSignalChangeTime = 0;
int lastSpawnTime = -1200;
int nextSpawnDirection = 0;
bool northSouthGreen = false;
bool gameOver = false;
int gameOverTime = 0;
const int autoRestartSeconds = 5;
int simulationStartTime = 0; // ms — set on launch and reset

// Traffic lights toggled only by T key

// ---- Pedestrians ----
struct Person {
    float x, z;           // world position
    float speed;          // units/sec
    int   sidewalk;       // 0=NS-left(x=-4.45), 1=NS-right(x=+4.45)
                          // 2=EW-top(z=-4.45),  3=EW-bottom(z=+4.45)
    int   dir;            // +1 or -1 along the sidewalk axis
    float phase;          // animation phase offset (radians)
    bool  isJogger;
    float skinR, skinG, skinB;
    float shirtR, shirtG, shirtB;
    float pantsR, pantsG, pantsB;
    float heightScale;
};

const int MAX_PEOPLE = 16;
Person people[MAX_PEOPLE];
bool   peopleInit = false;

void initPeople()
{
    // sidewalk center x/z values
    // NS-left: x=-4.45  NS-right: x=+4.45
    // EW-top:  z=-4.45  EW-bottom: z=+4.45
    struct PDef {
        int sw; int dir; float start;
        bool jog;
        float sR,sG,sB;   // skin
        float tR,tG,tB;   // shirt (top)
        float pR,pG,pB;   // pants
        float ht;          // height scale
    };

    PDef d[MAX_PEOPLE] = {
        // NS-left (x=-4.45), walking/jogging in ±Z
        {0, +1,-30.f,false, 0.88f,0.68f,0.48f, 0.18f,0.42f,0.85f, 0.12f,0.12f,0.32f, 1.00f},
        {0, -1, 20.f,false, 0.55f,0.35f,0.20f, 0.85f,0.18f,0.18f, 0.22f,0.18f,0.14f, 0.93f},
        {0, +1, -8.f, true, 0.92f,0.72f,0.52f, 0.10f,0.72f,0.28f, 0.08f,0.08f,0.08f, 1.06f},
        {0, -1, 12.f,false, 0.48f,0.28f,0.14f, 0.72f,0.72f,0.08f, 0.18f,0.28f,0.48f, 0.96f},
        // NS-right (x=+4.45)
        {1, +1,-22.f, true, 0.85f,0.75f,0.60f, 0.92f,0.28f,0.08f, 0.08f,0.08f,0.28f, 1.08f},
        {1, -1, 28.f,false, 0.38f,0.22f,0.12f, 0.28f,0.28f,0.82f, 0.18f,0.18f,0.18f, 0.91f},
        {1, +1,  4.f,false, 0.76f,0.56f,0.36f, 0.62f,0.08f,0.62f, 0.14f,0.22f,0.38f, 1.01f},
        {1, -1,-14.f, true, 0.90f,0.66f,0.46f, 0.08f,0.08f,0.82f, 0.28f,0.28f,0.28f, 1.03f},
        // EW-top (z=-4.45)
        {2, +1,-26.f,false, 0.62f,0.42f,0.26f, 0.18f,0.62f,0.18f, 0.24f,0.18f,0.32f, 0.97f},
        {2, -1, 20.f, true, 0.88f,0.70f,0.50f, 0.82f,0.82f,0.08f, 0.08f,0.14f,0.08f, 1.07f},
        {2, +1,  6.f,false, 0.45f,0.28f,0.15f, 0.92f,0.92f,0.92f, 0.12f,0.12f,0.12f, 0.94f},
        {2, -1,-10.f,false, 0.82f,0.62f,0.42f, 0.72f,0.18f,0.52f, 0.18f,0.18f,0.38f, 1.00f},
        // EW-bottom (z=+4.45)
        {3, +1,-16.f, true, 0.78f,0.58f,0.38f, 0.08f,0.82f,0.82f, 0.08f,0.08f,0.22f, 1.04f},
        {3, -1, 14.f,false, 0.52f,0.34f,0.20f, 0.62f,0.38f,0.18f, 0.18f,0.18f,0.18f, 0.95f},
        {3, +1, -6.f,false, 0.86f,0.66f,0.46f, 0.42f,0.08f,0.72f, 0.28f,0.22f,0.18f, 1.00f},
        {3, -1, 26.f, true, 0.42f,0.25f,0.14f, 0.18f,0.38f,0.92f, 0.14f,0.18f,0.28f, 0.92f},
    };

    for (int i = 0; i < MAX_PEOPLE; ++i) {
        PDef& p = d[i];
        people[i].sidewalk    = p.sw;
        people[i].dir         = p.dir;
        people[i].isJogger    = p.jog;
        people[i].speed       = p.jog ? 4.2f + (i % 3) * 0.4f : 1.8f + (i % 4) * 0.25f;
        people[i].phase       = i * 0.72f;
        people[i].skinR = p.sR; people[i].skinG = p.sG; people[i].skinB = p.sB;
        people[i].shirtR= p.tR; people[i].shirtG= p.tG; people[i].shirtB= p.tB;
        people[i].pantsR= p.pR; people[i].pantsG= p.pG; people[i].pantsB= p.pB;
        people[i].heightScale = p.ht;
        switch (p.sw) {
            case 0: people[i].x = -4.45f; people[i].z = p.start; break;
            case 1: people[i].x =  4.45f; people[i].z = p.start; break;
            case 2: people[i].z = -4.45f; people[i].x = p.start; break;
            case 3: people[i].z =  4.45f; people[i].x = p.start; break;
        }
    }
    peopleInit = true;
}

void updatePeople(float dt)
{
    if (!peopleInit) initPeople();
    const float lim = 38.0f;
    for (int i = 0; i < MAX_PEOPLE; ++i) {
        Person& p = people[i];
        float move = p.speed * dt * p.dir;
        if (p.sidewalk == 0 || p.sidewalk == 1) {
            p.z += move;
            if (p.z >  lim) p.z = -lim;
            if (p.z < -lim) p.z =  lim;
        } else {
            p.x += move;
            if (p.x >  lim) p.x = -lim;
            if (p.x < -lim) p.x =  lim;
        }
    }
}




void resetCar(Car& car, int direction)
{
    car.active = true;
    car.direction = direction;
    car.y = 0.35f;

    if (direction == 0)
    {
        car.x = -1.2f;
        car.z = -18.0f;
        car.speed = 4.8f;
    }
    else if (direction == 1)
    {
        car.x = 1.2f;
        car.z = 18.0f;
        car.speed = 5.3f;
    }
    else if (direction == 2)
    {
        car.x = 18.0f;
        car.z = 1.2f;
        car.speed = 4.6f;
    }
    else
    {
        car.x = -18.0f;
        car.z = -1.2f;
        car.speed = 5.1f;
    }

    // Color by vehicle type (not by lane)
    // 0,1=car -> purple, 2=bus -> blue, 3=bike -> red, 4=truck -> dark yellow
    switch (car.vehicleType)
    {
        case 0: case 1: // car - purple
            car.r = 0.55f; car.g = 0.05f; car.b = 0.75f; break;
        case 2: // bus - blue
            car.r = 0.10f; car.g = 0.25f; car.b = 0.85f; break;
        case 3: // bike - red
            car.r = 0.85f; car.g = 0.08f; car.b = 0.08f; break;
        case 4: // truck - dark yellow
            car.r = 0.72f; car.g = 0.55f; car.b = 0.05f; break;
        default:
            car.r = 0.5f; car.g = 0.5f; car.b = 0.5f; break;
    }
}

void spawnCar(int currentTime)
{
    const int spawnInterval = 1200;
    if (currentTime - lastSpawnTime < spawnInterval)
        return;

    // Vehicle type sequence: car, car, bus, bike, truck (repeat)
    // Types 0,1 = car; 2 = bus; 3 = bike; 4 = truck
    static const int vehicleSequence[] = {0, 1, 2, 3, 4};
    static const int seqLen = 5;

    for (int i = 0; i < MAX_CARS; ++i)
    {
        if (!cars[i].active)
        {
            cars[i].vehicleType = vehicleSequence[nextVehicleType % seqLen];
            nextVehicleType = (nextVehicleType + 1) % seqLen;
            resetCar(cars[i], nextSpawnDirection);
            nextSpawnDirection = (nextSpawnDirection + 1) % 4;
            lastSpawnTime = currentTime;
            return;
        }
    }
}

void updateCars(float dt, int currentTime)
{
    spawnCar(currentTime);

    const float minGap = 2.2f; // Min follow dist
    const float resetDistance = 20.0f;

    // Handle queues per direction
    for (int dir = 0; dir < 4; ++dir)
    {
        std::vector<int> laneIndices;
        for (int i = 0; i < MAX_CARS; ++i)
            if (cars[i].active && cars[i].direction == dir)
                laneIndices.push_back(i);

        if (laneIndices.empty()) continue;

        // Leader first
        if (dir == 0)
            std::sort(laneIndices.begin(), laneIndices.end(), [&](int a,int b){ return cars[a].z > cars[b].z; });
        else if (dir == 1) // moving -Z
            std::sort(laneIndices.begin(), laneIndices.end(), [&](int a,int b){ return cars[a].z < cars[b].z; });
        else if (dir == 2) // moving -X
            std::sort(laneIndices.begin(), laneIndices.end(), [&](int a,int b){ return cars[a].x < cars[b].x; });
        else // dir == 3 moving +X
            std::sort(laneIndices.begin(), laneIndices.end(), [&](int a,int b){ return cars[a].x > cars[b].x; });

        for (size_t idx = 0; idx < laneIndices.size(); ++idx)
        {
            Car &car = cars[laneIndices[idx]];

            float stopLinePos = 0.0f;
            bool isNorthSouth = (dir == 0 || dir == 1);
            if (dir == 0) stopLinePos = -5.4f;
            if (dir == 1) stopLinePos = 5.4f;
            if (dir == 2) stopLinePos = 5.4f;
            if (dir == 3) stopLinePos = -5.4f;

            float move = car.speed * dt;

            if (idx == 0)
            {
                if (isNorthSouth)
                {
                    if (!northSouthGreen && dir == 0)
                    {
                        float maxZ = stopLinePos;
                        if (car.z + move > maxZ)
                            car.z = maxZ;
                        else
                            car.z += move;
                    }
                    else if (!northSouthGreen && dir == 1)
                    {
                        float minZ = stopLinePos;
                        if (car.z - move < minZ)
                            car.z = minZ;
                        else
                            car.z -= move;
                    }
                    else
                    {
                        if (dir == 0) car.z += move; else car.z -= move;
                    }
                }
                else
                {
                    if (northSouthGreen)
                    {
                        if (dir == 2)
                        {
                            float minX = stopLinePos;
                            if (car.x - move < minX)
                                car.x = minX;
                            else
                                car.x -= move;
                        }
                        else
                        {
                            float maxX = stopLinePos;
                            if (car.x + move > maxX)
                                car.x = maxX;
                            else
                                car.x += move;
                        }
                    }
                    else
                    {
                        if (dir == 2) car.x -= move; else car.x += move;
                    }
                }
            }
            else
            {
                Car &lead = cars[laneIndices[idx-1]];

                if (isNorthSouth)
                {
                    float distance = 0.0f;
                    if (dir == 0) distance = lead.z - car.z;
                    else distance = car.z - lead.z;

                    float maxMove = distance - minGap;
                    if (maxMove < 0.0f) maxMove = 0.0f;
                    float actualMove = std::min(move, maxMove);

                    if (dir == 0) car.z += actualMove; else car.z -= actualMove;
                }
                else
                {
                    float distance = 0.0f;
                    if (dir == 2) distance = car.x - lead.x; else distance = lead.x - car.x;
                    float maxMove = distance - minGap;
                    if (maxMove < 0.0f) maxMove = 0.0f;
                    float actualMove = std::min(move, maxMove);

                    if (dir == 2) car.x -= actualMove; else car.x += actualMove;
                }
            }

            // Despawn out of bounds
            if (car.direction == 0 && car.z > resetDistance) car.active = false;
            if (car.direction == 1 && car.z < -resetDistance) car.active = false;
            if (car.direction == 2 && car.x < -resetDistance) car.active = false;
            if (car.direction == 3 && car.x > resetDistance) car.active = false;
        }
    }
}

void drawGround()
{
    // Concrete base
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_QUADS);
        glVertex3f(-45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f,  45.0f);
        glVertex3f(-45.0f, -0.01f,  45.0f);
    glEnd();

    // Darker tiles for detail
    glColor3f(0.40f, 0.40f, 0.40f);
    for (float x = -44.0f; x < 44.0f; x += 8.0f)
    {
        for (float z = -44.0f; z < 44.0f; z += 8.0f)
        {
            glBegin(GL_QUADS);
                glVertex3f(x, -0.009f, z);
                glVertex3f(x + 6.0f, -0.009f, z);
                glVertex3f(x + 6.0f, -0.009f, z + 6.0f);
                glVertex3f(x, -0.009f, z + 6.0f);
            glEnd();
        }
    }
}

// Draw a building with a window grid on all 4 vertical faces
void drawBuildingWithWindows(float bx, float bz, float sx, float sy, float sz,
                             float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(bx, sy * 0.5f, bz);

    // ---- Main body ----
    glColor3f(r, g, b);
    drawBox(sx, sy, sz);

    // ---- Roof trim (slightly darker) ----
    glColor3f(std::max(0.0f, r - 0.12f), std::max(0.0f, g - 0.12f), std::max(0.0f, b - 0.10f));
    glPushMatrix();
    glTranslatef(0.0f, sy * 0.5f - 0.12f, 0.0f);
    drawBox(sx + 0.12f, 0.22f, sz + 0.12f);
    glPopMatrix();

    // ---- Windows ----
    // Window size and grid spacing
    const float winW  = 0.32f;
    const float winH  = 0.42f;
    const float eps   = 0.025f;    // push windows just outside face
    const float spacX = 1.0f;      // horizontal spacing
    const float spacY = 1.05f;     // vertical spacing
    const float yStart = -sy * 0.5f + 0.65f; // first row baseline

    int colsX = std::max(1, (int)(sx / spacX)); // cols on +Z/-Z faces
    int colsZ = std::max(1, (int)(sz / spacX)); // cols on +X/-X faces
    int rows   = std::max(1, (int)((sy - 0.9f) / spacY));

    // Helper lambda: pick window color based on row/col pattern
    auto winColor = [](int row, int col, int seed) {
        int v = (row * 7 + col * 3 + seed) % 5;
        if (v == 0) { // unlit
            glColor3f(0.10f, 0.16f, 0.32f);
        } else { // lit warm yellow
            float bright = 0.80f + (v % 3) * 0.06f;
            glColor3f(bright, bright * 0.88f, bright * 0.45f);
        }
    };

    // +Z face
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < colsX; ++col) {
            winColor(row, col, 1);
            float wx = -sx * 0.5f + spacX * (col + 0.5f);
            float wy = yStart + spacY * row;
            float wz =  sz * 0.5f + eps;
            glBegin(GL_QUADS);
                glVertex3f(wx - winW*0.5f, wy,          wz);
                glVertex3f(wx + winW*0.5f, wy,          wz);
                glVertex3f(wx + winW*0.5f, wy + winH,   wz);
                glVertex3f(wx - winW*0.5f, wy + winH,   wz);
            glEnd();
        }
    }

    // -Z face
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < colsX; ++col) {
            winColor(row, col, 2);
            float wx = -sx * 0.5f + spacX * (col + 0.5f);
            float wy = yStart + spacY * row;
            float wz = -sz * 0.5f - eps;
            glBegin(GL_QUADS);
                glVertex3f(wx + winW*0.5f, wy,          wz);
                glVertex3f(wx - winW*0.5f, wy,          wz);
                glVertex3f(wx - winW*0.5f, wy + winH,   wz);
                glVertex3f(wx + winW*0.5f, wy + winH,   wz);
            glEnd();
        }
    }

    // +X face
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < colsZ; ++col) {
            winColor(row, col, 3);
            float wx =  sx * 0.5f + eps;
            float wy = yStart + spacY * row;
            float wz = -sz * 0.5f + spacX * (col + 0.5f);
            glBegin(GL_QUADS);
                glVertex3f(wx, wy,         wz - winW*0.5f);
                glVertex3f(wx, wy,         wz + winW*0.5f);
                glVertex3f(wx, wy + winH,  wz + winW*0.5f);
                glVertex3f(wx, wy + winH,  wz - winW*0.5f);
            glEnd();
        }
    }

    // -X face
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < colsZ; ++col) {
            winColor(row, col, 4);
            float wx = -sx * 0.5f - eps;
            float wy = yStart + spacY * row;
            float wz = -sz * 0.5f + spacX * (col + 0.5f);
            glBegin(GL_QUADS);
                glVertex3f(wx, wy,         wz + winW*0.5f);
                glVertex3f(wx, wy,         wz - winW*0.5f);
                glVertex3f(wx, wy + winH,  wz - winW*0.5f);
                glVertex3f(wx, wy + winH,  wz + winW*0.5f);
            glEnd();
        }
    }

    glPopMatrix();
}

void drawPlayground(float px, float pz)
{
    glPushMatrix();
    glTranslatef(px, 0.0f, pz);

    // --- Grass base ---
    glColor3f(0.22f, 0.68f, 0.22f);
    glBegin(GL_QUADS);
        glVertex3f(-3.5f, 0.012f, -3.5f);
        glVertex3f( 3.5f, 0.012f, -3.5f);
        glVertex3f( 3.5f, 0.012f,  3.5f);
        glVertex3f(-3.5f, 0.012f,  3.5f);
    glEnd();

    // --- Sandbox (tan square in corner) ---
    glColor3f(0.85f, 0.75f, 0.45f);
    glBegin(GL_QUADS);
        glVertex3f( 1.2f, 0.02f,  1.2f);
        glVertex3f( 3.0f, 0.02f,  1.2f);
        glVertex3f( 3.0f, 0.02f,  3.0f);
        glVertex3f( 1.2f, 0.02f,  3.0f);
    glEnd();
    // Sandbox border
    glColor3f(0.55f, 0.35f, 0.15f);
    glPushMatrix(); glTranslatef( 2.1f, 0.06f,  1.2f); drawBox(1.8f, 0.12f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.1f, 0.06f,  3.0f); drawBox(1.8f, 0.12f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 1.2f, 0.06f,  2.1f); drawBox(0.10f, 0.12f, 1.8f); glPopMatrix();
    glPushMatrix(); glTranslatef( 3.0f, 0.06f,  2.1f); drawBox(0.10f, 0.12f, 1.8f); glPopMatrix();

    // --- Swing set ---
    // Two A-frame legs (left)
    glColor3f(0.55f, 0.55f, 0.60f);
    // Left post pair
    glPushMatrix(); glTranslatef(-2.5f, 0.75f, -1.2f); glRotatef(12.0f, 0,0,1); drawBox(0.10f, 1.5f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.5f, 0.75f,  1.2f); glRotatef(-12.0f,0,0,1); drawBox(0.10f, 1.5f, 0.10f); glPopMatrix();
    // Right post pair
    glPushMatrix(); glTranslatef( 0.2f, 0.75f, -1.2f); glRotatef(12.0f, 0,0,1); drawBox(0.10f, 1.5f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.2f, 0.75f,  1.2f); glRotatef(-12.0f,0,0,1); drawBox(0.10f, 1.5f, 0.10f); glPopMatrix();
    // Top bar
    glPushMatrix(); glTranslatef(-1.15f, 1.55f, 0.0f); drawBox(1.5f, 0.10f, 0.10f); glPopMatrix();
    // Swing 1 chains
    glColor3f(0.70f, 0.70f, 0.70f);
    glPushMatrix(); glTranslatef(-1.6f, 1.1f, -0.5f); drawBox(0.04f, 0.9f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.6f, 1.1f,  0.5f); drawBox(0.04f, 0.9f, 0.04f); glPopMatrix();
    // Swing 1 seat
    glColor3f(0.20f, 0.40f, 0.80f);
    glPushMatrix(); glTranslatef(-1.6f, 0.62f, 0.0f); drawBox(0.50f, 0.06f, 0.24f); glPopMatrix();
    // Swing 2 chains
    glColor3f(0.70f, 0.70f, 0.70f);
    glPushMatrix(); glTranslatef(-0.65f, 1.1f, -0.5f); drawBox(0.04f, 0.9f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f, 1.1f,  0.5f); drawBox(0.04f, 0.9f, 0.04f); glPopMatrix();
    // Swing 2 seat
    glColor3f(0.85f, 0.25f, 0.25f);
    glPushMatrix(); glTranslatef(-0.65f, 0.62f, 0.0f); drawBox(0.50f, 0.06f, 0.24f); glPopMatrix();

    // --- Slide ---
    // Platform
    glColor3f(0.90f, 0.65f, 0.10f);
    glPushMatrix(); glTranslatef( 2.5f, 0.90f, -2.5f); drawBox(0.90f, 0.10f, 0.90f); glPopMatrix();
    // Support legs
    glColor3f(0.55f, 0.55f, 0.60f);
    glPushMatrix(); glTranslatef( 2.1f, 0.44f, -2.1f); drawBox(0.08f, 0.88f, 0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.9f, 0.44f, -2.1f); drawBox(0.08f, 0.88f, 0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.1f, 0.44f, -2.9f); drawBox(0.08f, 0.88f, 0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.9f, 0.44f, -2.9f); drawBox(0.08f, 0.88f, 0.08f); glPopMatrix();
    // Ramp (inclined quad)
    glColor3f(0.85f, 0.30f, 0.30f);
    glBegin(GL_QUADS);
        glVertex3f( 2.1f, 0.85f, -2.2f);
        glVertex3f( 2.9f, 0.85f, -2.2f);
        glVertex3f( 2.9f, 0.02f, -0.6f);
        glVertex3f( 2.1f, 0.02f, -0.6f);
    glEnd();
    // Slide side rails
    glColor3f(0.90f, 0.65f, 0.10f);
    glBegin(GL_QUADS);
        glVertex3f( 2.1f, 0.85f, -2.2f);
        glVertex3f( 2.1f, 0.22f, -2.2f);
        glVertex3f( 2.1f, 0.02f, -0.6f);
        glVertex3f( 2.1f, 0.22f, -0.6f);
    glEnd();
    glBegin(GL_QUADS);
        glVertex3f( 2.9f, 0.22f, -2.2f);
        glVertex3f( 2.9f, 0.85f, -2.2f);
        glVertex3f( 2.9f, 0.22f, -0.6f);
        glVertex3f( 2.9f, 0.02f, -0.6f);
    glEnd();

    glPopMatrix();
}

void drawBuildings()
{
    // 4 quadrants
    // Spread out buildings and trees

    struct Item {
        int type; // 0: building, 1: tree, 2: playground
        float x, z;
        float sx, sy, sz; // Bldg scale
        float r, g, b; // Bldg color
    };

    Item items[] = {
        // NW
        {0, -18.0f, -18.0f, 6.0f, 5.0f, 6.0f, 0.55f, 0.55f, 0.60f},
        {0, -32.0f, -24.0f, 8.0f, 4.0f, 8.0f, 0.50f, 0.52f, 0.58f},
        {1, -12.0f, -28.0f, 0, 0, 0, 0, 0, 0},
        {1, -26.0f, -14.0f, 0, 0, 0, 0, 0, 0},
        {1, -20.0f, -34.0f, 0, 0, 0, 0, 0, 0},

        // NE
        {0,  20.0f, -16.0f, 6.0f, 6.0f, 8.0f, 0.58f, 0.56f, 0.62f},
        {0,  30.0f, -30.0f, 7.0f, 4.5f, 7.0f, 0.52f, 0.55f, 0.59f},
        {1,  14.0f, -26.0f, 0, 0, 0, 0, 0, 0},
        {1,  28.0f, -14.0f, 0, 0, 0, 0, 0, 0},
        {1,  18.0f, -34.0f, 0, 0, 0, 0, 0, 0},

        // SW
        {0, -22.0f,  20.0f, 7.0f, 5.5f, 7.0f, 0.56f, 0.58f, 0.60f},
        {0, -16.0f,  34.0f, 6.0f, 4.0f, 6.0f, 0.53f, 0.51f, 0.55f},
        {1, -12.0f,  16.0f, 0, 0, 0, 0, 0, 0},
        {1, -30.0f,  26.0f, 0, 0, 0, 0, 0, 0},
        {1, -24.0f,  12.0f, 0, 0, 0, 0, 0, 0},

        // SE
        {0,  18.0f,  22.0f, 6.0f, 7.0f, 6.0f, 0.54f, 0.54f, 0.61f},
        {0,  32.0f,  28.0f, 8.0f, 3.5f, 6.0f, 0.51f, 0.53f, 0.57f},
        {1,  14.0f,  32.0f, 0, 0, 0, 0, 0, 0},
        {1,  30.0f,  16.0f, 0, 0, 0, 0, 0, 0},
        {1,  22.0f,  12.0f, 0, 0, 0, 0, 0, 0},

        // --- Extra buildings ---
        // NW: tall slim skyscraper
        {0, -26.0f, -36.0f, 4.0f, 9.0f, 4.0f, 0.48f, 0.50f, 0.58f},
        // NE: wide low warehouse
        {0,  38.0f, -20.0f, 9.0f, 2.5f, 6.0f, 0.57f, 0.54f, 0.50f},
        // NE: mid-rise office block
        {0,  24.0f, -38.0f, 5.0f, 7.0f, 5.0f, 0.60f, 0.58f, 0.65f},
        // SW: slim glass tower
        {0, -36.0f,  16.0f, 3.5f, 10.0f, 3.5f, 0.52f, 0.60f, 0.68f},
        // SW: large block
        {0, -30.0f,  38.0f, 8.0f, 5.0f, 7.0f, 0.54f, 0.52f, 0.56f},
        // SE: corner tower
        {0,  36.0f,  36.0f, 5.0f, 8.0f, 5.0f, 0.56f, 0.57f, 0.64f},

        // --- More buildings ---
        // NW: squat wide block near road
        {0, -14.0f, -20.0f, 5.0f, 3.5f, 7.0f, 0.53f, 0.56f, 0.54f},
        // NE: tall narrow tower
        {0,  36.0f, -10.0f, 3.0f, 11.0f, 3.0f, 0.50f, 0.55f, 0.65f},
        // SW: medium brick-tone block
        {0, -38.0f,  30.0f, 6.0f, 6.0f, 6.0f, 0.60f, 0.50f, 0.46f},
        // SE: long low strip
        {0,  22.0f,  36.0f, 10.0f, 3.0f, 5.0f, 0.52f, 0.54f, 0.58f},
        // NW: mid office with warm tone
        {0, -38.0f, -12.0f, 5.0f, 8.0f, 5.0f, 0.62f, 0.58f, 0.52f},

        // --- Fill-in buildings (mid zones) ---
        // NW mid: small shop block
        {0, -10.0f, -14.0f, 4.0f, 2.8f, 5.0f, 0.58f, 0.55f, 0.50f},
        // NE mid: medium office
        {0,  14.0f, -14.0f, 5.0f, 5.0f, 5.0f, 0.56f, 0.58f, 0.63f},
        // NE outer: secondary block
        {0,  38.0f, -36.0f, 6.0f, 4.5f, 6.0f, 0.53f, 0.52f, 0.57f},
        // SW mid: corner low block
        {0, -12.0f,  22.0f, 4.5f, 3.0f, 4.5f, 0.55f, 0.57f, 0.54f},
        // SE mid: compact office
        {0,  14.0f,  16.0f, 5.0f, 6.0f, 5.0f, 0.54f, 0.55f, 0.62f},
        // SW outer: tall residential
        {0, -20.0f,  38.0f, 4.0f, 7.5f, 4.0f, 0.57f, 0.53f, 0.58f},
        // SE outer: wide flat depot
        {0,  36.0f,  18.0f, 7.0f, 2.5f, 8.0f, 0.52f, 0.54f, 0.50f},

        // --- Fill-in trees (break up empty concrete) ---
        {1, -10.0f, -16.0f, 0, 0, 0, 0, 0, 0},
        {1,  12.0f, -18.0f, 0, 0, 0, 0, 0, 0},
        {1,  10.0f,  14.0f, 0, 0, 0, 0, 0, 0},
        {1, -10.0f,  28.0f, 0, 0, 0, 0, 0, 0},
        {1,  34.0f, -38.0f, 0, 0, 0, 0, 0, 0},
        {1, -36.0f, -30.0f, 0, 0, 0, 0, 0, 0},
        {1,  26.0f,  28.0f, 0, 0, 0, 0, 0, 0},
        {1, -16.0f,  12.0f, 0, 0, 0, 0, 0, 0},

        // --- More buildings ---
        // NW outer: tall cold-grey slab
        {0, -34.0f, -38.0f, 5.0f, 9.5f, 5.0f, 0.50f, 0.52f, 0.60f},
        // NE mid: squat brick block
        {0,  16.0f, -30.0f, 6.0f, 4.0f, 5.0f, 0.61f, 0.52f, 0.47f},
        // SE mid: slim residential
        {0,  26.0f,  14.0f, 3.5f, 7.0f, 3.5f, 0.55f, 0.57f, 0.63f},
        // SW mid: low civic block
        {0, -28.0f,  14.0f, 6.0f, 3.5f, 6.0f, 0.57f, 0.60f, 0.56f},
        // NW inner: narrow office
        {0, -16.0f, -30.0f, 4.0f, 6.5f, 4.0f, 0.54f, 0.55f, 0.61f},

        // --- More trees ---
        {1, -32.0f, -10.0f, 0, 0, 0, 0, 0, 0},
        {1,  18.0f, -10.0f, 0, 0, 0, 0, 0, 0},
        {1,  32.0f,  10.0f, 0, 0, 0, 0, 0, 0},
        {1, -22.0f,  32.0f, 0, 0, 0, 0, 0, 0},
        {1,  18.0f,  30.0f, 0, 0, 0, 0, 0, 0},

        // --- Playgrounds ---
        {2, -28.0f, -10.0f, 0, 0, 0, 0, 0, 0},
        {2,  24.0f,  24.0f, 0, 0, 0, 0, 0, 0},

        // --- Even More Buildings ---
        // NW: small filler building
        {0, -18.0f, -10.0f, 4.0f, 3.0f, 4.0f, 0.51f, 0.58f, 0.51f},
        // NE: small filler building
        {0,  18.0f, -22.0f, 3.5f, 4.0f, 3.5f, 0.59f, 0.51f, 0.59f},
        // SW: small filler building
        {0, -20.0f,  26.0f, 4.0f, 4.5f, 4.0f, 0.53f, 0.58f, 0.61f},
        // SE: small filler building
        {0,  28.0f,  20.0f, 4.5f, 3.5f, 4.5f, 0.61f, 0.55f, 0.52f},

        // --- Even More Trees ---
        {1, -14.0f, -10.0f, 0, 0, 0, 0, 0, 0},
        {1,  22.0f, -18.0f, 0, 0, 0, 0, 0, 0},
        {1, -26.0f,  20.0f, 0, 0, 0, 0, 0, 0},
        {1,  20.0f,  28.0f, 0, 0, 0, 0, 0, 0},
        {1,  10.0f, -20.0f, 0, 0, 0, 0, 0, 0},
        {1, -12.0f,  32.0f, 0, 0, 0, 0, 0, 0}
    };

    int numItems = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < numItems; ++i)
    {
        if (items[i].type == 0)
        {
            drawBuildingWithWindows(
                items[i].x, items[i].z,
                items[i].sx, items[i].sy, items[i].sz,
                items[i].r,  items[i].g,  items[i].b);
        }
        else if (items[i].type == 1)
        {
            drawGrassPatch(items[i].x, items[i].z);
            drawTree(items[i].x, items[i].z);
        }
        else if (items[i].type == 2)
        {
            drawPlayground(items[i].x, items[i].z);
        }
    }
}

// Congestion tracking
float occupancyNS = 0.0f; // N/S occ (0-1)
float occupancyEW = 0.0f; // E/W occ (0-1)
int laneCapacity = 1;

void drawTree(float x, float z)
{
    // Basic tree
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glColor3f(0.12f, 0.45f, 0.12f);
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glutSolidCone(0.8f, 2.0f, 12, 8);
    glPopMatrix();
    glColor3f(0.4f, 0.25f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.3f, 0.0f);
    drawBox(0.2f, 0.6f, 0.2f);
    glPopMatrix();
    glPopMatrix();
}

void drawGrassPatch(float x, float z)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glColor3f(0.18f, 0.6f, 0.18f);
    glBegin(GL_QUADS);
        glVertex3f(-2.5f, 0.01f, -2.5f);
        glVertex3f( 2.5f, 0.01f, -2.5f);
        glVertex3f( 2.5f, 0.01f,  2.5f);
        glVertex3f(-2.5f, 0.01f,  2.5f);
    glEnd();
    glPopMatrix();
}

void computeOccupancy()
{
    const float minGap = 2.2f;
    const float approachLength = 12.6f; // Spawn to stop line dist
    int cap_per_lane = std::max(1, int(approachLength / minGap));
    laneCapacity = cap_per_lane * 2; // Both ways

    int ns_count = 0; // North/South cars
    int ew_count = 0; // East/West cars

    for (int i = 0; i < MAX_CARS; ++i)
    {
        if (!cars[i].active) continue;
        if (cars[i].direction == 0)
        {
            if (cars[i].z >= -18.0f && cars[i].z <= -5.4f) ns_count++;
        }
        else if (cars[i].direction == 1)
        {
            if (cars[i].z <= 18.0f && cars[i].z >= 5.4f) ns_count++;
        }
        else if (cars[i].direction == 2)
        {
            if (cars[i].x <= 18.0f && cars[i].x >= 5.4f) ew_count++;
        }
        else if (cars[i].direction == 3)
        {
            if (cars[i].x >= -18.0f && cars[i].x <= -5.4f) ew_count++;
        }
    }

    // 2 lanes per axis
    occupancyNS = float(ns_count) / float(cap_per_lane * 2);
    occupancyEW = float(ew_count) / float(cap_per_lane * 2);

    if (occupancyNS > 1.0f) occupancyNS = 1.0f;
    if (occupancyEW > 1.0f) occupancyEW = 1.0f;

    // Fail on full approach
    if ((occupancyNS >= 1.0f || occupancyEW >= 1.0f) && !gameOver)
    {
        gameOver = true;
        gameOverTime = glutGet(GLUT_ELAPSED_TIME);
    }
}


void drawText2D(int x, int y, const char* text)
{
    glRasterPos2i(x, y);
    for (const char* p = text; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

void drawHUD()
{
    computeOccupancy();

    // Ortho for HUD
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    // NS/EW occupancy — centered at bottom, black
    {
        char nsb[64], ewb[64];
        int nsCountDisplay = int(round(occupancyNS * laneCapacity));
        int ewCountDisplay = int(round(occupancyEW * laneCapacity));
        sprintf(nsb, "NS: %.0f%%  EW: %.0f%%  (capacity %d)",
                occupancyNS * 100.0f, occupancyEW * 100.0f, laneCapacity);
        int bw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)nsb);
        int bx = (windowWidth - bw) / 2;
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText2D(bx, 28, nsb);
    }

    // Controls - centered at top
    {
        const char* ctrl = "R=restart  Space=toggle lights  Esc=quit";
        int ctrlW = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)ctrl);
        int ctrlX = (windowWidth - ctrlW) / 2;
        drawText2D(ctrlX, windowHeight - 24, ctrl);
    }

    // Timer — centered, black, just below controls
    {
        int elapsedMs = glutGet(GLUT_ELAPSED_TIME) - simulationStartTime;
        if (elapsedMs < 0) elapsedMs = 0;
        int totalSecs = elapsedMs / 1000;
        int mins = totalSecs / 60;
        int secs = totalSecs % 60;
        char timeBuf[32];
        sprintf(timeBuf, "Time: %02d:%02d", mins, secs);
        int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)timeBuf);
        int tx = (windowWidth - tw) / 2;
        glColor3f(0.0f, 0.0f, 0.0f);
        drawText2D(tx, windowHeight - 46, timeBuf);
    }

    // Dim overlay on game over

    if (gameOver)
    {
        int w = windowWidth;
        int h = windowHeight;
        int bw = (w * 6) / 10;
        int bh = (h * 3) / 10;
        int bx = (w - bw) / 2;
        int by = (h - bh) / 2;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glBegin(GL_QUADS);
            glVertex2i(bx, by);
            glVertex2i(bx + bw, by);
            glVertex2i(bx + bw, by + bh);
            glVertex2i(bx, by + bh);
        glEnd();
        glDisable(GL_BLEND);

        const char* go = "GAME OVER";
        const char* hint = "Press R to restart";
        glColor3f(1.0f, 0.2f, 0.2f);
        int gx = w / 2 - (int)(6 * strlen(go));
        int gy = h / 2 + 10;
        drawText2D(gx, gy, go);
        glColor3f(1.0f, 1.0f, 1.0f);
        int hx = w / 2 - (int)(7 * strlen(hint));
        drawText2D(hx, gy - 28, hint);

        // Restart timer
        if (gameOverTime > 0)
        {
            int elapsedMs = glutGet(GLUT_ELAPSED_TIME) - gameOverTime;
            int secsLeft = autoRestartSeconds - (elapsedMs / 1000);
            if (secsLeft < 0) secsLeft = 0;
            char tbuf[64];
            sprintf(tbuf, "Restarting in %d s", secsLeft);
            int tx = w / 2 - (int)(7 * strlen(tbuf));
            drawText2D(tx, gy - 56, tbuf);
        }
    }

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// End congestion tracking

void drawRoad()
{
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
        glVertex3f(-3.5f, 0.0f, -40.0f);
        glVertex3f( 3.5f, 0.0f, -40.0f);
        glVertex3f( 3.5f, 0.0f,  40.0f);
        glVertex3f(-3.5f, 0.0f,  40.0f);

        glVertex3f(-40.0f, 0.0f, -3.5f);
        glVertex3f( 40.0f, 0.0f, -3.5f);
        glVertex3f( 40.0f, 0.0f,  3.5f);
        glVertex3f(-40.0f, 0.0f,  3.5f);
    glEnd();
}

void drawSidewalks()
{
    glColor3f(0.6f, 0.6f, 0.6f);

    glBegin(GL_QUADS);
        glVertex3f(-5.2f, 0.015f, -40.0f);
        glVertex3f(-3.7f, 0.015f, -40.0f);
        glVertex3f(-3.7f, 0.015f,  40.0f);
        glVertex3f(-5.2f, 0.015f,  40.0f);

        glVertex3f( 3.7f, 0.015f, -40.0f);
        glVertex3f( 5.2f, 0.015f, -40.0f);
        glVertex3f( 5.2f, 0.015f,  40.0f);
        glVertex3f( 3.7f, 0.015f,  40.0f);

        glVertex3f(-40.0f, 0.015f, -5.2f);
        glVertex3f( 40.0f, 0.015f, -5.2f);
        glVertex3f( 40.0f, 0.015f, -3.7f);
        glVertex3f(-40.0f, 0.015f, -3.7f);

        glVertex3f(-40.0f, 0.015f,  3.7f);
        glVertex3f( 40.0f, 0.015f,  3.7f);
        glVertex3f( 40.0f, 0.015f,  5.2f);
        glVertex3f(-40.0f, 0.015f,  5.2f);
    glEnd();
}

void drawLaneMarkings()
{
    glColor3f(0.95f, 0.95f, 0.2f);

    for (float z = -37.0f; z <= 37.0f; z += 4.0f)
    {
        if (z > -2.0f && z < 2.0f)
            continue;

        glBegin(GL_QUADS);
            glVertex3f(-0.15f, 0.02f, z);
            glVertex3f( 0.15f, 0.02f, z);
            glVertex3f( 0.15f, 0.02f, z + 2.0f);
            glVertex3f(-0.15f, 0.02f, z + 2.0f);
        glEnd();
    }

    for (float x = -37.0f; x <= 37.0f; x += 4.0f)
    {
        if (x > -2.0f && x < 2.0f)
            continue;

        glBegin(GL_QUADS);
            glVertex3f(x, 0.02f, -0.15f);
            glVertex3f(x + 2.0f, 0.02f, -0.15f);
            glVertex3f(x + 2.0f, 0.02f,  0.15f);
            glVertex3f(x, 0.02f,  0.15f);
        glEnd();
    }
}

void drawStopLines()
{
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
        glVertex3f(-3.2f, 0.021f, -5.0f);
        glVertex3f( 3.2f, 0.021f, -5.0f);
        glVertex3f( 3.2f, 0.021f, -4.6f);
        glVertex3f(-3.2f, 0.021f, -4.6f);

        glVertex3f(-3.2f, 0.021f,  4.6f);
        glVertex3f( 3.2f, 0.021f,  4.6f);
        glVertex3f( 3.2f, 0.021f,  5.0f);
        glVertex3f(-3.2f, 0.021f,  5.0f);

        glVertex3f(-5.0f, 0.021f, -3.2f);
        glVertex3f(-4.6f, 0.021f, -3.2f);
        glVertex3f(-4.6f, 0.021f,  3.2f);
        glVertex3f(-5.0f, 0.021f,  3.2f);

        glVertex3f( 4.6f, 0.021f, -3.2f);
        glVertex3f( 5.0f, 0.021f, -3.2f);
        glVertex3f( 5.0f, 0.021f,  3.2f);
        glVertex3f( 4.6f, 0.021f,  3.2f);
    glEnd();
}

void drawIntersectionFootprint()
{
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex3f(-4.0f, 0.015f, -4.0f);
        glVertex3f( 4.0f, 0.015f, -4.0f);
        glVertex3f( 4.0f, 0.015f,  4.0f);
        glVertex3f(-4.0f, 0.015f,  4.0f);
    glEnd();
}

void drawBox(float sx, float sy, float sz)
{
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawTrafficLight(float x, float z, float rotationY, bool verticalFacingGreen)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    glColor3f(0.25f, 0.25f, 0.25f);
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 0.0f);
    drawBox(0.2f, 4.0f, 0.2f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 4.1f, 0.0f);
    drawBox(0.3f, 0.3f, 0.3f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 3.8f, 0.18f);
    drawBox(0.6f, 0.9f, 0.35f);
    glPopMatrix();

    bool greenActive = (northSouthGreen == verticalFacingGreen);

    // Dim inactive lights
    float redActiveR = 0.95f, redActiveG = 0.05f, redActiveB = 0.05f;
    float redDimR = 0.25f, redDimG = 0.05f, redDimB = 0.05f;

    float yellowR = 0.9f, yellowG = 0.8f, yellowB = 0.1f;

    float greenActiveR = 0.05f, greenActiveG = 0.95f, greenActiveB = 0.05f;
    float greenDimR = 0.05f, greenDimG = 0.25f, greenDimB = 0.05f;

    glPushMatrix();
    glTranslatef(0.0f, 4.1f, 0.36f);
    if (!greenActive) glColor3f(redActiveR, redActiveG, redActiveB); else glColor3f(redDimR, redDimG, redDimB);
    glutSolidSphere(0.12, 14, 14);
    glTranslatef(0.0f, -0.28f, 0.0f);
    glColor3f(yellowR, yellowG, yellowB);
    glutSolidSphere(0.12, 14, 14);
    glTranslatef(0.0f, -0.28f, 0.0f);
    if (greenActive) glColor3f(greenActiveR, greenActiveG, greenActiveB); else glColor3f(greenDimR, greenDimG, greenDimB);
    glutSolidSphere(0.12, 14, 14);
    glPopMatrix();

    glPopMatrix();
}

void drawTrafficLights()
{
    drawTrafficLight(-5.9f, -5.9f, 90.0f, false);
    drawTrafficLight( 5.9f, -5.9f, 180.0f, true);
    drawTrafficLight(-5.9f,  5.9f, 0.0f, true);
    drawTrafficLight( 5.9f,  5.9f, -90.0f, false);
}

// Draw a single wheel (small dark cube)
void drawWheel(float ox, float oy, float oz)
{
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(ox, oy, oz);
    drawBox(0.22f, 0.22f, 0.22f);
    glPopMatrix();
}

void drawCar_Shape(const Car& car)
{
    // Body - purple
    glColor3f(car.r, car.g, car.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.22f, 0.0f);
    drawBox(1.0f, 0.4f, 2.0f);
    glPopMatrix();

    // Cabin (slightly darker)
    glColor3f(std::max(0.0f, car.r - 0.18f), std::max(0.0f, car.g - 0.05f), std::max(0.0f, car.b - 0.18f));
    glPushMatrix();
    glTranslatef(0.0f, 0.57f, -0.05f);
    drawBox(0.78f, 0.30f, 1.0f);
    glPopMatrix();

    // 4 wheels - black
    drawWheel(-0.40f, -0.04f,  0.65f);
    drawWheel( 0.40f, -0.04f,  0.65f);
    drawWheel(-0.40f, -0.04f, -0.65f);
    drawWheel( 0.40f, -0.04f, -0.65f);
}

void drawBus_Shape(const Car& car)
{
    // Long tall rectangular body - blue
    glColor3f(car.r, car.g, car.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.50f, 0.0f);
    drawBox(1.1f, 0.9f, 2.8f);
    glPopMatrix();

    // Roof stripe (slightly lighter)
    glColor3f(std::min(1.0f, car.r + 0.20f), std::min(1.0f, car.g + 0.20f), std::min(1.0f, car.b + 0.20f));
    glPushMatrix();
    glTranslatef(0.0f, 0.97f, 0.0f);
    drawBox(1.12f, 0.08f, 2.82f);
    glPopMatrix();

    // Windows strip (light blue-grey)
    glColor3f(0.60f, 0.80f, 0.95f);
    glPushMatrix();
    glTranslatef(0.0f, 0.58f, 0.0f);
    drawBox(1.12f, 0.28f, 2.6f);
    glPopMatrix();

    // 6 wheels (dual rear)
    drawWheel(-0.46f, -0.04f,  1.00f);
    drawWheel( 0.46f, -0.04f,  1.00f);
    drawWheel(-0.46f, -0.04f,  0.00f);
    drawWheel( 0.46f, -0.04f,  0.00f);
    drawWheel(-0.46f, -0.04f, -1.00f);
    drawWheel( 0.46f, -0.04f, -1.00f);
}

void drawBike_Shape(const Car& car)
{
    // Narrow body - red
    glColor3f(car.r, car.g, car.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.28f, 0.0f);
    drawBox(0.45f, 0.28f, 1.4f);
    glPopMatrix();

    // Fuel tank / upper body hump
    glColor3f(std::max(0.0f, car.r - 0.15f), std::max(0.0f, car.g), std::max(0.0f, car.b));
    glPushMatrix();
    glTranslatef(0.0f, 0.52f, 0.10f);
    drawBox(0.36f, 0.20f, 0.70f);
    glPopMatrix();

    // Handlebars
    glColor3f(0.55f, 0.55f, 0.55f);
    glPushMatrix();
    glTranslatef(0.0f, 0.62f, 0.60f);
    drawBox(0.55f, 0.06f, 0.10f);
    glPopMatrix();

    // 2 wheels (thinner)
    drawWheel(0.0f, -0.04f,  0.58f);
    drawWheel(0.0f, -0.04f, -0.58f);
}

void drawTruck_Shape(const Car& car)
{
    // Long cargo body - dark yellow
    glColor3f(car.r, car.g, car.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.45f, 0.40f);
    drawBox(1.2f, 0.8f, 2.2f);
    glPopMatrix();

    // Cab (front, shorter & taller)
    glColor3f(std::max(0.0f, car.r - 0.12f), std::max(0.0f, car.g - 0.10f), std::max(0.0f, car.b));
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, -1.10f);
    drawBox(1.1f, 1.0f, 0.85f);
    glPopMatrix();

    // Exhaust stack
    glColor3f(0.4f, 0.4f, 0.4f);
    glPushMatrix();
    glTranslatef(0.40f, 1.22f, -1.0f);
    drawBox(0.08f, 0.5f, 0.08f);
    glPopMatrix();

    // 6 wheels (dual rear axle)
    drawWheel(-0.52f, -0.04f,  1.10f);
    drawWheel( 0.52f, -0.04f,  1.10f);
    drawWheel(-0.52f, -0.04f,  0.20f);
    drawWheel( 0.52f, -0.04f,  0.20f);
    drawWheel(-0.52f, -0.04f, -1.00f);
    drawWheel( 0.52f, -0.04f, -1.00f);
}

void drawVehicle(const Car& car)
{
    glPushMatrix();
    glTranslatef(car.x, car.y, car.z);

    if (car.direction == 0)
        glRotatef(0.0f, 0.0f, 1.0f, 0.0f);
    else if (car.direction == 1)
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    else if (car.direction == 2)
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    else if (car.direction == 3)
        glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);

    switch (car.vehicleType)
    {
        case 0: case 1: drawCar_Shape(car);   break;
        case 2:         drawBus_Shape(car);   break;
        case 3:         drawBike_Shape(car);  break;
        case 4:         drawTruck_Shape(car); break;
        default:        drawCar_Shape(car);   break;
    }

    glPopMatrix();
}

void drawPerson(const Person& p)
{
    float t       = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float swAmt   = p.isJogger ? 38.0f : 24.0f;
    float swSpeed = p.isJogger ?  7.5f :  4.2f;
    float sw      = sinf(t * swSpeed + p.phase);
    float legA    = sw * swAmt;
    float armA    = -sw * swAmt * 0.55f;
    float bobY    = p.isJogger ? fabsf(sw) * 0.055f : 0.0f;

    glPushMatrix();
    glTranslatef(p.x, 0.02f + bobY, p.z);

    // Face direction of travel
    float rotY = 0.0f;
    if (p.sidewalk == 0 || p.sidewalk == 1)
        rotY = (p.dir > 0) ? 0.0f : 180.0f;
    else
        rotY = (p.dir > 0) ? -90.0f : 90.0f;
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glScalef(1.0f, p.heightScale, 1.0f);

    // -- Left leg (hip pivot at y=0.40) --
    glColor3f(p.pantsR, p.pantsG, p.pantsB);
    glPushMatrix();
        glTranslatef(-0.07f, 0.40f, 0.0f);
        glRotatef(legA, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.20f, 0.0f);
        drawBox(0.09f, 0.40f, 0.09f);
    glPopMatrix();
    // -- Right leg --
    glPushMatrix();
        glTranslatef(0.07f, 0.40f, 0.0f);
        glRotatef(-legA, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.20f, 0.0f);
        drawBox(0.09f, 0.40f, 0.09f);
    glPopMatrix();

    // -- Torso --
    glColor3f(p.shirtR, p.shirtG, p.shirtB);
    glPushMatrix();
        glTranslatef(0.0f, 0.60f, 0.0f);
        drawBox(0.24f, 0.36f, 0.13f);
    glPopMatrix();

    // -- Left arm (shoulder pivot at y=0.74) --
    glPushMatrix();
        glTranslatef(-0.16f, 0.74f, 0.0f);
        glRotatef(armA, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.12f, 0.0f);
        drawBox(0.07f, 0.24f, 0.07f);
    glPopMatrix();
    // -- Right arm --
    glPushMatrix();
        glTranslatef(0.16f, 0.74f, 0.0f);
        glRotatef(-armA, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.12f, 0.0f);
        drawBox(0.07f, 0.24f, 0.07f);
    glPopMatrix();

    // -- Head --
    glColor3f(p.skinR, p.skinG, p.skinB);
    glPushMatrix();
        glTranslatef(0.0f, 0.92f, 0.0f);
        glutSolidSphere(0.12f, 10, 8);
    glPopMatrix();

    glPopMatrix();
}

void drawPeople()
{
    for (int i = 0; i < MAX_PEOPLE; ++i)
        drawPerson(people[i]);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0f, 18.0f, 28.0f,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);

    drawGround();
    drawBuildings();
    drawSidewalks();
    drawRoad();
    drawIntersectionFootprint();
    drawLaneMarkings();
    drawStopLines();
    drawTrafficLights();

    for (int i = 0; i < MAX_CARS; ++i)
    {
        if (cars[i].active)
            drawVehicle(cars[i]);
    }

    drawPeople();

    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    if (h == 0) h = 1;
    float ratio = w * 1.0f / h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 0.1f, 120.0f);

    glMatrixMode(GL_MODELVIEW);
}

void idle()
{
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    if (lastUpdateTime == 0)
        lastUpdateTime = currentTime;

    float dt = (currentTime - lastUpdateTime) / 1000.0f;
    lastUpdateTime = currentTime;

    if (!gameOver)
    {
        updateCars(dt, currentTime);
        updatePeople(dt);
    }
    else if (gameOverTime > 0)
    {
        int elapsed = currentTime - gameOverTime;
        if (elapsed >= autoRestartSeconds * 1000)
            resetSimulation();
    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    mciSendString("open \"../Media/traffic_sound.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Traffic Master");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.55f, 0.75f, 0.95f, 1.0f);
    srand((unsigned)time(NULL));
    simulationStartTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}

void resetSimulation()
{
    for (int i = 0; i < MAX_CARS; ++i)
        cars[i].active = false;
    lastSpawnTime = -1200;
    lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    nextSpawnDirection = 0;
    nextVehicleType = 0;
    northSouthGreen = false;
    gameOver = false;
    gameOverTime = 0;
    occupancyNS = occupancyEW = 0.0f;
    simulationStartTime = glutGet(GLUT_ELAPSED_TIME);
    // Re-init people so they respawn at starting positions
    peopleInit = false;
    initPeople();
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'r' || key == 'R')
    {
        resetSimulation();
    }
    else if (key == ' ') // Spacebar
    {
        // Toggle traffic lights
        northSouthGreen = !northSouthGreen;
    }
    else if (key == 27) // ESC
    {
        std::exit(0);
    }
}

