#include <GL/glut.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cmath>

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
};

const int MAX_CARS = 12;
Car cars[MAX_CARS];
int lastUpdateTime = 0;
int lastSignalChangeTime = 0;
int lastSpawnTime = -1200;
int nextSpawnDirection = 0;
bool northSouthGreen = false;

void updateTrafficLights(int currentTime)
{
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
    glColor3f(0.18f, 0.45f, 0.18f);
    glBegin(GL_QUADS);
        glVertex3f(-45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f, -45.0f);
        glVertex3f( 45.0f, -0.01f,  45.0f);
        glVertex3f(-45.0f, -0.01f,  45.0f);
    glEnd();
}

// --- Stage 10: Congestion tracking ---
float occupancyNS = 0.0f; // combined north/south occupancy (0..1)
float occupancyEW = 0.0f; // combined east/west occupancy (0..1)
int laneCapacity = 1;

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

    float redR = greenActive ? 0.12f : 0.85f;
    float redG = greenActive ? 0.45f : 0.10f;
    float redB = greenActive ? 0.12f : 0.10f;

    float yellowR = 0.35f;
    float yellowG = 0.30f;
    float yellowB = 0.05f;

    float greenR = greenActive ? 0.15f : 0.05f;
    float greenG = greenActive ? 0.85f : 0.20f;
    float greenB = greenActive ? 0.15f : 0.05f;

    glPushMatrix();
    glTranslatef(0.0f, 4.1f, 0.36f);
    glColor3f(redR, redG, redB);
    glutSolidSphere(0.08, 12, 12);
    glTranslatef(0.0f, -0.22f, 0.0f);
    glColor3f(yellowR, yellowG, yellowB);
    glutSolidSphere(0.08, 12, 12);
    glTranslatef(0.0f, -0.22f, 0.0f);
    glColor3f(greenR, greenG, greenB);
    glutSolidSphere(0.08, 12, 12);
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

    glColor3f(0.85f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, 0.2f, 0.0f);
    drawBox(1.0f, 0.4f, 2.0f);
    glPopMatrix();

    glColor3f(0.65f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, -0.05f);
    drawBox(0.8f, 0.3f, 1.0f);
    glPopMatrix();

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

    updateTrafficLights(currentTime);
    updateCars(dt, currentTime);
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Traffic Master");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.55f, 0.75f, 0.95f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
