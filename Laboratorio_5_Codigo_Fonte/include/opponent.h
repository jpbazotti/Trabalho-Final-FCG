#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
// returns the opponent model matrix, also updates oldposition, forward and position
glm::mat4 opponentMovement(glm::mat4 model, float bezierTime, std::vector<glm::vec4> controlPoints1, std::vector<glm::vec4> controlPoints2, int degree, glm::vec4 &forward, glm::vec4 &pos, glm::vec4 &oldpos)
{
    glm::mat4 returnModel = model;
    glm::vec4 BezierPoint;
    if (bezierTime <= 1)
    {
        BezierPoint = Bezier(controlPoints1, degree, bezierTime);
    }
    else if(bezierTime<=2)
    {
        BezierPoint = Bezier(controlPoints2, degree, bezierTime-1);
    }
    else{
        return returnModel;
    }
    glm::vec4 newPoint = BezierPoint - oldpos;


    float dotprod = dotproduct(normalize(newPoint), forward);
    if (dotprod > 1)
    {
        dotprod = 1;
    }
    if (dotprod < -1)
    {
        dotprod = -1;
    }
    float angle = acos(dotprod);
    glm::vec4 cross = crossproduct(newPoint, forward);
    if (cross.y < 0)
    {
        angle = -1 * angle;
    }

    forward = normalize(newPoint);
    returnModel = Matrix_Translate(pos.x, pos.y, pos.z) * Matrix_Rotate_Y(-angle) * Matrix_Translate(-pos.x, -pos.y, -pos.z) * returnModel;
    returnModel = Matrix_Translate(newPoint.x, newPoint.y, newPoint.z) * returnModel;
    pos = Matrix_Translate(newPoint.x, newPoint.y, newPoint.z) * pos;

    oldpos = BezierPoint;
    return returnModel;
}
