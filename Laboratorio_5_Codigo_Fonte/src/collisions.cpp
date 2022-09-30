#include <glm/vec4.hpp>
#include <vector>
#include "matrices.h"

glm::vec4 nullvector = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

typedef struct bbox
{
    glm::vec4 minPoint;
    glm::vec4 maxPoint;
    glm::vec4 normal;
} bbox;

bool bbcollision(bbox bbox1, bbox bbox2)
{
    return (
        bbox1.minPoint.x <= bbox2.maxPoint.x &&
        bbox1.maxPoint.x >= bbox2.minPoint.x &&
        bbox1.minPoint.y <= bbox2.maxPoint.y &&
        bbox1.maxPoint.y >= bbox2.minPoint.y &&
        bbox1.minPoint.z <= bbox2.maxPoint.z &&
        bbox1.maxPoint.z >= bbox2.minPoint.z);
}

glm::vec4 checkAllbbox(bbox player, std::vector<bbox> list)
{
    for (int i = 0; i < list.size(); i++)
    {
        if (bbcollision(player, list.at(i)))
        {
            return list.at(i).normal;
        }
    }
    return nullvector;
}

bool spheres_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius)
{
    return norm(hitbox1Center - hitbox2Center) < hitbox1Radius + hitbox2Radius;
}
