#include <cmath>
#include <array>
#include <iostream>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <physfs.h>
#include "filestream.hpp"
#include "bsp.hpp"

enum
{
    ENTITY = 0,
    SHADER,
    PLANE,
    NODE,
    LEAF,
    LEAFFACE,
    LEAFBRUSH,
    MODEL,
    BRUSH,
    BRUSHSIDE,
    VERTEX,
    MESHVERTEX,
    EFFECT,
    FACE,
    LIGHTMAP,
    LIGHTVOL,
    VISDATA
};

struct Lump
{
    int offset;
    int size;
};

struct Header
{
    char magic[4];
    int version;
    Lump lumps[17];
};

struct RawShader
{
    char name[64];
    int surface;
    int contents;
};

struct RawLightVol
{
    unsigned char ambient[3];
    unsigned char directional[3];
    unsigned char direction[2];
};

Vertex operator+(const Vertex& v1, const Vertex& v2)
{
    Vertex temp;
    temp.position = v1.position + v2.position;
    temp.texCoord = v1.texCoord + v2.texCoord;
    temp.lmCoord = v1.lmCoord + v2.lmCoord;
    temp.normal = v1.normal + v2.normal;
    return temp;
}

Vertex operator*(const Vertex& v1, const float& d)
{
    Vertex temp;
    temp.position = v1.position * d;
    temp.texCoord = v1.texCoord * d;
    temp.lmCoord = v1.lmCoord * d;
    temp.normal = v1.normal * d;
    return temp;
}

void Bezier::tesselate(int L, Vertex* controls)
{
    level = L;
    int L1 = L + 1;

    vertexArray.resize(L1 * L1);

    for (int i = 0; i <= L; ++i)
    {
        float a = (float)i / L;
        float b = 1.f - a;
        vertexArray[i] = controls[0] * b * b + controls[3] * 2 * b * a + controls[6] * a * a;
    }

    for (int i = 1; i <= L; ++i)
    {
        float a = (float)i / L;
        float b = 1.f - a;

        Vertex temp[3];

        for (int j = 0; j < 3; ++j)
        {
            int k = 3 * j;
            temp[j] = controls[k + 0] * b * b + controls[k + 1] * 2 * b * a + controls[k + 2] * a * a;
        }

        for (int j = 0; j <= L; ++j)
        {
            float a = (float)j / L;
            float b = 1.f - a;

            vertexArray[i * L1 + j] = temp[0] * b * b + temp[1] * 2 * b * a + temp[2] * a * a;
        }
    }

    indexArray.resize(L * L1 * 2);
    for (int row = 0; row < L; ++row)
    {
        for (int col = 0; col <= L; ++col)
        {
            indexArray[(row * L1 + col) * 2 + 1] = row * L1 + col;
            indexArray[(row * L1 + col) * 2] = (row + 1) * L1 + col;
        }
    }

    rowCountArray.resize(level);
    rowIndexArray.resize(level);
    for (int row = 0; row < level; ++row)
    {
        rowCountArray[row] = 2 * (level + 1);
        rowIndexArray[row] = row * 2 * (level + 1);
    }
}

void Patch::generate(int faceIndex, Map* parent)
{
    Face* face = &parent->faceArray[faceIndex];
    int dimX = (face->size[0] - 1) / 2;
    int dimY = (face->size[1] - 1) / 2;
    bezierArray.resize(dimX * dimY);

    int bIndex = 0;
    int i, n, j, m;
    for (i = 0, n = 0; n < dimX; n++, i = 2 * n)
    {
        for (j = 0, m = 0; m < dimY; m++, j = 2 * m)
        {
            Vertex controls[9];
            int cIndex = 0;
            for (int ctr = 0; ctr < 3; ctr++)
            {
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

#include "shaders.inc"

RenderPass::RenderPass(Map* parent, const glm::vec3& position, const glm::mat4& matrix)
    : pos(position)
    , frutsum(matrix)
{
    renderedFaces.resize(parent->faceArray.size(), false);
}

TracePass::TracePass(Map* parent, const glm::vec3& inPos, float inRadius)
    : pos(inPos)
    , radius(inRadius)
{
    tracedBrushes.resize(parent->brushArray.size(), false);
}

Map::Map()
{
    GLint status;

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc, NULL);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &length);
        char* log = new char[length + 1];
        log[length] = '\0';
        glGetShaderInfoLog(vertShader, length, &length, log);
        std::cout << log << std::endl;
		delete[] log;
        return;
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc, NULL);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &length);
		char* log = new char[length + 1];
        log[length] = '\0';
        glGetShaderInfoLog(fragShader, length, &length, log);
        std::cout << log << std::endl;
        glDeleteShader(vertShader);
		delete[] log;
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    glBindAttribLocation(program, 0, "vertex");
    //glBindAttribLocation(program, 1, "normal");
    glBindAttribLocation(program, 2, "texcoord");
    glBindAttribLocation(program, 3, "lmcoord");

    glLinkProgram(program);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char* log = new char[length + 1];
        log[length] = '\0';
        glGetProgramInfoLog(program, length, &length, log);
        std::cout << log << std::endl;
		delete[] log;
        return;
    }

    programLoc["matrix"] = glGetUniformLocation(program, "matrix");
    programLoc["texture"] = glGetUniformLocation(program, "texture");
    programLoc["lightmap"] = glGetUniformLocation(program, "lightmap");
    programLoc["uselightmap"] = glGetUniformLocation(program, "uselightmap");
}

bool Map::load(std::string filename)
{
    glEnable(GL_TEXTURE_2D);

    PHYSFS_File* file = PHYSFS_openRead(filename.c_str());
    if (!file)
    {
        std::cout << filename.c_str() << ": " << PHYSFS_getLastError() << std::endl;
        return false;
    }

    PHYSFS_seek(file, 0);
    Header header;
    PHYSFS_read(file, &header, sizeof(Header), 1);
    if (std::string(header.magic, 4) != "IBSP")
    {
        std::cout << "Invalid file" << std::endl;
        return false;
    }
    if (header.version != 0x2E)
    {
        std::cout << "File version not supported" << std::endl;
        return false;
    }

    PHYSFS_seek(file, header.lumps[ENTITY].offset);
    std::string rawEntity;
    rawEntity.resize(header.lumps[ENTITY].size);
    PHYSFS_read(file, &rawEntity[0], 1, header.lumps[ENTITY].size);

    int shaderCount = header.lumps[SHADER].size / sizeof(RawShader);
    PHYSFS_seek(file, header.lumps[SHADER].offset);
    shaderArray.reserve(shaderCount);
    for (int i = 0; i < shaderCount; i++)
    {
        RawShader rawshader;
        PHYSFS_read(file, &rawshader, sizeof(RawShader), 1);
        rawshader.name[63] = '\0';
        Shader shader;
        shader.render = true;
        shader.transparent = false;
        shader.solid = ((rawshader.contents & 0x10000) != 0);
        shader.name = std::string(rawshader.name);
        if (shader.name == "noshader") shader.render = false;
        if (shader.render)
        {
            if (PHYSFS_exists(std::string(shader.name + ".jpg").c_str()))
            {
                shader.name += ".jpg";
            }
            else if (PHYSFS_exists(std::string(shader.name + ".tga").c_str()))
            {
                shader.name += ".tga";
            }
            if ((rawshader.surface & 0x80) == 0)
            {
                FileStream filestream(shader.name);
                if (filestream.isOpen())
                {
                    if (shader.texture.loadFromStream(filestream))
                    {
                        shader.render = true;
#if SFML_VERSION_MAJOR > 2 || (SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR >= 4)
                        shader.texture.generateMipmap();
#endif
                        shader.texture.setRepeated(true);
                        shader.texture.setSmooth(true);
                    }
                }
                else
                {
                    std::cout << shader.name << ": Texture not found" << std::endl;
                }
            }
        }
        shaderArray.push_back(shader);
    }

    int planeCount = header.lumps[PLANE].size / sizeof(Plane);
    PHYSFS_seek(file, header.lumps[PLANE].offset);
    planeArray.resize(planeCount);
    if (planeCount > 0)
        PHYSFS_read(file, &planeArray[0], sizeof(Plane), planeCount);

    int nodeCount = header.lumps[NODE].size / sizeof(Node);
    PHYSFS_seek(file, header.lumps[NODE].offset);
    nodeArray.resize(nodeCount);
    if (nodeCount > 0)
        PHYSFS_read(file, &nodeArray[0], sizeof(Node), nodeCount);

    int leafCount = header.lumps[LEAF].size / sizeof(Leaf);
    PHYSFS_seek(file, header.lumps[LEAF].offset);
    leafArray.resize(leafCount);
    if (leafCount > 0)
        PHYSFS_read(file, &leafArray[0], sizeof(Leaf), leafCount);

    int leafFaceCount = header.lumps[LEAFFACE].size / sizeof(int);
    PHYSFS_seek(file, header.lumps[LEAFFACE].offset);
    leafFaceArray.resize(leafFaceCount);
    if (leafFaceCount > 0)
        PHYSFS_read(file, &leafFaceArray[0], sizeof(int), leafFaceCount);

    int leafBrushCount = header.lumps[LEAFBRUSH].size / sizeof(int);
    PHYSFS_seek(file, header.lumps[LEAFBRUSH].offset);
    leafBrushArray.resize(leafBrushCount);
    if (leafBrushCount > 0)
        PHYSFS_read(file, &leafBrushArray[0], sizeof(int), leafBrushCount);

    int modelCount = header.lumps[MODEL].size / sizeof(Model);
    PHYSFS_seek(file, header.lumps[MODEL].offset);
    modelArray.resize(modelCount);
    if (modelCount > 0)
        PHYSFS_read(file, &modelArray[0], sizeof(Model), modelCount);

    int brushCount = header.lumps[BRUSH].size / sizeof(Brush);
    PHYSFS_seek(file, header.lumps[BRUSH].offset);
    brushArray.resize(brushCount);
    if (brushCount > 0)
        PHYSFS_read(file, &brushArray[0], sizeof(Brush), brushCount);

    int brushSideCount = header.lumps[BRUSHSIDE].size / sizeof(BrushSide);
    PHYSFS_seek(file, header.lumps[BRUSHSIDE].offset);
    brushSideArray.resize(brushSideCount);
    if (brushSideCount > 0)
        PHYSFS_read(file, &brushSideArray[0], sizeof(BrushSide), brushSideCount);

    int vertexCount = header.lumps[VERTEX].size / sizeof(Vertex);
    PHYSFS_seek(file, header.lumps[VERTEX].offset);
    vertexArray.resize(vertexCount);
    if (vertexCount > 0)
        PHYSFS_read(file, &vertexArray[0], sizeof(Vertex), vertexCount);

    int meshVertexCount = header.lumps[MESHVERTEX].size / sizeof(int);
    PHYSFS_seek(file, header.lumps[MESHVERTEX].offset);
    meshVertexArray.resize(meshVertexCount);
    if (meshVertexCount > 0)
        PHYSFS_read(file, &meshVertexArray[0], sizeof(int), meshVertexCount);

    int effectCount = header.lumps[EFFECT].size / sizeof(Effect);
    PHYSFS_seek(file, header.lumps[EFFECT].offset);
    effectArray.resize(effectCount);
    if (effectCount > 0)
        PHYSFS_read(file, &effectArray[0], sizeof(Effect), effectCount);

    int faceCount = header.lumps[FACE].size / sizeof(Face);
    int patchCount = 0;
    PHYSFS_seek(file, header.lumps[FACE].offset);
    faceArray.resize(faceCount);
    for (int i = 0; i < faceCount; i++)
    {
        PHYSFS_read(file, &faceArray[i], sizeof(Face), 1);
        if (faceArray[i].type == 2)
            patchCount++;
    }

    int lightMapCount = header.lumps[LIGHTMAP].size / (128 * 128 * 3);
    PHYSFS_seek(file, header.lumps[LIGHTMAP].offset);
    lightMapArray.resize(lightMapCount);
    for (int i = 0; i < lightMapCount; i++)
    {
        std::array<sf::Uint8, 128 * 128 * 4> rawLightMap;
        for (int i = 0; i < 128 * 128; i++)
        {
            PHYSFS_read(file, &rawLightMap[i * 4], 3, 1);
            rawLightMap[i * 4 + 3] = 255;
        }
        sf::Image image;
        image.create(128, 128, &rawLightMap[0]);
        sf::Texture &texture = lightMapArray[i];
        texture.loadFromImage(image);
        texture.setRepeated(true);
        texture.setSmooth(true);
    }

    int lightVolCount = header.lumps[LIGHTVOL].size / sizeof(RawLightVol);
    PHYSFS_seek(file, header.lumps[LIGHTVOL].offset);
    lightVolArray.reserve(lightVolCount);
    for (int i = 0; i < lightVolCount; i++)
    {
        RawLightVol rawLightVol;
        PHYSFS_read(file, &rawLightVol, sizeof(RawLightVol), 1);
        LightVol lightVol;

        lightVol.ambient.x = rawLightVol.ambient[0];
        lightVol.ambient.y = rawLightVol.ambient[1];
        lightVol.ambient.z = rawLightVol.ambient[2];
        lightVol.ambient = lightVol.ambient / 256.f;

        lightVol.directional.x = rawLightVol.directional[0];
        lightVol.directional.y = rawLightVol.directional[1];
        lightVol.directional.z = rawLightVol.directional[2];
        lightVol.directional = lightVol.directional / 256.f;

        float phi = (int(rawLightVol.direction[0]) - 128) / 256.f * 180;
        float thetha = int(rawLightVol.direction[1]) / 256.f * 360;

        lightVol.direction.x = sin(thetha) * cos(phi);
        lightVol.direction.y = cos(thetha) * cos(phi);
        lightVol.direction.z = sin(phi);
        lightVol.direction = glm::normalize(lightVol.direction);

        lightVolArray.push_back(lightVol);
    }

    PHYSFS_seek(file, header.lumps[VISDATA].offset);
    if (header.lumps[VISDATA].size > 0)
    {
        PHYSFS_read(file, &visData.clusterCount, sizeof(int), 1);
        PHYSFS_read(file, &visData.bytesPerCluster, sizeof(int), 1);
        unsigned int size = visData.clusterCount * visData.bytesPerCluster;
        std::vector<char> rawVisData(size);
        PHYSFS_read(file, &rawVisData[0], size, 1);

        visData.data.resize(size * 8, false);
        for (unsigned long int byteIndex = 0; byteIndex < size; byteIndex++)
        {
            unsigned char byte = rawVisData.at(byteIndex);
            for (unsigned int bit = 0; bit < 8; bit++)
            {
                if (byte & (1 << bit))
                    visData.data[byteIndex * 8 + bit] = true;
            }
        }
    }

    PHYSFS_close(file);

    lightVolSizeX = int(floor(modelArray[0].max.x / 64) - ceil(modelArray[0].min.x / 64) + 1);
    lightVolSizeY = int(floor(modelArray[0].max.y / 64) - ceil(modelArray[0].min.y / 64) + 1);
    lightVolSizeZ = int(floor(modelArray[0].max.z / 128) - ceil(modelArray[0].min.z / 128) + 1);

    facePatchArray.resize(faceCount);
    patchArray.reserve(patchCount);
    for (int i = 0; i < faceCount; i++)
    {
        if (faceArray[i].type == 2)
        {
            int patchIndex = int(patchArray.size());
            patchArray.push_back(Patch());
            facePatchArray[i] = patchIndex;
            patchArray[patchIndex].generate(i, this);
        }
    }

    glDisable(GL_TEXTURE_2D);
    return true;
}

bool Map::clusterVisible(int test, int cam)
{
    if (visData.data.size() == 0 || cam < 0 || test < 0)
        return true;

    return visData.data[test * visData.bytesPerCluster * 8 + cam];
}

int Map::findLeaf(glm::vec3& pos)
{
    int index = 0;
    while (index >= 0)
    {
        Node* node = &nodeArray[index];
        Plane* plane = &planeArray[node->plane];
        if (glm::dot(plane->normal, pos) >= plane->distance)
        {
            index = node->children[0];
        }
        else
        {
            index = node->children[1];
        }
    };
    return ~index;
}

LightVol Map::findLightVol(glm::vec3& pos)
{
    if (lightVolArray.size() == 0)
        return LightVol();
    int cellX = int(floor(pos.x / 64) - ceil(modelArray[0].min.x / 64));
    int cellY = int(floor(pos.y / 64) - ceil(modelArray[0].min.y / 64));
    int cellZ = int(floor(pos.z / 128) - ceil(modelArray[0].min.z / 128));
    cellX = std::min(std::max(cellX, 0), (int)lightVolSizeX);
    cellY = std::min(std::max(cellY, 0), (int)lightVolSizeY);
    cellZ = std::min(std::max(cellZ, 0), (int)lightVolSizeZ);
    unsigned int index = cellX;
    index += cellY * lightVolSizeX;
    index += cellZ * lightVolSizeX * lightVolSizeY;
    return lightVolArray[index];
}

void Map::renderFace(int index, RenderPass& pass, bool solid)
{
    if (pass.renderedFaces[index])
        return;
    Face* face = &faceArray[index];
    if (shaderArray[face->shader].transparent == solid)
        return;
    if (!shaderArray[face->shader].render)
        return;

    glFrontFace(GL_CW);
    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&shaderArray[face->shader].texture);
    glActiveTexture(GL_TEXTURE1);

    if (face->lightMap >= 0)
    {
        sf::Texture::bind(&lightMapArray[face->lightMap]);
        glUniform1i(programLoc["uselightmap"], GL_TRUE);
    }
    else
    {
        sf::Texture::bind(NULL);
        glUniform1i(programLoc["uselightmap"], GL_FALSE);
    }

    // Disabled as it is still very buggy.
    /*
    if (false)
    {
        LightVol lightVol = findLightVol(face->lightMapOrigin);
        glUniform1i(programLoc["usedynamic"], GL_TRUE);
        glUniform3fv(programLoc["ambeint"], 1, &lightVol.ambient[0]);
        glUniform3fv(programLoc["directional"], 1, &lightVol.directional[0]);
        glUniform3fv(programLoc["direction"], 1, &lightVol.direction[0]);
    }
    else
    {
        glUniform1i(programLoc["usedynamic"], GL_FALSE);
    }
    */

    if (face->type == 1 || face->type == 3)
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertexArray[face->vertexOffset].position);
        //glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE,  sizeof(Vertex), &vertexArray[face->vertexOffset].normal);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertexArray[face->vertexOffset].texCoord);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &vertexArray[face->vertexOffset].lmCoord);
        glDrawElements(GL_TRIANGLES, face->meshVertexCount, GL_UNSIGNED_INT, &meshVertexArray[face->meshVertexOffset]);
    }
    else if (face->type == 2)
    {
        Patch* patch = &patchArray[facePatchArray[index]];
        for (int i = 0; i < patch->bezierArray.size(); i++)
        {
            Bezier* bezier = &patch->bezierArray[i];
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), &bezier->vertexArray[0].position);
            //glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE,  sizeof(Vertex), &bezier->vertexArray[0].normal);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &bezier->vertexArray[0].texCoord);
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), &bezier->vertexArray[0].lmCoord);
            if (face->meshVertexCount != 0)
            {
                std::cout << "Hi There!" << std::endl;
                glDrawElements(GL_TRIANGLES, face->meshVertexCount, GL_UNSIGNED_INT, &meshVertexArray[face->meshVertexOffset]);
            }
            for (int l = 0; l < bezier->level; l++)
            {
                glDrawElements(GL_TRIANGLE_STRIP, bezier->rowCountArray[l], GL_UNSIGNED_INT, &bezier->indexArray[bezier->rowIndexArray[l]]);
            }
        }
    }

    pass.renderedFaces[index] = true;
}

void Map::renderLeaf(int index, RenderPass& pass, bool solid)
{
    Leaf* leaf = &leafArray[index];
    if (!clusterVisible(leaf->cluster, pass.cluster))
        return;
    if (!pass.frutsum.insideAABB(leaf->max, leaf->min))
        return;
    for (int i = 0; i < leaf->faceCount; i++)
    {
        int faceIndex = leafFaceArray[i + leaf->faceOffset];
        renderFace(faceIndex, pass, solid);
    }
}

void Map::renderNode(int index, RenderPass& pass, bool solid)
{
    if (index < 0)
    {
        renderLeaf(-index - 1, pass, solid);
        return;
    }

    Node* node = &nodeArray[index];
    Plane* plane = &planeArray[node->plane];
    if (!pass.frutsum.insideAABB(node->max, node->min))
        return;

    if ((glm::dot(plane->normal, pass.pos) >= plane->distance) == solid)
    {
        renderNode(node->children[0], pass, solid);
        renderNode(node->children[1], pass, solid);
    }
    else
    {
        renderNode(node->children[1], pass, solid);
        renderNode(node->children[0], pass, solid);
    }
}

void Map::renderWorld(glm::mat4 matrix, glm::vec3 pos)
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if (nodeArray.size() == 0)
        return;

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    //glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glUniformMatrix4fv(programLoc["matrix"], 1, GL_FALSE, &matrix[0][0]);
    glUniform1i(programLoc["texture"], 0);
    glUniform1i(programLoc["lightmap"], 1);

    RenderPass pass(this, pos, matrix);
    pass.cluster = leafArray[findLeaf(pos)].cluster;

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

void Map::traceBrush(int index, TracePass& pass)
{
    if (pass.tracedBrushes[index])
        return;
    Brush* brush = &brushArray[index];
    if (!shaderArray[brush->shader].solid)
        return;

    Plane* collidingPlane = NULL;
    float collidingDist = 0.0;

    for (int i = 0; i < brush->sideCount; i++)
    {
        BrushSide* side = &brushSideArray[i + brush->sideOffset];
        Plane* plane = &planeArray[side->plane];

        float dist = glm::dot(plane->normal, pass.pos) - plane->distance - pass.radius;

        if (dist > 0)
            return;

        if (collidingPlane == NULL || dist > collidingDist)
        {
            collidingPlane = plane;
            collidingDist = dist;
        }
    }

    if (collidingPlane == NULL)
        return;
    pass.pos -= collidingPlane->normal * collidingDist;

    pass.tracedBrushes[index] = true;
}

void Map::traceNode(int index, TracePass& pass)
{
    if (index < 0)
    {
        Leaf* leaf = &leafArray[-index - 1];
        for (int i = 0; i < leaf->brushCount; i++)
        {
            traceBrush(leafBrushArray[i + leaf->brushOffset], pass);
        }
        return;
    }

    Node* node = &nodeArray[index];
    Plane* plane = &planeArray[node->plane];
    float dist = glm::dot(plane->normal, pass.pos) - plane->distance;

    if (dist > -pass.radius)
    {
        traceNode(node->children[0], pass);
    }

    if (dist < pass.radius)
    {
        traceNode(node->children[1], pass);
    }
}

glm::vec3 Map::traceWorld(glm::vec3 pos, float radius)
{
    TracePass pass(this, pos, radius);
    traceNode(0, pass);

    return pass.pos;
}
