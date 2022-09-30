#include <glm/vec4.hpp>
#include "matrices.h"

typedef struct box{
    float width;//largura é z
    float height;//altura é y
    float depth;//profundidade é x
    //vetores da base ortonormal
    glm::vec4 newX;
    glm::vec4 newY;
    glm::vec4 newZ;
    glm::vec4 origem;
}Box;

//box para facilitar cálculos de colisão, definida somente por 2 pontos
typedef struct envelopment_box{
    glm::vec4 p1;
    glm::vec4 p2;
}Ebox;

//refazer (era da esfera)
Ebox box_to_Ebox(Box box){
    Ebox e_box;
    //swap para facilitar cálculos posteriormente

    glm::vec4 minv = box.origem;

    glm::vec4 maxv = box.origem;

    glm::vec4 aux;

    for(int i = 0; i < 2; i++){
        aux = box.origem;
        if(i == 0){
            aux += box.width*box.newZ;
        }
        for(int i = 0; i < 2; i++){
            if(i == 0){
                aux += box.height*box.newY;
            }
            for(int i = 0; i < 2; i++){
                if(i == 0){
                    aux += box.depth*box.newX;
                }
                if(aux.x < minv.x){
                    minv.x = aux.x;
                }
                if(aux.x > maxv.x){
                    maxv.x = aux.x;
                }
                if(aux.y < minv.y){
                    minv.y = aux.y;
                }
                if(aux.y > maxv.y){
                    maxv.y = aux.y;
                }
                if(aux.z < minv.z){
                    minv.z = aux.z;
                }
                if(aux.z > maxv.z){
                    maxv.z = aux.z;
                }
            }
        }
    }

    e_box.p1 = minv;
    e_box.p2 = maxv;

    return e_box;
}

bool spheres_collision(glm::vec4 hitbox1Center, float hitbox1Radius, glm::vec4 hitbox2Center, float hitbox2Radius){
    return norm(hitbox1Center - hitbox2Center) < hitbox1Radius + hitbox2Radius;
}

bool Eboxes_collision(Ebox ebox1, Ebox ebox2){
    return (
        ebox1.p1.x <= ebox2.p2.x &&
        ebox1.p2.x >= ebox2.p1.x &&
        ebox1.p1.y <= ebox2.p2.y &&
        ebox1.p2.y >= ebox2.p1.y &&
        ebox1.p1.z <= ebox2.p2.z &&
        ebox1.p2.z >= ebox2.p1.z
    );
}

bool boxes_collision(Box box1, Box box2){
    //criar sistema de coordenadas para facilitar a descrição da box1

    //passar vértices da box2 para coordenadas da box1

    //fazer ebox da box1 e da box2 no novo sistema de coordenadas

    //fazer teste de colisão entre elas

    //criar sistema de coordenadas para facilitar a descrição da box2

    //passar vértices da box1 para coordenadas da box2

    //fazer ebox da box1 e da box2 no novo sistema de coordenadas

    //fazer teste de colisão entre elas

    //se os dois testes colidirem então há realmente uma colisão

    return true;
}