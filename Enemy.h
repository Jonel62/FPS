#pragma once

#include "colliders.h"
#include "Model.h"
#include "shaderprogram.h"

class Enemy {
private:
    glm::vec3 position;
    glm::vec3 scale;
    float rotationY;
    bool alive;

    AABB collider;

    Model* model;

public:
    Enemy(Model* sharedModel, glm::vec3 startPos) {
        model = sharedModel;
        position = startPos;
        rotationY = 0.0f;
		scale = glm::vec3(0.5f); 
		alive = true;
        updateCollider();
    }

    void update(float deltaTime, glm::vec3 playerPos) {
        position = glm::mix(glm::vec3(position.x, 0, position.z), glm::vec3(playerPos.x, 0, playerPos.z), deltaTime * 0.5f);
		rotationY = atan2(playerPos.x - position.x, playerPos.z - position.z); 
        updateCollider();
    }

    void draw(ShaderProgram* sp) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, position);
        M = glm::scale(M, scale);
        M = glm::rotate(M, rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
        M = glm::rotate(M, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));
        model->Draw(sp->id);
    }
	bool isAlive() const { return alive; }
    AABB getCollider() const { return collider; }
	glm::vec3 getPosition() const { return position; }
	void kill() { alive = false; }
private:
    void updateCollider() {
        collider.min = position - glm::vec3(0.4f, 0.0f, 0.4f);
        collider.max = position + glm::vec3(0.4f, 2.0f, 0.4f);
    }
};