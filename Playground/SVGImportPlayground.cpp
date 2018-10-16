// This example shows the very basic steps to create a Paper Document
// and draw it.

// we use GLFW to open a simple window
#include <GLFW/glfw3.h>

// include some paper headers.
#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Paint.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Symbol.hpp>
#include <Paper2/Tarp/TarpRenderer.hpp>

#include <Crunch/Randomizer.hpp>

#include <memory>

// we want to use the paper, brick, crunch & stick namespaces
using namespace paper;  // paper namespace
using namespace crunch; // crunch namespace for math
using namespace stick;  // stick namespace for core data structures/containers etc.

int main(int _argc, const char * _args[])
{
    // initialize glfw
    if (!glfwInit())
        return EXIT_FAILURE;

    // and set some hints to get the correct opengl versions/profiles
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create the window
    GLFWwindow * window = glfwCreateWindow(800, 600, "Hello Paper Example", NULL, NULL);
    if (window)
    {
        glfwMakeContextCurrent(window);

        // create the document.
        Document doc;
        doc.setSize(800, 600);
        doc.translateTransform(400, 0);

        // create the opengl renderer for the document
        tarp::TarpRenderer renderer;
        auto err = renderer.init(doc);
        if (err)
        {
            printf("Error %s\n", err.message().cString());
            return EXIT_FAILURE;
        }

        auto res = doc.loadSVG("../../Playground/Assets/simple.svg");
        if(res.error())
        {
            printf("ERROR: %s\n", res.error().message().cString());
            return EXIT_FAILURE;
        }

        printf("RES C C %lu\n", res.group()->children().count());

        Path * c2 = doc.createCircle(Vec2f(200, 200), 50);
        c2->setFill("blue");

        // the main loop
        while (!glfwWindowShouldClose(window))
        {
            // clear the background to black
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            // get the window size from the glfw window
            // and set it as the viewport on the renderer
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            renderer.setViewport(0, 0, width, height);

            glfwGetWindowSize(window, &width, &height);
            renderer.setProjection(Mat4f::ortho(0, width, height, 0, -1, 1));
            // renderer.setTransform(Mat3f::identity());

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            auto err = renderer.draw();
            if (err)
            {
                printf("ERROR: %s\n", err.message().cString());
                return EXIT_FAILURE;
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    else
    {
        glfwTerminate();
        printf("Could not open GLFW window :(\n");
        return EXIT_FAILURE;
    }

    // clean up glfw
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
