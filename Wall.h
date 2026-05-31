#pragma once

#include "colliders.h"
#include <GL/glew.h>
#include "shaderprogram.h"
#include <glm/gtc/type_ptr.hpp>
#include "wall_vertices.h"

class Wall {
private:
    glm::vec3 position;
    glm::vec3 scale;

    AABB collider;
    GLuint texture;

    static GLuint staticVAO;
    static GLuint staticVBO;

    void updateCollider() {
        glm::vec3 halfScale = scale * 0.5f;
        collider.min = position - halfScale;
        collider.max = position + halfScale;
    }

    static void initStaticGeometry() {
        glGenVertexArrays(1, &staticVAO);
        glGenBuffers(1, &staticVBO);

        glBindVertexArray(staticVAO);
        glBindBuffer(GL_ARRAY_BUFFER, staticVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        glBindVertexArray(0);
    }

public:
    Wall(glm::vec3 pos, glm::vec3 scale, GLuint tex) {
        this->position = pos;
        this->scale = scale;
        this->texture = tex;

        if (staticVAO == 0) {
            initStaticGeometry();
        }

        updateCollider();
    }

    void draw(ShaderProgram* sp) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, position);
        M = glm::scale(M, scale);
        glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(staticVAO);

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
    }

    AABB getCollider() const { return collider; }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
        updateCollider();
    }
};

inline GLuint Wall::staticVAO = 0;
inline GLuint Wall::staticVBO = 0;