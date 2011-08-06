#ifndef BSP_HPP
#define BSP_HPP

#include <QtOpenGL>
#include <QGLShaderProgram>
#include <QBitArray>
#include <QVector3D>
#include <QMatrix4x4>
#include "frutsum.hpp"

namespace BSP {

    class Map;

    struct Plane {
        QVector3D normal;
        float distance;
    };

    struct Node {
        int plane;
        int children[2];
        int min[3];
        int max[3];
    };

    struct Leaf {
        int cluster;
        int area;
        int min[3];
        int max[3];
        int faceOffset;
        int faceCount;
        int brushOffset;
        int brushCount;
    };

    struct Model {
        QVector3D min;
        QVector3D max;
        int faceOffset;
        int faceCount;
        int brushOffset;
        int brushCount;
    };

    struct Brush {
        int sideOffset;
        int sideCount;
        int shader;
    };

    struct BrushSide {
        int plane;
        int shader;
    };

    struct Vertex {
        QVector3D position;
        QVector2D texCoord;
        QVector2D lmCoord;
        QVector3D normal;
        unsigned char colour[4];
    };

    struct Effect {
        char name[64];
        int brush;
        int unknown;
    };

    struct Face {
        int shader;
        int effect;
        int type;
        int vertexOffset;
        int vertexCount;
        int meshVertexOffset;
        int meshVertexCount;
        int lightMap;
        int lightMapStart[2];
        int lightMapSize[2];
        QVector3D lightMapOrigin;
        QVector3D lightMapVecs[2];
        QVector3D normal;
        int size[2];
    };

    struct LightVol {
        QVector3D ambient;
        QVector3D directional;
        QVector3D direction;
    };

    struct VisData {
        int clusterCount;
        int bytesPerCluster;
        QBitArray data;
    };

    struct Bezier {
        int level;
        QVector<Vertex> vertexArray;
        QVector<int> indexArray;
        QVector<int> rowCountArray;
        QVector<int> rowIndexArray;

        void tesselate(int L, Vertex *controls);
    };

    struct Patch {
        QVector<Bezier> bezierArray;

        void generate(int faceIndex, Map *parent);
    };

    struct Shader {
        bool cullFace;
        bool render;
        bool solid;
        QString name;
        GLuint texture;
    };
    
    struct RenderPass {
        QVector3D pos;
        Frutsum frutsum;
        
        int cluster;
        QBitArray renderedFaces;
        
        RenderPass(Map* parent, QVector3D &position, QMatrix4x4 &matrix);
    };
    
    struct TracePass {
        QVector3D pos;
        float radius;
        
        QBitArray tracedBrushes;
        
        TracePass(Map* parent, QVector3D &inPos, float inRadius);
    };

    class Map {
    protected:
        QGLWidget *parent;
        QGLShaderProgram program;

        QString entity;
        VisData visData;

        QVector<Plane> planeArray;
        QVector<Node> nodeArray;
        QVector<Leaf> leafArray;
        QVector<int> leafFaceArray;
        QVector<int> leafBrushArray;
        QVector<Model> modelArray;
        QVector<Brush> brushArray;
        QVector<BrushSide> brushSideArray;
        QVector<Vertex> vertexArray;
        QVector<int> meshVertexArray;
        QVector<Effect> effectArray;
        QVector<Face> faceArray;
        QVector<GLuint> lightMapArray;
        QVector<LightVol> lightVolArray;

        QVector<Shader> shaderArray;
        QVector<int> facePatchArray;
        QVector<Patch> patchArray;

        unsigned int lightVolSizeX;
        unsigned int lightVolSizeY;
        unsigned int lightVolSizeZ;

        bool clusterVisible(int test, int cam);
        int findLeaf(QVector3D &pos);
        int findLeafCluster(QVector3D &pos);
        LightVol findLightVol(QVector3D &pos);

        void drawMesh(int faceIndex);
        void drawPatch(int faceIndex);

        void renderFace(int index, RenderPass &pass, bool solid);
        void renderLeaf(int index, RenderPass &pass, bool solid);
        void renderNode(int index, RenderPass &pass, bool solid);
        
        void traceBrush(int index, TracePass &pass);
        void traceNode(int index, TracePass &pass);
    public:
        Map(QGLWidget *Parent);

        bool load(QString fileName);
        void renderWorld(QMatrix4x4 matrix, QVector3D pos, float yaw, float pitch);
        QVector3D traceWorld(QVector3D pos, float radius);

        friend class Bezier;
        friend class Patch;
        friend class RenderPass;
        friend class TracePass;
    };
}

#endif // BSP_HPP
