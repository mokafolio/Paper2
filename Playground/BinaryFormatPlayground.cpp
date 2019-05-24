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

        // create the opengl renderer for the document
        tarp::TarpRenderer renderer;
        auto err = renderer.init(doc);
        if (err)
        {
            printf("Error %s\n", err.message().cString());
            return EXIT_FAILURE;
        }

        LinearGradientPtr g = doc.createLinearGradient(Vec2f(100, 100), Vec2f(200, 200));
        g->addStop(ColorRGBA(1, 1, 0, 1), 0);
        g->addStop(ColorRGBA(0, 1, 1, 1), 1);

        Path * path = doc.createCircle(Vec2f(100, 100), 100);
        path->setStroke("black");
        path->setFill(g);
        path->translateTransform(200, 100);
        path->setStrokeWidth(5);
        path->setDashArray({10, 5});
        path->setStrokeCap(StrokeCap::Round);

        Path * p2 = doc.createCircle(Vec2f(100, 100), 50);
        p2->translateTransform(0, 50);
        path->addChild(p2);

        Path * p3 = doc.createRoundedRectangle(Vec2f(50, 50), Vec2f(200, 150), Vec2f(10));
        p3->setFill(g);
        Group * grp = doc.createGroup();
        grp->addChild(path);
        grp->addChild(p3);


        Path * p4 = doc.createCircle(Vec2f(0, 0), 50);
        p4->setFill("red");

        printf("PREV EXP\n");
        auto data = grp->exportBinary().ensure();

        printf("DA DATA COUNT %lu\n", data.count());

        grp->remove();

        Item * item = doc.parseBinary(&data[0], data.count()).ensure();

        int width, height;
        // the main loop
        while (!glfwWindowShouldClose(window))
        {
            // clear the background to black
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            // get the window size from the glfw window
            // and set it as the viewport on the renderer
            // int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            renderer.setViewport(0, 0, width, height);

            glfwGetWindowSize(window, &width, &height);
            renderer.setDefaultProjection();
            // renderer.setTransform(Mat3f::identity());

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
