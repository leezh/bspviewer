#include <QApplication>
#include "glwidget.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QGLFormat format = QGLFormat::defaultFormat();
    //format.setSwapInterval(1); // VSync, sensible frame-limiter
    BSPViewer *viewer = new BSPViewer(format);
    
    viewer->show();

    return app.exec();
}
