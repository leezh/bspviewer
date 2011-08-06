#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#include <QGLWidget>
#include <QtOpenGL>
#include <QVector3D>
#include <QTime>
#include <QTimer>

#include "bsp.hpp"

class BSPViewer : public QGLWidget {
    Q_OBJECT
public:
    BSPViewer(QGLFormat format = QGLFormat::defaultFormat());

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent *event);

    QTimer timer;
    QTime time;

    QString mapPath;
    BSP::Map *map;

    QVector3D pos;
    float yaw;
    float pitch;

    bool keyForward;
    bool keyBackward;
    bool keyStrafeLeft;
    bool keyStrafeRight;
    bool keyUpwards;
    bool keyDownwards;
};

#endif // GLWIDGET_HPP
