// std includes
#include <iostream>
#include <thread>
#include <string>
#include <vector>

// lib includes
#include "SDL2/SDL.h"

// mirage includes
#include "config.h"
#include "macros.h"
#include "core/input.h"
#include "core/display.h"
#include "core/ray.h"
#include "core/primitive.h"
#include "core/accelerator.h"
#include "core/shape.h"
#include "shapes/sphere.h"
#include "cameras/orthographic.h"
#include "cameras/perspective.h"
#include "accelerators/kdtree.h"

using namespace mirage;

void dispose()
{
    SDL_Quit();

    LOG("atexit(dispose) Hook called.");
}

int main(int argc, char **argv)
{
    // Print program name and version
    LOG("MirageRender, version " << VERSION_R << "." << VERSION_B << "." << VERSION_A);

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        ERR("SDL_Init Error: " << SDL_GetError());
        return 1;
    }
    SDL_Event event;

    // Set-up atexit hooks
    atexit(dispose);

    // Initialize multithreading related stuff
#if THREADS>0
    const unsigned int tcount = THREADS;
#else
    const unsigned int tcount = SDL_GetCPUCount();
#endif
    std::thread *threads = new std::thread[tcount];

    // Initialize miragerender related variables/objects
    Display display("MirageRender", WIDTH, HEIGHT, SCALE);

    uint32_t startTime = SDL_GetTicks();
    uint32_t currentTime = SDL_GetTicks();
    uint32_t lastTime = 0;
    uint32_t frameCount = 0;
    float deltaTime = 0;
    float fps = 0;
    bool running = true;

    // Testing...
    Transform objToWorld, worldToObj;
    objToWorld.setPosition(vec3(0.0f, 0.0f, 0.0f));
    worldToObj = objToWorld.inverse();
    Sphere test(objToWorld, worldToObj, vec3(2.0f, 0.0f, 0.0f), 1.0f);
    Sphere test2(objToWorld, worldToObj, vec3(-5.0f, 2.0f, 1.0f), 1.0f);
    //std::cout << test.objectBound().toString() << std::endl;
    //std::cout << test.worldBound().toString() << std::endl;
    //std::cout << objToWorld.getMatrix().toString() << std::endl;
    //std::cout << worldToObj.getMatrix().toString() << std::endl;

    Film film(WIDTH, HEIGHT);
    CameraOrtho camera(Transform(vec3(0.0f, 0.0f, -10.0f)), film, 0.2f);
    CameraPersp camera2(Transform(vec3(0.0f, 0.0f, -10.0f)), film, 70.0f);

    //std::cout << camera.getTransform().getOrientation().getForwardVector().toString() << std::endl;

    std::vector<Shape *> shapes;
    int numShapes = 512;
    for (size_t i = 0; i < numShapes; i++)
    {
        shapes.push_back(new Sphere(objToWorld, worldToObj, vec3(g_rng() * 10 - 5, g_rng() * 10 - 5, g_rng() * 10 - 5), 0.1f + g_rng() * 0.5f));
    }

    KDTreeAccel testaccelstruct(objToWorld, worldToObj, shapes, 1, 1, 128, 1);
    testaccelstruct.init();
    //std::cout << testaccelstruct.objectBound().toString() << std::endl;
    //std::cout << testaccelstruct.worldBound().toString() << std::endl;

    while (running)
    {
        // Calculate framerate related variables
        currentTime = SDL_GetTicks();
        deltaTime = static_cast<float>(currentTime - lastTime) / 1000.0f;
        fps = frameCount / (static_cast<float>(SDL_GetTicks() - startTime) / 1000.0f);
        lastTime = currentTime;

        // Update and render scene
        display.clear(0x00000000);

        // Test rendering of a k-d tree of shapes
        Ray r_primary;
        for (size_t j = 0; j < camera.getFilm().getResolutionY(); j++)
        {
            for (size_t i = 0; i < camera.getFilm().getResolutionX(); i++)
            {
                Intersection iSect;
                camera.calcCamRay(i, j, r_primary);
                if (testaccelstruct.intersect(r_primary, iSect))
                {
                    camera.getFilm().setSample(i, j, vec3(0.75f, 0.25f, 1) * std::max(vec3::dot(iSect.getNormal(), vec3(1, 1, -1).normalize()), 0.1f));
                    display.setPixel(i, j, camera.getFilm().getSample(i, j).getColor());
                }
            }
        }

        display.render();

        // Display info
        if (frameCount % 16 == 1)
        {
            std::string fps_str = std::to_string(fps);
            std::string dt_str = std::to_string(deltaTime);
            std::string title = "MirageRender | FPS: " + fps_str + " DT: " + dt_str;
            display.setTitle(title);
        }

        // Process input
        if (g_keys[SDL_SCANCODE_W])
        {
            camera.move(camera.getTransform().getOrientation().getForwardVector(), deltaTime);
        }
        else if (g_keys[SDL_SCANCODE_S])
        {
            camera.move(camera.getTransform().getOrientation().getForwardVector(), -deltaTime);
        }
        if (g_keys[SDL_SCANCODE_A])
        {
            camera.move(camera.getTransform().getOrientation().getRightVector(), -deltaTime);
        }
        else if (g_keys[SDL_SCANCODE_D])
        {
            camera.move(camera.getTransform().getOrientation().getRightVector(), deltaTime);
        }

        if (g_keys[SDL_SCANCODE_UP])
        {
            camera.rotate(camera.getTransform().getOrientation().getRightVector(), -deltaTime * 32);
        }
        if (g_keys[SDL_SCANCODE_DOWN])
        {
            camera.rotate(camera.getTransform().getOrientation().getRightVector(), deltaTime * 32);
        }
        if (g_keys[SDL_SCANCODE_LEFT])
        {
            camera.rotate(camera.getTransform().getOrientation().getUpVector(), -deltaTime * 32);
        }
        if (g_keys[SDL_SCANCODE_RIGHT])
        {
            camera.rotate(camera.getTransform().getOrientation().getUpVector(), deltaTime * 32);
        }

        // Process SDL events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
            {
                auto id = event.key.keysym.scancode;
                if (id > 0 && id < sizeof(g_keys) / sizeof(*g_keys))
                {
                    g_keys[id] = true;
                }
            }
            break;
            case SDL_KEYUP:
            {
                auto id = event.key.keysym.scancode;
                if (id > 0 && id < sizeof(g_keys) / sizeof(*g_keys))
                {
                    g_keys[id] = false;
                }
            }
            break;
            }
        }

        frameCount++;
    }

    delete[] threads;
    LOG("MirageRender, exit program successfully.");

    return 0;
}
