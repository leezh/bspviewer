#ifndef FRUTSUM_HPP
#define FRUTSUM_HPP

#include <QVector3D>
#include <QMatrix4x4>

class Frutsum {
private:
    QVector3D normals[6];
    qreal distances[6];
public:
    Frutsum(QMatrix4x4 matrix);
    bool inside(QVector3D pos);
    bool insideAABB(QVector3D max, QVector3D min);
    bool insideAABB(int *max, int *min);
};

#endif // FRUTSUM_HPP
