#include <glm/vec4.hpp>
#include "matrices.h"

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
Ebox box_to_Ebox(glm::vec4 sphereCenter, float sphereRadius){
    Ebox e_box;
    e_box.p1 = sphereCenter-sphereRadius*glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    e_box.p2 = sphereCenter+sphereRadius*glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    float aux;
    //swap para facilitar cálculos posteriormente
    if(e_box.p1.x > e_box.p2.x){
        aux = e_box.p1.x;
        e_box.p1.x = e_box.p2.x;
        e_box.p2.x = e_box.p1.x;
    }
    if(e_box.p1.y > e_box.p2.y){
        aux = e_box.p1.y;
        e_box.p1.y = e_box.p2.y;
        e_box.p2.y = e_box.p1.y;
    }
    if(e_box.p1.z > e_box.p2.z){
        aux = e_box.p1.z;
        e_box.p1.z = e_box.p2.z;
        e_box.p2.z = e_box.p1.z;
    }
    return e_box;
}

bool spheres_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius){
    return norm(hitbox1Center - hitbox2Center) < hitbox1Radius + hitbox2Radius;
}

bool Eboxes_collision(Ebox ebox1, Ebox ebox2){
    if(ebox1.p1.x > ebox2.p2.x && ebox1.p1.y > ebox2.p2.y && ebox1.p1.z > ebox2.p2.z){
        return false;
    }
    if(ebox1.p2.x < ebox2.p1.x && ebox1.p2.y < ebox2.p1.y && ebox1.p2.z < ebox2.p1.z){
        return false;
    }
}

//fazer
bool boxes_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius){
    return true;
}