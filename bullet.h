#pragma once

#include "colliders.h"
#include "shaderprogram.h"

class Bullet
{
private:
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 size;
	float speed;
	AABB collider;
	bool active;
public:
	Bullet(glm::vec3 position, glm::vec3 direction) {
		this->position = position;
		this->direction = glm::normalize(direction);
		this->speed = 10.0f;
		this->size = glm::vec3(0.2f, 0.2f, 0.2f);
		this->active = true;
		updateCollider();
	};
	void update(float deltaTime) {
		this->position += direction * speed * deltaTime;
		updateCollider();
	};
	void draw(ShaderProgram* sp) {

	};
	void updateCollider() {
		glm::vec3 halfSize = size * 0.5f;
		collider.min = position - halfSize;
		collider.max = position + halfSize;
	}
	AABB getCollider() {
		return collider;
	}
	void deactivate() {
		active = false;
	}
	bool isActive() {
		return active;
	}
	glm::vec3 getPosition() {
		return position;
	}
};