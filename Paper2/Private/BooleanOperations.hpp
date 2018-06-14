#ifndef PAPER_PRIVATE_BOOLEANOPERATIONS_HPP
#define PAPER_PRIVATE_BOOLEANOPERATIONS_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
    class Path;

    namespace detail
    {
        struct STICK_LOCAL MonoCurve
        {
            Bezier bezier;
            Int32 winding;
        };

        using MonoCurveArray = stick::DynamicArray<MonoCurve>;

        struct STICK_LOCAL MonoCurveLoop
        {
            Mat32f inverseTransform;
            bool bTransformed;
            MonoCurveArray monoCurves;
            MonoCurve last;
        };

        using MonoCurveLoopArray = stick::DynamicArray<MonoCurveLoop>;

        //wrapped in a class so we can easily friend it with Path.
        //not the prettiest solution but imo the best one without
        //having to clutter the Path header even more.
        struct STICK_LOCAL BooleanOperations
        {
            static const MonoCurveLoopArray & monoCurves(const Path * _path);
            static stick::Int32 winding(const Vec2f & _point, const MonoCurveLoopArray & _loops, bool _bHorizontal);
        };
    }
}

#endif //PAPER_PRIVATE_BOOLEANOPERATIONS_HPP
