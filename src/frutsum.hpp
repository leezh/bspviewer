#ifndef FRUTSUM_HPP
#define FRUTSUM_HPP

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Frutsum
{
private:
    glm::vec4 planes[6];

public:
    Frutsum(glm::mat4 matrix);
    bool inside(glm::vec3 pos);
    bool insideAABB(glm::vec3 max, glm::vec3 min);
    bool insideAABB(int *max, int *min);
};

#endif // FRUTSUM_HPP
