#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING  1
#include "ScriptManager.h"
#include "dukglue/dukglue.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <unordered_map> 
#include <queue>
#include <stack>
using namespace std;
#include <zmq.hpp>
#ifndef _WIN32
#else
#include <thread>
#include <cassert>
#include <string>
#define sleep(n)    Sleep(n)
#endif
using namespace std::tr1;
#pragma warning(disable : 4996)

/**
 * * * * * * * * * * * * * * *
 *                           *
 *  TIME LINE CLASSES START  *
 *                           *
 * * * * * * * * * * * * * * *
 */

 /**
  * The super class for time line related classes.
  * Refered to this tutorial to learn about virtual functions:
  * https://www.geeksforgeeks.org/virtual-function-cpp/
  */
class Timeline
{
public:
    virtual int getTime() = 0;
    virtual void changeTic(float tic_size) = 0;
    virtual void pause() = 0;
protected:
    float tic_size;
};

/**
* A real time class. Subclass of timeline.
* Referred to this link to discover chrono package types and functions.
* https://en.cppreference.com/w/cpp/chrono
* Furthermore looked at this duration_cast documentation and example to
* learn about finding the difference between two times:
* https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
*/
class RealTime : public Timeline
{
public:
    RealTime(float tic_size)
    {
        this->tic_size = tic_size;
        start_time = std::chrono::system_clock::now();
    }

    int getTime() {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli> time_diff = current_time - start_time;
        float tics = (time_diff.count()) / (tic_size * 50);
        return floor(tics);
    }

    void pause() {
        pause_time = std::chrono::system_clock::now();
    }

    int getPausedTime() {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<float, std::milli> time_diff = current_time - pause_time;
        float tics = (time_diff.count()) / (tic_size * 50);
        return floor(tics);
    }

    void changeTic(float tic_size) {
        this->tic_size = tic_size;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> pause_time;
};

/**
* A real time class. Subclass of timeline.
* Referred to this link to discover chrono package types and functions.
* https://en.cppreference.com/w/cpp/chrono
*/
class GameTime : public Timeline
{
public:
    GameTime(float tic_size, RealTime* realTime)
    {
        this->tic_size = tic_size;
        t = realTime;
        paused_tics = 0;
    }

    int getTime() {
        return (*t).getTime() - paused_tics;
    }

    void pause() {
        if (paused) {
            paused_tics += (*t).getPausedTime();
            paused = false;
        }
        else {
            (*t).pause();
            paused = true;
        }
    }

    bool onPause() {
        return paused;
    }

    void changeTic(float tic_size) {
        float ratio = (this->tic_size) / tic_size;
        this->tic_size = tic_size;
        (*t).changeTic(tic_size);
        paused_tics *= ratio;
    }

    float getTic() {
        return this->tic_size;
    }

private:
    RealTime* t;
    int paused_tics;
    bool paused;
};

/**
 * * * * * * * * * * * * * *
 *                         *
 *  TIME LINE CLASSES END  *
 *                         *
 * * * * * * * * * * * * * *
 */


 /**
  * * * * * * * * * * * * *
  *                       *
  *  GAME ENGINE START    *
  *                       *
  * * * * * * * * * * * * *
  */

  // Engine Enums
enum bodyType { SINGLE_SHAPE, MULTI_SHAPE };
enum materials { LETHAL, SOLID, BOUND, AVATAR_LETHAL, AVATAR_SOLID, AVATAR_OTHER, OTHER };
enum actions { STILL, FALL, JUMP, LEFT, RIGHT, UP, DOWN, PUSH, DIE, SCROLL_RIGHT, SCROLL_LEFT };
enum jumpStatus { NONE, FALLING, JUMPING };
enum slideStatus { SLIDING_RIGHT, SLIDING_LEFT, SLIDING_UP, SLIDING_DOWN };

// Engine Classes
class GameObject; // Game Objects
class Renderer; // Render Object / Objects To Window
class CollisionDetector; // Detects Collisions & Collision Types
class SkinProvider; // Provides Skin To The Object

/** The Game Object Superclass*/
class GameObject
{
public:

    /** Returns the body of the shape */
    virtual sf::Shape* getBody() {
        sf::CircleShape point(1);
        return &point;
    }

    /** Returns the shapes that make up the body of the game object */
    virtual std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(3);
        sf::CircleShape point(1);
        bodies.push_back(&point);
        return bodies;
    }

    /** Returns the material of the shape */
    int getMaterial() {
        return material;
    }

    /** Get the X coordinate in respect to the game's world */
    int getCoordX() {
        return coordX;
    }

    /** Get the Y coordinate in respect to the game's world */
    int getCoordY() {
        return coordY;
    }

    int getId() {
        return id;
    }

    /** Returns the X coordinate (or coordinates) as an array */
    virtual std::vector<int> getCoordinatesX() {
        std::vector<int> xcoords(1);
        xcoords.push_back(coordX);
        return xcoords;
    }

    /** Returns the Y coordinate (or coordinates) as an array  */
    virtual std::vector<int> getCoordinatesY() {
        std::vector<int> ycoords(1);
        ycoords.push_back(coordY);
        return ycoords;
    }

    virtual void render(sf::RenderWindow& window, int transform) {
        // Nothing Yet
    }

    /** Get the X & Y coordinates in the world */
    void setCoordinates(int newX, int newY) {
        this->coordX = newX;
        this->coordY = newY;
    }

    /** Returns The Object's Action*/
    virtual int getAction() {
        return this->action;
    }

    virtual std::string toString() {
        return std::to_string(id);
    }

protected:

    int id;
    int material;
    int bodyType;
    int coordX;
    int coordY;
    int action;
    Renderer* renderer;
    CollisionDetector* collisionDetector;
    SkinProvider* skinProvider;
    //MovementManager* movementManager;
};

/** A Rendering Tool */
class Renderer
{
public:
    // Draws A Single Object To The Window 
    void render(sf::RenderWindow& window, sf::Shape* body, int x, int y, int transform) {
        (*body).setPosition(x + transform, y);
        window.draw(*body);
    }

    // Draws A Collection Of Objects To The Window
    void renderCollection(sf::RenderWindow& window, GameObject* object, int transform) {

        std::vector<sf::Shape*> bodies = (*object).getBodyCollection();
        std::vector<int> xcoords = (*object).getCoordinatesX();
        std::vector<int> ycoords = (*object).getCoordinatesY();

        std::vector<sf::Shape*>::iterator itShape = bodies.begin();
        std::vector<int>::iterator itX = xcoords.begin();
        std::vector<int>::iterator itY = ycoords.begin();

        while (itShape != bodies.end() && itX != xcoords.end() && itY != ycoords.end()) {
            if (*itShape == NULL || *itX == NULL || *itY == NULL) {
                if (*itShape == NULL) {
                    ++itShape;
                }
                if (*itX == NULL) {
                    ++itX;
                }
                if (*itY == NULL) {
                    ++itY;
                }
            }
            else {
                //std::cout << "got here" << endl;
                sf::Shape* body = *itShape;
                int x = *itX;
                int y = *itY;
                render(window, body, x, y, transform);
                ++itShape;
                ++itX;
                ++itY;
            }

        }
    }
};

/** A Tool For Detecting Collisions */
class CollisionDetector
{
public:
    // Detects Collisons & Returns The Collision Response
    std::vector<int> collide(std::vector<GameObject*> otherObjects, GameObject* thisObject, int action) {

        //std::cout << "Action " << action << std::endl;
        if (action == STILL || action == FALL) // check below for falling 
        {
            (*(*thisObject).getBody()).move(0, 5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, -5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == JUMP) // check above for jump / Up collision
        {
            (*(*thisObject).getBody()).move(0, -20);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, 20);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == UP) // check above for jump / Up collision
        {
            (*(*thisObject).getBody()).move(0, -1);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(0, 1);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            return ret;
        }

        if (action == RIGHT) // check the right for right collision
        {
            (*(*thisObject).getBody()).move(8, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(-8, 5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }

        if (action == LEFT) // check left for left collision
        {
            (*(*thisObject).getBody()).move(-8, -5);
            sf::FloatRect myBounds = (*(*thisObject).getBody()).getGlobalBounds();
            (*(*thisObject).getBody()).move(8, 5);
            std::vector<int> ret = intersection(otherObjects, myBounds, action);
            //std::cout << "Right Requested; Result Is- Still(0) Fall(1) RIGHT(4) LEFT(3) - " << ret << std::endl;
            return ret;
        }
        // std::cout << "EMERGENCY" << std::endl;
        std::vector<int> ret;
        ret.push_back(0);
        ret.push_back(0);
        return ret;
    }

private:

    // Checks Fpr Intersections Between Objects
    std::vector<int> intersection(std::vector<GameObject*> otherObjects, sf::FloatRect myBounds, int action) {
        bool scrollRight = false;
        bool scrollLeft = false;
        std::vector<int> ret;
        ret.push_back(0);
        ret.push_back(0);
        ret.push_back(-1);
        for (std::vector<GameObject*>::iterator it = otherObjects.begin(); it != otherObjects.end(); ++it) {
            int mat = (*it)->getMaterial();
            sf::Shape* body = (*it)->getBody();
            sf::FloatRect otherBounds = (*body).getGlobalBounds();
            if (myBounds.intersects(otherBounds) == true) {
                ret.at(0) = 1;
                ret.at(2) = (*it)->getId();
                if (mat == LETHAL) {
                    //std::cout << "LETHAL DETECTED (1)" << std::endl;
                    ret.at(1) = DIE;
                    return ret;
                }
                else if (mat == SOLID) {
                    //if (action == RIGHT || action == LEFT) {
                        //std::cout << "SOLID DETECTED" << std::endl;
                    //}
                    if (action == STILL || action == FALL || action == RIGHT || action == LEFT) {
                        //std::cout << "Hit Solid - Still(0) Fall(1) RIGHT(4) LEFT(3) - " << action << std::endl;
                        ret.at(1) = STILL;
                        return ret;
                    }
                    else  if (action == JUMP) {
                        ret.at(1) == FALL;
                        return ret;
                        // std::cout << "Jump & Hit Solid Fall" << action << std::endl;
                    }
                }
                else if (mat == BOUND) {
                    if (action == RIGHT) {
                        //return SCROLL_RIGHT;
                        //std::cout << "BOUND DETECTED" << std::endl;
                        scrollRight = true;
                    }
                    else if (action == LEFT) {
                        //return SCROLL_LEFT;
                        //std::cout << "BOUND DETECTED" << std::endl;
                        scrollLeft = true;
                    }
                }
                else if (mat == AVATAR_SOLID) {
                    //std::cout << "AVATAR SOLID DETECTED" << std::endl;
                    if (action == UP) {
                        ret.at(1) = PUSH;
                        return ret;
                    }
                }
            }
        }
        if (action == STILL) {
            //std::cout << "Falling" << std::endl;
            ret.at(1) = FALL;
            return ret;
            // std::cout << "No Intersection Fall" << action << std::endl;
        }
        if (scrollRight) {
            //std::cout << "Right Scroll" << std::endl;
            ret.at(1) = SCROLL_RIGHT;
            return ret;
        }
        if (scrollLeft) {
            //std::cout << "Left Scroll" << std::endl;
            ret.at(1) = SCROLL_LEFT;
            return ret;
        }
        if (action == JUMP && myBounds.top < 15) {
            //std::cout << "Ceiling Intersect" << action << std::endl;
            ret.at(0) = 1;
            ret.at(1) = FALL;
            return ret;
        }
        ret.at(1) = action;
        return ret;
    }
};

/** Horizontal Movement Manager Class */
class HorizontalSliderMovementManager
{
public:
    // The Default Constructor 
    HorizontalSliderMovementManager() {
        this->maxRight = 0;
        this->minLeft = 700;
        this->leftLength = 0;
        this->rightLength = 50;
        this->movingRight = true;
    }

    // The Preferred Constructor 
    HorizontalSliderMovementManager(int left, int right, int rightLength, int leftLength, int action) {
        this->maxRight = right;
        this->minLeft = left;
        this->leftLength = leftLength;
        this->rightLength = rightLength;
        if (action == RIGHT) {
            this->movingRight = true;
        }
        else {
            this->movingRight = false;
        }
    }

    // The Next Position 
    int* nextPosition(int x, int y) {
        int ret[2];
        ret[0] = x;
        ret[1] = y;
        if (movingRight) { // moving right
            if ((x + leftLength + 1) <= maxRight) { // w/in range
                ret[0]++;
            }
            else { // hit boundary
                movingRight = false;
            }
        }
        else { // moving Left
            if ((x - rightLength - 1) >= minLeft) { // w/in range
                ret[0]--;
            }
            else { // hit boundary
                movingRight = true;
            }
        }

        return ret;
    }

    // The Slider Status
    int getStatus() {
        if (movingRight) {
            return SLIDING_RIGHT;
        }
        return SLIDING_LEFT;
    }

private:
    bool movingRight;
    int maxRight;
    int minLeft;
    int leftLength;
    int rightLength;
};

/** A Class / Tool For Managing Verticle Sliding Movement */
class VerticalSliderMovementManager
{
public:
    // The Default Constructor 
    VerticalSliderMovementManager() {
        this->minUp = 0;
        this->maxDown = 450;
        this->lowerLength = 50;
        this->upperLength = 0;
        this->movingUp = true;
    }

    // The Preferred Constructor
    VerticalSliderMovementManager(int up, int down, int upLength, int downLength, int action) {
        this->minUp = up;
        this->maxDown = down;
        this->lowerLength = downLength;
        this->upperLength = upLength;
        if (action == UP) {
            this->movingUp = true;
        }
        else {
            this->movingUp = false;
        }
    }

    // The Next Position
    int* nextPosition(int x, int y) {
        int ret[2];
        ret[0] = x;
        ret[1] = y;
        if (movingUp) { // moving up
            if ((y - upperLength - 1) > minUp) { // w/in range
                ret[1]--;
            }
            else { // hit boundary
                movingUp = false;
            }
        }
        else { // moving down
            if ((y + lowerLength + 1) < maxDown) { // w/in range
                ret[1]++;
            }
            else { // hit boundary
                movingUp = true;
            }
        }
        return ret;
    }

    // The Status Of The Slider
    int getStatus() {
        if (movingUp) {
            return SLIDING_UP;
        }
        return SLIDING_DOWN;
    }

private:
    bool movingUp;
    int minUp;
    int maxDown;
    int lowerLength;
    int upperLength;
};

/** A Class / Tool For Managing Jumping Falling Movement */
class JumpingFallingMovementManager
{
public:
    // The Default Constructor
    JumpingFallingMovementManager() {
        this->jumping = false;
        this->falling = false;
        this->currentJumpSpeed = 2;
        this->startJumpSpeed = 2;
        this->currentFallingSpeed = 2;
        this->startFallingSpeed = 2;
        this->jumpAccel = 0.02;
        this->fallingAccel = 0.001;
    }

    // The Prefered Constructor 
    JumpingFallingMovementManager(float jumpingSpeed, float jumpingAccel, float fallingSpeed, float fallingAccel) {
        this->jumping = false;
        this->falling = false;
        this->currentJumpSpeed = jumpingSpeed;
        this->startJumpSpeed = jumpingSpeed;
        this->currentFallingSpeed = fallingSpeed;
        this->startFallingSpeed = fallingSpeed;
        this->jumpAccel = jumpingAccel;
        this->fallingAccel = fallingAccel;
    }

    // Returns The Next Position Of The Character
    int* nextPosition(int x, int y, int action) {
        int ret[3];
        ret[0] = x;
        ret[1] = y;
        ret[2] = 0;
        if (action == STILL) {
            falling = false;
            jumping = false;
            currentFallingSpeed = startFallingSpeed;
            currentJumpSpeed = startJumpSpeed;
            return ret;
        }
        if (action == JUMP && !falling && y > 0) {
            currentJumpSpeed -= jumpAccel;
            ret[1] -= currentJumpSpeed;
            if (currentJumpSpeed < 0) {
                jumping = true;
                falling = false;
                return ret;
            }
            else {
                ret[1] -= currentJumpSpeed;
                return ret;
            }
        }
        if (action == JUMP && falling || action == JUMP && y <= 0) {
            jumping = false;
            falling = true;
            ret[2] = FALL;
            return ret;
        }
        if (action == FALL) {
            currentFallingSpeed += fallingAccel;
            ret[1] += currentFallingSpeed;
            return ret;
        }
        // std::cout << "action: " << action << std::endl;
        return ret;
    }

    // Returns Whether The Object Is Jumping Or Falling or Neither
    int status() {
        // std::cout << "jumping status: " << jumping << std::endl;
        if (jumping) {
            return JUMP;
        }
        else if (falling) {
            return FALL;
        }
        else {
            return STILL;
        }
    }

private:
    bool jumping;
    float currentJumpSpeed;
    float startJumpSpeed;
    float jumpAccel;

    bool falling;
    float currentFallingSpeed;
    float startFallingSpeed;
    float fallingAccel;
};

/** The Super Class For Skin Providers */
class SkinProvider
{
public:
    void virtual wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        // Nothing Happens
    }

    void virtual wearSkinOutline(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        // Nothing Happens
    }
};

/** The Skin Provider For Textured Skin */
class TextureSkinProvider : public SkinProvider
{
public:

    /**
     * https://www.youtube.com/watch?v=NGnjDIOGp8s (loading textures )
     */

     /** Wear A Textured Skin */
    void wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        sf::Texture texture;
    }

    void wearSkin2(sf::Shape* body, std::string stringSkin, sf::Texture* texture) {

        if (!(*texture).loadFromFile("Textures/dark_wood.JPG"))
        {
            std::cout << "ERROR LOADING IMAGE" << std::endl;
        }
        (*body).setTexture(texture);
    }

    /** The Skin Outline */
    void wearSkinOutline(sf::Shape* body, std::string stringOutline, int red, int green, int blue) {
        (*body).setOutlineThickness(5);
        if (stringOutline.compare("Blue") == 0) {
            (*body).setOutlineColor(sf::Color::Blue);
            return;
        }
        if (stringOutline.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringOutline.compare("White") == 0) {
            (*body).setOutlineColor(sf::Color::White);
            return;
        }
        if (stringOutline.compare("Red") == 0) {
            (*body).setOutlineColor(sf::Color::Red);
            return;
        }
        if (stringOutline.compare("Green") == 0) {
            (*body).setOutlineColor(sf::Color::Green);
            return;
        }
        if (stringOutline.compare("Yellow") == 0) {
            (*body).setOutlineColor(sf::Color::Yellow);
            return;
        }
        if (stringOutline.compare("Magenta") == 0) {
            (*body).setOutlineColor(sf::Color::Magenta);
            return;
        }
        if (stringOutline.compare("Cyan") == 0) {
            (*body).setOutlineColor(sf::Color::Cyan);
            return;
        }
        if (stringOutline.compare("Transparent") == 0) {
            (*body).setOutlineColor(sf::Color::Transparent);
            return;
        }
        (*body).setOutlineColor(sf::Color::Color(red, green, blue, 255));
    }
};

/** The Skin Provider For Solid Colors */
class ColorSkinProvider : public SkinProvider
{
public:

    /** Add Solid Color Skin */
    void wearSkin(sf::Shape* body, std::string stringSkin, int red, int green, int blue) {
        if (stringSkin.compare("Blue") == 0) {
            (*body).setFillColor(sf::Color::Blue);
            return;
        }
        if (stringSkin.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringSkin.compare("White") == 0) {
            (*body).setFillColor(sf::Color::White);
            return;
        }
        if (stringSkin.compare("Red") == 0) {
            (*body).setFillColor(sf::Color::Red);
            return;
        }
        if (stringSkin.compare("Green") == 0) {
            (*body).setFillColor(sf::Color::Green);
            return;
        }
        if (stringSkin.compare("Yellow") == 0) {
            (*body).setFillColor(sf::Color::Yellow);
            return;
        }
        if (stringSkin.compare("Magenta") == 0) {
            (*body).setFillColor(sf::Color::Magenta);
            return;
        }
        if (stringSkin.compare("Cyan") == 0) {
            (*body).setFillColor(sf::Color::Cyan);
            return;
        }
        if (stringSkin.compare("Transparent") == 0) {
            (*body).setFillColor(sf::Color::Transparent);
            return;
        }
        (*body).setFillColor(sf::Color::Color(red, green, blue, 255));
    }

    /** Add A Colored Outline */
    void wearSkinOutline(sf::Shape* body, std::string stringOutline, int red, int green, int blue) {
        (*body).setOutlineThickness(5);
        if (stringOutline.compare("Blue") == 0) {
            (*body).setOutlineColor(sf::Color::Blue);
            return;
        }
        if (stringOutline.compare("Black") == 0) {
            (*body).setFillColor(sf::Color::Black);
            return;
        }
        if (stringOutline.compare("White") == 0) {
            (*body).setOutlineColor(sf::Color::White);
            return;
        }
        if (stringOutline.compare("Red") == 0) {
            (*body).setOutlineColor(sf::Color::Red);
            return;
        }
        if (stringOutline.compare("Green") == 0) {
            (*body).setOutlineColor(sf::Color::Green);
            return;
        }
        if (stringOutline.compare("Yellow") == 0) {
            (*body).setOutlineColor(sf::Color::Yellow);
            return;
        }
        if (stringOutline.compare("Magenta") == 0) {
            (*body).setOutlineColor(sf::Color::Magenta);
            return;
        }
        if (stringOutline.compare("Cyan") == 0) {
            (*body).setOutlineColor(sf::Color::Cyan);
            return;
        }
        if (stringOutline.compare("Transparent") == 0) {
            (*body).setOutlineColor(sf::Color::Transparent);
            return;
        }
        (*body).setOutlineColor(sf::Color::Color(red, green, blue, 255));
    }
};

/**
 * * * * * * * * * * * *
 *                     *
 *  GAME ENGINE END    *
 *                     *
 * * * * * * * * * * * *
 */

 /**
  * * * * * * * * * * * * * *
  *                         *
  *  ACTUAL OBJECTS START   *
  *                         *
  * * * * * * * * * * * * * *
  */


/** SpaceInvader Moving Object*/
class SpaceInvader : public GameObject
{
public:

    SpaceInvader() {} // Irelevant Default Const. 

    SpaceInvader(int id, int startX, int startY, int distance) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = startX;
        this->coordY = startY;

        this->startX = startX;
        this->startY = startY;
        this->endX = distance;
        this->endY = startY;
        this->direction = 0;
        this->dead = false;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = RIGHT;

        // Set Width & Height and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(50, 30));
        this->body = rectangle;

        this->distance = distance;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 216, 228, 188);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 34, 139, 34);
    }

    void die() {
        this->dead = true;
    }

    void restart() {
        this->dead = false;
    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        if (this->dead == false) {
            (*this->renderer).renderCollection(window, this, transform);
        }
    }

    // Returns The Action Of This Moving Platform
    int getAction() {
        return this->action;
    }

    // Returns The Action Of This Moving Platform
    void changeAction(int action) {
        this->action = action;
    }

    int getDistance() {
        return distance;
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

    int getStartX() {
        return this->startX;
    }

    int getStartY() {
        return this->startY;
    }

    int getEndX() {
        return this->endX;
    }

    int getEndY() {
        return this->endY;
    }

    int getDirection() {
        return this->direction;
    }

    void setDirection(int direction) {
        this->direction = direction;
    }

private:

    sf::RectangleShape body;
    int width;
    int height;
    int distance;
    int startX;
    int startY;
    int endX;
    int endY;
    int direction;
    bool dead;
};

/** Character Object */
class Character : public GameObject
{
public:
    Character(int id) {

        // Set Id
        this->id = id;

        // Set Coordinates
        this->coordX = 400;
        this->coordY = 500;

        // Set Material & Body Type
        this->material = AVATAR_SOLID;
        this->bodyType = SINGLE_SHAPE;

        // Set The Body
        sf::CircleShape circle{};
        circle.setRadius(22.5);
        circle.setPosition(200, 500);
        this->body = circle;

        // Set The Action
        this->action = STILL;

        // Set Up The Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 8, 78, 30);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 69, 77, 50);

    }

    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

    // Move Right
    void moveRight() {
        coordX += 6;
    }

    // Move Left
    void moveLeft() {
        coordX -= 6;
    }

    // Change Character Action 
    void changeAction(int action) {
        this->action = action;
    }

    // Kill Character
    void die() {
        this->action = DIE;
    }

    // Render Another Character
    static void renderOtherCharacter(int x, int y, sf::RenderWindow& window, int transform) {

        sf::CircleShape circle{};
        circle.setRadius(22.5);
        circle.setPosition(x + transform, y);

        ColorSkinProvider skinProvider = ColorSkinProvider{};
        (skinProvider).wearSkin(&(circle), "", 0, 32, 194);
        (skinProvider).wearSkinOutline(&(circle), "", 21, 27, 84);

        window.draw(circle);
    }

private:
    sf::CircleShape body;
};

/** The Game World */
class Bullet : public GameObject
{
public:
    Bullet(int x, int y)
    {
        this->coordX = x;
        this->coordY = y;

        // Set Material & Body Type
        this->material = SOLID;
        this->bodyType = SINGLE_SHAPE;

        this->action = RIGHT;

        // Set Width & Height and Initalize Body
        sf::RectangleShape rectangle(sf::Vector2f(15, 30));
        this->body = rectangle;

        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;

        // Color Skin
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;
        (*this->skinProvider).wearSkin(&(this->body), "", 255, 255, 155);
        (*this->skinProvider).wearSkinOutline(&(this->body), "", 34, 139, 34);
    }
    // Render to Window
    void render(sf::RenderWindow& window, int transform) {
        (*this->renderer).renderCollection(window, this, transform);
    }

    // Returns The Action Of This Moving Platform
    int getAction() {
        return this->action;
    }

    // Returns The Action Of This Moving Platform
    void changeAction(int action) {
        this->action = action;
    }

    // Return a Refernece To The Body
    sf::Shape* getBody() {
        return &body;
    }

    // Returns a Reference To The Body Collection
    std::vector<sf::Shape*> getBodyCollection() {
        std::vector<sf::Shape*> bodies(1);
        bodies.push_back(&body);
        return bodies;
    }

private:

    sf::RectangleShape body;
    int width;
    int height;

};

class World : public GameObject
{
public:

    World() {} // Irelevant Default Const. 

    World(int id)
    {
        // Initialize Tools
        Renderer newRenderer = Renderer{};
        this->renderer = &newRenderer;
        CollisionDetector newCollisionDetector;
        this->collisionDetector = &newCollisionDetector;
        ColorSkinProvider skinProvider = ColorSkinProvider{};
        this->skinProvider = &skinProvider;

        // Initialize Characteristics 
        this->id = id;
        this->material = OTHER;
        this->bodyType = SINGLE_SHAPE;
        this->action = STILL;
        this->coordX = 0;
        this->coordY = 0;

    }

    // Add A Generic Object To Game Object Array
    void addObject(GameObject* gameObject) {
        renderObjects.push_back(gameObject);
    }

    void checkBulletCollision() {
        
    }

    // Add A Cumulus Cloud
    void addSpaceInvader(SpaceInvader* cc) {
        renderObjects.push_back(cc);
        objectInvaders.push_back(cc);
        invaders.push_back(cc);
    }

    void addBullet(Bullet* b) {
        renderObjects.push_back(b);
        bullets.push_back(b);
    }

    // Get Avatar Coord  X
    int getAvatarCoordX() {
        return avatar->getCoordX();
    }

    // Get Avatar Coord Y
    int getAvatarCoordY() {
        return avatar->getCoordY();
    }

    // Add The Avatar
    void addAvatar(Character* avatar) {
        this->avatar = avatar;
        this->avatarTranslate == STILL;
    }

    // Add Game Time Object
    void addGameTime(GameTime* gameTime) {
        this->gameTime = gameTime;
    }

    // Set Given Tic To World Tic 
    void addTic(int* tic) {
        this->worldTic = tic;
    }

    // Returns Reference To Avatar
    Character* getAvatar() {
        return avatar;
    }

    // Returns Reference To GameTime Object
    GameTime* getGameTime() {
        return gameTime;
    }

    // Rrturns the Tic
    int* getTic() {
        return worldTic;
    }

    // Returns The World Object Array
    std::vector<GameObject*> getObjects()
    {
        return renderObjects;
    }

    void restartInvaders() {
        for (std::vector<SpaceInvader*>::iterator it3 = invaders.begin(); it3 != invaders.end(); ++it3) {
            (*it3)->restart();
        }
    }

    void updateBullets() {

        std::vector<int> erase;
        int count = 0;
        for (std::vector<Bullet*>::iterator it = bullets.begin(); it != bullets.end(); ++it) {
            (*it)->setCoordinates((*it)->getCoordX(), (*it)->getCoordY() - 5);
            int hit = collisionDetector->collide(objectInvaders, *it, UP)[0];
            int invader = collisionDetector->collide(objectInvaders, *it, UP)[2];
            // std::cout << "hit " + std::to_string(hit) << std::endl;
            // std::cout << "invader " + std::to_string(invader) << std::endl;
            if (hit == 1) {
                // erase.push_back(count);
                // std::cout << "invader added " << std::endl;
                erase.push_back(invader);
            }
            count++;
        }

        for (std::vector<int>::iterator it2 = erase.begin(); it2 != erase.end(); ++it2)
        {
            // std::cout << "erase loop 1 " << std::endl;
            int count2 = 0;
            for (std::vector<SpaceInvader*>::iterator it3 = invaders.begin(); it3 != invaders.end(); ++it3) {
                if ((*it2) == (*it3)->getId()) {
                    (*it3)->die();
                    // std::cout << "dead " << std::endl;
                }
                count2++;
            }
        }

        
    }

    // Returns The Cumulus Cloud Array
    std::vector<SpaceInvader*> getInvaders()
    {
        return invaders;
    }

    // Renders World Objects In The World
    void render(sf::RenderWindow& window, int transform) {
        for (std::vector<GameObject*>::iterator it = renderObjects.begin(); it != renderObjects.end(); ++it) {
            (*it)->render(window, 0);
        }
        avatar->render(window, 0);
    }

private:
    int avatarJump;
    int avatarTranslate;
    // std::vector<GameObject*> collideObjects;
    std::vector<GameObject*> renderObjects;
    std::vector<GameObject*> objectInvaders;
    std::vector<SpaceInvader*> invaders;
    std::vector<Bullet*> bullets;
    std::vector<int> deadInvaders;
    Character* avatar;
    GameTime* gameTime;
    int* worldTic;
};

/**
 * * * * * * * * * * * * *
 *                       *
 *  ACTUAL OBJECTS END   *
 *                       *
 * * * * * * * * * * * * *
*/

/**
 * * * * * * * * * * * * *
 *                       *
 *  EVENTS START         *
 *                       *
 * * * * * * * * * * * * *
*/

/** The Types Of Events */
enum eventType
{
    CHARACTER_DEATH = 4,// When the character dies 
    USER_INPUT = 8, // When the user clicks something (&  its not related to recording)
    CLIENT_CONNECT = 11, // When the user attempts to end a recording
    RESTART = 12
};

enum keyboardInput { ARROW, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Shift, Space };

/** Variant Struct */
struct variant
{
    enum Type
    {
        TYPE_TNTEGER = 0,
        TYPE_FLOAT = 1,
        TYPE_GAMEOBJECT_PTR = 2,

    };
    Type mType;
    union
    {
        int m_asInteger;
        float m_asFloat;
        GameObject* m_asGameObjectPtr;
    };
};

/**
 * Event Class
 *
 * Referred to:
 * https://thispointer.com/how-to-iterate-over-an-unordered_map-in-c11/
 * -- for iterating over an unorderd map & this link:
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 * -- to learn more about unordered maps.
 */
class Event
{
public:

    Event(eventType type, unordered_map<std::string, variant> argum, int tic, int priority) {
        this->type = type;
        this->arguments = argum;
        this->executionTime = tic;
        this->priority = tic * 20 + priority;
    }

    Event(std::string eventString) {

        std::string delimiter = ",";
        size_t pos = eventString.find(delimiter);
        int event_type = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->type = (eventType)event_type;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int event_time = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->executionTime = event_time;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int event_priority = std::stoi(eventString.substr(0, eventString.find(delimiter)));
        this->priority = event_priority;

        eventString.erase(0, pos + delimiter.length());
        pos = eventString.find(delimiter);
        int arguments_size = std::stoi(eventString.substr(0, eventString.find(delimiter)));

        std::vector<variant> variants;
        for (int i = 0; i < arguments_size; i++) {

            variants.push_back(struct variant());

            eventString.erase(0, pos + delimiter.length());
            pos = eventString.find(delimiter);
            std::string argument_name = eventString.substr(0, eventString.find(delimiter));

            eventString.erase(0, pos + delimiter.length());
            pos = eventString.find(delimiter);
            int argument_value_type = std::stoi(eventString.substr(0, eventString.find(delimiter)));

            if (argument_value_type == 1) {

                variants[i].mType = variant::TYPE_TNTEGER;

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                int argument_value = std::stoi(eventString.substr(0, eventString.find(delimiter)));

                variants[i].m_asInteger = argument_value;
                arguments[argument_name] = variants[i];

            }
            else if (argument_value_type == 2) {

                variants[i].mType = variant::TYPE_FLOAT;

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                float argument_value = std::stof(eventString.substr(0, eventString.find(delimiter)));

                variants[i].m_asFloat = argument_value;
                arguments[argument_name] = variants[i];

            }
            else {

                eventString.erase(0, pos + delimiter.length());
                pos = eventString.find(delimiter);
                std::string argument_value = eventString.substr(0, eventString.find(delimiter));

                this->obj_values.push_back(argument_value);
                this->obj_keys.push_back(argument_name);

            }

        }

    }

    std::vector < std::string > getObjectKeys() {
        return this->obj_keys;
    }

    std::vector < std::string > getObjectValues() {
        return this->obj_values;
    }

    eventType getType() {
        return this->type;
    }

    int getTime() {
        return this->executionTime;
    }

    unordered_map<std::string, variant> getArguments() {
        return arguments;
    }

    std::string getString() {
        std::string ret = std::to_string(this->type) + "," + std::to_string(executionTime)
            + "," + std::to_string(priority) + "," + std::to_string(arguments.size());
        for (std::pair<std::string, variant> element : arguments)
        {
            std::string varCategory = element.first;
            int varType = element.second.mType;
            ret += "," + varCategory + ",";
            if (varType == element.second.TYPE_TNTEGER) {
                ret += "1," + std::to_string(element.second.m_asInteger);
            }
            else if (varType == element.second.TYPE_FLOAT) {
                ret += "2," + std::to_string(element.second.m_asFloat);
            }
            else if (varType == element.second.TYPE_GAMEOBJECT_PTR) {
                ret += "3," + element.second.m_asGameObjectPtr->toString();
            }
        }
        return ret;
    }

    int priority;

private:
    std::vector<std::string> obj_keys;
    std::vector<std::string> obj_values;
    eventType type; // type of event
    unordered_map<std::string, variant> arguments; // arguments to complete event 
    int executionTime; // point in time event is executed
};

/**
 * Event Handler Abstract Class
 */
class EventHandler
{
public:
    void virtual onEvent(Event e, World* w) = 0;
    int virtual getId() = 0;
};

/**
 * -- Character Event Handler Class --
 *
 * Handles Events In A Way That Is More Applicable
 * To The CHARACTER in the Wolrd Object
 */
class CharacterEventHandler : public EventHandler
{
public:
    CharacterEventHandler(int id) {
        this->id = id;
    }

    int getId() {
        return this->id;
    }

    void onEvent(Event e, World* world) {

        // Character Death
        if (e.getType() == CHARACTER_DEATH) {
            (*world).getAvatar()->die();
        }

        // User Input
        else if (e.getType() == USER_INPUT) {

            int valid = e.getArguments()["valid"].m_asInteger;
            int key = e.getArguments()["key"].m_asInteger;

            if (valid == 1) {

                if (key == ARROW) {

                    int type = e.getArguments()["type"].m_asInteger;

                    if (type == RIGHT) {
                        (*world).getAvatar()->changeAction(RIGHT);
                        (*world).getAvatar()->moveRight();
                    }

                    if (type == LEFT) {
                        (*world).getAvatar()->changeAction(LEFT);
                        (*world).getAvatar()->moveLeft();
                    }

                }
            }
        }
    }
private:
    int id;
};

// ------------------- Globals (1) ------------------------ //
World world;
std::stack<Bullet> bullets;

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS START 1   *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

void restart() {
    world.restartInvaders();
}

void printMessage(string message) {
    std::cout << message << std::endl;
}

int getCharacterX() {
    return world.getAvatar()->getCoordX();
}

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS END 1     *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

/**
 * -- World Event Handler Class --
 *
 * Handles Events In A Way That Is More
 * Applicable To The World Object
 */
class WorldEventHandler : public EventHandler
{
public:
    WorldEventHandler(int id) {
        this->id;
    }

    int getId() {
        return this->id;
    }

    void onEvent(Event e, World* world) {

         // Non-User Input Events Are Handled Via Script
         ScriptManager scripter = ScriptManager();
         dukglue_register_function(scripter.getContext(), &printMessage, "printMessage");
         dukglue_register_function(scripter.getContext(), &restart, "restart");
         std::string paramer =  e.getString();
         const char* c = paramer.c_str();
         scripter.loadScript("scriptFile.js");
         scripter.runScript("handleWorldEvent", 0, 1, c);

    }
private:
    int id;
};

class CompareEvents {
public:
    bool operator() (const Event& e1, const Event& e2)
    {
        return e1.priority < e2.priority;
    }
};

/**
 * Event Manager
 */
class EventManager
{
public:
    EventManager() {

        registration[CHARACTER_DEATH] = std::vector<EventHandler*>(); // 4
        registration[USER_INPUT] = std::vector<EventHandler*>(); // 8
        registration[CLIENT_CONNECT] = std::vector<EventHandler*>(); // 11
        registration[RESTART] = std::vector<EventHandler*>(); // 12

    }

    void registerHandler(eventType type, EventHandler* handler) {
        registration[type].push_back(handler);
    }

    /**
     * Referred to these links for iteration tips.
     * https://riptutorial.com/cplusplus/example/1678/iterating-over-std--vector
     * http://www.cplusplus.com/reference/vector/vector/erase/
     */
    void unregisterHandler(EventHandler* handler, eventType type) {

        int remove_index = -1;
        for (std::size_t i = 0; i < registration[type].size(); ++i) {
            int id = registration[type].at(i)->getId();
            if (id == (*handler).getId()) {
                remove_index = 0;
            }
        }
        if (remove_index != -1) {
            registration[type].erase(registration[type].begin() + remove_index - 1);
        }
    }

    void raiseEvent(Event e) {
        //qEvents.push(e);
        pEvents.push(e);
    }

    void handleEvents(World* world) {
        while (!pEvents.empty())
        {
            Event currentEvent = pEvents.top();
            eventType type = currentEvent.getType();
            for (std::size_t i = 0; i < registration[type].size(); ++i) {
                registration[type].at(i)->onEvent(currentEvent, world);
            }
            pEvents.pop();
        }

    }

    void handleEvents(World* world, int TimePriority) {
        bool done = false;
        while (!pEvents.empty() && done == false)
        {
            Event currentEvent = pEvents.top();
            eventType type = currentEvent.getType();
            int time = currentEvent.getTime();
            if (time <= TimePriority) {
                for (std::size_t i = 0; i < registration[type].size(); ++i) {
                    registration[type].at(i)->onEvent(currentEvent, world);
                }
                pEvents.pop();
            }
            else {
                done == true;
            }
        }
    }

private:
    unordered_map<eventType, std::vector<EventHandler*>> registration;
    priority_queue<Event, std::vector<Event>, CompareEvents> pEvents;
};

/**
 * * * * * * * * * * * * *
 *                       *
 *  EVENTS END           *
 *                       *
 * * * * * * * * * * * * *
*/

// ------------------- Globals (2) ------------------------ //
EventManager eventManager;

/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS START 2   *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/

void createRaiseEvent(int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values) {
    // create variants
    std::vector<struct variant> vars;
    for (int i = 0; i < args; i++) {

        vars.push_back(struct variant());        
        vars[i].mType = variant::TYPE_TNTEGER;
        vars[i].m_asInteger = values[i];
    }

    // create unordered map
    std::unordered_map<std::string, variant> arg_map;
    for (int i = 0; i < args; i++) {
        arg_map[keys[i]] = vars[i];
    }

    Event ret((eventType)type, arg_map, *world.getTic(), priority);
    eventManager.raiseEvent(ret);
}


/**
 * * * * * * * * * * * * * * * * * * * * * *
 *                                         *
 *  SCRIPTING SPECIFIC FUNCTIONS END 2     *
 *                                         *
 * * * * * * * * * * * * * * * * * * * * * *
*/


int main()
{
    // Window Parameters
    int x_max = 800;
    int y_max = 600;

    // Connect To Socket
    zmq::context_t context(1);
    zmq::socket_t requester(context, ZMQ_REQ);
    requester.connect("tcp://localhost:5555");

    //  -- --  Request Id -- -- \\

    std::unordered_map<std::string, variant> new_client_args;
    Event ccEvent(CLIENT_CONNECT, new_client_args, 0, 0);

    // Request Id 
    std::string messageA = "e," + ccEvent.getString();
    zmq::message_t messageT1(messageA.size());
    memcpy(messageT1.data(), messageA.data(), messageA.size());
    requester.send(messageT1, 0);

    // Recieve Id & World Start-Up Data
    zmq::message_t messageT2;
    requester.recv(&messageT2, 0);
    std::string stringA = std::string(static_cast<char*>(messageT2.data()), messageT2.size());

    // Create World
    world = World(0);

    // Determine Deliminator 
    std::string delimiter = ",";

    // Count Variables 
    std::string stringId;
    int id = -1;
    size_t pos;
    std::string stringInvaderCount;
    int intInvaderCount;

    // Set Id
    id = std::stoi(stringA);

    //  -- -- Request Invader Data -- -- \\

    std::string messageC = "New_Invaders";
    zmq::message_t messageT1C(messageC.size());
    memcpy(messageT1C.data(), messageC.data(), messageC.size());
    requester.send(messageT1C, 0);

    // Recieve Id & World Start-Up Data
    zmq::message_t messageT2C;
    requester.recv(&messageT2C, 0);
    std::string stringC = std::string(static_cast<char*>(messageT2C.data()), messageT2C.size());

    // -- Create & Add Invaders 

    // Find Invader Count
    pos = stringC.find(delimiter);
    stringInvaderCount = stringC.substr(0, stringC.find(delimiter));
    intInvaderCount = std::stoi(stringInvaderCount);

    std::vector<SpaceInvader> spaceInvaders;

    // Add Invader To World
    for (int i = 0; i < intInvaderCount; i++) {

        // Space Invader Variables
        std::string stringInvaderId;
        int intInvaderId;
        std::string stringInvaderX;
        int intInvaderX;
        std::string stringInvaderY;
        int intInvaderY;
        std::string stringInvaderDistance;
        int intInvaderDistance;

        // Finding Invader Id
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringInvaderId = stringC.substr(0, stringC.find(delimiter));
        intInvaderId = std::stoi(stringInvaderId);

        // Finding Invader X
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringInvaderX = stringC.substr(0, stringC.find(delimiter));
        intInvaderX = std::stoi(stringInvaderX);

        // Finding Invader Y
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringInvaderY = stringC.substr(0, stringC.find(delimiter));
        intInvaderY = std::stoi(stringInvaderY);

        // Finding Distance
        stringC.erase(0, pos + delimiter.length());
        pos = stringC.find(delimiter);
        stringInvaderDistance = stringC.substr(0, stringC.find(delimiter));
        intInvaderDistance = std::stoi(stringInvaderDistance);

        spaceInvaders.push_back(SpaceInvader(intInvaderId, intInvaderX, intInvaderY, intInvaderDistance));
    }

    for (std::vector<SpaceInvader>::iterator it = spaceInvaders.begin(); it != spaceInvaders.end(); ++it) {
        world.addSpaceInvader(&(*it));
    }

    // -- Create Main Avatar
    int idCounter = 100;
    Character mainChar(idCounter);
    idCounter++;
    world.addAvatar(&mainChar);

    // -- Set Up Time Variables 
    RealTime realTime(1.0);
    GameTime gameTime(1.0, &realTime);
    int ticCount = 0;
    world.addGameTime(&gameTime);
    world.addTic(&ticCount);

    /// ----- Set Up Event Handlers & Manager ----- \\\
    
    CharacterEventHandler charEH(idCounter);
    idCounter++;
    WorldEventHandler worldEH(idCounter);
    idCounter++;


    // Register Collision Handlers
    eventManager.registerHandler(USER_INPUT, &charEH);
    eventManager.registerHandler(RESTART, &worldEH);
    
    /// ----- Variables For Communicating W/ The Server ----- \\\
    
    //Avatar Coord For Server 
    int x = world.getAvatarCoordX();
    int y = world.getAvatarCoordY();

    int intClients = 1;

    //  Server Message 
    std::string stringD = "";

    // Other Char Array
    std::vector<int> otherCharacters;

    int speedPressed = false;

    ScriptManager scripter = ScriptManager();
    dukglue_register_function(scripter.getContext(), &printMessage, "printMessage");
    dukglue_register_function(scripter.getContext(), &getCharacterX, "getCharacterX");
    dukglue_register_function(scripter.getContext(), &createRaiseEvent, "createRaiseEvent");
    dukglue_register_function(scripter.getContext(), &restart, "restart");

    std::cout << "-------------- Button Presses --------------" << std::endl;
    std::cout << "LEFT ARROW - MOVE LEFT" << std::endl;
    std::cout << "RIGHT ARROW - MOVE RIGHT" << std::endl;
    std::cout << "UP ARROW - SHOOT" << std::endl;
    std::cout << "SPACE - RESTART" << std::endl;
    std::cout << "--------------------------------------------" << std::endl;

    // Set-Up Window
    sf::RenderWindow window(sf::VideoMode(x_max, y_max), "Homework5 (SPACE INVADERS) - Client", sf::Style::Close | sf::Style::Resize);
    window.setKeyRepeatEnabled(false);

    while (window.isOpen()) {

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);

        /** //                                           \\ **\
         *                                                   *
         * ---------------- MAIN LOOP  --------------------- *
         *                                                   *
         *///                                             \\\*

         // Communicate w/ Server 

         // Send Id & Character Data
        std::string messageD = std::to_string(id) + "," + std::to_string(x) + "," + std::to_string(y);
        //std::cout << "Sending -- " << messageD << std::endl;
        zmq::message_t messageT1D(messageD.size());
        memcpy(messageT1D.data(), messageD.data(), messageD.size());
        requester.send(messageT1D, 0);

        // Recieve Update
        zmq::message_t messageT2D;
        requester.recv(&messageT2D, 0);
        stringD = std::string(static_cast<char*>(messageT2D.data()), messageT2D.size());
        //std::cout << "Received: " << stringD << std::endl;

        // Clients 
        pos = stringD.find(delimiter);
        std::string stringClients = stringD.substr(0, stringD.find(delimiter));
        intClients = std::stoi(stringClients);

        // Multiples Clients
        if (intClients > 1) {

            otherCharacters.clear();

            // Update  Other Characters
            for (int i = 1; i < intClients; i++) {

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                std::string clientX = stringD.substr(0, stringD.find(delimiter));
                int intClientX = std::stoi(clientX);

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                std::string clientY = stringD.substr(0, stringD.find(delimiter));
                int intClientY = std::stoi(clientY);

                otherCharacters.push_back(intClientX);
                otherCharacters.push_back(intClientY);

            }

        }

        // Render To Window
        window.clear(sf::Color::Black);

        /** //                                           \\ **\
         *                                                   *
         * ---------------- COUNT TIME  ---------------------*
         *                                                   *
         *///                                             \\\*

        if (*world.getTic() < world.getGameTime()->getTime()) {
            // std::cout << "MAIN LOOP" << std::endl;

            bool validJump = false;

            // Update Space Invaders Position
            std::vector<SpaceInvader*> invadersVec = world.getInvaders();
            for (std::vector<SpaceInvader*>::iterator it = invadersVec.begin(); it != invadersVec.end(); ++it) {

                std::string siStringX;
                std::string siStringY;
                int siX;
                int siY;

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                siStringX = stringD.substr(0, stringD.find(delimiter));
                siX = std::stoi(siStringX);

                stringD.erase(0, pos + delimiter.length());
                pos = stringD.find(delimiter);
                siStringY = stringD.substr(0, stringD.find(delimiter));
                siY = std::stoi(siStringY);

                (*it)->setCoordinates(siX, siY);
            }

            // Update X & Y w/ Avatar Coord
            x = world.getAvatarCoordX();
            y = world.getAvatarCoordY();

            // Respond To Keyboard Input
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && window.hasFocus()) {

                // reactToKeyboard
                // scripter.loadScript("scriptFile.js");
                // scripter.runScript("reactToKeyboard", 1, 1, 2);
                // std::cout << bullets.size() << endl;
                bullets.push(Bullet(world.getAvatar()->getCoordX(), world.getAvatar()->getCoordY()));
                // std::cout << bullets.size() << endl;
                world.addBullet(&bullets.top());

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && window.hasFocus()) {

                // reactToKeyboard
                scripter.loadScript("scriptFile.js");
                scripter.runScript("reactToKeyboard", 1, 1, 0);

            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && window.hasFocus()) {

                // reactToKeyboard
                scripter.loadScript("scriptFile.js");
                scripter.runScript("reactToKeyboard", 1, 1, 1);
                
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus()) {

                std::string messageE = "Restart";
                zmq::message_t messageT1E(messageE.size());
                memcpy(messageT1E.data(), messageE.data(), messageE.size());
                requester.send(messageT1E, 0);

                // Recieve Id & World Start-Up Data
                zmq::message_t messageT2E;
                requester.recv(&messageT2E, 0);
                std::string stringE = std::string(static_cast<char*>(messageT2E.data()), messageT2E.size());

                // world.restartInvaders();
                // restart();

                createRaiseEvent(RESTART, 0, 0, std::vector<string>(), std::vector<int>());

            }

            /// -------- CHECK FOR AUTOMATIC EVENTS ------- \\\
            
            world.updateBullets();
            

            // ---------------------- HANDLE EVENTS ----------------------- \\

            eventManager.handleEvents(&world, *world.getTic());


            // ---------------------- UPDATE TIME ----------------------- \\

            *world.getTic() = world.getGameTime()->getTime();
        }

        // Draw Other Characters
        if (intClients > 1) {

            for (int i = 0; i < otherCharacters.size(); i += 2) {
                int intClientX = otherCharacters[i];
                int intClientY = otherCharacters[i + 1];
                mainChar.renderOtherCharacter(intClientX, intClientY, window, 0);
            }
        }

        // Draw World Objects
        world.render(window, 0);
        window.display();
    }

    /// --------------------- END MAIN LOOP ------------------------------- \\\

    return 0;
}