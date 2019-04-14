#ifndef PAPER_PRIVATE_BEZIERVIEW_HPP
#define PAPER_PRIVATE_BEZIERVIEW_HPP

namespace paper
{
namespace detail
{   
    template<class T>
    class STICK_API ContainerBezierViewT
    {
    public:

        using SegmentContainer = T;
        using SegmentData = typename T::ValueType;

        ContainerBezierViewT(T & _segments)
        {
        }
    };
}
}

#endif //PAPER_PRIVATE_BEZIERVIEW_HPP
