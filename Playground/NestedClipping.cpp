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

#include <Stick/SystemClock.hpp>

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
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create the window
    GLFWwindow * window = glfwCreateWindow(800, 600, "Nested Clipping", NULL, NULL);
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

        // Group * grp = doc.createGroup("Outer");
        // grp->addChild(doc.createRectangle(Vec2f(100, 100), Vec2f(200, 200)));
        // grp->setClipped(true);

        // Path * bg = doc.createRectangle(Vec2f(0, 0), Vec2f(300, 200));
        // bg->setFill("yellow");
        // bg->setStroke("red");
        // bg->setStrokeWidth(10);
        // grp->addChild(bg);

        // Group * inner = doc.createGroup("Inner");
        // inner->addChild(doc.createRectangle(Vec2f(150, 150), Vec2f(30, 30)));
        // inner->setClipped(true);
        // Path * c = doc.createCircle(Vec2f(160, 160), 20);
        // c->setFill("red");
        // inner->addChild(c);
        // grp->addChild(inner);

        Group * inner = doc.createGroup("Inner");
        inner->translateTransform(100, 0);
        Path * bg = doc.createRectangle(Vec2f(0, 0), Vec2f(300, 100), "BG");
        bg->setFill("yellow");
        Path * cp = doc.createRectangle(Vec2f(30, 10), Vec2f(270, 90), "CM");
        cp->setFill("red");

        Path * c = doc.createCircle(Vec2f(100, 0), 40, "DA CIRC");
        c->setFill("green");
        inner->addChild(cp);
        inner->addChild(bg);
        inner->addChild(c);
        inner->setClipped(true);

        // Group * inner2 = inner->clone();
        // inner2->scaleTransform(0.5);
        // inner2->rotateTransform(0.1);
        // inner->addChild(inner2);

        Symbol * s = doc.createSymbol(inner, "SYASJH");
        s->translateTransform(Vec2f(0, 200));
        s->setFill("red");
        s->setStroke("blue");
        s->setStrokeWidth(10);

        // Symbol * s2 = doc.createSymbol(bg);
        // s2->setStroke("red");
        // s2->setStrokeWidth(5);
        // s2->translateTransform(350, 150);
        // inner->addChild(s2);
        // Path * c2 = doc.createCircle(Vec2f(140, 130), 30);
        // c2->setFill("blue");
        // grp->addChild(c2);

        // Path * t = doc.createPath();
        // t->setFill("red");
        // t->addPoint(Vec2f(100, 100));
        // // t->arcTo(Vec2f(110, 50), Vec2f(200, 100));
        // // t->arcTo(Vec2f(200, 100), true);
        // t->arcTo(Vec2f(200, 100), Vec2f(5, 10), toRadians(40.0), true, false);

        // Path * c2 = doc.createCircle(Vec2f(0, 0), 50);
        // c2->setFill("blue");

        // Symbol * s = doc.createSymbol(c2);
        // s->translateTransform(100, 100);

        // Group * grp = doc.createGroup();
        // grp->addChild(s);
        // grp->setClipped(true);
        // Path * rect = doc.createRectangle(Vec2f(100, 100), Vec2f(200, 200));
        // rect->setFill("red");
        // grp->addChild(rect);

        // Group * grp = doc.createGroup();
        // grp->setStroke("red");
        // grp->setStrokeWidth(10);
        // grp->addChild(c2);

        // printf("SW: %f\n", c2->strokeWidth());
        // printf("SC: %f %f %f\n", c2->stroke().get<ColorRGBA>().r,
        // c2->stroke().get<ColorRGBA>().g, c2->stroke().get<ColorRGBA>().b);

        // for(auto seg : c2->segments())
        // {
        //     printf("hi %f %f\n", seg.handleInAbsolute().x, seg.handleInAbsolute().y);
        //     printf("pos %f %f\n", seg.position().x, seg.position().y);
        //     printf("ho %f %f\n", seg.handleOutAbsolute().x, seg.handleOutAbsolute().y);
        // }

        // Path * r = doc.createRectangle(Vec2f(50, 50), Vec2f(300, 200));
        // r->setFill("black");
        // for(auto seg : r->segments())
        // {
        //     printf("pos2 %f %f\n", seg.position().x, seg.position().y);
        // }

        SystemClock clk;
        Path * np = nullptr;
        auto theStart = clk.now();
        // the main loop
        while (!glfwWindowShouldClose(window))
        {
            auto start = clk.now();

            // if((start - theStart).seconds() > 1 && !np)
            // {
            //     np = doc.createCircle(Vec2f(100, 40), 20, "DA CIRC2");
            //     np->setFill("pink");
            //     np->insertBelow(c);
            // }

            // clear the background to black
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            // get the window size from the glfw window
            // and set it as the viewport on the renderer
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            renderer.setViewport(0, 0, width, height);

            glfwGetWindowSize(window, &width, &height);
            renderer.setDefaultProjection();
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

            auto delta = clk.now() - start;
            printf("SECONDS %f\n", delta.seconds());
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
