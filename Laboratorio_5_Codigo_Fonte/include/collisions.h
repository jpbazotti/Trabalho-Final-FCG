#include <glm/vec4.hpp>

//refazer
typedef struct box{
    float width;//largura é para a esquerda
    float height;//altura é para cima
    float depth;//profundidade é para frente
    glm::vec4 direction;
    glm::vec4 position;
}Box;

//box para facilitar cálculos de colisão, definida somente por 2 pontos
typedef struct envelopment_box{
    glm::vec4 p1;
    glm::vec4 p2;
}Ebox;

//refazer (era da esfera)
Ebox box_to_Ebox(glm::vec4 sphereCenter, float sphereRadius);
bool spheres_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius);
bool Eboxes_collision(Ebox ebox1, Ebox ebox2);
bool boxes_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius);