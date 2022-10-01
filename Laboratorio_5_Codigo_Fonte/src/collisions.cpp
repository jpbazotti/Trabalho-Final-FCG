#include <glm/vec4.hpp>
#include <vector>
#include "matrices.h"
#include "bezier.h"
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

bool sphere_point(glm::vec4 hitbox1Center, float hitbox1Radius,glm::vec4 point){
    return norm(hitbox1Center-point)<hitbox1Radius;
}

glm::vec4 checkBezier(glm::vec4 hitbox1Center, float hitbox1Radius, std::vector<glm::vec4> controlPoints,float step)
{
    for(float i = 0;i<=1;i+=step){
        glm::vec4 point=Bezier(controlPoints,3,i);
        if(sphere_point(hitbox1Center,hitbox1Radius,point)){
            return point;
        }
    }
    return nullvector;
}
glm::vec4 checkAllBezier(glm::vec4 hitbox1Center, float hitbox1Radius, std::vector<std::vector<glm::vec4>> bezierList,float step)
{
    for (int i = 0; i < bezierList.size(); i++)
    {   
       std::vector<glm::vec4> current=bezierList.at(i);
       glm::vec4 point = checkBezier(hitbox1Center,hitbox1Radius,current,step);
       if(point!=nullvector){
        return point;
       }
    }
    return nullvector;
}
