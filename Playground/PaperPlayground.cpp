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

Maybe<Paint> bla;
Paint bla2;

void fu(RadialGradientPtr _grad)
{
    static_assert(stick::detail::Traits<decltype(_grad),
                                        NoPaint,
                                        ColorRGBA,
                                        LinearGradientPtr,
                                        RadialGradientPtr>::bIsDirect,
                  "DAFUQ");
    static_assert(
        TypeInfoT<
            stick::detail::
                Traits<decltype(_grad), NoPaint, ColorRGBA, LinearGradientPtr, RadialGradientPtr>::
                    TargetType>::typeID() == TypeInfoT<RadialGradientPtr>::typeID(),
        "FUUUUCK");

    // bla = _grad;
    printf("TYPE %s\n", typeid(decltype(_grad)).name());
    Paint bla2 = _grad;
}

int main(int _argc, const char * _args[])
{
    RadialGradientPtr b = makeShared<RadialGradient>(Vec2f(0), Vec2f(0));

    Variant<NoPaint, ColorRGBA, LinearGradientPtr, RadialGradientPtr> blubb(std::move(b));

    // fu(createRadialGradient(Vec2f(0), Vec2f(0)));
    // bla = ColorRGBA(1.0, 1.0, 1.0, 1.0);
    // bla = makeShared<RadialGradient>(Vec2f(100, 100), Vec2f(200, 200));

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

        Path * path = doc.createPath();
        path->addPoint(Vec2f(100, 100));
        path->addPoint(Vec2f(150, 110));
        path->addPoint(Vec2f(150, 130));
        path->addPoint(Vec2f(135, 200));
        path->closePath();
        path->setFill("red");

        Curve longest;
        Float32 maxLen = 0;
        Size idx = 0;
        for (Curve curve : path->curves())
        {
            printf("IDX %lu\n", idx);
            if (curve.length() > maxLen)
            {
                longest = curve;
                maxLen = curve.length();
                printf("GOT LONGEST %f\n", maxLen);
            }
            printf("lidx %i\n", longest.index());
            idx++;
        }

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
            renderer.setProjection(Mat4f::ortho(0, width, height, 0, -1, 1));
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
