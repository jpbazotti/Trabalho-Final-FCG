glm::vec4 Bezier(std::vector<glm::vec4> controlPoints, int degree, float point)
{
    std::vector<glm::vec4> oldPoints = controlPoints;
    for (int i = 0; i <= degree; i++)
    {
        std::vector<glm::vec4> newPoints;
        for (int j = 0; j < degree-i; j++)
        {
            newPoints.push_back(oldPoints[i] + point * (oldPoints[i + 1] - oldPoints[i]));
        }
        oldPoints = newPoints;
    }
    return oldPoints[0];
}