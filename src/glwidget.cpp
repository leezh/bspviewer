#include <iostream>
#include <cmath>

#define PI 3.14159265359
inline double rad(double angle) {return angle * PI / 180.0;}
#define cosD(x) cos(rad(x))
#define sinD(x) sin(rad(x))

#include <QKeyEvent>
#include <QMatrix4x4>
#include "glwidget.hpp"

BSPViewer::BSPViewer(QGLFormat format) : QGLWidget(format) {
    setWindowTitle("BSPViewer");
    setMinimumSize(800, 600);
    setMouseTracking(true);
    setAutoFillBackground(false);
    
    pos = QVector3D(-216, 288, 50);
    yaw = 0;
    pitch = 0;
    keyForward = false;
    keyBackward = false;
    keyStrafeLeft = false;
    keyStrafeRight = false;
    keyUpwards = false;
    keyDownwards = false;
}

void BSPViewer::initializeGL() {
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0);

    timer.setInterval(0);
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    timer.start();

    map = new BSP::Map(this);
    map->load("map.bsp");
}

void BSPViewer::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void BSPViewer::paintGL() {
    double duration = time.elapsed() / 1000.0;
    time.restart();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QVector3D lookVect(cosD(yaw) * cosD(pitch), -sinD(yaw) * cosD(pitch), -sinD(pitch));
    QVector3D walkVel;

    if (keyForward) {
        walkVel += lookVect * 1;
    }

    if (keyBackward) {
        walkVel += lookVect * -1;
    }

    if (keyStrafeLeft) {
        walkVel.setX(walkVel.x() + sinD(yaw));
        walkVel.setY(walkVel.y() + cosD(yaw));
    }

    if (keyStrafeRight){
        walkVel.setX(walkVel.x() - sinD(yaw));
        walkVel.setY(walkVel.y() - cosD(yaw));
    }

    if (keyUpwards){
        walkVel.setZ(walkVel.z() + 1);
    }

    if (keyDownwards){
        walkVel.setZ(walkVel.z() - 1);
    }

    pos += walkVel.normalized() * duration * 200;
    pos = map->traceWorld(pos, 30);

    QMatrix4x4 matrix;
    matrix.perspective(75.0, (qreal) width() / height(), 1.0, 9000.0);

    map->renderWorld(matrix, pos, yaw, pitch);
}

void BSPViewer::mouseMoveEvent(QMouseEvent *event) {
    if (!isActiveWindow()) {
        setCursor(QCursor());
        return;
    }
    int dx = event->x() - width() / 2;
    int dy = event->y() - height() / 2;

    yaw += dx * 0.05;
    pitch += dy * 0.05;
    if (yaw > 180) yaw -= 360;
    if (yaw < -180) yaw += 360;
    if (pitch > 90) pitch = 90;
    if (pitch < -90) pitch = -90;

    QCursor cursor;
    cursor.setShape(Qt::BlankCursor);
    cursor.setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    setCursor(cursor);
}

void BSPViewer::keyPressEvent(QKeyEvent* event) {
    switch(event->key()) {
    case Qt::Key_Escape:
        if (parentWidget() == 0) {
            close();
        } else {
            parentWidget()->close();
        }
        timer.stop();
        break;
    case Qt::Key_Up:
    case Qt::Key_W:
        keyForward = true;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        keyBackward = true;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        keyStrafeLeft = true;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        keyStrafeRight = true;
        break;
    case Qt::Key_PageUp:
    case Qt::Key_Space:
        keyUpwards = true;
        break;
    case Qt::Key_PageDown:
    case Qt::Key_Shift:
        keyDownwards = true;
        break;
    default:
        event->ignore();
        break;
    }
}

void BSPViewer::keyReleaseEvent(QKeyEvent* event) {
    switch(event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        keyForward = false;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        keyBackward = false;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        keyStrafeLeft = false;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        keyStrafeRight = false;
        break;
    case Qt::Key_PageUp:
    case Qt::Key_Space:
        keyUpwards = false;
        break;
    case Qt::Key_PageDown:
    case Qt::Key_Shift:
        keyDownwards = false;
        break;
    default:
        event->ignore();
        break;
    }
}
