#include <GL/glut.h>
#include <cmath>

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_POLYGON);
        float r = 0.5;
        for(float t = 0; t <= 2 * M_PI; t += 0.001) {
            glVertex2f(r * cos(t), r * sin(t));
        }
    glEnd();
    glFlush();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitWindowSize(500, 500);
    glutCreateWindow("OpenGL 64bit");
    glutDisplayFunc(display);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glutMainLoop();
    return 0;
}
