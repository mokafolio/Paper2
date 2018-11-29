#include <Paper2/Document.hpp>
#include <Paper2/Private/JoinAndCap.hpp>
#include <Paper2/Private/PathFitter.hpp>
#include <Paper2/Private/PathFlattener.hpp>

#include <Crunch/MatrixFunc.hpp>
#include <Crunch/StringConversion.hpp>

namespace paper
{
using namespace stick;

CurveLocation::CurveLocation()
{
}

CurveLocation::CurveLocation(ConstCurve _c, Float _parameter, Float _offset) :
    m_curve(_c),
    m_parameter(_parameter),
    m_offset(_offset)
{
}

CurveLocation::operator bool() const
{
    return (bool)m_curve;
}

bool CurveLocation::operator==(const CurveLocation & _other) const
{
    return m_curve.m_path == _other.m_curve.m_path && m_parameter == _other.m_parameter;
}

bool CurveLocation::operator!=(const CurveLocation & _other) const
{
    return !(*this == _other);
}

bool CurveLocation::isSynonymous(const CurveLocation & _other) const
{
    if (isValid() && _other.isValid())
    {
        if (_other.curve().path() == curve().path())
        {
            Float diff = std::abs(m_offset - _other.m_offset);
            if (diff < detail::PaperConstants::geometricEpsilon() ||
                std::abs(curve().path()->length() - diff) <
                    detail::PaperConstants::geometricEpsilon())
            {
                return true;
            }
        }
    }
    return false;
}

Vec2f CurveLocation::position() const
{
    return m_curve.positionAtParameter(m_parameter);
}

Vec2f CurveLocation::normal() const
{
    return m_curve.normalAtParameter(m_parameter);
}

Vec2f CurveLocation::tangent() const
{
    return m_curve.tangentAtParameter(m_parameter);
}

Float CurveLocation::curvature() const
{
    return m_curve.curvatureAtParameter(m_parameter);
}

Float CurveLocation::angle() const
{
    return m_curve.angleAtParameter(m_parameter);
}

Float CurveLocation::parameter() const
{
    return m_parameter;
}

Float CurveLocation::offset() const
{
    return m_offset;
}

bool CurveLocation::isValid() const
{
    return (bool)m_curve;
}

ConstCurve CurveLocation::curve() const
{
    return m_curve;
}

namespace segments
{
void addPoint(SegmentDataArray & _segs, const Vec2f & _to)
{
    _segs.append({ _to, _to, _to });
}

void cubicCurveTo(SegmentDataArray & _segs,
                  const Vec2f & _handleOne,
                  const Vec2f & _handleTwo,
                  const Vec2f & _to)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();

    // make relative
    current.handleOut = _handleOne;
    _segs.append({ _handleTwo, _to, _to });
}

void quadraticCurveTo(SegmentDataArray & _segs, const Vec2f & _handle, const Vec2f & _to)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();

    auto fact = 2.0 / 3.0;
    cubicCurveTo(_segs,
                 current.position + (_handle - current.position) * fact,
                 _to + (_handle - _to) * fact,
                 _to);
}

void curveTo(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _to, Float _parameter)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();

    Float t1 = 1 - _parameter;
    Float tt = _parameter * _parameter;
    Vec2f handle = (_through - current.position * tt - _to * tt) / (2 * _parameter * t1);

    quadraticCurveTo(_segs, handle, _to);
}

Error arcTo(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _to)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();

    const Vec2f & from = current.position;

    Vec2f lineOneStart = (from + _through) * 0.5;
    crunch::Line<Vec2f> lineOne(
        lineOneStart, crunch::rotate(_through - from, crunch::Constants<Float>::halfPi()));

    Vec2f lineTwoStart = (_through + _to) * 0.5;
    crunch::Line<Vec2f> lineTwo(lineTwoStart,
                                crunch::rotate(_to - _through, crunch::Constants<Float>::halfPi()));

    auto result = crunch::intersect(lineOne, lineTwo);
    //@TODO: double check if turning this into a line segment fixes:
    // https://github.com/paperjs/paper.js/issues/1477
    crunch::LineSegment<Vec2f> line(from, _to);
    Int32 throughSide = line.side(_through);

    if (!result)
    {
        if (throughSide == 0)
        {
            // From paper.js
            // If the two lines are colinear, there cannot be an arc as the
            // circle is infinitely big and has no center point. If side is
            // 0, the connecting arc line of this huge circle is a line
            // between the two points, so we can use #lineTo instead.
            // Otherwise we bail out:
            addPoint(_segs, _to);
            return Error();
        }
        return Error(ec::InvalidArgument,
                     String::concat("Cannot put an arc through the given points: ",
                                    crunch::toString(_through),
                                    " and ",
                                    crunch::toString(_to)),
                     STICK_FILE,
                     STICK_LINE);
    }
    Vec2f vec = from - *result;
    Float extent = crunch::toDegrees(crunch::directedAngle(vec, _to - *result));
    Int32 centerSide = line.side(*result);

    if (centerSide == 0)
    {
        // If the center is lying on the line, we might have gotten the
        // wrong sign for extent above. Use the sign of the side of the
        // through point.
        extent = throughSide * crunch::abs(extent);
    }
    else if (throughSide == centerSide)
    {
        // If the center is on the same side of the line (from, to) as
        // the through point, we're extending bellow 180 degrees and
        // need to adapt extent.
        extent -= 360.0 * (extent < 0 ? -1 : 1);
    }

    return arcHelper(_segs, extent, vec, _to, *result, nullptr);
}

Error arcTo(SegmentDataArray & _segs, const Vec2f & _to, bool _bClockwise)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();

    Vec2f mid = (current.position + _to) * 0.5;
    Vec2f dir = (mid - current.position);
    dir = !_bClockwise ? Vec2f(-dir.y, dir.x) : Vec2f(dir.y, -dir.x);
    return arcTo(_segs, mid + dir, _to);
}

Error arcTo(SegmentDataArray & _segs,
            const Vec2f & _to,
            const Vec2f & _radii,
            Float _rotation,
            bool _bClockwise,
            bool _bLarge)
{
    STICK_ASSERT(_segs.count());
    if (crunch::isClose(_radii.x, (Float)0.0) || crunch::isClose(_radii.y, (Float)0.0))
    {
        addPoint(_segs, _to);
        return Error();
    }

    Vec2f from = _segs.last().position;
    Vec2f middle = (from + _to) * 0.5;
    Vec2f pt = crunch::rotate(from - middle, -_rotation);
    Float rx = crunch::abs(_radii.x);
    Float ry = crunch::abs(_radii.y);
    printf("RX, RY %f %f\n", rx, ry);
    Float rxSq = rx * rx;
    Float rySq = ry * ry;
    Float xSq = pt.x * pt.x;
    Float ySq = pt.y * pt.y;
    Float factor = std::sqrt(xSq / rxSq + ySq / rySq);
    if (factor > 1)
    {
        printf("APPLY FACTOR\n");
        rx *= factor;
        ry *= factor;
        rxSq = rx * rx;
        rySq = ry * ry;
    }
    printf("RX2, RY2 %f %f\n", rx, ry);

    factor = (rxSq * rySq - rxSq * ySq - rySq * xSq) / (rxSq * ySq + rySq * xSq);
    if (crunch::abs(factor) < detail::PaperConstants::trigonometricEpsilon())
    {
        printf("SET FACTOR TO 0\n");
        factor = 0;
    }

    if (factor < 0)
    {
        return Error(ec::InvalidArgument,
                     "Cannot create an arc with the given arguments",
                     STICK_FILE,
                     STICK_LINE);
    }

    Vec2f center(rx * pt.y / ry, -ry * pt.x / rx);
    center = crunch::rotate(center * ((_bLarge == _bClockwise ? -1 : 1) * std::sqrt(factor)),
                            _rotation) +
             middle;
    // Now create a matrix that maps the unit circle to the ellipse,
    // for easier construction below.

    printf("CENTER %f %f\n", center.x, center.y);
    Mat32f matrix = Mat32f::scaling(_radii);
    matrix.rotate(_rotation);
    matrix.translate(center);
    printf("ROT %f\n", _rotation);
    Mat32f inv = crunch::inverse(matrix);
    Vec2f vect = inv * from;
    Float extent = crunch::directedAngle(vect, inv * _to);
    printf("EXt %f %f %f\n", extent, vect.x, vect.y);
    if (!_bClockwise && extent > 0)
        extent -= crunch::Constants<Float>::twoPi();
    else if (_bClockwise && extent < 0)
        extent += crunch::Constants<Float>::twoPi();

    return arcHelper(_segs, crunch::toDegrees(extent), vect, _to, center, &matrix);
}

Error arcHelper(SegmentDataArray & _segs,
                Float _extentDeg,
                const Vec2f & _direction,
                const Vec2f & _to,
                const Vec2f & _center,
                const Mat32f * _transform)
{
    Float ext = crunch::abs(_extentDeg);
    Int32 count = ext >= 360.0 ? 4 : crunch::ceil(ext / 90.0);
    Float inc = _extentDeg / (Float)count;
    Float half = inc * crunch::Constants<Float>::pi() / 360.0;
    Float z = 4.0 / 3.0 * std::sin(half) / (1.0 + std::cos(half));
    Vec2f dir = _direction;

    for (Int32 i = 0; i <= count; ++i)
    {
        // Explicitely use to point for last segment, since depending
        // on values the calculation adds imprecision:
        Vec2f pt = _to;
        Vec2f out(-dir.y * z, dir.x * z);
        if (i < count)
        {
            if (_transform)
            {
                printf("transFORMING CUNT\n");
                pt = *_transform * dir;
                out = (*_transform * (dir + out)) - pt;
            }
            else
            {
                pt = _center + dir;
            }
        }
        if (i == 0)
        {
            // Modify startSegment
            _segs.last().handleOut = _segs.last().position + out;
        }
        else
        {
            // Add new Segment
            Vec2f in(dir.y * z, -dir.x * z);
            printf("NEW SEG BROOOO\n");
            if (!_transform)
            {
                printf("NEW SEG UNTRANS\n");
                _segs.append({ pt + in, pt, i < count ? pt + out : pt });
            }
            else
            {
                printf("NEW SEG TRANS\n");
                _segs.append({ *_transform * (dir + in), pt, i < count ? pt + out : pt });
                // _segs.append({pt, pt, pt});
            }
        }
        dir = crunch::rotate(dir, crunch::toRadians(inc));
    }
    return Error();
}

void cubicCurveBy(SegmentDataArray & _segs,
                  const Vec2f & _handleOne,
                  const Vec2f & _handleTwo,
                  const Vec2f & _by)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();
    cubicCurveTo(_segs,
                 current.position + _handleOne,
                 current.position + _handleTwo,
                 current.position + _by);
}

void quadraticCurveBy(SegmentDataArray & _segs, const Vec2f & _handle, const Vec2f & _by)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();
    quadraticCurveTo(_segs, current.position + _handle, current.position + _by);
}

void curveBy(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _by, Float _parameter)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();
    curveTo(_segs, current.position + _through, current.position + _by, _parameter);
}

Error arcBy(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _by)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();
    return arcTo(_segs, current.position + _through, current.position + _by);
}

Error arcBy(SegmentDataArray & _segs, const Vec2f & _to, bool _bClockwise)
{
    STICK_ASSERT(_segs.count());
    SegmentData & current = _segs.last();
    return arcTo(_segs, current.position + _to, _bClockwise);
}
} // namespace segments

Path::Path(stick::Allocator & _alloc, Document * _document, const char * _name) :
    Item(_alloc, _document, ItemType::Path, _name),
    m_segmentData(_alloc),
    m_curveData(_alloc),
    m_bIsClosed(false),
    m_bGeometryDirty(false)
{
}

void Path::addPoint(const Vec2f & _to)
{
    // createSegment(_to, Vec2f(0.0), Vec2f(0.0));
    segments::addPoint(m_segmentData, _to);
    appendedSegment();
}

void Path::cubicCurveTo(const Vec2f & _handleOne, const Vec2f & _handleTwo, const Vec2f & _to)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();

    // // make relative
    // current.handleOut = _handleOne - current.position;
    // createSegment(_to, _handleTwo - _to, Vec2f(0.0));
    segments::cubicCurveTo(m_segmentData, _handleOne, _handleTwo, _to);
    appendedSegment();
}

void Path::quadraticCurveTo(const Vec2f & _handle, const Vec2f & _to)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();

    // // Comment from paper.js Path.js:
    // // This is exact:
    // // If we have the three quad points: A E D,
    // // and the cubic is A B C D,
    // // B = E + 1/3 (A - E)
    // // C = E + 1/3 (D - E)

    // cubicCurveTo(
    //     _handle + (current.position - _handle) / 3.0, _handle + (_to - _handle) / 3.0, _to);

    segments::quadraticCurveTo(m_segmentData, _handle, _to);
    appendedSegment();
}

void Path::curveTo(const Vec2f & _through, const Vec2f & _to, Float _parameter)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();

    // Float t1 = 1 - _parameter;
    // Float tt = _parameter * _parameter;
    // Vec2f handle = (_through - current.position * tt - _to * tt) / (2 * _parameter * t1);

    // quadraticCurveTo(handle, _to);
    segments::curveTo(m_segmentData, _through, _to, _parameter);
    appendedSegment();
}

Error Path::arcTo(const Vec2f & _through, const Vec2f & _to)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();

    // const Vec2f & from = current.position;

    // Vec2f lineOneStart = (from + _through) * 0.5;
    // crunch::Line<Vec2f> lineOne(
    //     lineOneStart, crunch::rotate(_through - from, crunch::Constants<Float>::halfPi()));

    // Vec2f lineTwoStart = (_through + _to) * 0.5;
    // crunch::Line<Vec2f> lineTwo(lineTwoStart,
    //                             crunch::rotate(_to - _through,
    //                             crunch::Constants<Float>::halfPi()));

    // auto result = crunch::intersect(lineOne, lineTwo);
    // //@TODO: double check if turning this into a line segment fixes:
    // // https://github.com/paperjs/paper.js/issues/1477
    // crunch::LineSegment<Vec2f> line(from, _to);
    // Int32 throughSide = line.side(_through);

    // if (!result)
    // {
    //     if (throughSide == 0)
    //     {
    //         // From paper.js
    //         // If the two lines are colinear, there cannot be an arc as the
    //         // circle is infinitely big and has no center point. If side is
    //         // 0, the connecting arc line of this huge circle is a line
    //         // between the two points, so we can use #lineTo instead.
    //         // Otherwise we bail out:
    //         addPoint(_to);
    //         return Error();
    //     }
    //     return Error(ec::InvalidArgument,
    //                  String::concat("Cannot put an arc through the given points: ",
    //                                 crunch::toString(_through),
    //                                 " and ",
    //                                 crunch::toString(_to)),
    //                  STICK_FILE,
    //                  STICK_LINE);
    // }
    // Vec2f vec = from - *result;
    // Float extent = crunch::toDegrees(crunch::directedAngle(vec, _to - *result));
    // Int32 centerSide = line.side(*result);

    // if (centerSide == 0)
    // {
    //     // If the center is lying on the line, we might have gotten the
    //     // wrong sign for extent above. Use the sign of the side of the
    //     // through point.
    //     extent = throughSide * crunch::abs(extent);
    // }
    // else if (throughSide == centerSide)
    // {
    //     // If the center is on the same side of the line (from, to) as
    //     // the through point, we're extending bellow 180 degrees and
    //     // need to adapt extent.
    //     extent -= 360.0 * (extent < 0 ? -1 : 1);
    // }

    // return arcHelper(extent, Segment(this, m_segmentData.count() - 1), vec, _to, *result,
    // nullptr);

    auto err = segments::arcTo(m_segmentData, _through, _to);
    if (!err)
        appendedSegment();
    return err;
}

Error Path::arcTo(const Vec2f & _to, bool _bClockwise)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();

    // Vec2f mid = (current.position + _to) * 0.5;
    // Vec2f dir = (mid - current.position);
    // dir = !_bClockwise ? Vec2f(-dir.y, dir.x) : Vec2f(dir.y, -dir.x);
    // return arcTo(mid + dir, _to);

    auto err = segments::arcTo(m_segmentData, _to, _bClockwise);
    if (!err)
        appendedSegment();
    return err;
}

Error Path::arcTo(
    const Vec2f & _to, const Vec2f & _radii, Float _rotation, bool _bClockwise, bool _bLarge)
{
    // STICK_ASSERT(m_segmentData.count());
    // if (crunch::isClose(_radii.x, (Float)0.0) || crunch::isClose(_radii.y, (Float)0.0))
    // {
    //     addPoint(_to);
    //     return Error();
    // }

    // Vec2f from = m_segmentData.last().position;
    // Vec2f middle = (from + _to) * 0.5;
    // Vec2f pt = crunch::rotate(from - middle, -_rotation);
    // Float rx = crunch::abs(_radii.x);
    // Float ry = crunch::abs(_radii.y);
    // // printf("RX, RY %f %f\n", rx, ry);
    // Float rxSq = rx * rx;
    // Float rySq = ry * ry;
    // Float xSq = pt.x * pt.x;
    // Float ySq = pt.y * pt.y;
    // Float factor = std::sqrt(xSq / rxSq + ySq / rySq);
    // if (factor > 1)
    // {
    //     // printf("APPLY FACTOR\n");
    //     rx *= factor;
    //     ry *= factor;
    //     rxSq = rx * rx;
    //     rySq = ry * ry;
    // }
    // // printf("RX2, RY2 %f %f\n", rx, ry);

    // factor = (rxSq * rySq - rxSq * ySq - rySq * xSq) / (rxSq * ySq + rySq * xSq);
    // if (crunch::abs(factor) < detail::PaperConstants::trigonometricEpsilon())
    // {
    //     // printf("SET FACTOR TO 0\n");
    //     factor = 0;
    // }

    // if (factor < 0)
    // {
    //     return Error(ec::InvalidArgument,
    //                  "Cannot create an arc with the given arguments",
    //                  STICK_FILE,
    //                  STICK_LINE);
    // }

    // Vec2f center(rx * pt.y / ry, -ry * pt.x / rx);
    // center = crunch::rotate(center * ((_bLarge == _bClockwise ? -1 : 1) * std::sqrt(factor)),
    //                         _rotation) +
    //          middle;
    // // Now create a matrix that maps the unit circle to the ellipse,
    // // for easier construction below.
    // Mat32f matrix = Mat32f::translation(center);
    // matrix.rotate(_rotation);
    // matrix.scale(_radii);
    // Mat32f inv = crunch::inverse(matrix);
    // Vec2f vect = inv * from;
    // Float extent = crunch::directedAngle(vect, inv * _to);

    // if (!_bClockwise && extent > 0)
    //     extent -= crunch::Constants<Float>::twoPi();
    // else if (_bClockwise && extent < 0)
    //     extent += crunch::Constants<Float>::twoPi();

    // return arcHelper(crunch::toDegrees(extent),
    //                  Segment(this, m_segmentData.count() - 1),
    //                  vect,
    //                  _to,
    //                  center,
    //                  &matrix);

    auto err = segments::arcTo(m_segmentData, _to, _radii, _rotation, _bClockwise, _bLarge);
    if (!err)
        appendedSegment();
    return err;
}

Error Path::arcHelper(Float _extentDeg,
                      Segment _segment,
                      const Vec2f & _direction,
                      const Vec2f & _to,
                      const Vec2f & _center,
                      const Mat32f * _transform)
{
    Float ext = crunch::abs(_extentDeg);
    Int32 count = ext >= 360.0 ? 4 : crunch::ceil(ext / 90.0);
    Float inc = _extentDeg / (Float)count;
    Float half = inc * crunch::Constants<Float>::pi() / 360.0;
    Float z = 4.0 / 3.0 * std::sin(half) / (1.0 + std::cos(half));
    Vec2f dir = _direction;

    for (Int32 i = 0; i <= count; ++i)
    {
        // Explicitely use to point for last segment, since depending
        // on values the calculation adds imprecision:
        Vec2f pt = _to;
        Vec2f out(-dir.y * z, dir.x * z);
        if (i < count)
        {
            if (_transform)
            {
                pt = *_transform * dir;
                out = (*_transform * (dir + out)) - pt;
            }
            else
            {
                pt = _center + dir;
            }
        }
        if (i == 0)
        {
            // Modify startSegment
            _segment.setHandleOut(out);
        }
        else
        {
            // Add new Segment
            Vec2f in(dir.y * z, -dir.x * z);
            if (!_transform)
            {
                createSegment(pt, in, i < count ? out : Vec2f(0.0));
            }
            else
            {
                createSegment(pt, (*_transform * (dir + in)) - pt, i < count ? out : Vec2f(0.0));
            }
        }
        dir = crunch::rotate(dir, crunch::toRadians(inc));
    }
    return Error();
}

void Path::cubicCurveBy(const Vec2f & _handleOne, const Vec2f & _handleTwo, const Vec2f & _by)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();
    // cubicCurveTo(
    //     current.position + _handleOne, current.position + _handleTwo, current.position + _by);
    segments::cubicCurveBy(m_segmentData, _handleOne, _handleTwo, _by);
    appendedSegment();
}

void Path::quadraticCurveBy(const Vec2f & _handle, const Vec2f & _by)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();
    // quadraticCurveTo(current.position + _handle, current.position + _by);
    segments::quadraticCurveBy(m_segmentData, _handle, _by);
    appendedSegment();
}

void Path::curveBy(const Vec2f & _through, const Vec2f & _by, Float _parameter)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();
    // curveTo(current.position + _through, current.position + _by, _parameter);
    segments::curveBy(m_segmentData, _through, _by, _parameter);
    appendedSegment();
}

Error Path::arcBy(const Vec2f & _through, const Vec2f & _by)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();
    // return arcTo(current.position + _through, current.position + _by);
    auto err = segments::arcBy(m_segmentData, _through, _by);
    if (!err)
        appendedSegment();
    return err;
}

Error Path::arcBy(const Vec2f & _to, bool _bClockwise)
{
    // STICK_ASSERT(m_segmentData.count());
    // SegmentData & current = m_segmentData.last();
    // return arcTo(current.position + _to, _bClockwise);

    auto err = segments::arcBy(m_segmentData, _to, _bClockwise);
    if (!err)
        appendedSegment();
    return err;
}

void Path::closePath()
{
    if (isClosed())
        return;

    if (m_segmentData.count() > 1)
    {
        Segment first(this, 0);
        Segment last(this, m_segmentData.count() - 1);

        if (crunch::isClose(first.position(), last.position(), detail::PaperConstants::tolerance()))
        {
            first.setHandleIn(last.handleIn());
            m_segmentData.removeLast();
            m_curveData.last() = CurveData{};
        }
        else
        {
            m_curveData.append(CurveData{});
        }

        m_bIsClosed = true;
        markGeometryDirty(true);
    }
}

void Path::smooth(Smoothing _type, bool _bSmoothChildren)
{
    smooth(0, m_segmentData.count() - 1, _type);
    if (_bSmoothChildren)
    {
        for (Item * c : m_children)
            static_cast<Path *>(c)->smooth(_type, true);
    }
}

static Size smoothIndex(Int64 _idx, Int64 _length, bool _bClosed)
{
    // Handle negative values based on whether a path is open or not:
    // Ranges on closed paths are allowed to wrapped around the
    // beginning/end (e.g. start near the end, end near the beginning),
    // while ranges on open paths stay within the path's open range.
    return std::min(_idx < 0 && _bClosed ? _idx % _length : _idx < 0 ? _idx + _length : _idx,
                    _length - 1);
}

void Path::smooth(Int64 _from, Int64 _to, Smoothing _type)
{
    // Continuous smoothing approach based on work by Lubos Brieda,
    // Particle In Cell Consulting LLC, but further simplified by
    // addressing handle symmetry across segments, and the possibility
    // to process x and y coordinates simultaneously. Also added
    // handling of closed paths.
    // https://www.particleincell.com/2012/bezier-splines/
    //
    // We use different parameters for the two supported smooth methods
    // that use this algorithm: continuous and asymmetric. asymmetric
    // was the only approach available in v0.9.25 & below.

    Size from = smoothIndex(_from, m_segmentData.count(), isClosed());
    Size to = smoothIndex(_to, m_segmentData.count(), isClosed());
    if (from > to)
    {
        if (isClosed())
        {
            from -= m_segmentData.count();
        }
        else
        {
            Size tmp = _from;
            from = _to;
            to = tmp;
        }
    }

    if (_type == Smoothing::Asymmetric || _type == Smoothing::Continuous)
    {
        bool bAsymmetric = _type == Smoothing::Asymmetric;
        Size amount = to - from + 1;
        Size n = amount - 1;
        bool bLoop = isClosed() && _from == 0 && to == m_segmentData.count() - 1;

        // Overlap by up to 4 points on closed paths since a current
        // segment is affected by its 4 neighbors on both sides (?).
        Size padding = bLoop ? min(amount, (Size)4) : 1;
        Size paddingLeft = padding;
        Size paddingRight = padding;

        if (!isClosed())
        {
            // If the path is open and a range is defined, try using a
            // padding of 1 on either side.
            paddingLeft = min((Size)1, from);
            paddingRight = min((Size)1, (Size)m_segmentData.count() - to - 1);
        }

        n += paddingLeft + paddingRight;
        if (n <= 1)
            return;

        DynamicArray<Vec2f> knots(n + 1);

        for (Int64 i = 0, j = _from - paddingLeft; i <= n; i++, j++)
        {
            knots[i] =
                m_segmentData[(j < 0 ? j + m_segmentData.count() : j) % m_segmentData.count()]
                    .position;
        }

        // In the algorithm we treat these 3 cases:
        // - left most segment (L)
        // - internal segments (I)
        // - right most segment (R)
        //
        // In both the continuous and asymmetric method, c takes these
        // values and can hence be removed from the loop starting in n - 2:
        // c = 1 (L), 1 (I), 0 (R)
        //
        // continuous:
        // a = 0 (L), 1 (I), 2 (R)
        // b = 2 (L), 4 (I), 7 (R)
        // u = 1 (L), 4 (I), 8 (R)
        // v = 2 (L), 2 (I), 1 (R)
        //
        // asymmetric:
        // a = 0 (L), 1 (I), 1 (R)
        // b = 2 (L), 4 (I), 2 (R)
        // u = 1 (L), 4 (I), 3 (R)
        // v = 2 (L), 2 (I), 0 (R)

        // (L): u = 1, v = 2
        Float x = knots[0].x + 2 * knots[1].x;
        Float y = knots[0].y + 2 * knots[1].y;
        Float f = 2;
        Float n1 = n - 1;
        DynamicArray<Float> rx(n + 1);
        rx[0] = x;
        DynamicArray<Float> ry(n + 1);
        ry[0] = y;
        DynamicArray<Float> rf(n + 1);
        rf[0] = f;

        DynamicArray<Float> px(n + 1);
        DynamicArray<Float> py(n + 1);

        // Solve with the Thomas algorithm
        for (Size i = 1; i < n; i++)
        {
            bool bInternal = i < n1;
            // internal--(I)  asymmetric--(R) (R)--continuous
            Float a = bInternal ? 1 : bAsymmetric ? 1 : 2;
            Float b = bInternal ? 4 : bAsymmetric ? 2 : 7;
            Float u = bInternal ? 4 : bAsymmetric ? 3 : 8;
            Float v = bInternal ? 2 : bAsymmetric ? 0 : 1;
            Float m = a / f;
            f = rf[i] = b - m;
            x = rx[i] = u * knots[i].x + v * knots[i + 1].x - m * x;
            y = ry[i] = u * knots[i].y + v * knots[i + 1].y - m * y;
        }

        px[n1] = rx[n1] / rf[n1];
        py[n1] = ry[n1] / rf[n1];
        for (Int64 i = n - 2; i >= 0; i--)
        {
            px[i] = (rx[i] - px[i + 1]) / rf[i];
            py[i] = (ry[i] - py[i + 1]) / rf[i];
        }
        px[n] = (3 * knots[n].x - px[n1]) / 2;
        py[n] = (3 * knots[n].y - py[n1]) / 2;

        // Now update the segments
        for (Int64 i = paddingLeft, max = n - paddingRight, j = _from; i <= max; i++, j++)
        {
            Int64 index = j < 0 ? j + m_segmentData.count() : j;
            SegmentData & segment = m_segmentData[index];
            Float hx = px[i] - segment.position.x;
            Float hy = py[i] - segment.position.y;
            if (bLoop || i < max)
            {
                segment.handleOut = Vec2f(hx, hy);
            }
            if (bLoop || i > paddingLeft)
            {
                segment.handleIn = Vec2f(-hx, -hy);
            }
        }

        rebuildCurves();
        markGeometryDirty(true);
    }
}

void Path::simplify(Float _tolerance)
{
    detail::PathFitter fitter(this, _tolerance, true);
    fitter.fit();
}

void Path::addSegment(const Vec2f & _point, const Vec2f & _handleIn, const Vec2f & _handleOut)
{
    // createSegment(_point, _handleIn, _handleOut);
    m_segmentData.append({ _point + _handleIn, _point, _point + _handleOut });
    appendedSegment();
}

void Path::addSegments(const SegmentData * _segments, Size _count)
{
    insertSegments(m_segmentData.count(), _segments, _count);
}

Segment Path::insertSegment(Size _index, const SegmentData & _seg)
{
    insertSegments(_index, &_seg, 1);
    return Segment(this, _index);
}

void Path::swapSegments(SegmentDataArray & _segments, bool _bClose)
{
    m_bIsClosed = _bClose;
    m_segmentData.swap(_segments);
    rebuildCurves();
    markGeometryDirty(true);
}

void Path::insertSegments(Size _index, const SegmentData * _segments, Size _count)
{
    // append case
    if (_index >= m_segmentData.count())
    {
        m_segmentData.insert(m_segmentData.end(), _segments, _segments + _count);

        if (m_segmentData.count() > 1)
        {
            if (isClosed() && m_curveData.count())
                m_curveData.removeLast();

            m_curveData.reserve(m_curveData.count() + _count + 1);
            for (Size i = 0; i < std::min(_count, m_segmentData.count() - 1); ++i)
                m_curveData.append(CurveData{});

            if (isClosed())
                m_curveData.append(CurveData{});
        }
    }
    // insert case
    else
    {
        m_segmentData.insert(m_segmentData.begin() + _index, _segments, _segments + _count);

        // insert new curves and reset all affected ones
        //@TODO: technically we should be able to use an end index, too no?
        m_curveData.resize(m_curveData.count() + _count);
        auto cit = m_curveData.begin() + _index;
        for (; cit != m_curveData.end(); ++cit)
            *cit = CurveData{};
    }

    markGeometryDirty(true);
}

void Path::removeSegment(Size _index)
{
    removeSegments(_index, _index + 1);
}

void Path::removeSegments(Size _from)
{
    removeSegments(_from, m_segmentData.count());
}

void Path::removeSegments(Size _from, Size _to)
{
    STICK_ASSERT(_from < m_segmentData.count());
    STICK_ASSERT(_to < m_segmentData.count());
    m_segmentData.remove(m_segmentData.begin() + _from, m_segmentData.begin() + _to);
    //@TODO: Instead of rebuilding all curves only update the ones directly affected?
    rebuildCurves();
    markGeometryDirty(true);
}

void Path::removeSegments()
{
    m_bIsClosed = false;
    m_segmentData.clear();
    m_curveData.clear();
    markGeometryDirty(true);
}

Segment Path::createSegment(const Vec2f & _pos, const Vec2f & _handleIn, const Vec2f & _handleOut)
{
    return insertSegment(m_segmentData.count(),
                         SegmentData{ _pos + _handleIn, _pos, _pos + _handleOut });
}

void Path::appendedSegment()
{
    if (m_segmentData.count() > 1)
        m_curveData.append(CurveData{});
    markGeometryDirty(true);
}

void Path::reverse()
{
    // TODO: Can be optimized
    for (auto & seg : m_segmentData)
    {
        std::swap(seg.handleIn, seg.handleOut);
    }
    std::reverse(m_segmentData.begin(), m_segmentData.end());

    for (Item * c : this->children())
        if (c->itemType() == ItemType::Path)
            static_cast<Path *>(c)->reverse();

    rebuildCurves();
    markGeometryDirty(true);
}

void Path::setClockwise(bool _b)
{
    if (isClockwise() != _b)
        reverse();
}

void Path::flatten(Float _angleTolerance,
                   bool _bFlattenChildren,
                   Float _minDistance,
                   Size _maxRecursion)
{
    detail::PathFlattener::PositionArray newSegmentPositions(m_segmentData.allocator());
    newSegmentPositions.reserve(1024);
    detail::PathFlattener::flatten(
        this, newSegmentPositions, nullptr, _angleTolerance, _minDistance, _maxRecursion);

    SegmentDataArray segs(newSegmentPositions.count(), m_segmentData.allocator());
    for (Size i = 0; i < newSegmentPositions.count(); ++i)
        segs[i] = SegmentData{ Vec2f(0), newSegmentPositions[i], Vec2f(0) };

    swapSegments(segs, isClosed());

    if (_bFlattenChildren)
    {
        for (Item * c : m_children)
            static_cast<Path *>(c)->flatten(_angleTolerance, true, _minDistance, _maxRecursion);
    }
}

void Path::flattenRegular(Float _maxDistance, bool _bFlattenChildren)
{
    SegmentDataArray segs(m_segmentData.allocator());
    segs.reserve(m_segmentData.count() * 2);
    // auto stepAndSampleCount = regularOffsetAndSampleCount(_maxDistance);
    // Float step = stepAndSampleCount.offset;
    // Float currentOffset = 0;
    // Float len = length();
    // for (Int32 i = 0; i < stepAndSampleCount.sampleCount; ++i)
    // {
    //     segs.append({Vec2f(0), positionAt(std::min(currentOffset, len)), Vec2f(0)});
    //     // Float delta = len - currentOffset;
    //     // if (delta < step)
    //     //     step = delta * 0.5;
    //     currentOffset += step;
    // }

    Float len = length();
    Float off = 0;
    while (off < len)
    {
        segs.append({ Vec2f(0), positionAt(std::min(off, len)), Vec2f(0) });
        off += _maxDistance;
    }

    if (off - len > 0)
        segs.append({ Vec2f(0), positionAt(len), Vec2f(0) });

    swapSegments(segs, isClosed());

    if (_bFlattenChildren)
    {
        for (Item * c : m_children)
            static_cast<Path *>(c)->flattenRegular(_maxDistance, true);
    }
}

Path::OffsetAndSampleCount Path::regularOffsetAndSampleCount(Float _maxDistance)
{
    Float len = length();
    Size sampleCount = std::ceil(len / _maxDistance);
    return { std::min(len, len / (Float)sampleCount),
             /*isClosed() ? sampleCount - 1 :*/ sampleCount };
}

Float Path::regularOffset(Float _maxDistance)
{
    return regularOffsetAndSampleCount(_maxDistance).offset;
}

SegmentView Path::segments()
{
    return SegmentView(this, m_segmentData.begin(), &m_segmentData);
}

CurveView Path::curves()
{
    return CurveView(this, m_curveData.begin(), &m_curveData);
}

ConstSegmentView Path::segments() const
{
    return ConstSegmentView(this, m_segmentData.begin(), &m_segmentData);
}

ConstCurveView Path::curves() const
{
    return ConstCurveView(this, m_curveData.begin(), &m_curveData);
}

const SegmentDataArray & Path::segmentData() const
{
    return m_segmentData;
}

const CurveDataArray & Path::curveData() const
{
    return m_curveData;
}

Vec2f Path::positionAt(Float _offset) const
{
    return curveLocationAt(_offset).position();
}

Vec2f Path::normalAt(Float _offset) const
{
    return curveLocationAt(_offset).normal();
}

Vec2f Path::tangentAt(Float _offset) const
{
    return curveLocationAt(_offset).tangent();
}

Float Path::curvatureAt(Float _offset) const
{
    return curveLocationAt(_offset).curvature();
}

Float Path::angleAt(Float _offset) const
{
    return curveLocationAt(_offset).angle();
}

//@TODO: BIG FAT TODO
Path * Path::splitAt(Float _offset)
{
    STICK_ASSERT(false);
    return nullptr;
}

Path * Path::splitAt(CurveLocation _loc)
{
    STICK_ASSERT(false);
    return nullptr;
}

Path * Path::slice(Float _from, Float _to) const
{
    return slice(curveLocationAt(_from), curveLocationAt(_to));
}

Path * Path::slice(CurveLocation _from, CurveLocation _to) const
{
    STICK_ASSERT(_from.isValid());
    STICK_ASSERT(_to.isValid());
    STICK_ASSERT(_from.curve().path() == _to.curve().path());

    if (_from == _to)
        return nullptr;

    //@TODO: is this cloning good enough?
    Path * ret = clone();
    ret->m_segmentData.clear();
    ret->m_curveData.clear();
    ret->m_bIsClosed = false;

    // add the first segment based on the start curve location
    Bezier bez, bez2;
    if (_from.curve().m_index != _to.curve().m_index)
    {
        bez = _from.curve().bezier().slice(_from.parameter(), 1);
        bez2 = _to.curve().bezier().slice(0, _to.parameter());
    }
    else
        bez = bez2 = _from.curve().bezier().slice(_from.parameter(), _to.parameter());

    SegmentDataArray tmp(m_segmentData.allocator());
    tmp.reserve(_to.curve().segmentOne().m_index - _from.curve().segmentTwo().m_index + 1);
    tmp.append({ Vec2f(0.0), bez.positionOne(), bez.handleOne() - bez.positionOne() });

    // add all the segments inbetween
    for (Size i = _from.curve().segmentTwo().m_index; i <= _to.curve().segmentOne().m_index; ++i)
    {
        Segment seg = segment(i);
        Vec2f handleIn = seg.handleIn();
        Vec2f handleOut = seg.handleOut();

        if (i == _from.curve().segmentTwo().m_index && i == _to.curve().segmentOne().m_index)
        {
            handleIn = bez.handleTwo() - bez.positionTwo();
            handleOut = bez2.handleOne() - bez2.positionOne();
        }
        else if (i == _from.curve().segmentTwo().m_index)
        {
            handleIn = bez.handleTwo() - bez.positionTwo();
        }
        else if (i == _to.curve().segmentOne().m_index)
        {
            handleOut = bez2.handleOne() - bez2.positionOne();
        }

        tmp.append({ handleIn, seg.position(), handleOut });
    }

    // add the last segment based on the end curve location
    tmp.append({ bez2.handleTwo() - bez2.positionTwo(), bez2.positionTwo(), Vec2f(0.0f) });

    ret->swapSegments(tmp, false);

    ret->insertAbove(this);
    return ret;
}

CurveLocation Path::closestCurveLocation(const Vec2f & _point, Float & _outDistance) const
{
    Float minDist = std::numeric_limits<Float>::infinity();
    Float currentDist;
    Float closestParameter;
    Float currentParameter;

    auto cv = curves();
    auto closestCurve = cv.end();

    auto it = cv.begin();
    for (; it != cv.end(); ++it)
    {
        currentParameter = (*it).closestParameter(_point, currentDist);
        if (currentDist < minDist)
        {
            minDist = currentDist;
            closestParameter = currentParameter;
            closestCurve = it;
        }
    }

    if (closestCurve != cv.end())
    {
        _outDistance = minDist;
        return (*closestCurve).curveLocationAtParameter(closestParameter);
    }

    return CurveLocation();
}

CurveLocation Path::closestCurveLocation(const Vec2f & _point) const
{
    Float tmp;
    return closestCurveLocation(_point, tmp);
}

CurveLocation Path::curveLocationAt(Float _offset) const
{
    Float len = 0;
    Float start;

    auto cv = curves();
    auto it = cv.begin();

    for (; it != cv.end(); ++it)
    {
        start = len;
        len += (*it).length();

        // we found the curve
        if (len >= _offset)
        {
            return (*it).curveLocationAt(_offset - start);
        }
    }

    // comment from paper.js source in Path.js:
    // It may be that through impreciseness of getLength (length) , that the end
    // of the curves was missed:
    if (_offset <= length())
    {
        return ConstCurve(this, m_curveData.count() - 1).curveLocationAtParameter(1);
    }

    return CurveLocation();
}

Float Path::length() const
{
    if (!m_length)
    {
        Float len = 0.0f;
        for (Size i = 0; i < m_curveData.count(); ++i)
        {
            ConstCurve c = curve(i);
            len += c.length();
        }
        m_length = len;
    }
    return *m_length;
}

void Path::peaks(stick::DynamicArray<CurveLocation> & _peaks) const
{
    DynamicArray<Float> tmp(m_segmentData.allocator());
    for (ConstCurve c : curves())
    {
        c.peaks(tmp);
        for (auto p : tmp)
            _peaks.append(c.curveLocationAtParameter(p));
    }
}

void Path::extrema(stick::DynamicArray<CurveLocation> & _extrema) const
{
    DynamicArray<Float> tmp(m_segmentData.allocator());
    for (ConstCurve c : curves())
    {
        c.extrema(tmp);
        for (auto p : tmp)
            _extrema.append(c.curveLocationAtParameter(p));
    }
}

stick::DynamicArray<CurveLocation> Path::extrema() const
{
    DynamicArray<CurveLocation> ret(m_segmentData.allocator());
    extrema(ret);
    return ret;
}

Float Path::area() const
{
    Float ret = 0;
    for (ConstCurve c : curves())
    {
        ret += c.area();
    }

    // take children into account
    for (Item * c : this->children())
    {
        if (c->itemType() == ItemType::Path)
            ret += static_cast<Path *>(c)->area();
    }

    return ret;
}

bool Path::isClosed() const
{
    return m_bIsClosed;
}

bool Path::isPolygon() const
{
    for (ConstSegment seg : segments())
    {
        STICK_ASSERT(seg.m_path == this);
        if (!seg.isLinear())
            return false;
    }
    return true;
}

bool Path::isClockwise() const
{
    //@TODO: I think we need to ignore children for this to worl reliably
    return area() >= 0;
}

void Path::rebuildCurves()
{
    m_curveData.clear();
    m_curveData.resize(m_segmentData.count() - 1, CurveData{});
    if (isClosed())
    {
        m_bIsClosed = false;
        closePath();
    }
}

namespace detail
{
// helpers for updateStrokeBounds
static void mergeStrokeCap(Rect & _rect,
                           StrokeCap _cap,
                           const SegmentData & _a,
                           const SegmentData & _b,
                           bool _bStart,
                           const Vec2f & _strokePadding,
                           const Mat32f & _strokeMat,
                           const Mat32f * _transform)
{
    Bezier curve(_a.position, _a.handleOut, _b.handleIn, _b.position);
    Vec2f dir = _bStart ? -curve.tangentAt(0) : curve.tangentAt(1);
    Vec2f pos = _bStart ? _a.position : _b.position;
    switch (_cap)
    {
    case StrokeCap::Square:
    {
        Vec2f a, b, c, d;
        detail::capSquare(pos, dir, a, b, c, d);
        c = _strokeMat * c;
        d = _strokeMat * d;
        _rect = crunch::merge(_rect, _transform ? *_transform * c : c);
        _rect = crunch::merge(_rect, _transform ? *_transform * d : d);
        break;
    }
    case StrokeCap::Round:
    {
        Vec2f p = _strokeMat * pos;
        if (_transform)
        {
            p = *_transform * p;
        }
        Rect r(p - _strokePadding, p + _strokePadding);
        _rect = crunch::merge(_rect, r);
        break;
    }
    case StrokeCap::Butt:
    {
        Vec2f min, max;
        detail::capOrJoinBevelMinMax(pos, dir, min, max);
        min = _strokeMat * min;
        max = _strokeMat * max;
        _rect = crunch::merge(_rect, _transform ? *_transform * min : min);
        _rect = crunch::merge(_rect, _transform ? *_transform * max : max);
        break;
    }
    default:
        break;
    }
}

static void mergeStrokeJoin(Rect & _rect,
                            StrokeJoin _join,
                            Float _miterLimit,
                            const SegmentData & _prev,
                            const SegmentData & _current,
                            const SegmentData & _next,
                            const Vec2f & _strokePadding,
                            const Mat32f & _strokeMat,
                            const Mat32f * _transform)
{
    switch (_join)
    {
    case StrokeJoin::Round:
    {
        Vec2f p = _strokeMat * _current.position;
        if (_transform)
        {
            p = *_transform * p;
        }
        Rect r(p - _strokePadding, p + _strokePadding);
        _rect = crunch::merge(_rect, r);
        break;
    }
    case StrokeJoin::Miter:
    {
        Bezier curveIn(_prev.position, _prev.handleOut, _current.handleIn, _current.position);
        Bezier curveOut(_current.position, _current.handleOut, _next.handleIn, _next.position);

        // to know which side of the stroke is outside
        Vec2f lastDir = curveIn.tangentAt(1);
        Vec2f nextDir = curveOut.tangentAt(0);
        Vec2f lastPerp(lastDir.y, -lastDir.x);
        Vec2f perp(nextDir.y, -nextDir.x);
        Float cross = lastDir.x * nextDir.y - nextDir.x * lastDir.y;

        Vec2f miter;
        Float miterLen;
        Vec2f pos = _current.position;

        if (cross >= 0.0)
        {
            miter = detail::joinMiter(pos, pos + lastPerp, lastDir, pos + perp, nextDir, miterLen);
        }
        else
        {
            miter = detail::joinMiter(pos, pos - lastPerp, lastDir, pos - perp, nextDir, miterLen);
        }

        if (miterLen <= _miterLimit)
        {
            miter = _strokeMat * miter;
            _rect = crunch::merge(_rect, _transform ? *_transform * miter : miter);
            break;
        }
        // else fall back to Bevel
    }
    case StrokeJoin::Bevel:
    {
        Bezier curveIn(_prev.position, _prev.handleOut, _current.handleIn, _current.position);
        Bezier curveOut(_current.position, _current.handleOut, _next.handleIn, _next.position);

        Vec2f min, max;
        Vec2f dirA = curveIn.tangentAt(1);
        Vec2f dirB = curveOut.tangentAt(0);
        detail::capOrJoinBevelMinMax(_current.position, dirA, min, max);
        min = _strokeMat * min;
        max = _strokeMat * max;

        _rect = crunch::merge(_rect, _transform ? *_transform * min : min);
        _rect = crunch::merge(_rect, _transform ? *_transform * max : max);
        detail::capOrJoinBevelMinMax(_current.position, dirB, min, max);
        min = _strokeMat * min;
        max = _strokeMat * max;
        _rect = crunch::merge(_rect, _transform ? *_transform * min : min);
        _rect = crunch::merge(_rect, _transform ? *_transform * max : max);
        break;
    }
    default:
        break;
    }
}
} // namespace detail

bool Path::contains(const Vec2f & _point) const
{
    if (!handleBounds().contains(_point))
        return false;

    if (windingRule() == WindingRule::EvenOdd)
        return detail::BooleanOperations::winding(
                   _point, detail::BooleanOperations::monoCurves(this), false) &
               1;
    else
        return detail::BooleanOperations::winding(
                   _point, detail::BooleanOperations::monoCurves(this), false) > 0;
}

Path * Path::clone() const
{
    Path * ret = m_document->createPath(m_name.cString() ? m_name.cString() : "");

    // clone path specific things
    ret->m_segmentData = m_segmentData;
    ret->m_curveData = m_curveData;
    ret->m_bGeometryDirty = m_bGeometryDirty;
    ret->m_bIsClosed = m_bIsClosed;
    ret->m_length = m_length;

    // clone properties and children
    cloneItemTo(ret);

    return ret;
}

Curve Path::curve(Size _index)
{
    STICK_ASSERT(_index < m_curveData.count());
    return Curve(this, _index);
}

const Curve Path::curve(Size _index) const
{
    STICK_ASSERT(_index < m_curveData.count());
    return Curve(const_cast<Path *>(this), _index);
}

Segment Path::segment(Size _index)
{
    STICK_ASSERT(_index < m_segmentData.count());
    return Segment(this, _index);
}

const Segment Path::segment(Size _index) const
{
    STICK_ASSERT(_index < m_segmentData.count());
    return Segment(const_cast<Path *>(this), _index);
}

Size Path::curveCount() const
{
    return m_curveData.count();
}

Size Path::segmentCount() const
{
    return m_segmentData.count();
}

namespace detail
{
static inline bool isAdjacentCurve(Size _a, Size _b, Size _curveCount, bool _bIsClosed)
{
    if (_b == _a + 1 || (_bIsClosed && _a == 0 && _b == _curveCount - 1))
        return true;
    return false;
}

static inline void intersectPaths(const Path * _self,
                                  const Path * _other,
                                  IntersectionArray & _intersections)
{
    bool bSelf = _self == _other;

    for (Size i = 0; i < _self->curveCount(); ++i)
    {
        ConstCurve a = _self->curve(i);
        for (Size j = bSelf ? i + 1 : 0; j < _other->curveCount(); ++j)
        {
            ConstCurve b = _other->curve(j);
            auto intersections = a.bezier().intersections(b.bezier());
            for (Int32 z = 0; z < intersections.count; ++z)
            {
                bool bAdd = true;
                // for self intersection we only add the intersection if its not where
                // adjacent curves connect.
                if (bSelf)
                {
                    if (isAdjacentCurve(i, j, _self->curveCount(), _self->isClosed()))
                    {
                        if ((crunch::isClose(intersections.values[z].parameterOne,
                                             1.0f,
                                             detail::PaperConstants::curveTimeEpsilon()) &&
                             crunch::isClose(intersections.values[z].parameterTwo,
                                             0.0f,
                                             detail::PaperConstants::curveTimeEpsilon())) ||
                            // this case can only happen for closed paths where the first curve
                            // meets the last one
                            (_self->isClosed() &&
                             crunch::isClose(intersections.values[z].parameterOne,
                                             0.0f,
                                             detail::PaperConstants::curveTimeEpsilon()) &&
                             crunch::isClose(intersections.values[z].parameterTwo,
                                             1.0f,
                                             detail::PaperConstants::curveTimeEpsilon())))
                        {
                            bAdd = false;
                        }
                    }
                }

                if (bAdd)
                {
                    // @TODO: make sure we don't add an intersection twice. This can happen if the
                    // intersection is located between two adjacent curves of the path.
                    CurveLocation cl =
                        a.curveLocationAtParameter(intersections.values[z].parameterOne);
                    for (auto & isec : _intersections)
                    {
                        if (cl.isSynonymous(isec.location))
                        {
                            bAdd = false;
                            break;
                        }
                    }
                    if (bAdd)
                    {
                        _intersections.append({ cl, intersections.values[z].position });
                    }
                }
            }
        }
    }
}

// helper to recursively intersect paths and its children (compound path)
static inline void recursivelyIntersect(const Path * _self,
                                        const Path * _other,
                                        IntersectionArray & _intersections)
{
    intersectPaths(_self, _other, _intersections);

    for (Item * c : _other->children())
        recursivelyIntersect(_self, static_cast<Path *>(c), _intersections);
}

static inline void flattenPathChildren(const Path * _path,
                                       stick::DynamicArray<const Path *> & _outPaths)
{
    _outPaths.append(_path);
    for (Item * c : _path->children())
        flattenPathChildren(static_cast<Path *>(c), _outPaths);
}
} // namespace detail

IntersectionArray Path::intersections() const
{
    IntersectionArray ret(m_segmentData.allocator());
    intersectionsImpl(this, ret);
    return ret;
}

IntersectionArray Path::intersections(const Path * _other) const
{
    if (!bounds().overlaps(_other->bounds()))
        return IntersectionArray();
    IntersectionArray ret(m_segmentData.allocator());
    intersectionsImpl(_other, ret);
    return ret;
}

void Path::intersectionsImpl(const Path * _other, IntersectionArray & _outIntersections) const
{
    // @TODO: Take transformation matrix into account!!
    if (this != _other)
    {
        detail::recursivelyIntersect(this, _other, _outIntersections);

        for (Size i = 0; i < m_children.count(); ++i)
            detail::recursivelyIntersect(
                static_cast<Path *>(m_children[i]), _other, _outIntersections);
    }
    else
    {
        // for self intersection we create a flat list of all nested paths to avoid double
        // comparisons
        stick::DynamicArray<const Path *> paths(m_children.allocator());
        paths.reserve(16);
        detail::flattenPathChildren(this, paths);
        for (Size i = 0; i < paths.count(); ++i)
            for (Size j = i + 1; j < paths.count(); ++j)
                detail::intersectPaths(paths[i], paths[j], _outIntersections);
    }
}

void Path::markGeometryDirty(bool _bMarkLengthDirty, bool _bMarkParentsBoundsDirty)
{
    m_bGeometryDirty = true;
    markBoundsDirty(_bMarkParentsBoundsDirty);
    if (_bMarkLengthDirty)
        m_length.reset();
    m_monoCurves.clear();
    Item::markSymbolsDirty();
}

void Path::transformChanged(bool _bCalledFromParent)
{
    Item::transformChanged(_bCalledFromParent);
    m_monoCurves.clear();
}

bool Path::canAddChild(Item * _e) const
{
    return _e->itemType() == ItemType::Path;
}

void Path::addedChild(Item * _e)
{
    // for non zero winding rule we adjust the direction of the added path if needed
    // @TODO?????
    // if (windingRule() == WindingRule::NonZero)
    //     static_cast<Path *>(_e)->setClockwise(!isClockwise());
}

static Mat32f strokeTransformHelper(const Mat32f & _transform,
                                    Float _strokeWidth,
                                    bool _bIsScalingStroke)
{
    Float hsw = _strokeWidth * 0.5;
    Mat32f tmp;
    if (_bIsScalingStroke)
        tmp = Mat32f::scaling(hsw);
    else
    {
        tmp = crunch::inverse(_transform);
        tmp.scale(hsw);
    }

    return tmp;
}

static Vec2f strokePadding(Float _strokeWidth, const Mat32f & _mat)
{
    // If a matrix is provided, we need to rotate the stroke circle
    // and calculate the bounding box of the resulting rotated elipse:
    // Get rotated hor and ver vectors, and determine rotation angle
    // and elipse values from them:
    Mat32f shiftless(_mat[0], _mat[1], Vec2f(0));

    Vec2f hor = shiftless * Vec2f(_strokeWidth, 0.0f);
    Vec2f vert = shiftless * Vec2f(0.0f, _strokeWidth);
    Float phi = atan2(hor.y, hor.x);
    Float hlen = crunch::length(hor);
    Float vlen = crunch::length(vert);
    // Formula for rotated ellipses:
    // x = cx + a*cos(t)*cos(phi) - b*sin(t)*sin(phi)
    // y = cy + b*sin(t)*cos(phi) + a*cos(t)*sin(phi)
    // Derivates (by Wolfram Alpha):
    // derivative of x = cx + a*cos(t)*cos(phi) - b*sin(t)*sin(phi)
    // dx/dt = a sin(t) cos(phi) + b cos(t) sin(phi) = 0
    // derivative of y = cy + b*sin(t)*cos(phi) + a*cos(t)*sin(phi)
    // dy/dt = b cos(t) cos(phi) - a sin(t) sin(phi) = 0
    // This can be simplified to:
    // tan(t) = -b * tan(phi) / a // x
    // tan(t) =  b * cot(phi) / a // y
    // Solving for t gives:
    // t = pi * n - arctan(b * tan(phi) / a) // x
    // t = pi * n + arctan(b * cot(phi) / a)
    //   = pi * n + arctan(b / tan(phi) / a) // y
    Float s = std::sin(phi);
    Float c = std::cos(phi);
    Float t = std::tan(phi);
    Float tx = std::atan2(vlen * t, hlen);
    Float ty = std::atan2(vlen, t * hlen);
    return crunch::abs(Vec2f(hlen * std::cos(tx) * c + vlen * std::sin(tx) * s,
                             vlen * std::sin(ty) * c + hlen * std::cos(ty) * s));
}

Maybe<Rect> Path::computeFillBounds(const Mat32f * _transform, Float _padding) const
{
    if (!m_segmentData.count())
        return Maybe<Rect>();

    if (m_segmentData.count() == 1)
    {
        Vec2f p = _transform ? *_transform * m_segmentData[0].position : m_segmentData[0].position;
        return Rect(p, p);
    }

    Rect ret;
    if (!_transform && !isTransformed())
    {
        for (Size i = 0; i < m_curveData.count(); ++i)
        {
            ConstCurve c = curve(i);
            if (i == 0)
                ret = _padding > 0 ? c.bounds(_padding) : c.bounds();
            else
                ret = crunch::merge(ret, _padding > 0 ? c.bounds(_padding) : c.bounds());
        }
    }
    else
    {
        // if not, we have to bring the beziers to document space in order
        // to calculate the bounds.

        // we iterate over segments instead so we only do the matrix multiplication
        // once for each segment.

        _transform = _transform ? _transform : &absoluteTransform();

        ConstSegment seg(this, 0);
        Vec2f lastPosition = *_transform * seg.position();
        Vec2f firstPosition = lastPosition;
        Vec2f lastHandle = *_transform * seg.handleOutAbsolute();
        Vec2f currentPosition, handleIn;
        for (Size i = 1; i < m_segmentData.count(); ++i)
        {
            seg = segment(i);
            handleIn = *_transform * seg.handleInAbsolute();
            currentPosition = *_transform * seg.position();

            Bezier bez(lastPosition, lastHandle, handleIn, currentPosition);
            if (i == 1)
                ret = bez.bounds(_padding);
            else
                ret = crunch::merge(ret, bez.bounds(_padding));

            lastHandle = *_transform * seg.handleOutAbsolute();
            lastPosition = currentPosition;
        }

        if (isClosed())
        {
            Bezier bez(lastPosition,
                       lastHandle,
                       *_transform * ConstSegment(this, 0).handleInAbsolute(),
                       firstPosition);
            ret = crunch::merge(ret, bez.bounds(_padding));
        }
    }
    return ret;
}

Maybe<Rect> Path::computeStrokeBounds(const Mat32f * _transform) const
{
    if (!hasStroke())
        return computeFillBounds(_transform, 0);

    StrokeJoin join = strokeJoin();
    StrokeCap cap = strokeCap();
    Float sw = strokeWidth();
    Float strokeRad = sw * 0.5;
    Float ml = miterLimit();
    bool bIsScalingStroke = scaleStroke();
    //@TODO: only do the stroke transform / padding work if there is a matrix
    _transform = _transform ? _transform : isTransformed() ? &absoluteTransform() : nullptr;
    Mat32f smat =
        strokeTransformHelper(_transform ? *_transform : absoluteTransform(), sw, bIsScalingStroke);
    Vec2f sp = strokePadding(strokeRad,
                             bIsScalingStroke ? _transform ? *_transform : absoluteTransform()
                                              : Mat32f::identity());

    //@TODO: use proper 2D padding for non uniformly transformed strokes?
    auto result = computeFillBounds(_transform, std::max(sp.x, sp.y));

    // if there is no bounds, we are done
    if (!result)
        return result;

    Mat32f ismat = crunch::inverse(smat);

    SegmentDataArray strokeSegs(m_segmentData.count(), m_segmentData.allocator());
    for (Size i = 0; i < strokeSegs.count(); ++i)
    {
        strokeSegs[i] = { ismat * (m_segmentData[i].position + m_segmentData[i].handleIn),
                          ismat * m_segmentData[i].position,
                          ismat * (m_segmentData[i].position + m_segmentData[i].handleOut) };
    }

    for (Size i = 1; i < m_segmentData.count(); ++i)
    {
        detail::mergeStrokeJoin(*result,
                                join,
                                ml,
                                strokeSegs[i - 1],
                                strokeSegs[i],
                                strokeSegs[i + 1],
                                sp,
                                smat,
                                _transform);
    }

    if (isClosed())
    {
        // closing join
        detail::mergeStrokeJoin(*result,
                                join,
                                ml,
                                strokeSegs[strokeSegs.count() - 1],
                                strokeSegs[0],
                                strokeSegs[1],
                                sp,
                                smat,
                                _transform);
    }
    else
    {
        // caps
        detail::mergeStrokeCap(
            *result, cap, strokeSegs[0], strokeSegs[1], true, sp, smat, _transform);
        detail::mergeStrokeCap(*result,
                               cap,
                               strokeSegs[strokeSegs.count() - 2],
                               strokeSegs[strokeSegs.count() - 1],
                               false,
                               sp,
                               smat,
                               _transform);
    }

    // return the merged box;
    return result;
}

Maybe<Rect> Path::computeHandleBounds(const Mat32f * _transform) const
{
    auto ret = computeStrokeBounds(_transform);

    if (!ret)
        return ret;

    if (_transform)
    {
        for (auto & seg : m_segmentData)
        {
            ret = crunch::merge(*ret, *_transform * (seg.position + seg.handleIn));
            ret = crunch::merge(*ret, *_transform * (seg.position + seg.handleOut));
        }
    }
    else
    {
        for (auto & seg : m_segmentData)
        {
            ret = crunch::merge(*ret, seg.position + seg.handleIn);
            ret = crunch::merge(*ret, seg.position + seg.handleIn);
        }
    }

    return ret;
}

Maybe<Rect> Path::computeBounds(const Mat32f * _transform, BoundsType _type) const
{
    Maybe<Rect> ret;
    if (_type == BoundsType::Fill)
        ret = computeFillBounds(_transform, 0);
    else if (_type == BoundsType::Stroke)
        ret = computeStrokeBounds(_transform);
    else if (_type == BoundsType::Handle)
        ret = computeHandleBounds(_transform);

    return mergeWithChildrenBounds(ret, _transform, _type);
}

void Path::applyTransformToSegment(Size _index, const Mat32f & _transform)
{
    SegmentData & segData = m_segmentData[_index];
    segData.handleIn = _transform * (segData.position + segData.handleIn);
    segData.handleOut = _transform * (segData.position + segData.handleOut);
    segData.position = _transform * segData.position;
    segData.handleIn -= segData.position;
    segData.handleOut -= segData.position;
}

void Path::applyTransform(const Mat32f & _transform, bool _bMarkParentsBoundsDirty)
{
    for (Size i = 0; i < m_segmentData.count(); ++i)
        applyTransformToSegment(i, _transform);

    for (Size i = 0; i < m_curveData.count(); ++i)
        m_curveData[i] = CurveData{};

    // mark the geometry, length and bounds of the path dirty
    markGeometryDirty(true, _bMarkParentsBoundsDirty);

    // apply the transform to the children
    applyTransformToChildrenAndPivot(_transform);
}

bool Path::cleanDirtyGeometry()
{
    bool ret = m_bGeometryDirty;
    m_bGeometryDirty = false;
    return ret;
}
} // namespace paper
