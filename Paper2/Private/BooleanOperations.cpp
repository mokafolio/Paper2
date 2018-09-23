#include <Paper2/Path.hpp>

#include <Crunch/MatrixFunc.hpp>
#include <Crunch/StringConversion.hpp>

namespace paper
{
namespace detail
{
using namespace stick;

static void insertCurve(const Bezier & _c, MonoCurveLoop & _target)
{
    Float y0 = _c.positionOne().y;
    Float y1 = _c.positionTwo().y;

    Int32 winding;
    if (crunch::abs((y0 - y1) / (_c.positionOne().x - _c.positionTwo().x)) <
        detail::PaperConstants::geometricEpsilon())
    {
        winding = 0;
    }
    else if (y0 > y1)
    {
        winding = -1;
    }
    else
    {
        winding = 1;
    }

    MonoCurve c = {_c, winding};
    _target.monoCurves.append(c);

    // Keep track of the last non-horizontal curve (with winding).
    if (winding)
        _target.last = c;
}

static void handleCurve(const Bezier & _c, MonoCurveLoop & _target)
{
    // Filter out curves of zero length.
    // TODO: Do not filter this here.
    if (_c.length() == 0)
    {
        return;
    }

    Float y0, y1, y2, y3;
    y0 = _c.positionOne().y;
    y1 = _c.handleOne().y;
    y2 = _c.handleTwo().y;
    y3 = _c.positionTwo().y;

    if (_c.isStraight() || (y0 >= y1 == y1 >= y2 && y1 >= y2 == y2 >= y3))
    {
        // Straight curves and curves with end and control points sorted
        // in y direction are guaranteed to be monotonic in y direction.
        insertCurve(_c, _target);
    }
    else
    {
        // Split the curve at y extrema, to get bezier curves with clear
        // orientation: Calculate the derivative and find its roots.

        Float a = (y1 - y2) * 3.0 - y0 + y3;
        Float b = (y0 + y2) * 2.0 - y1 * 4.0;
        Float c = y1 - y0;

        Float tMin = detail::PaperConstants::curveTimeEpsilon();
        Float tMax = 1 - tMin;

        // Keep then range to 0 .. 1 (excluding) in the search for y
        // extrema.
        auto res = crunch::solveQuadratic(a, b, c, tMin, tMax);
        if (res.count == 0)
        {
            insertCurve(_c, _target);
        }
        else
        {
            std::sort(&res.values[0], &res.values[0] + res.count);
            Float t = res.values[0];

            auto curves = _c.subdivide(t);
            insertCurve(curves.first, _target);
            if (res.count > 1)
            {
                // If there are two extrema, renormalize t to the range
                // of the second range and split again.
                t = (res.values[1] - t) / (1 - t);
                // Since we already processed curves.first, we can override
                // the parts array with the new pair now.
                curves = curves.second.subdivide(t);
                insertCurve(curves.first, _target);
            }
            insertCurve(curves.second, _target);
        }
    }
}

const MonoCurveLoopArray & BooleanOperations::monoCurves(const Path * _path)
{
    if (!_path->m_monoCurves.count())
    {
        MonoCurveLoop data;
        if (_path->isTransformed())
        {
            data.bTransformed = true;
            data.inverseTransform = crunch::inverse(_path->absoluteTransform());
        }
        else
            data.bTransformed = false;

        for (Size i = 0; i < _path->curveCount(); ++i)
            handleCurve(_path->curve(i).bezier(), data);

        // If the path is not closed, we need to join the end points with a
        // straight line, just like how filling open paths works.

        if (!_path->isClosed() && _path->segmentData().count() > 1)
        {
            Bezier tmp(_path->segmentData().last().position,
                       _path->segmentData().last().position,
                       _path->segmentData().first().position,
                       _path->segmentData().first().position);
            handleCurve(tmp, data);
        }

        _path->m_monoCurves.append(data);

        // If this is a compound path, get the child mono curves and append them
        for (Item * c : _path->children())
        {
            const MonoCurveLoopArray & cc = BooleanOperations::monoCurves(static_cast<Path *>(c));
            _path->m_monoCurves.insert(_path->m_monoCurves.end(), cc.begin(), cc.end());

            // we don't want to cache the curves twice, so we remove em from da kid
            static_cast<Path *>(c)->m_monoCurves.clear();
        }
    }

    return _path->m_monoCurves;
}

//@TODO: Update this to the changed implementation (apparently more stable) from paper.js develop
Int32 BooleanOperations::winding(const Vec2f & _point,
                                 const MonoCurveLoopArray & _loops,
                                 bool _bHorizontal)
{
    Float epsilon = detail::PaperConstants::windingEpsilon();
    Int32 windingLeft = 0;
    Int32 windingRight = 0;

    // Horizontal curves may return wrong results, since the curves are
    // monotonic in y direction and this is an indeterminate state.
    if (_bHorizontal)
    {
        Float yTop = -std::numeric_limits<Float>::infinity();
        Float yBottom = std::numeric_limits<Float>::infinity();
        Float yBefore;
        Float yAfter;

        for (const MonoCurveLoop & loop : _loops)
        {
            Vec2f p = loop.bTransformed ? loop.inverseTransform * _point : _point;
            yBefore = p.y - epsilon;
            yAfter = p.y + epsilon;
            // Find the closest top and bottom intercepts for the vertical line.
            for (const MonoCurve & c : loop.monoCurves)
            {
                auto result = c.bezier.solveCubic(p.x, true, 0, 1);
                for (Int32 j = result.count - 1; j >= 0; j--)
                {
                    Float y = c.bezier.positionAt(result.values[j]).y;
                    if (y < yBefore && y > yTop)
                    {
                        yTop = y;
                    }
                    else if (y > yAfter && y < yBottom)
                    {
                        yBottom = y;
                    }
                }
            }
        }

        // Shift the point lying on the horizontal curves by half of the
        // closest top and bottom intercepts.
        yTop = (yTop + _point.y) * 0.5;
        yBottom = (yBottom + _point.y) * 0.5;
        if (yTop > -std::numeric_limits<Float>::max())
            windingLeft = winding(Vec2f(_point.x, yTop), _loops, false);
        if (yBottom < std::numeric_limits<Float>::max())
            windingRight = winding(Vec2f(_point.x, yBottom), _loops, false);
    }
    else
    {
        Float xBefore;
        Float xAfter;
        Int32 prevWinding;
        Float prevXEnd;
        bool bIsOnCurve = false;
        // Separately count the windings for points on curves.
        Int32 windLeftOnCurve = 0;
        Int32 windRightOnCurve = 0;

        for (const MonoCurveLoop & loop : _loops)
        {
            Vec2f p = loop.bTransformed ? loop.inverseTransform * _point : _point;
            xBefore = p.x - epsilon;
            xAfter = p.x + epsilon;
            for (Size i = 0; i < loop.monoCurves.count(); ++i)
            {
                const MonoCurve & curve = loop.monoCurves[i];
                Float yStart = curve.bezier.positionOne().y;
                Float yEnd = curve.bezier.positionTwo().y;
                Int32 winding = curve.winding;

                // The first curve of a loop holds the last curve with non-zero
                // winding. Retrieve and use it here.
                if (i == 0)
                {
                    prevWinding = loop.last.winding;
                    prevXEnd = loop.last.bezier.positionTwo().x;
                    // Reset the on curve flag for each loop.
                    bIsOnCurve = false;
                }

                // Since the curves are monotonic in y direction, we can just
                // compare the endpoints of the curve to determine if the ray
                // from query point along +-x direction will intersect the
                // monotonic curve.
                if ((p.y >= yStart && p.y <= yEnd) || (p.y >= yEnd && p.y <= yStart))
                {
                    if (winding != 0)
                    {
                        // Calculate the x value for the ray's intersection.
                        Float x;
                        bool bGotX = true;
                        if (p.y == yStart)
                        {
                            x = curve.bezier.positionOne().x;
                        }
                        else if (p.y == yEnd)
                        {
                            x = curve.bezier.positionTwo().x;
                        }
                        else
                        {
                            auto roots = curve.bezier.solveCubic(p.y, false, 0, 1);
                            if (roots.count == 1)
                            {
                                x = curve.bezier.positionAt(roots.values[0]).x;
                            }
                            else
                            {
                                bGotX = false;
                            }
                        }

                        if (bGotX)
                        {
                            // Test if the point is on the current mono-curve.
                            if (x >= xBefore && x <= xAfter)
                            {
                                bIsOnCurve = true;
                            }
                            else if (
                                // Count the intersection of the ray with the
                                // monotonic curve if the crossing is not the
                                // start of the curve, except if the winding
                                // changes...
                                (p.y != yStart || winding != prevWinding)
                                // ...and the point is not on the curve or on
                                // the horizontal connection between the last
                                // non-horizontal curve's end point and the
                                // current curve's start point.
                                && !(p.y == yStart && (p.x - x) * (p.x - prevXEnd) < 0))
                            {
                                if (x < xBefore)
                                {
                                    windingLeft += winding;
                                }
                                else if (x > xAfter)
                                {
                                    windingRight += winding;
                                }
                            }
                        }

                        // Update previous winding and end coordinate whenever
                        // the ray intersects a non-horizontal curve.
                        prevWinding = winding;
                        prevXEnd = curve.bezier.positionTwo().x;
                    }
                    // Test if the point is on the horizontal curve.
                    else if ((p.x - curve.bezier.positionOne().x) *
                                 (p.x - curve.bezier.positionTwo().x) <=
                             0)
                    {
                        bIsOnCurve = true;
                    }
                }

                // If we are at the end of a loop and the point was on a curve
                // of the loop, we increment / decrement the on-curve winding
                // numbers as if the point was inside the path.
                if (bIsOnCurve && (i >= loop.monoCurves.count() - 1))
                {
                    windLeftOnCurve += 1;
                    windRightOnCurve -= 1;
                }
            }
        }

        // Use the on-curve windings if no other intersections were found or
        // if they canceled each other. On single paths this ensures that
        // the overall winding is 1 if the point was on a monotonic curve.
        if (windingLeft == 0 && windingRight == 0)
        {
            windingLeft = windLeftOnCurve;
            windingRight = windRightOnCurve;
        }
    }

    return max(abs(windingLeft), abs(windingRight));
}

} // namespace detail
} // namespace paper
