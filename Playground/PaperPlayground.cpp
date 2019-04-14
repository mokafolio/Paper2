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

Document * d;
Path * path;
const Mat32f * trans = nullptr;

void mouse_button_callback(GLFWwindow * window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // printf("HIT TESTING\n");
        // auto res = d->hitTest(Vec2f(xpos, ypos));
        // if(res)
        //     printf("WE GOT SOMETHING\n");

        // if(trans && path->contains(Vec2f(xpos, ypos), *trans))
        //     printf("SYMBOL HIT\n");
    }
}

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

    printf("SIZES BRO %lu %lu\n", sizeof(CurveLocation), sizeof(Bezier));

    // create the window
    GLFWwindow * window = glfwCreateWindow(800, 600, "Hello Paper Example", NULL, NULL);
    if (window)
    {
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwMakeContextCurrent(window);

        // create the document.
        Document doc;
        doc.setSize(800, 600);
        d = &doc;

        printf(
            "ITEM SIZE %i %i\n", sizeof(std::shared_ptr<float>), sizeof(stick::SharedPtr<float>));

        // create the opengl renderer for the document
        tarp::TarpRenderer renderer;
        auto err = renderer.init(doc);
        if (err)
        {
            printf("Error %s\n", err.message().cString());
            return EXIT_FAILURE;
        }

        path = doc.createCircle(Vec2f(100, 100), 50);
        path->setFill("red");
        Path * rect = doc.createRectangle(Vec2f(40, 90), Vec2f(160, 110));
        rect->setStroke("yellow");

        auto pos = Vec2f(0, 10);
        auto cl = path->closestCurveLocation(pos);
        printf("CL %f %f %f %lu\n", cl.parameter(), cl.position().x, cl.position().y, cl.curve().index());
        Path * clp = doc.createCircle(cl.position(), 2);
        clp->setFill("white");

        Path * clone1 = path->clone();
        Path * clone2 = rect->clone();
        clone1->translateTransform(100, 200);
        clone2->translateTransform(150, 200);
        clone2->rotateTransform(0.3);

        auto isecs = clone2->intersections(clone1);
        for (auto & isec : isecs)
        {
            Path * c = doc.createCircle(isec.position, 3);
            c->setFill("pink");

            printf("ISEC idx %lu param %f\n",
                   isec.location.curve().index(),
                   isec.location.parameter());
            auto pos = rect->curve(isec.location.curve().index())
                           .positionAtParameter(isec.location.parameter());
            Path * c2 = doc.createCircle(pos, 2);
            c2->setFill("green");

            Path * c3 = doc.createCircle(clone2->absoluteTransform() * pos, 2);
            c3->setFill("black");
        }

        auto isecs2 = clone1->intersections(clone2, nullptr, nullptr);
        for (auto & isec : isecs2)
        {
            Path * c = doc.createCircle(isec.position, 3);
            c->setFill("pink");
        }

        // Path * p2 = doc.createCircle(Vec2f(100, 100), 30);
        // path->addChild(p2);
        // // path->addPoint(Vec2f(100, 100));
        // // path->addPoint(Vec2f(150, 110));
        // // path->addPoint(Vec2f(150, 130));
        // // path->addPoint(Vec2f(135, 200));
        // // path->closePath();
        // // path->smooth();
        // path->setFill("red");
        // path->translateTransform(100, 200);
        // path->scaleTransform(0.5, 2.0);
        // path->rotateTransform(0.1);

        // Symbol * s = doc.createSymbol(path);
        // s->translateTransform(200, 100);
        // s->scaleTransform(2.0, 1.0);
        // s->rotateTransform(0.1);
        // s->setFill("yellow");
        // s->setStroke("green");
        // trans = &s->absoluteTransform();

        // Path * clone = path->clone();
        // clone->setStroke("yellow");
        // clone->simplify();

        int counter = 0;

        // the main loop
        while (!glfwWindowShouldClose(window))
        {
            // clear the background to black
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            // get the window size from the glfw window
            // and set it as the viewport on the renderer
            int width, height;
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

            // counter++;
            // if(counter > 200)
            // {
            //     // printf("CC %lu\n", circle2.children().count());
            //     circle.removeChild(circle2);
            //     // circle3.remove();
            // }

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
