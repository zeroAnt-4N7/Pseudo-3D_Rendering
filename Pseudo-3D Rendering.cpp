#include <SFML/Graphics.hpp>
#include <iostream>
#include <Windows.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <math.h>

#define _WIN32_WINNT 0x0500


// console size
// increase to increase resolution
const int windowWidth = 1200;
const int windowHeight = 600;

// game map size
const int mapWidth = 30;
const int mapHeight = 30;

int previousMouseX = windowWidth / 2;
int previousMouseY = windowHeight / 2;

// player stats
float playerX = (float)mapWidth / 2.0;
float playerY = (float)mapHeight / 2.0;
float playerA = 0.0f;
float playerZAngle = 0.0f;

//field of view
float viewField = 30.0f;
float viewFieldZ = 180.0f;

// the step with which rays go
// increase to increase quality
float stepSize = 0.01f;

// rotation and walking sensetivity relatively
float rotationSesativity = 0.4f;
float rotationSesativityZ = 1.0f;
float speed = 2.5f;
float shiftAcceleration = 4.0f;

// how far textures could be seen
float depth = 10.0f;

bool accelerating = false;
bool wasAccelerating = false;

bool skyTextureOn = true;
bool floorTextureOn = true;

bool debuggingOn = false;

sf::Color backgroundColor(0, 0, 0, 0);

std::string filename = "map";

unsigned char *map;


void handleKeyboardEvents(unsigned char* map, float fElapsedTime) {
	// foward & backward movements handling
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
		playerX += cosf(playerA) * speed * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerX -= cosf(playerA) * speed * fElapsedTime;
		}
		playerY -= sinf(playerA) * speed * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerY += sinf(playerA) * speed * fElapsedTime;
		}

	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
		playerX -= cosf(playerA) * speed * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerX += cosf(playerA) * speed * fElapsedTime;
		}
		playerY += sinf(playerA) * speed * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerY -= sinf(playerA) * speed * fElapsedTime;
		}
	}

	// left & right movements handling
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
		playerX -= sinf(playerA) * speed / 2.0 * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerX += sinf(playerA) * speed / 2.0 * fElapsedTime;
		}
		playerY -= cosf(playerA) * speed / 2.0 * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerY += cosf(playerA) * speed / 2.0 * fElapsedTime;
		}
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
		playerX += sinf(playerA) * speed / 2.0 * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerX -= sinf(playerA) * speed / 2.0 * fElapsedTime;
		}
		playerY += cosf(playerA) * speed / 2.0 * fElapsedTime;
		if (map[(int)playerY * mapWidth + (int)playerX] == 9) {
			playerY -= cosf(playerA) * speed / 2.0 * fElapsedTime;
		}
	}

	// accelerating handling
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
		if (!accelerating) {
			speed *= shiftAcceleration;
			accelerating = true;
		}
		wasAccelerating = true;

	}
	else {
		if (wasAccelerating) {
			accelerating = false;
			wasAccelerating = false;
			speed /= shiftAcceleration;
		}
	}
}


void handleMouseEvents(sf::RenderWindow& window) {
	sf::Vector2i localPosition = sf::Mouse::getPosition(window);
	previousMouseX = localPosition.x;
	previousMouseY = localPosition.y;
	sf::Mouse::setPosition(sf::Vector2i(windowWidth / 2, windowHeight / 2), window);
	localPosition = sf::Mouse::getPosition(window);

	playerA += (localPosition.x - previousMouseX)*rotationSesativity / 100;
	playerZAngle += (localPosition.y - previousMouseY)*rotationSesativityZ / 50;
	if ((playerZAngle > viewFieldZ) || (playerZAngle < -viewFieldZ))
		playerZAngle -= (localPosition.y - previousMouseY)*rotationSesativityZ / 50;
}


void draw(sf::RenderWindow& window, int sky, int floor, float distanceToWall, int x) {
	// draw sky
	if (skyTextureOn) {
		sf::Vertex sky_line[] =
		{
			sf::Vertex(sf::Vector2f(x, 0), sf::Color(0, 0, 255, 200)),
			sf::Vertex(sf::Vector2f(x, windowHeight / 2.0f + 100 * playerZAngle), sf::Color(0, 0, 255, 50))
		};
		window.draw(sky_line, 2, sf::Lines);
	}

	// draw floor
	if (floorTextureOn) {

		sf::Vertex floor_line[] =
		{
			sf::Vertex(sf::Vector2f(x, windowHeight / 2.0f + 100 * playerZAngle), sf::Color(0, 255, 0, 50)),
			sf::Vertex(sf::Vector2f(x, windowHeight), sf::Color(0, 255, 0, 200))
		};
		window.draw(floor_line, 2, sf::Lines);
	}

	// if distance is out of bounds
	if (distanceToWall >= depth - 1) {
		sf::Vertex line[] =
		{
			sf::Vertex(sf::Vector2f(x, sky), sf::Color(0, 0, 0, 0)),
			sf::Vertex(sf::Vector2f(x, floor), sf::Color(0, 0, 0, 0))
		};
		window.draw(line, 2, sf::Lines);
	}

	// draw walls
	else {
		float gradient = 255 - (distanceToWall / depth * 255);
		sf::Vertex line[] =
		{
			sf::Vertex(sf::Vector2f(x, sky), sf::Color(gradient, gradient, gradient, 255)),
			sf::Vertex(sf::Vector2f(x, floor), sf::Color(gradient, gradient, gradient, 255))
		};
		window.draw(line, 2, sf::Lines);
	}

}


void getHorizon(float distanceToWall, float rayAngle, int *sky, int *floor) {
	*sky = (float)(windowHeight / 2.0) - windowHeight / ((float)distanceToWall*cosf(playerA - rayAngle));
	*floor = windowHeight - *sky;

	// change sky and floor position on vertical rotation
	*sky += 100 * playerZAngle;
	*floor += 100 * playerZAngle;
}


float getDistance(unsigned char *map, float rayAngle) {

	float distanceToWall = 0.0f;

	bool hitWall = false;

	//cursor position from player
	float eyeY = sinf(rayAngle);
	float eyeX = cosf(rayAngle);

	while (!hitWall && distanceToWall < depth) {
		distanceToWall += stepSize;
		// calculating new cursor position
		float curPositionX = eyeX * distanceToWall;
		float curPositionY = eyeY * distanceToWall;

		// cursor position from [0, 0]
		int testX = (int)(playerX + curPositionX);
		int testY = (int)(playerY - curPositionY);

		// checking if there is no wall
		if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight)
		{
			hitWall = true;
			distanceToWall = depth;
		}

		// checking if cursor lies in a wall
		else {
			if (map[testY * mapWidth + testX] == 9) {
				hitWall = true;
			}
		}
	}

	return distanceToWall;
}

int main() {
	// converting degrees to radians
	viewField *= 3.14159f / 180.0f;
	playerA *= 3.14159f / 180.0f;
	playerZAngle *= 3.14159f / 180.0f;
	viewFieldZ *= 3.14159f / 180.0f;

	// activate & disable console
	HWND hWnd = GetConsoleWindow();
	if (debuggingOn)
		ShowWindow(hWnd, SW_SHOW);
	if (!debuggingOn)
		ShowWindow(hWnd, SW_HIDE);

	// create sfml window
	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "3D Rendering");

	// disable cursorv
	window.setMouseCursorVisible(false);
	unsigned char map[mapWidth*mapHeight] = { 9, 9, 9 ,9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 ,9, 9, 9,
									                          0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 0, 0, 9, 9, 9, 9, 0, 9,
									                          9, 9, 9, 0, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 9, 0, 9, 9, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 9,
									                          9, 0, 0, 0, 9, 0, 9, 9, 0, 0, 9, 9, 9, 9, 9, 0, 9, 9, 9, 9, 0, 9, 9, 9, 9, 0, 9, 9, 9, 9,
									                          9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
									                          9, 0, 0, 0, 9, 0, 9, 9, 9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 9, 9, 0, 9, 9, 9, 9, 9, 0, 9,
									                          9, 9, 9, 0, 9, 0, 9, 9, 0, 0, 9, 0, 9, 0, 9, 9, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									                          9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 0, 9, 9, 9, 9, 9, 9, 0, 9, 9, 9, 9, 9, 9,
									                          9, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 9,
									                          9, 0, 9, 0, 0, 0, 9, 0, 9, 9, 0, 9, 9, 9, 9, 9, 0, 9, 0, 9, 9, 0, 9, 0, 9, 0, 9, 9, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 9, 0, 9, 9, 0, 9, 0, 9, 0, 9, 9, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 9, 9, 0, 9, 0, 9, 9, 0, 0, 9, 0, 9, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 0, 9, 0, 0, 9, 0, 9, 9, 0, 0, 0, 0, 0, 9, 9, 9, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 9, 9, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 9, 9, 0, 0, 0, 9, 9, 9, 0, 0, 9, 0, 9, 0, 9, 0, 9, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 9, 0, 9, 0, 9, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 9, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 9, 9, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 9, 9, 0, 9, 9, 0, 9, 9, 0, 9, 0, 9, 0, 9, 9, 9, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 9, 9, 0, 9, 9, 0, 9, 9, 0, 9, 0, 9, 0, 9, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 9, 9, 9, 0, 9, 0, 9, 9, 0, 9, 9, 0, 9, 9, 0, 9, 0, 9, 0, 9, 0, 0, 9,
									                          9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 9,
								                          	9, 0, 9, 0, 9, 0, 9, 0, 9, 9, 0, 9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 9, 0, 9, 0, 9, 9, 9, 9,
							                          		9, 0, 9, 0, 9, 0, 9, 0, 9, 9, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 9,
							                          		9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 9, 0, 9, 9, 9, 0, 9, 0, 9, 0, 9, 9, 0, 9, 9, 9,
							                          		9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 9, 0, 9, 9, 9, 0, 0, 0, 9, 9, 9, 0, 0, 0, 9, 9,
						                          			9, 0, 9, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 9, 0, 0, 9,
								                          	9, 0, 9, 0, 9, 0, 9, 9, 9, 9, 9, 0, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 9, 0, 9, 9, 9, 0, 9, 9,
							                          		9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 9,
								                          	9, 9, 9, 9 ,9, 9 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };

	// game map
	//std::ifstream file(filename + ".map");
	//mapWidth = getMapWidth(&file);
	//mapHeight = getMapHeight(&file);
	// getMap(&file);
	//unsigned char map[mapWidth*mapHeight] 


	// clocks to control FPS
	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	while (window.isOpen()) {

		// close sfml window if requested
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// reassigning clocks
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;

		// time to draw previous scene
		float fElapsedTime = elapsedTime.count();

		// movement & rotation handling handling
		handleKeyboardEvents(map, fElapsedTime);
		handleMouseEvents(window);

		// empty sfml window
		window.clear(backgroundColor);

		// going through each pixel on screen
		for (int x = 0; x < windowWidth; x++)
		{
			// calculating new angle at which a ray will be send
			float rayAngle = (playerA + viewField / 2.0f) - ((float)x / (float)windowWidth * viewField);

			// getting distance
			float distanceToWall = getDistance(map, rayAngle);

			// getting sky and floor of the world
			int sky, floor;
			getHorizon(distanceToWall, rayAngle, &sky, &floor);

			// draw scene
			draw(window, sky, floor, distanceToWall, x);
		}
		// draw new frame
		window.display();
	}


	return 0;
}
