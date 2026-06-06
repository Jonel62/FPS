/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define WALL_THICKNESS 0.1f
#define WALL_HEIGHT 5.0f
#define WALL_LENGTH 100.0f

#include <random>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "Model.h"
#include "Enemy.h"
#include "colliders.h"
#include "Wall.h"
#include "bullet.h"
#include <corecrt_math_defines.h>

float speed_x=0;
float speed_y=0;
float aspectRatio=1;
float deltaTime = 0.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.8f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;

float lastX = 250.0f, lastY = 250.0f; 
bool firstMouse = true;

Model* skeleton_model;

ShaderProgram *sp;

GLuint skeleton_diffuse;
GLuint skeleton_specular;

GLuint floor_diffuse;
GLuint wall_diffuse;

bool spacewaspressed = false;

int health = 100;

void ereaseEnemiesandBullets(std::vector<Enemy*>* enemies, std::vector<Bullet*>* bullets) {
	for (int i = 0; i < bullets->size(); i++) {
		(*bullets)[i]->update(deltaTime);

		if (!(*bullets)[i]->isActive()) {
			continue;
		}

		for (int j = 0; j < enemies->size(); j++) {
			if (checkCollision((*bullets)[i]->getCollider(), (*enemies)[j]->getCollider())) {
				(*enemies)[j]->kill();
				(*bullets)[i]->deactivate();
				break;
			}
		}
		glm::vec3 bulletPos = (*bullets)[i]->getPosition();
		if (bulletPos.x > 50.0f || bulletPos.x < -50.0f ||
			bulletPos.z > 50.0f || bulletPos.z < -50.0f){
			(*bullets)[i]->deactivate();
		}
	}

	enemies->erase(std::remove_if(enemies->begin(), enemies->end(), [](Enemy* e) {
		if (!e->isAlive()) {
			delete e;
			return true;
		}
		return false;
		}), enemies->end());

	bullets->erase(std::remove_if(bullets->begin(), bullets->end(), [](Bullet* b) {
		if (!b->isActive()) {
			delete b;
			return true;
		}
		return false;
		}), bullets->end());
}

glm::vec3 randomPosition(float radius, glm::vec3 playerPos) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 2.0f * M_PI);

	float angle = dis(gen);

	float x = radius * std::cos(angle) + playerPos.x;
	float y = radius * std::sin(angle) + playerPos.z;
	if (x > 50.0f) x = 50.0f;
	if (x < -50.0f) x = -50.0f;
	if (y > 50.0f) y = 50.0f;
	if (y < -50.0f) y = -50.0f;

	return glm::vec3(x, 0.0f, y);
}

void applyPlayerKnockback(glm::vec3 enemyPos, float force) {
	glm::vec3 knockbackDir = cameraPos - enemyPos;

	knockbackDir.y = 0.0f;

	float distance = glm::length(knockbackDir);
	if (distance > 0.001f) {
		knockbackDir = glm::normalize(knockbackDir);
	}
	else {
		knockbackDir = glm::vec3(0.0f, 0.0f, 1.0f);
	}
	cameraPos += knockbackDir * force;
}

void player_input(GLFWwindow* window, float cameraSpeed, std::vector<Enemy*> enemies, std::vector<Wall*> walls, std::vector<Bullet*>* bullets) {

	glm::vec3 lastPos = cameraPos;
	bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (spacePressed && !spacewaspressed) {
		glm::vec3 bulletPos = cameraPos + cameraFront * 0.5f;
		glm::vec3 bulletDir = cameraFront;
		bullets->push_back(new Bullet(bulletPos, bulletDir));
	}

	spacewaspressed = spacePressed;
	AABB playerBox;
	playerBox.min = cameraPos - glm::vec3(0.25f, 0.0f, 0.25f);
	playerBox.max = cameraPos + glm::vec3(0.25f, 1.8f, 0.25f);

	for (const auto& wall : walls) {
		if (checkCollision(playerBox, wall->getCollider())) {
			cameraPos = lastPos;
			return;
		}
	}

	for (const auto& enemy : enemies) {
		if (checkCollision(playerBox, enemy->getCollider())) {
			applyPlayerKnockback(enemy->getPosition(), 2.0f);
			health -= 10;
			return;
		}
	}
}

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f; 
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)  pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = 0;//sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
    glViewport(0,0,width,height);
}


GLuint readTexture(const char* filename) {
    GLuint tex;
    glActiveTexture(GL_TEXTURE0);

    //Wczytanie do pamięci komputera
    std::vector<unsigned char> image;   //Alokuj wektor do wczytania obrazka
    unsigned width, height;   //Zmienne do których wczytamy wymiary obrazka
    //Wczytaj obrazek
    unsigned error = lodepng::decode(image, width, height, filename);

    //Import do pamięci karty graficznej
    glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
    glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
    //Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
    glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return tex;
}


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0,0,0,1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);
	glfwSetWindowSizeCallback(window,windowResizeCallback);

	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
	skeleton_diffuse = readTexture("skeleton_diffuse.png");
	skeleton_specular = readTexture("skeleton_specular.jpeg");
	skeleton_model = new Model("skeleton.fbx", &skeleton_diffuse, &skeleton_specular);
	
	floor_diffuse = readTexture("floor.png");
	wall_diffuse = readTexture("wall.png");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	delete skeleton_model;
    delete sp;
}

//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, std::vector<Enemy*> enemies, std::vector<Wall*> walls) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 P = glm::perspective(glm::radians(45.0f), aspectRatio, 0.01f, 100.0f);

	// Przekazujemy pozycję gracza jako pozycję obserwatora
	glUniform3fv(glGetUniformLocation(sp->id, "viewPos"), 1, glm::value_ptr(cameraPos));

	glm::vec3 light1Pos(0.f, 2.0f, 100.f);
	glm::vec3 light1Color(1.0f, 1.0f, 1.0f); 
	glUniform3fv(glGetUniformLocation(sp->id, "light1.position"), 1, glm::value_ptr(light1Pos));
	glUniform3fv(glGetUniformLocation(sp->id, "light1.color"), 1, glm::value_ptr(light1Color));

	glm::vec3 light2Pos(5.0f, 2.0f, -100.0f);
	glm::vec3 light2Color(1.0f, 0.5f, 0.0f); 
	glUniform3fv(glGetUniformLocation(sp->id, "light2.position"), 1, glm::value_ptr(light2Pos));
	glUniform3fv(glGetUniformLocation(sp->id, "light2.color"), 1, glm::value_ptr(light2Color));

	sp->use();
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));

	for (int i = 0; i < walls.size(); i++) {
		walls[i]->draw(sp);
	}

	for (int i = 0; i < enemies.size(); i++) {
		enemies[i]->update(deltaTime, cameraPos);
		enemies[i]->draw(sp);
	}

	glfwSwapBuffers(window);
}

int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno
	std::vector<Enemy*> enemies;
	std::vector<Wall*> walls;
	std::vector<Bullet*> bullets;

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące
	glfwSetTime(0); //Zeruj timer

	walls.push_back(new Wall(glm::vec3(0.0f, -1.5f, .0f), glm::vec3(WALL_LENGTH, WALL_THICKNESS, WALL_LENGTH), floor_diffuse));
	walls.push_back(new Wall(glm::vec3(0.0f, 0.0f, -50.0f), glm::vec3(WALL_LENGTH, WALL_HEIGHT, WALL_THICKNESS), wall_diffuse));
	walls.push_back(new Wall(glm::vec3(0.0f, 0.0f, 50.0f), glm::vec3(WALL_LENGTH, WALL_HEIGHT, WALL_THICKNESS), wall_diffuse));
	walls.push_back(new Wall(glm::vec3(-50.0f, 0.0f, 0.0f), glm::vec3(WALL_THICKNESS, WALL_HEIGHT, WALL_LENGTH), wall_diffuse));
	walls.push_back(new Wall(glm::vec3(50.0f, 0.0f, 0.0f), glm::vec3(WALL_THICKNESS, WALL_HEIGHT, WALL_LENGTH), wall_diffuse));
	while (!glfwWindowShouldClose(window)) 
	{
		float currentFrame = glfwGetTime();
		static float lastFrame = 0.0f;
		if ((int(currentFrame) % 3 == 0) && (int(currentFrame)!=int(lastFrame))) {
			enemies.push_back(new Enemy(skeleton_model, randomPosition(15, cameraPos)));
		}
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		float cameraSpeed = 5.f * deltaTime;

		player_input(window, cameraSpeed, enemies, walls, &bullets);
		if(health <= 0) {
			std::cout << "Game Over!" << std::endl;
			break;
		}
		std::cout << health << std::endl;
		ereaseEnemiesandBullets(&enemies, &bullets);
		drawScene(window, enemies, walls); 
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
