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

// forward declarations
void drawBox(float sx, float sy, float sz);
void drawBuildings();
void drawHUD();
void resetSimulation();
void keyboard(unsigned char key, int x, int y);

// small decorative helpers
void drawTree(float x, float z);
void drawGrassPatch(float x, float z);
void drawSmallBuilding(float sx, float sy, float sz);

int windowWidth = 1280;
int windowHeight = 720;

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
};

const int MAX_CARS = 12;
Car cars[MAX_CARS];
int lastUpdateTime = 0;
int lastSignalChangeTime = 0;
int lastSpawnTime = -1200;
int nextSpawnDirection = 0;
bool northSouthGreen = false;
bool gameOver = false;
int gameOverTime = 0;
const int autoRestartSeconds = 5;

// manual light control mode: when true, automatic signal switching is disabled
bool manualControl = false;

void updateTrafficLights(int currentTime)
{
    // Automatic switching disabled in manual control mode
    if (manualControl)
        return;

    if (lastSignalChangeTime == 0)
        lastSignalChangeTime = currentTime;

    if (currentTime - lastSignalChangeTime >= 5000)
    {
        northSouthGreen = !northSouthGreen;
        lastSignalChangeTime = currentTime;
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

    // assign a base color per direction with a small random variation
    if (direction == 0) { car.r = 0.85f; car.g = 0.15f; car.b = 0.15f; }
    else if (direction == 1) { car.r = 0.15f; car.g = 0.15f; car.b = 0.85f; }
    else if (direction == 2) { car.r = 0.15f; car.g = 0.85f; car.b = 0.15f; }
    else { car.r = 0.85f; car.g = 0.60f; car.b = 0.15f; }

    // small random tint
    float vr = (float)(rand() % 21 - 10) / 255.0f;
    float vg = (float)(rand() % 21 - 10) / 255.0f;
    float vb = (float)(rand() % 21 - 10) / 255.0f;
    car.r = std::min(0.95f, std::max(0.05f, car.r + vr));
    car.g = std::min(0.95f, std::max(0.05f, car.g + vg));
    car.b = std::min(0.95f, std::max(0.05f, car.b + vb));
}

void spawnCar(int currentTime)
{
    const int spawnInterval = 1200;
    if (currentTime - lastSpawnTime < spawnInterval)
        return;

    for (int i = 0; i < MAX_CARS; ++i)
    {
        if (!cars[i].active)
        {
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

    const float minGap = 2.2f; // minimum following distance
    const float resetDistance = 20.0f;

    // Process each direction separately to handle queues
    for (int dir = 0; dir < 4; ++dir)
    {
        std::vector<int> laneIndices;
        for (int i = 0; i < MAX_CARS; ++i)
            if (cars[i].active && cars[i].direction == dir)
                laneIndices.push_back(i);

        if (laneIndices.empty()) continue;

        // Sort lane cars so leader is first
        if (dir == 0) // moving +Z
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

            // determine stop line for this lane
            float stopLinePos = 0.0f;
            bool isNorthSouth = (dir == 0 || dir == 1);
            if (dir == 0) stopLinePos = -5.4f;      // northbound stops before -5.4
            if (dir == 1) stopLinePos = 5.4f;       // southbound stops before 5.4
            if (dir == 2) stopLinePos = 5.4f;       // eastbound stops before x=5.4 when northSouthGreen
            if (dir == 3) stopLinePos = -5.4f;      // westbound stops before x=-5.4 when northSouthGreen

            // intended movement
            float move = car.speed * dt;

            // leader handling
            if (idx == 0)
            {
                // leader must obey red light
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
                        // green for north-south
                        if (dir == 0) car.z += move; else car.z -= move;
                    }
                }
                else
                {
                    // east-west movement
                    if (northSouthGreen)
                    {
                        // east-west must stop
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
                        // east-west green
                        if (dir == 2) car.x -= move; else car.x += move;
                    }
                }
            }
            else
            {
                // follow the car ahead
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

            // reset when out of bounds
            if (car.direction == 0 && car.z > resetDistance) car.active = false;
            if (car.direction == 1 && car.z < -resetDistance) car.active = false;
            if (car.direction == 2 && car.x < -resetDistance) car.active = false;
            if (car.direction == 3 && car.x > resetDistance) car.active = false;
        }
    }
}

void drawGround()
{
    // Concrete ground instead of grass
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_QUADS);
        glVertex3f(-45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f,  45.0f);
        glVertex3f(-45.0f, -0.01f,  45.0f);
    glEnd();

    // subtle darker tiles for visual variety
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

void drawBuildings()
{
    // The 4 quadrants (tiles areas)
    // We will place a few buildings and trees in each quadrant to make it look less crowded.

    struct Item {
        int type; // 0 = building, 1 = tree
        float x, z;
        float sx, sy, sz; // for building
        float r, g, b; // for building color
    };

    Item items[] = {
        // Top-Left (NW)
        {0, -18.0f, -18.0f, 6.0f, 5.0f, 6.0f, 0.55f, 0.55f, 0.60f},
        {0, -32.0f, -24.0f, 8.0f, 4.0f, 8.0f, 0.50f, 0.52f, 0.58f},
        {1, -12.0f, -28.0f, 0, 0, 0, 0, 0, 0},
        {1, -26.0f, -14.0f, 0, 0, 0, 0, 0, 0},
        {1, -20.0f, -34.0f, 0, 0, 0, 0, 0, 0},

        // Top-Right (NE)
        {0,  20.0f, -16.0f, 6.0f, 6.0f, 8.0f, 0.58f, 0.56f, 0.62f},
        {0,  30.0f, -30.0f, 7.0f, 4.5f, 7.0f, 0.52f, 0.55f, 0.59f},
        {1,  14.0f, -26.0f, 0, 0, 0, 0, 0, 0},
        {1,  28.0f, -14.0f, 0, 0, 0, 0, 0, 0},
        {1,  18.0f, -34.0f, 0, 0, 0, 0, 0, 0},

        // Bottom-Left (SW)
        {0, -22.0f,  20.0f, 7.0f, 5.5f, 7.0f, 0.56f, 0.58f, 0.60f},
        {0, -16.0f,  34.0f, 6.0f, 4.0f, 6.0f, 0.53f, 0.51f, 0.55f},
        {1, -12.0f,  16.0f, 0, 0, 0, 0, 0, 0},
        {1, -30.0f,  26.0f, 0, 0, 0, 0, 0, 0},
        {1, -24.0f,  12.0f, 0, 0, 0, 0, 0, 0},

        // Bottom-Right (SE)
        {0,  18.0f,  22.0f, 6.0f, 7.0f, 6.0f, 0.54f, 0.54f, 0.61f},
        {0,  32.0f,  28.0f, 8.0f, 3.5f, 6.0f, 0.51f, 0.53f, 0.57f},
        {1,  14.0f,  32.0f, 0, 0, 0, 0, 0, 0},
        {1,  30.0f,  16.0f, 0, 0, 0, 0, 0, 0},
        {1,  22.0f,  12.0f, 0, 0, 0, 0, 0, 0}
    };

    int numItems = sizeof(items) / sizeof(items[0]);

    for (int i = 0; i < numItems; ++i)
    {
        if (items[i].type == 0)
        {
            glPushMatrix();
            glTranslatef(items[i].x, items[i].sy * 0.5f, items[i].z);
            glColor3f(items[i].r, items[i].g, items[i].b);
            drawBox(items[i].sx, items[i].sy, items[i].sz);
            glPopMatrix();
        }
        else
        {
            drawGrassPatch(items[i].x, items[i].z);
            drawTree(items[i].x, items[i].z);
        }
    }
}

// --- Stage 10: Congestion tracking ---
float occupancyNS = 0.0f; // combined north/south occupancy (0..1)
float occupancyEW = 0.0f; // combined east/west occupancy (0..1)
int laneCapacity = 1;

void drawTree(float x, float z)
{
    // simple conical tree
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

void drawSmallBuilding(float sx, float sy, float sz)
{
    glPushMatrix();
    glColor3f(0.6f, 0.6f, 0.65f);
    drawBox(sx*0.375f, sy, sz*0.416f);
    glColor3f(0.3f, 0.3f, 0.35f);
    glPushMatrix();
    glTranslatef(0.0f, sy*0.3f, sz*0.32f);
    drawBox(sx*0.125f, sy*0.4f, sz*0.16f);
    glPopMatrix();
    glPopMatrix();
}


void computeOccupancy()
{
    const float minGap = 2.2f;
    const float approachLength = 12.6f; // distance from spawn (~18) to stop line (5.4)
    int cap_per_lane = std::max(1, int(approachLength / minGap));
    laneCapacity = cap_per_lane * 2; // both directions per axis

    int ns_count = 0; // northbound (dir0) + southbound (dir1)
    int ew_count = 0; // eastbound (dir2) + westbound (dir3)

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

    // two lanes per axis (north and south), so total capacity for NS = cap_per_lane * 2
    occupancyNS = float(ns_count) / float(cap_per_lane * 2);
    occupancyEW = float(ew_count) / float(cap_per_lane * 2);

    if (occupancyNS > 1.0f) occupancyNS = 1.0f;
    if (occupancyEW > 1.0f) occupancyEW = 1.0f;

    // Game over if any approach is completely full
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

    // switch to orthographic projection for HUD
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    char buf[128];
    int nsCountDisplay = int(round(occupancyNS * laneCapacity));
    int ewCountDisplay = int(round(occupancyEW * laneCapacity));
    sprintf(buf, "NS occupancy: %.0f%% (%d/%d)", occupancyNS * 100.0f, nsCountDisplay, laneCapacity);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(10, windowHeight - 24, buf);

    sprintf(buf, "EW occupancy: %.0f%% (%d/%d)", occupancyEW * 100.0f, ewCountDisplay, laneCapacity);
    drawText2D(10, windowHeight - 44, buf);

    // controls hint
    drawText2D(10, windowHeight - 64, "Controls: R=restart  L=toggle manual lights  T=toggle lights  Esc=quit");

    // If game over, draw a large centered overlay with a translucent background

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

        // show auto-restart countdown if available
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

// --- end congestion tracking ---

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

    glColor3f(0.08f, 0.08f, 0.08f);
    glBegin(GL_QUADS);
        glVertex3f(-4.0f, 0.01f, -4.0f);
        glVertex3f( 4.0f, 0.01f, -4.0f);
        glVertex3f( 4.0f, 0.01f,  4.0f);
        glVertex3f(-4.0f, 0.01f,  4.0f);
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

    // Make active lights bright and inactive ones dimmer for clarity
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

void drawCar(const Car& car)
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

    // main body color from car
    glColor3f(car.r, car.g, car.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.2f, 0.0f);
    drawBox(1.0f, 0.4f, 2.0f);
    glPopMatrix();

    // roof/upper slightly darker
    glColor3f(std::max(0.0f, car.r - 0.15f), std::max(0.0f, car.g - 0.15f), std::max(0.0f, car.b - 0.15f));
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, -0.05f);
    drawBox(0.8f, 0.3f, 1.0f);
    glPopMatrix();

    // wheels
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(-0.35f, -0.05f, 0.65f);
    drawBox(0.25f, 0.25f, 0.25f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.35f, -0.05f, 0.65f);
    drawBox(0.25f, 0.25f, 0.25f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.35f, -0.05f, -0.65f);
    drawBox(0.25f, 0.25f, 0.25f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.35f, -0.05f, -0.65f);
    drawBox(0.25f, 0.25f, 0.25f);
    glPopMatrix();

    glPopMatrix();
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
            drawCar(cars[i]);
    }

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

    if (lastSignalChangeTime == 0)
        lastSignalChangeTime = currentTime;

    float dt = (currentTime - lastUpdateTime) / 1000.0f;
    lastUpdateTime = currentTime;

    if (!gameOver)
    {
        updateTrafficLights(currentTime);
        updateCars(dt, currentTime);
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
    lastSignalChangeTime = glutGet(GLUT_ELAPSED_TIME);
    lastUpdateTime = glutGet(GLUT_ELAPSED_TIME);
    nextSpawnDirection = 0;
    northSouthGreen = false;
    gameOver = false;
    gameOverTime = 0;
    occupancyNS = occupancyEW = 0.0f;
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 'r' || key == 'R')
    {
        resetSimulation();
    }
    else if (key == 'l' || key == 'L')
    {
        // toggle manual control mode
        manualControl = !manualControl;
        // if enabling manual, freeze the current signal state
        lastSignalChangeTime = glutGet(GLUT_ELAPSED_TIME);
    }
    else if (key == 't' || key == 'T')
    {
        // manual toggle of current signal (useful when manualControl is on)
        northSouthGreen = !northSouthGreen;
        lastSignalChangeTime = glutGet(GLUT_ELAPSED_TIME);
        // if we toggle while in auto mode, this simply flips immediately
    }
    else if (key == 27) // ESC
    {
        std::exit(0);
    }
}

