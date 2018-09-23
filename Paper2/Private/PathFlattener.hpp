#ifndef PAPER_PRIVATE_PATHFLATTENER_HPP
#define PAPER_PRIVATE_PATHFLATTENER_HPP

#include <Crunch/GeometricFunc.hpp>
#include <Paper2/BasicTypes.hpp>
#include <Stick/DynamicArray.hpp>

namespace paper
{
class Path;

namespace detail
{
struct STICK_LOCAL PathFlattener
{
    typedef stick::DynamicArray<Vec2f> PositionArray;
    typedef stick::DynamicArray<bool> JoinArray;

    static bool isFlatEnough(const Bezier & _curve, Float _tolerance);

    static void flatten(const Path * _path,
                        PositionArray & _outPositions,
                        JoinArray * _outJoins,
                        Float _angleTolerance,
                        Float _minDistance,
                        Size _maxRecursionDepth);

    static void flattenCurve(const Bezier & _curve,
                             const Bezier & _initialCurve,
                             PositionArray & _outPositions,
                             JoinArray * _outJoins,
                             Float _angleTolerance,
                             Float _minDistance,
                             Size _recursionDepth,
                             Size _maxRecursionDepth,
                             bool _bIsClosed,
                             bool _bLastCurve);
};
} // namespace detail
} // namespace paper

#endif // PAPER_PRIVATE_PATHFLATTENER_HPP
