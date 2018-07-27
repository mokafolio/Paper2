// This example shows the very basic steps to create a Paper Document
// and draw it.

// we use GLFW to open a simple window
#include <GLFW/glfw3.h>

//include some paper headers.
#include <Paper2/Document.hpp>
#include <Paper2/Paint.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Symbol.hpp>
#include <Paper2/Tarp/TarpRenderer.hpp>

#include <Crunch/Randomizer.hpp>

#include <memory>

//we want to use the paper, brick, crunch & stick namespaces
using namespace paper; // paper namespace
using namespace crunch; // crunch namespace for math
using namespace stick; // stick namespace for core data structures/containers etc.

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

    //create the window
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

        Path * path = doc.createCircle(Vec2f(100, 100), 50);
        // path->translateTransform(100, 50);

        auto grad = createLinearGradient(Vec2f(50, 50), Vec2f(150, 150));
        grad->addStop(ColorRGBA(0.5, 1.0, 0.0, 1.0), 0.0);
        grad->addStop(ColorRGBA(0.5, 0.0, 1.0, 1.0), 1.0);
        // grad->setFocalPointOffset(Vec2f(20, -10));
        // grad->setRatio(0.5);
        path->setFill(grad);
        path->setfillPaintTransform(Mat32f::rotation(1.2));


        Path * clone = path->clone();
        clone->translateTransform(100, 50);
        clone->setfillPaintTransform(Mat32f::rotation(0.75));
        clone->setStroke("red");
        clone->setStrokeWidth(3);
        clone->setDashArray({10, 5});
        clone->setStrokeCap(StrokeCap::Round);
        clone->setStrokeJoin(StrokeJoin::Round);

        Path * path2 = doc.createCircle(Vec2f(200, 100), 30);
        path2->setFill(grad);


        Group * grp = doc.createGroup();
        grp->setClipped(true);
        grp->addChild(doc.createCircle(Vec2f(100, 100), 100));
        grp->addChild(path);
        grp->addChild(clone);
        grp->addChild(path2);

        Group * grp2 = doc.createGroup();
        grp2->setClipped(true);
        Path * mask = doc.createRectangle(Vec2f(50, 90), Vec2f(300, 170));
        grp2->addChild(mask);
        grp2->addChild(grp);


        crunch::Randomizer rnd;
        for (int i = 0; i < 20; i++)
        {
            Path * p = doc.createPath();
            Vec2f a = Vec2f(rnd.randomf(50, 300), rnd.randomf(90, 170));
            Vec2f b = a + Vec2f(rnd.randomf(-10, 10), rnd.randomf(-10, 10));
            p->addPoint(a);
            p->addPoint(b);
            p->setStroke("black");
            grp2->addChild(p);
        }


        auto grad2 = createRadialGradient(Vec2f(200, 200), Vec2f(300, 200));
        grad2->addStop(ColorRGBA(0.5, 0.3, 1.0, 1.0), 0.0);
        grad2->addStop(ColorRGBA(0.5, 1.0, 0.3, 1.0), 0.35);
        grad2->addStop(ColorRGBA(0.25, 0.0, 0.6, 1.0), 1.0);
        grad2->setFocalPointOffset(Vec2f(30, -60));
        // grad2->setRatio(1.0);

        Path * anotherCircle = doc.createCircle(Vec2f(200, 200), 100);
        anotherCircle->setFill(grad2);


        Path * rpath = doc.createPath();
        for(int i = 0; i < 10; ++i)
        {
            rpath->addPoint(Vec2f(rnd.randomf(50, 300), rnd.randomf(90, 170)));
        }
        rpath->closePath();
        rpath->smooth();
        rpath->setFill("purple");

        Path * ellipse = doc.createEllipse(Vec2f(100, 100), Vec2f(100, 20));
        ellipse->setFill("blue");

        // Path * cmask = mask->clone();
        // cmask->insertAbove(grp2);
        // cmask->setFill("pink");

        auto res = doc.exportSVG();
        if (!res)
        {
            printf("Error %s\n", err.message().cString());
            return EXIT_FAILURE;
        }

        printf("SVG: %s\n", res.get().cString());

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
