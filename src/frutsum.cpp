#include <iostream>
#include <glm/glm.hpp>
#include "frutsum.hpp"

Frutsum::Frutsum(glm::mat4 matrix)
{
    matrix = glm::transpose(matrix);
    for (int i = 0; i < 6; i++)
    {
        glm::vec4 plane;

        int row = i >> 1;
        if (i & 1)
        {
            plane = matrix[3] + matrix[row];
        }
        else
        {
            plane = matrix[3] - matrix[row];
        }
        planes[i] = plane / glm::length(plane.xyz());
    }
}

bool Frutsum::inside(glm::vec3 pos)
{
    for (int i = 0; i < 6; i++)
    {
        if (glm::dot(planes[i].xyz(), pos) > planes[i].w)
        {
            return false;
        }
    }
    return true;
}

bool Frutsum::insideAABB(glm::vec3 max, glm::vec3 min)
{
    for (int i = 0; i < 6; i++)
    {
        glm::vec3 pVert = max;
        if (planes[i].x < 0)
            pVert.x = min.x;
        if (planes[i].y < 0)
            pVert.y = min.y;
        if (planes[i].z < 0)
            pVert.z = min.z;

        if (glm::dot(planes[i].xyz(), pVert) + planes[i].w <= 0)
        {
            return false;
        }
    }
    return true;
}

bool Frutsum::insideAABB(int *max, int *min)
{
    return insideAABB(glm::vec3(max[0], max[1], max[2]), glm::vec3(min[0], min[1], min[2]));
}
