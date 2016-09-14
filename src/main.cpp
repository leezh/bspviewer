#define GLM_SWIZZLE
#include <iostream>
#include <physfs.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <SFML/Window.hpp>
#include "bsp.hpp"

#define PI 3.14159265359f

inline float deg2rad(float deg)
{
    return deg * PI / 180.f;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3)
    {
        std::cout << "Usage: bspviewer [path_to_data_folder [map_to_load]]" << std::endl;
        return -1;
    }

    PHYSFS_init(argv[0]);

    if (!PHYSFS_mount(argv[1], NULL, 0))
    {
        std::cout << "Path not found" << std::endl;
        return -1;
    }

    char** files = PHYSFS_enumerateFiles("/");
    for (char** i = files; *i != NULL; i++)
    {
        std::string file(*i);
        if (file.length() > 4 && file.substr(file.length() - 4) == ".pk3")
        {
            std::string dirsep = PHYSFS_getDirSeparator();
            std::string path = PHYSFS_getRealDir(file.c_str());
            if (path.length() > 1 && path.substr(path.length() - 1) != dirsep)
                path.append(dirsep);
            path.append(file);
            PHYSFS_mount(path.c_str(), NULL, 0);
        }
    }
    PHYSFS_freeList(files);

    if (argc == 2)
    {
        char** files = PHYSFS_enumerateFiles("/maps/");
        for (char** i = files; *i != NULL; i++)
        {
            std::string file(*i);
            if (file.length() > 4 && file.substr(file.length() - 4) == ".bsp")
            {
                std::cout << "/maps/" << file << std::endl;
            }
        }
        PHYSFS_freeList(files);
        return 0;
    }

    sf::ContextSettings settings;
    settings.depthBits = 24;

    int width = 800;
    int height = 600;
    sf::Window window(sf::VideoMode(width, height), "BSPViewer", sf::Style::Default, settings);
    window.setMouseCursorVisible(false);

    glewInit();

    Map map;
    if (!map.load(argv[2]))
    {
        return -1;
    }

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClearDepth(1.f);

    sf::Clock clock;
    glm::vec3 position(0.f, 0.f, 0.f);
    float yaw = 0.f;
    float pitch = 0.f;
    bool collision = false;

    while (window.isOpen())
    {
        // Events
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::Resized:
                width = event.size.width;
                height = event.size.height;
                glViewport(0, 0, width, height);
                break;
            case sf::Event::MouseMoved:
                yaw += (event.mouseMove.x - width / 2) * 0.1f;
                pitch += (event.mouseMove.y - height / 2) * 0.1f;
                if (yaw > 180.f) yaw -= 360.f;
                if (yaw < -180.f) yaw += 360.f;
                if (pitch > 90.f) pitch = 90.f;
                if (pitch < -90.f) pitch = -90.f;
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code)
                {
                case sf::Keyboard::E:
                    collision = !collision;
                    break;
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                }

                break;
            default:
                break;
            }
        }
        sf::Mouse::setPosition(sf::Vector2i(width, height) / 2, window);

        float elapsed = clock.restart().asSeconds();

        glm::vec3 forward = glm::vec3(std::cos(deg2rad(yaw)), -std::sin(deg2rad(yaw)), 0.f);
        glm::vec3 right = glm::vec3(std::sin(deg2rad(yaw)), std::cos(deg2rad(yaw)), 0.f);
        glm::vec3 up = glm::vec3(0.f, 0.f, 1.f);

        float speed = 200.f;
        glm::vec3 oldPos = position;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            position += forward * elapsed * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            position -= forward * elapsed * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            position += right * elapsed * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            position -= right * elapsed * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            position += up * elapsed * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            position -= up * elapsed * speed;

        if (collision)
            position = map.traceWorld(position, oldPos, 10.f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::perspective(deg2rad(75.f), float(width) / float(height), 1.f, 9000.f);
        view = glm::rotate(view, deg2rad(-90.f), glm::vec3(1.f, 0.f, 0.f));
        view = glm::rotate(view, deg2rad(pitch), glm::vec3(1.f, 0.f, 0.f));
        view = glm::rotate(view, deg2rad(yaw + 90.f), glm::vec3(0.f, 0.f, 1.f));
        view = glm::translate(view, -position);

        map.renderWorld(view, position);

        window.display();
    }

    return 0;
}
