#include <GL/glut.h>

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

void updateSingleCar(Car& car, float dt)
{
    const float northSouthStopNorth = -5.4f;
    const float northSouthStopSouth = 5.4f;
    const float eastWestStopWest = -5.4f;
    const float eastWestStopEast = 5.4f;
    const float resetDistance = 20.0f;

    if (car.direction == 0)
    {
        float nextZ = car.z + car.speed * dt;
        if (!northSouthGreen && nextZ > northSouthStopNorth)
            nextZ = northSouthStopNorth;
        car.z = nextZ;

        if (car.z > resetDistance)
            car.active = false;
    }
    else if (car.direction == 1)
    {
        float nextZ = car.z - car.speed * dt;
        if (!northSouthGreen && nextZ < northSouthStopSouth)
            nextZ = northSouthStopSouth;
        car.z = nextZ;

        if (car.z < -resetDistance)
            car.active = false;
    }
    else if (car.direction == 2)
    {
        float nextX = car.x - car.speed * dt;
        if (northSouthGreen && nextX < eastWestStopEast)
            nextX = eastWestStopEast;
        car.x = nextX;

        if (car.x < -resetDistance)
            car.active = false;
    }
    else
    {
        float nextX = car.x + car.speed * dt;
        if (northSouthGreen && nextX > eastWestStopWest)
            nextX = eastWestStopWest;
        car.x = nextX;

        if (car.x > resetDistance)
            car.active = false;
    }
}

void updateCars(float dt, int currentTime)
{
    spawnCar(currentTime);

    for (int i = 0; i < MAX_CARS; ++i)
    {
        if (cars[i].active)
            updateSingleCar(cars[i], dt);
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
