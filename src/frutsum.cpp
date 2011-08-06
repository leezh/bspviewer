#include "frutsum.hpp"

Frutsum::Frutsum(QMatrix4x4 matrix) {
    for (int i = 0; i < 6; i++) {
        qreal a, b, c, d;
        int col = i >> 1;
        int dir = 1 - 1 * (i & 2);

        a = matrix(3, 0) + matrix(col, 0) * dir;
        b = matrix(3, 1) + matrix(col, 1) * dir;
        c = matrix(3, 2) + matrix(col, 2) * dir;
        d = matrix(3, 3) + matrix(col, 3) * dir;

        normals[i] = QVector3D(a, b, c);
        distances[i] = -d / normals[i].length();
        normals[i].normalize();
    }
}

bool Frutsum::inside(QVector3D pos) {
    for (int i = 0; i < 6; i++) {
        if (QVector3D::dotProduct(normals[i], pos) < distances[i]) {
            return false;
        }
    }
    return true;
}

bool Frutsum::insideAABB(QVector3D max, QVector3D min) {
    for (int i = 0; i < 6; i++) {
        QVector3D pVert = max;
        if (normals[i].x() < 0)
            pVert.setX(min.x());
        if (normals[i].y() < 0)
            pVert.setY(min.y());
        if (normals[i].z() < 0)
            pVert.setZ(min.z());

        if (QVector3D::dotProduct(normals[i], pVert) < distances[i]) {
            return false;
        }
    }
    return true;
}

bool Frutsum::insideAABB(int *max, int *min) {
    return insideAABB(QVector3D(max[0], max[1], max[2]), QVector3D(min[0], min[1], min[2]));
}
