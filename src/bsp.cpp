#include <iostream>
#include <cmath>
#include <QFile>
#include "bsp.hpp"

#define PI 3.14159265359f

namespace BSP {
    QString vertexShaderSrc(
        "uniform mat4 matrix;"
        "attribute vec4 vertex;"
        "attribute vec3 normal;"
        "attribute vec4 texcoord;"
        "attribute vec4 lmcoord;"
        "varying vec3 fragNormal;"
        ""
        "void main() {"
            "gl_TexCoord[0] = texcoord;"
            "gl_TexCoord[1] = lmcoord;"
            "gl_Position = matrix * vertex;"
            "fragNormal = normal;"
        "}");

    QString fragmentShaderSrc(
        "varying vec3 fragNormal;"
        "uniform sampler2D texture;"
        "uniform sampler2D lightmap;"
        "uniform bool uselightmap;"
        "uniform bool usedynamic;"
        ""
        "uniform vec3 ambient;"
        "uniform vec3 directional;"
        "uniform vec3 direction;"
        ""
        "void main() {"
            "vec4 texel = texture2D(texture, gl_TexCoord[0].st);"
            "if (uselightmap) {"
                "texel = texel * 3.0 * texture2D(lightmap, gl_TexCoord[1].st);"
            "}"
            "if (usedynamic) {"
                "float intensity = max(dot(normalize(fragNormal), direction), 0.0);"
                ""
                "texel = texel * vec4(intensity * directional + ambient, 1) * 1.5;"
            "}"
            "gl_FragColor = texel;"
        "}");
    enum {ENTITY = 0, SHADER, PLANE, NODE, LEAF, LEAFFACE, LEAFBRUSH, MODEL, BRUSH, BRUSHSIDE, VERTEX, MESHVERTEX, EFFECT, FACE, LIGHTMAP, LIGHTVOL, VISDATA};

    struct Lump {
        int offset;
        int size;
    };

    struct Header {
        char magic[4];
        int version;
        Lump lumps[17];
    };

    struct RawShader {
        char name[64];
        int surface;
        int contents;
    };

    struct RawLightMap {
        unsigned char data[128][128][3];
    };
    
    struct RawLightVol {
        unsigned char ambient[3];
        unsigned char directional[3];
        unsigned char direction[2];
    };

    Vertex operator+(const Vertex &v1, const Vertex &v2) {
        Vertex temp;
        temp.position = v1.position + v2.position;
        temp.texCoord = v1.texCoord + v2.texCoord;
        temp.lmCoord = v1.lmCoord + v2.lmCoord;
        temp.normal = v1.normal + v2.normal;
        return temp;
    }

    Vertex operator*(const Vertex &v1, const float &d) {
        Vertex temp;
        temp.position = v1.position * d;
        temp.texCoord = v1.texCoord * d;
        temp.lmCoord = v1.lmCoord * d;
        temp.normal = v1.normal * d;
        return temp;
    }

    void Bezier::tesselate(int L, Vertex *controls) {
        level = L;
        int L1 = L + 1;

        vertexArray.resize(L1 * L1);

        for (int i = 0; i <= L; ++i) {
            float a = (float)i / L;
            float b = 1.f - a;
            vertexArray[i] =
                controls[0] * b * b +
                controls[3] * 2 * b * a +
                controls[6] * a * a;
        }

        for (int i = 1; i <= L; ++i) {
            float a = (float)i / L;
            float b = 1.f - a;

            Vertex temp[3];

            for (int j = 0; j < 3; ++j) {
                int k = 3 * j;
                temp[j] =
                    controls[k + 0] * b * b +
                    controls[k + 1] * 2 * b * a +
                    controls[k + 2] * a * a;
            }

            for(int j = 0; j <= L; ++j) {
                float a = (float)j / L;
                float b = 1.0 - a;

                vertexArray[i * L1 + j] =
                    temp[0] * b * b +
                    temp[1] * 2 * b * a +
                    temp[2] * a * a;
            }
        }

        indexArray.resize(L * L1 * 2);
        for (int row = 0; row < L; ++row) {
            for(int col = 0; col <= L; ++col)	{
                indexArray[(row * L1 + col) * 2 + 1] = row * L1 + col;
                indexArray[(row * L1 + col) * 2] = (row + 1) * L1 + col;
            }
        }

        rowCountArray.resize(level);
        rowIndexArray.resize(level);
        for (int row = 0; row < level; ++row) {
            rowCountArray[row] = 2 * (level + 1);
            rowIndexArray[row] = row * 2 * (level + 1);
        }
    }

    void Patch::generate(int faceIndex, Map *parent) {
        Face *face = &parent->faceArray[faceIndex];
        int dimX = (face->size[0] - 1) / 2;
        int dimY = (face->size[1] - 1) / 2;
        bezierArray.resize(dimX * dimY);

        int bIndex = 0;
        int i, n, j, m;
        for (i = 0, n = 0; n < dimX; n++, i = 2 * n) {
            for (j = 0, m = 0; m < dimY; m++, j = 2 * m) {
                Vertex controls[9];
                int cIndex = 0;
                for (int ctr = 0; ctr < 3; ctr++) {
                    int pos = ctr * face->size[0];
                    controls[cIndex++] = parent->vertexArray[face->vertexOffset + i + face->size[0] * j + pos];
                    controls[cIndex++] = parent->vertexArray[face->vertexOffset + i + face->size[0] * j + pos + 1];
                    controls[cIndex++] = parent->vertexArray[face->vertexOffset + i + face->size[0] * j + pos + 2];
                }

                bezierArray[bIndex].tesselate(3, controls);
                bIndex++;
            }
        }
    }
    
    
    RenderPass::RenderPass(Map* parent, QVector3D &position, QMatrix4x4 &matrix):
        renderedFaces(parent->faceArray.count()),
        frutsum(matrix),
        pos(position) {
    }
    
    TracePass::TracePass(Map* parent, QVector3D &inPos, float inRadius):
        tracedBrushes(parent->brushArray.count()),
        pos(inPos),
        radius(inRadius) {
    }

    Map::Map(QGLWidget *Parent):parent(Parent), program(parent->context()){
        program.addShaderFromSourceCode(QGLShader::Vertex, vertexShaderSrc);
        program.addShaderFromSourceCode(QGLShader::Fragment, fragmentShaderSrc);
        program.link();
    }

    bool Map::load(QString filename) {
        glEnable(GL_TEXTURE_2D);
        
        QFile file;
        file.setFileName(filename);
        if (!file.open(QIODevice::ReadOnly)) return false;

        file.seek(0);
        Header header;
        file.read((char*)&header, sizeof(Header));
        if (strncmp(header.magic, (char*)"IBSP", 4) != 0) return false;
        if (header.version != 0x2E) return false;

        file.seek(header.lumps[ENTITY].offset);
        QByteArray entityData = file.read(header.lumps[ENTITY].size);
        entity = QString(entityData);
        std::cout << entity.toStdString() << std::endl;

        int shaderCount = header.lumps[SHADER].size / sizeof(RawShader);
        file.seek(header.lumps[SHADER].offset);
        shaderArray.reserve(shaderCount);
        for (int i = 0; i < shaderCount; i++) {
            RawShader rawshader;
            file.read((char*)&rawshader, sizeof(RawShader));
            Shader shader;
            shader.name = QString(rawshader.name).replace('\\', "/");
            
            QImage rawImage = QImage(shader.name);
            if (rawImage.isNull()) {
                std::cout << rawshader.name << std::endl;
            }
            QImage image = parent->convertToGLFormat(rawImage.mirrored());
            glGenTextures(1, &shader.texture);
            glBindTexture(GL_TEXTURE_2D, shader.texture);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifndef NO_MIPMAP
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, image.width(), image.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
#else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
#endif
            shader.cullFace = rawImage.hasAlphaChannel();
            shader.render = !rawImage.isNull() || (rawshader.surface & 0x80 != 0);
            shader.solid = (rawshader.contents & 0x10000 != 0);
            shaderArray.push_back(shader);
        }

        int planeCount = header.lumps[PLANE].size / sizeof(Plane);
        file.seek(header.lumps[PLANE].offset);
        planeArray.resize(planeCount);
        if (planeCount > 0)
            file.read((char*)&planeArray[0], sizeof(Plane) * planeCount);

        int nodeCount = header.lumps[NODE].size / sizeof(Node);
        file.seek(header.lumps[NODE].offset);
        nodeArray.resize(nodeCount);
        if (nodeCount > 0)
            file.read((char*)&nodeArray[0], sizeof(Node) * nodeCount);

        int leafCount = header.lumps[LEAF].size / sizeof(Leaf);
        file.seek(header.lumps[LEAF].offset);
        leafArray.resize(leafCount);
        if (leafCount > 0)
            file.read((char*)&leafArray[0], sizeof(Leaf) * leafCount);

        int leafFaceCount = header.lumps[LEAFFACE].size / sizeof(int);
        file.seek(header.lumps[LEAFFACE].offset);
        leafFaceArray.resize(leafFaceCount);
        if (leafFaceCount > 0)
            file.read((char*)&leafFaceArray[0], sizeof(int) * leafFaceCount);

        int leafBrushCount = header.lumps[LEAFBRUSH].size / sizeof(int);
        file.seek(header.lumps[LEAFBRUSH].offset);
        leafBrushArray.resize(leafBrushCount);
        if (leafBrushCount > 0)
            file.read((char*)&leafBrushArray[0], sizeof(int) * leafBrushCount);

        int modelCount = header.lumps[MODEL].size / sizeof(Model);
        file.seek(header.lumps[MODEL].offset);
        modelArray.resize(modelCount);
        if (modelCount > 0)
            file.read((char*)&modelArray[0], sizeof(Model) * modelCount);

        int brushCount = header.lumps[BRUSH].size / sizeof(Brush);
        file.seek(header.lumps[BRUSH].offset);
        brushArray.resize(brushCount);
        if (brushCount > 0)
            file.read((char*)&brushArray[0], sizeof(Brush) * brushCount);

        int brushSideCount = header.lumps[BRUSHSIDE].size / sizeof(BrushSide);
        file.seek(header.lumps[BRUSHSIDE].offset);
        brushSideArray.resize(brushSideCount);
        if (brushSideCount > 0)
            file.read((char*)&brushSideArray[0], sizeof(BrushSide) * brushSideCount);

        int vertexCount = header.lumps[VERTEX].size / sizeof(Vertex);
        file.seek(header.lumps[VERTEX].offset);
        vertexArray.resize(vertexCount);
        if (vertexCount > 0)
            file.read((char*)&vertexArray[0], sizeof(Vertex) * vertexCount);

        int meshVertexCount = header.lumps[MESHVERTEX].size / sizeof(int);
        file.seek(header.lumps[MESHVERTEX].offset);
        meshVertexArray.resize(meshVertexCount);
        if (meshVertexCount > 0)
            file.read((char*)&meshVertexArray[0], sizeof(int) * meshVertexCount);

        int effectCount = header.lumps[EFFECT].size / sizeof(Effect);
        file.seek(header.lumps[EFFECT].offset);
        effectArray.resize(effectCount);
        if (effectCount > 0)
            file.read((char*)&effectArray[0], sizeof(Effect) * effectCount);

        int faceCount = header.lumps[FACE].size / sizeof(Face);
        int patchCount = 0;
        file.seek(header.lumps[FACE].offset);
        faceArray.resize(faceCount);
        for (int i = 0; i < faceCount; i++) {
            file.read((char*)&faceArray[i], sizeof(Face));
            if (faceArray[i].type == 2) patchCount++;
        }

        int lightMapCount = header.lumps[LIGHTMAP].size / sizeof(RawLightMap);
        file.seek(header.lumps[LIGHTMAP].offset);
        lightMapArray.reserve(lightMapCount);
        for (int i = 0; i < lightMapCount; i++) {
            RawLightMap rawLightMap;
            file.read((char*)&rawLightMap, sizeof(RawLightMap));

            GLuint lightMap;
            glGenTextures(1, &lightMap);
            glBindTexture(GL_TEXTURE_2D, lightMap);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#ifndef NO_MIPMAP
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 128, 128, GL_RGB, GL_UNSIGNED_BYTE, rawLightMap.data);
#else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, rawLightMap.data);
#endif
            lightMapArray.push_back(lightMap);
        }

        int lightVolCount = header.lumps[LIGHTVOL].size / sizeof(RawLightVol);
        file.seek(header.lumps[LIGHTVOL].offset);
        lightVolArray.reserve(lightVolCount);
        for (int i = 0; i < lightVolCount; i++) {
            RawLightVol rawLightVol;
            file.read((char*)&rawLightVol, sizeof(RawLightVol));
            LightVol lightVol;
            
            lightVol.ambient.setX(rawLightVol.ambient[0]);
            lightVol.ambient.setY(rawLightVol.ambient[1]);
            lightVol.ambient.setZ(rawLightVol.ambient[2]);
            lightVol.ambient = lightVol.ambient / 256;
            
            lightVol.directional.setX(rawLightVol.directional[0]);
            lightVol.directional.setY(rawLightVol.directional[1]);
            lightVol.directional.setZ(rawLightVol.directional[2]);
            lightVol.directional = lightVol.directional / 256;
            
            float phi =    (int(rawLightVol.direction[0]) - 128)/ 256.f * 180;
            float thetha = int(rawLightVol.direction[1]) / 256.f * 360;
            
            lightVol.direction.setX(sin(thetha) * cos(phi));
            lightVol.direction.setY(cos(thetha) * cos(phi));
            lightVol.direction.setZ(sin(phi));
            lightVol.direction.normalize();
            
            lightVolArray.push_back(lightVol);
        }

        file.seek(header.lumps[VISDATA].offset);
        if (header.lumps[VISDATA].size > 0) {
            file.read((char*)&visData.clusterCount, sizeof(int));
            file.read((char*)&visData.bytesPerCluster, sizeof(int));
            unsigned int size = visData.clusterCount * visData.bytesPerCluster;
            QByteArray rawVisData = file.read(size);

            visData.data.resize(size * 8);
            for(unsigned long int byteIndex = 0; byteIndex < size; byteIndex++) {
                unsigned char byte = rawVisData.at(byteIndex);
                for(unsigned int bit = 0; bit < 8; bit++)
                    visData.data.setBit(byteIndex * 8 + bit, byte & (1 << bit));
            }
        }

        file.close();
        
        lightVolSizeX = floor(modelArray[0].max.x() / 64) - ceil(modelArray[0].min.x() / 64) + 1;
        lightVolSizeY = floor(modelArray[0].max.y() / 64) - ceil(modelArray[0].min.y() / 64) + 1;
        lightVolSizeZ = floor(modelArray[0].max.z() / 128) - ceil(modelArray[0].min.z() / 128) + 1;

        facePatchArray = QVector<int>(faceCount, 0);
        patchArray.reserve(patchCount);
        for (int i = 0; i < faceCount; i++) {
            if(faceArray[i].type == 2) {
                int patchIndex = patchArray.count();
                patchArray.push_back(Patch());
                facePatchArray[i] = patchIndex;
                patchArray[patchIndex].generate(i, this);
            }
        }
        
        glDisable(GL_TEXTURE_2D);
        return true;
    }

    bool Map::clusterVisible(int test, int cam) {
        if (visData.data.size() == 0 || cam  < 0 || test  < 0) return true;

        return visData.data.testBit(test * visData.bytesPerCluster * 8 + cam);
    }

    int Map::findLeaf(QVector3D &pos) {
        int index = 0;
        while (index >= 0) {
            Node *node = &nodeArray[index];
            Plane *plane = &planeArray[node->plane];
            if (QVector3D::dotProduct(plane->normal, pos) >= plane->distance) {
                index = node->children[0];
            } else {
                index = node->children[1];
            }
        };
        return ~index;
    }
    
    LightVol Map::findLightVol(QVector3D &pos) {
        if (lightVolArray.size() == 0) return LightVol();
        int cellX = floor(pos.x() / 64) - ceil(modelArray[0].min.x() / 64);
        int cellY = floor(pos.y() / 64) - ceil(modelArray[0].min.y() / 64);
        int cellZ = floor(pos.z() / 128) - ceil(modelArray[0].min.z() / 128);
        cellX = std::min(std::max(cellX, 0), (int)lightVolSizeX);
        cellY = std::min(std::max(cellY, 0), (int)lightVolSizeY);
        cellZ = std::min(std::max(cellZ, 0), (int)lightVolSizeZ);
        unsigned int index = cellX;
        index += cellY * lightVolSizeX;
        index += cellZ * lightVolSizeX * lightVolSizeY;
        return lightVolArray[index];
    }

    void Map::renderFace(int index, RenderPass &pass, bool solid) {
        if (pass.renderedFaces.testBit(index)) return;
        Face *face = &faceArray[index];
        if (shaderArray[face->shader].cullFace == solid) return;
        if (!shaderArray[face->shader].render) return;

        glFrontFace(GL_CW);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shaderArray[face->shader].texture);
        glActiveTexture(GL_TEXTURE1);

        if (face->lightMap >= 0) {
            glBindTexture(GL_TEXTURE_2D, lightMapArray[face->lightMap]);
            program.setUniformValue("uselightmap", true);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            program.setUniformValue("uselightmap", false);
        }
        
        // Disabled as it is still very buggy.
        if (false) {
            LightVol lightVol = findLightVol(face->lightMapOrigin);
            program.setUniformValue("usedynamic", true);
            program.setUniformValue("ambient", lightVol.ambient);
            program.setUniformValue("directional", lightVol.directional);
            program.setUniformValue("direction", lightVol.direction);
        } else {
            program.setUniformValue("usedynamic", false);
        }

        if (face->type == 1 || face->type == 3) {
            program.setAttributeArray("vertex", &vertexArray[face->vertexOffset].position, sizeof(Vertex));
            program.setAttributeArray("normal", &vertexArray[face->vertexOffset].normal, sizeof(Vertex));
            program.setAttributeArray("texcoord", &vertexArray[face->vertexOffset].texCoord, sizeof(Vertex));
            program.setAttributeArray("lmcoord", &vertexArray[face->vertexOffset].lmCoord, sizeof(Vertex));
            glDrawElements(GL_TRIANGLES, face->meshVertexCount, GL_UNSIGNED_INT, &meshVertexArray[face->meshVertexOffset]);
        } else if (face->type == 2) {
            Patch *patch = &patchArray[facePatchArray[index]];
            for (int i = 0; i < patch->bezierArray.size(); i++) {
                Bezier *bezier = &patch->bezierArray[i];
                program.setAttributeArray("vertex", &bezier->vertexArray[0].position, sizeof(Vertex));
                program.setAttributeArray("normal", &vertexArray[face->vertexOffset].normal, sizeof(Vertex));
                program.setAttributeArray("texcoord", &bezier->vertexArray[0].texCoord, sizeof(Vertex));
                program.setAttributeArray("lmcoord", &bezier->vertexArray[0].lmCoord, sizeof(Vertex));
                glDrawElements(GL_TRIANGLES, face->meshVertexCount, GL_UNSIGNED_INT, &meshVertexArray[face->meshVertexOffset]);
                for (int l = 0; l < bezier->level; l++) {
                    glDrawElements(GL_TRIANGLE_STRIP, bezier->rowCountArray[l], GL_UNSIGNED_INT, &bezier->indexArray[bezier->rowIndexArray[l]]);
                }
            }
        }

        pass.renderedFaces.setBit(index, true);
    }

    void Map::renderLeaf(int index, RenderPass &pass, bool solid) {
        Leaf *leaf = &leafArray[index];
        if (!clusterVisible(leaf->cluster, pass.cluster)) return;
        if (!pass.frutsum.insideAABB(leaf->max, leaf->min)) return;
        for (int i = 0; i < leaf->faceCount; i++) {
            int faceIndex = leafFaceArray[i + leaf->faceOffset];
            renderFace(faceIndex, pass, solid);
        }
    }

    void Map::renderNode(int index, RenderPass &pass, bool solid) {
        if (index < 0) {
            renderLeaf(- index - 1, pass, solid);
            return;
        }

        Node *node = &nodeArray[index];
        Plane *plane = &planeArray[node->plane];
        if (!pass.frutsum.insideAABB(node->max, node->min)) return;

        if ((QVector3D::dotProduct(plane->normal, pass.pos) >= plane->distance) == solid) {
            renderNode(node->children[0], pass, solid);
            renderNode(node->children[1], pass, solid);
        } else {
            renderNode(node->children[1], pass, solid);
            renderNode(node->children[0], pass, solid);
        }
    }
    
    void Map::renderWorld(QMatrix4x4 matrix, QVector3D pos, float yaw, float pitch) {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
        if (nodeArray.size() == 0) return;
        matrix.rotate(-90.0, 1.0, 0.0, 0.0);
        matrix.rotate(pitch, 1.0, 0.0, 0.0);
        matrix.rotate(yaw + 90, 0.0, 0.0, 1.0);
        matrix.translate(-pos);

        program.bind();
        program.enableAttributeArray("vertex");
        program.enableAttributeArray("normal");
        program.enableAttributeArray("texcoord");
        program.enableAttributeArray("lmcoord");
        program.setUniformValue("matrix", matrix);
        program.setUniformValue("texture", 0);
        program.setUniformValue("lightmap", 1);
        
        RenderPass pass(this, pos, matrix);
        pass.cluster = leafArray[findLeaf(pos)].cluster;
        
        for (int m = 1; m < modelArray.count(); m++) {
            for (int f = 0; f < modelArray[m].faceCount; f++) {
                pass.renderedFaces.setBit(f + modelArray[m].faceOffset, true);
            }
        }

        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        renderNode(0, pass, true);
        
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        renderNode(0, pass, false);
        
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
    
    void Map::traceBrush(int index, TracePass &pass) {
        if (pass.tracedBrushes.testBit(index)) return;
        Brush *brush = &brushArray[index];
        if (!shaderArray[brush->shader].solid) return;
        
        Plane *collidingPlane = NULL;
        double collidingDist = 0.0;
         
        for (int i = 0; i < brush->sideCount; i++) {
            BrushSide *side = &brushSideArray[i + brush->sideOffset];
            Plane *plane = &planeArray[side->plane];
            
            double dist = QVector3D::dotProduct(plane->normal, pass.pos) - plane->distance - pass.radius;
            
            if (dist > 0) return;
            
            if (collidingPlane == NULL || dist > collidingDist) {
                collidingPlane = plane;
                collidingDist = dist;
            }
        }
        
        if (collidingPlane == NULL) return; 
        pass.pos -= collidingPlane->normal * collidingDist;
        
        pass.tracedBrushes.setBit(index, true);
    }
    
    void Map::traceNode(int index, TracePass &pass) {
        if (index < 0) {
            Leaf *leaf = &leafArray[- index - 1];
            for (int i = 0; i < leaf->brushCount; i++) {
                traceBrush(leafBrushArray[i + leaf->brushOffset], pass);
            }
            return;
        }

        Node *node = &nodeArray[index];
        Plane *plane = &planeArray[node->plane];
        float dist = QVector3D::dotProduct(plane->normal, pass.pos) - plane->distance;

        if (dist > -pass.radius) {
            traceNode(node->children[0], pass);
        }
        
        if (dist < pass.radius) {
            traceNode(node->children[1], pass);
        }
    }
    
    QVector3D Map::traceWorld(QVector3D pos, float radius) {
        TracePass pass(this, pos, radius);
        traceNode(0, pass);
        
        return pass.pos;
    }
}

