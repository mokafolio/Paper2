#ifndef PAPER_RENDERDATA_HPP
#define PAPER_RENDERDATA_HPP

#include <Stick/UniquePtr.hpp>

namespace paper
{
    //used by the render interface to store rendering specific user data
    class RenderData;
    using RenderDataUniquePtr = stick::UniquePtr<RenderData>;

    class STICK_API RenderData
    {
    public:

        virtual ~RenderData() {}

        virtual RenderDataUniquePtr clone() const = 0;
    };
}

#endif //PAPER_RENDERDATA_HPP
