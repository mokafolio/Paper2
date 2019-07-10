#include <Paper2/Private/PathFitter.hpp>
#include <cmath>

namespace paper
{
namespace detail
{
using namespace stick;
using namespace crunch;

PathFitter::PathFitter(Path * _p, Float64 _error, bool _bIgnoreClosed) :
    m_path(_p),
    m_error(_error),
    m_bIgnoreClosed(_bIgnoreClosed),
    m_newSegments(_p->segmentData().allocator()),
    m_positions(_p->segmentData().allocator())
{
    auto & segs = m_path->segmentData();
    m_positions.reserve(segs.count());

    Vec2f prev, point;
    prev = point = Vec2f(0);
    // Copy over points from path and filter out adjacent duplicates.
    for (Size i = 0; i < segs.count(); ++i)
    {
        point = segs[i].position;
        if (i < 1 || prev != point)
        {
            m_positions.append(point);
            prev = point;
        }
    }

    if (m_path->isClosed() && !m_bIgnoreClosed)
    {
        // this is kinda unefficient :( we should propably just insert that point
        // first rather than shifting the whole array
        Vec2f last = m_positions.last();
        m_positions.insert(m_positions.begin(), last);
        m_positions.append(m_positions[1]); // The point previously at index 0 is now 1.
    }
}

void PathFitter::fit()
{
    if (m_positions.count() > 0)
    {
        m_newSegments.append({ m_positions[0], m_positions[0], m_positions[0] });

        // Size i = 0;
        // Size count = m_newSegments.count();
        // bool bReclose = false;

        if (m_positions.count() > 1)
        {
            fitCubic(0,
                     m_positions.count() - 1,
                     // Left Tangent
                     m_positions[1] - m_positions[0],
                     // Right Tangent
                     m_positions[m_positions.count() - 2] - m_positions[m_positions.count() - 1]);

            if (m_path->isClosed())
            {
                // i++;
                // if (count > 0)
                //     count--;
                // bReclose = true;
                // m_path->m_bIsClosed = false;

                // printf("F %f %f L %f %f\n", (*m_newSegments.begin()).position.x,
                // (*m_newSegments.begin()).position.y,
                //        m_newSegments.last().position.x,
                //        m_newSegments.last().position.y);
                if (!m_bIgnoreClosed)
                {
                    m_newSegments.remove(m_newSegments.begin());
                    m_newSegments.removeLast();
                }
            }
        }

        // m_path.removeSegments();
        // auto & segs = m_path.segmentArray();
        // segs.clear();
        // Document doc = m_path.document();
        // for (i = 0; i < m_newSegments.count(); ++i)
        // {
        //     segs.append(makeUnique<Segment>(doc.allocator(), m_path,
        //     m_newSegments[i].position, m_newSegments[i].handleIn,
        //     m_newSegments[i].handleOut, segs.count()));
        // }
        m_path->swapSegments(m_newSegments, m_path->isClosed());
    }
}

template<class T>
inline crunch::Vector2<T> normalizeSafe(const crunch::Vector2<T> & _vec)
{
    T len = length(_vec);
    return len > 0 ? _vec / len : _vec;
}

void PathFitter::fitCubic(Size _first, Size _last, const Vec2f & _tan1, const Vec2f & _tan2)
{
    // printf("FIRST %lu LAST %lu\n", _first, _last);
    // Use heuristic if region only has two points in it
    if (_last - _first == 1)
    {
        // printf("heuristic\n");
        const Vec2f & pt1 = m_positions[_first];
        const Vec2f & pt2 = m_positions[_last];

        STICK_ASSERT(!std::isnan(pt1.x));
        STICK_ASSERT(!std::isnan(pt1.y));
        STICK_ASSERT(!std::isnan(pt2.x));
        STICK_ASSERT(!std::isnan(pt2.y));

        Float64 dist = crunch::distance(pt1, pt2) / 3.0;
        STICK_ASSERT(!std::isnan(dist));
        addCurve(pt1, pt1 + normalizeSafe(_tan1) * dist, pt2 + normalizeSafe(_tan2) * dist, pt2);
        return;
    }

    DynamicArray<Float64> uPrime(m_newSegments.allocator());
    chordLengthParameterize(_first, _last, uPrime);

    // printf("NORMAL\n");
    Float64 maxError = std::max(m_error, m_error * m_error);
    Size split;
    bool bParametersInOrder = true;

    // Try 4 iterations
    for (Int32 i = 0; i <= 4; ++i)
    {
        Bezier curve = generateBezier(_first, _last, uPrime, _tan1, _tan2);
        // printf("GEN BEZ %f %f, %f %f, %f %f, %f %f\n", curve.positionOne().x,
        // curve.positionOne().y,
        //        curve.handleOne().x, curve.handleOne().y,
        //        curve.handleTwo().x, curve.handleTwo().y,
        //        curve.positionTwo().x, curve.positionTwo().y);

        //  Find max deviation of points to fitted curve
        auto max = findMaxError(_first, _last, curve, uPrime);

        if (max.value < m_error && bParametersInOrder)
        {
            // printf("ADDING CURVE\n");
            addCurve(
                curve.positionOne(), curve.handleOne(), curve.handleTwo(), curve.positionTwo());
            return;
        }
        split = max.index;

        // If error not too large, try reparameterization and iteration
        if (max.value >= maxError)
        {
            // printf("MAX ERROR %f %f %f\n", m_error, maxError, max.value);
            break;
        }

        bParametersInOrder = reparameterize(_first, _last, uPrime, curve);
        maxError = max.value;
    }

    // Fitting failed -- split at max error point and fit recursively
    // Vec2f v1 = m_positions[split - 1] - m_positions[split];
    // Vec2f v2 = m_positions[split] - m_positions[split + 1];
    // Vec2f tanCenter = crunch::normalize((v1 + v2) * 0.5);
    Vec2f tanCenter = m_positions[split - 1] - m_positions[split + 1];
    fitCubic(_first, split, _tan1, tanCenter);
    fitCubic(split, _last, -tanCenter, _tan2);
}

void PathFitter::addCurve(const Vec2f & _pointOne,
                          const Vec2f & _handleOne,
                          const Vec2f & _handleTwo,
                          const Vec2f & _pointTwo)
{
    STICK_ASSERT(!std::isnan(_pointOne.x));
    STICK_ASSERT(!std::isnan(_pointOne.y));
    STICK_ASSERT(!std::isnan(_handleOne.x));
    STICK_ASSERT(!std::isnan(_handleOne.y));
    STICK_ASSERT(!std::isnan(_handleTwo.x));
    STICK_ASSERT(!std::isnan(_handleTwo.y));
    STICK_ASSERT(!std::isnan(_pointTwo.x));
    STICK_ASSERT(!std::isnan(_pointTwo.y));
    m_newSegments.last().handleOut = _handleOne;
    m_newSegments.append({ _handleTwo, _pointTwo, _pointTwo });
}

Bezier PathFitter::generateBezier(Size _first,
                                  Size _last,
                                  const DynamicArray<Float64> & _uPrime,
                                  const Vec2f & _tan1,
                                  const Vec2f & _tan2)
{
    static const Float64 s_epsilon = detail::PaperConstants::geometricEpsilon();

    const Vec2f & pt1 = m_positions[_first];
    const Vec2f & pt2 = m_positions[_last];

    Float64 c[2][2] = { { 0, 0 }, { 0, 0 } };
    Float64 x[2] = { 0, 0 };

    for (Size i = 0, l = _last - _first + 1; i < l; ++i)
    {
        Float64 u = _uPrime[i];
        Float64 t = 1.0 - u;
        Float64 b = 3.0 * u * t;
        Float64 b0 = t * t * t;
        Float64 b1 = b * t;
        Float64 b2 = b * u;
        Float64 b3 = u * u * u;
        Vec2f a1 = normalizeSafe(_tan1) * b1;
        Vec2f a2 = normalizeSafe(_tan2) * b2;
        Vec2f tmp = m_positions[_first + i];
        tmp -= pt1 * (b0 + b1);
        tmp -= pt2 * (b2 + b3);

        c[0][0] += crunch::dot(a1, a1);
        c[0][1] += crunch::dot(a1, a2);

        c[1][0] = c[0][1];
        c[1][1] += crunch::dot(a2, a2);

        x[0] += crunch::dot(a1, tmp);
        x[1] += crunch::dot(a2, tmp);
    }

    Float64 detC0C1 = c[0][0] * c[1][1] - c[1][0] * c[0][1];
    Float64 alpha1, alpha2;

    if (crunch::abs(detC0C1) > s_epsilon)
    {
        // Kramer's rule
        Float64 detC0X = c[0][0] * x[1] - c[1][0] * x[0];
        Float64 detXC1 = x[0] * c[1][1] - x[1] * c[0][1];
        // Derive alpha values
        alpha1 = detXC1 / detC0C1;
        alpha2 = detC0X / detC0C1;
    }
    else
    {
        // Matrix is under-determined, try assuming alpha1 == alpha2
        Float64 c0 = c[0][0] + c[0][1];
        Float64 c1 = c[1][0] + c[1][1];

        if (crunch::abs(c0) > s_epsilon)
        {
            alpha1 = alpha2 = x[0] / c0;
        }
        else if (crunch::abs(c1) > s_epsilon)
        {
            alpha1 = alpha2 = x[1] / c1;
        }
        else
        {
            // Handle below
            alpha1 = alpha2 = 0;
        }
    }

    // If alpha negative, use the Wu/Barsky heuristic (see text)
    // (if alpha is 0, you get coincident control points that lead to
    // divide by zero in any subsequent NewtonRaphsonRootFind() call.
    Float64 segLength = crunch::distance(pt1, pt2);
    Float64 epsilon = s_epsilon * segLength;
    Vec2f handleOne(0);
    Vec2f handleTwo(0);
    if (alpha1 < epsilon || alpha2 < epsilon)
    {
        // fall back on standard (probably inaccurate) formula,
        // and subdivide further if needed.
        alpha1 = alpha2 = segLength / 3.0;
    }
    else
    {
        // Check if the found control points are in the right order when
        // projected onto the line through pt1 and pt2.
        Vec2f line = pt2 - pt1;

        handleOne = normalizeSafe(_tan1) * alpha1;
        handleTwo = normalizeSafe(_tan2) * alpha2;

        if (crunch::dot(handleOne, line) - crunch::dot(handleTwo, line) > segLength * segLength)
        {
            // Fall back to the Wu/Barsky heuristic above.
            alpha1 = alpha2 = segLength / 3.0;
            handleOne = normalizeSafe(_tan1) * alpha1;
            handleTwo = normalizeSafe(_tan2) * alpha2;
        }
    }

    STICK_ASSERT(!std::isnan(pt1.x));
    STICK_ASSERT(!std::isnan(pt1.y));
    STICK_ASSERT(!std::isnan(pt2.x));
    STICK_ASSERT(!std::isnan(pt2.y));
    STICK_ASSERT(!std::isnan(handleOne.x));
    STICK_ASSERT(!std::isnan(handleOne.y));
    STICK_ASSERT(!std::isnan(handleTwo.x));
    STICK_ASSERT(!std::isnan(handleTwo.y));

    // First and last control points of the Bezier curve are
    // positioned exactly at the first and last data points
    // Control points 1 and 2 are positioned an alpha distance out
    // on the tangent vectors, left and right, respectively
    return Bezier(pt1, pt1 + handleOne, pt2 + handleTwo, pt2);
}

Vec2f PathFitter::evaluate(Int32 _degree, const Bezier & _curve, Float64 _t)
{
    Vec2f tmp[4] = {
        _curve.positionOne(), _curve.handleOne(), _curve.handleTwo(), _curve.positionTwo()
    };
    for (Int32 i = 1; i <= _degree; ++i)
    {
        for (Int32 j = 0; j <= _degree - i; ++j)
        {
            tmp[j] = tmp[j] * (1 - _t) + tmp[j + 1] * _t;
        }
    }

    return tmp[0];
}

bool PathFitter::reparameterize(Size _first,
                                Size _last,
                                DynamicArray<Float64> & _u,
                                const Bezier & _curve)
{
    for (Size i = _first; i <= _last; ++i)
    {
        _u[i - _first] = findRoot(_curve, m_positions[i], _u[i - _first]);
    }

    // Detect if the new parameterization has reordered the points.
    // In that case, we would fit the points of the path in the wrong order.
    for (Size i = 1; i < _u.count(); ++i)
    {
        if (_u[i] <= _u[i - 1])
            return false;
    }

    return true;
}

Float64 PathFitter::findRoot(const Bezier & _curve, const Vec2f & _point, Float64 _u)
{
    Bezier curve1;
    Bezier curve2;

    // control vertices for Q'
    curve1.setPositionOne((_curve.handleOne() - _curve.positionOne()) * 3.0);
    curve1.setHandleOne((_curve.handleTwo() - _curve.handleOne()) * 3.0);
    curve1.setHandleTwo((_curve.positionTwo() - _curve.handleTwo()) * 3.0);

    // control vertices for Q''
    curve2.setPositionOne((curve1.handleOne() - curve1.positionOne()) * 2.0);
    curve2.setHandleOne((curve1.handleTwo() - curve1.handleOne()) * 2.0);

    // Compute Q(u), Q'(u) and Q''(u)
    Vec2f pt = evaluate(3, _curve, _u);
    Vec2f pt1 = evaluate(2, curve1, _u);
    Vec2f pt2 = evaluate(1, curve2, _u);
    Vec2f diff = pt - _point;
    Float64 df = crunch::dot(pt1, pt1) + crunch::dot(diff, pt2);

    // u = u - f(u) / f'(u)
    return crunch::isClose(df, (Float64)0.0) ? _u : _u - crunch::dot(diff, pt1) / df;
}

void PathFitter::chordLengthParameterize(Size _first,
                                         Size _last,
                                         DynamicArray<Float64> & _outResult)
{
    Size size = _last - _first;
    _outResult.resize(size + 1);
    _outResult[0] = 0;
    for (Size i = _first + 1; i <= _last; ++i)
    {
        _outResult[i - _first] =
            _outResult[i - _first - 1] + crunch::distance(m_positions[i], m_positions[i - 1]);
    }
    for (Size i = 1; i <= size; i++)
        _outResult[i] /= _outResult[size];
}

PathFitter::MaxError PathFitter::findMaxError(Size _first,
                                              Size _last,
                                              const Bezier & _curve,
                                              const DynamicArray<Float64> & _u)
{
    Size index = crunch::floor((_last - _first + 1) / 2.0);
    Float64 maxDist = 0;
    for (Size i = _first + 1; i < _last; ++i)
    {
        Vec2f p = evaluate(3, _curve, _u[i - _first]);
        Vec2f v = p - m_positions[i];
        Float64 dist = v.x * v.x + v.y * v.y; // squared
        if (dist >= maxDist)
        {
            maxDist = dist;
            index = i;
        }
    }
    return { maxDist, index };
}
} // namespace detail
} // namespace paper
