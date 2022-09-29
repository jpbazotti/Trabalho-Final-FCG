glm::vec4 Bezier(std::vector<glm::vec4> controlPoints, int degree, float point)
{
    std::vector<glm::vec4> points = controlPoints;
    int i = degree;
    while (i>0)
    {
        for (int j = 0; j < i; j++)
        {
            points.at(j)=(points[j] + point * (points[j + 1] - points[j]));
        }
        i--;
    }
    return points[0];
}