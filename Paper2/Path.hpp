#ifndef PAPER_PATH_HPP
#define PAPER_PATH_HPP

#include <Paper2/Item.hpp>
#include <Paper2/Private/BooleanOperations.hpp>
#include <Paper2/Private/ContainerView.hpp>

namespace paper
{
class Path;
class CurveLocation;

template <class PT>
class CurveT;

template <class PT>
class STICK_API SegmentT
{
    friend class Path;
    template <class PT2>
    friend class SegmentT;

  public:
    SegmentT(PT * _path = nullptr, Size _idx = -1);

    template <class OPT>
    SegmentT(const SegmentT<OPT> & _other);

    SegmentT(const SegmentT &) = default;
    SegmentT(SegmentT &&) = default;
    SegmentT & operator=(const SegmentT &) = default;
    SegmentT & operator=(SegmentT &&) = default;

    explicit operator bool() const;

    void setPosition(const Vec2f & _pos);

    void setHandleIn(const Vec2f & _pos);

    void setHandleOut(const Vec2f & _pos);

    const Vec2f & position() const;

    Vec2f handleIn() const;

    Vec2f handleOut() const;

    const Vec2f & handleInAbsolute() const;

    const Vec2f & handleOutAbsolute() const;

    CurveT<PT> curveIn() const;

    CurveT<PT> curveOut() const;

    bool isLinear() const;

    void remove();

    void transform(const Mat32f & _transform);

    Size index() const;

  private:
    void segmentChanged();

    PT * m_path;
    Size m_index;
};

template <class PT>
class STICK_API CurveT
{
    friend class Path;
    template <class PT2>
    friend class SegmentT;
    template <class PT2>
    friend class CurveT;
    friend class CurveLocation;

  public:
    CurveT();

    CurveT(PT * _path, Size _index);

    template <class OPT>
    CurveT(const CurveT<OPT> & _other);

    CurveT(const CurveT &) = default;
    CurveT(CurveT &&) = default;
    CurveT & operator=(const CurveT &) = default;
    CurveT & operator=(CurveT &&) = default;

    explicit operator bool() const;

    PT * path() const;

    void setPositionOne(const Vec2f & _vec);

    void setHandleOne(const Vec2f & _vec);

    void setPositionTwo(const Vec2f & _vec);

    void setHandleTwo(const Vec2f & _vec);

    const Vec2f & positionOne() const;

    const Vec2f & positionTwo() const;

    Vec2f handleOne() const;

    const Vec2f & handleOneAbsolute() const;

    Vec2f handleTwo() const;

    const Vec2f & handleTwoAbsolute() const;

    SegmentT<PT> segmentOne() const;

    SegmentT<PT> segmentTwo() const;

    Vec2f positionAt(Float _offset) const;

    Vec2f normalAt(Float _offset) const;

    Vec2f tangentAt(Float _offset) const;

    Float curvatureAt(Float _offset) const;

    Float angleAt(Float _offset) const;

    stick::Maybe<CurveT> divideAt(Float _offset);

    Vec2f positionAtParameter(Float _t) const;

    Vec2f normalAtParameter(Float _t) const;

    Vec2f tangentAtParameter(Float _t) const;

    Float curvatureAtParameter(Float _t) const;

    Float angleAtParameter(Float _t) const;

    stick::Maybe<CurveT> divideAtParameter(Float _t);

    Float parameterAtOffset(Float _offset) const;

    Float closestParameter(const Vec2f & _point) const;

    Float closestParameter(const Vec2f & _point, Float & _outDistance) const;

    Float lengthBetween(Float _tStart, Float _tEnd) const;

    Float pathOffset() const;

    void peaks(stick::DynamicArray<Float> & _peaks) const;

    void extrema(stick::DynamicArray<Float> & _extrema) const;

    CurveLocation closestCurveLocation(const Vec2f & _point) const;

    CurveLocation curveLocationAt(Float _offset) const;

    CurveLocation curveLocationAtParameter(Float _t) const;

    bool isLinear() const;

    bool isStraight() const;

    bool isArc() const;

    bool isOrthogonal(const CurveT & _other) const;

    bool isCollinear(const CurveT & _other) const;

    Float length() const;

    Float area() const;

    const Rect & bounds() const;

    Rect bounds(Float _padding) const;

    const Bezier & bezier() const;

    Size index() const;

  private:
    void markDirty();

    PT * m_path;
    Size m_index;
};

struct STICK_API SegmentData
{
    Vec2f handleIn;
    Vec2f position;
    Vec2f handleOut;
};

using SegmentDataArray = stick::DynamicArray<SegmentData>;
using Segment = SegmentT<Path>;
using SegmentView = detail::ContainerView<false, SegmentDataArray, Segment>;
using ConstSegment = SegmentT<const Path>;
using ConstSegmentView = detail::ContainerView<true, SegmentDataArray, ConstSegment>;

struct STICK_API CurveData
{
    stick::Maybe<Bezier> bezier;
    stick::Maybe<Float> length;
    stick::Maybe<Rect> bounds;
};

using CurveDataArray = stick::DynamicArray<CurveData>;
using Curve = CurveT<Path>;
using CurveView = detail::ContainerView<false, CurveDataArray, Curve>;
using ConstCurve = CurveT<const Path>;
using ConstCurveView = detail::ContainerView<true, CurveDataArray, ConstCurve>;

class STICK_API CurveLocation
{
  public:
    CurveLocation();

    CurveLocation(const CurveLocation &) = default;
    CurveLocation(CurveLocation &&) = default;
    CurveLocation & operator=(const CurveLocation &) = default;
    CurveLocation & operator=(CurveLocation &&) = default;

    CurveLocation(ConstCurve _c, Float _parameter, Float _offset);

    explicit operator bool() const;

    bool operator==(const CurveLocation & _other) const;

    bool operator!=(const CurveLocation & _other) const;

    bool isSynonymous(const CurveLocation & _other) const;

    Vec2f position() const;

    Vec2f normal() const;

    Vec2f tangent() const;

    Float curvature() const;

    Float angle() const;

    Float parameter() const;

    Float offset() const;

    bool isValid() const;

    ConstCurve curve() const;

  private:
    ConstCurve m_curve;
    Float m_parameter;
    Float m_offset;
};

struct STICK_API Intersection
{
    CurveLocation location;
    Vec2f position;
};

using IntersectionArray = stick::DynamicArray<Intersection>;

namespace segments
{
void addPoint(SegmentDataArray & _segs, const Vec2f & _to);

void cubicCurveTo(SegmentDataArray & _segs,
                  const Vec2f & _handleOne,
                  const Vec2f & _handleTwo,
                  const Vec2f & _to);

void quadraticCurveTo(SegmentDataArray & _segs, const Vec2f & _handle, const Vec2f & _to);

void curveTo(SegmentDataArray & _segs,
             const Vec2f & _through,
             const Vec2f & _to,
             Float _parameter = 0.5);

Error arcTo(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _to);

Error arcTo(SegmentDataArray & _segs, const Vec2f & _to, bool _bClockwise = true);

Error arcTo(SegmentDataArray & _segs,
            const Vec2f & _to,
            const Vec2f & _radii,
            Float _rotation,
            bool _bClockwise,
            bool _bLarge);

// relative post-script style drawing commands
void cubicCurveBy(SegmentDataArray & _segs,
                  const Vec2f & _handleOne,
                  const Vec2f & _handleTwo,
                  const Vec2f & _by);

void quadraticCurveBy(SegmentDataArray & _segs, const Vec2f & _handle, const Vec2f & _by);

void curveBy(SegmentDataArray & _segs,
             const Vec2f & _through,
             const Vec2f & _by,
             Float _parameter = 0.5);

Error arcBy(SegmentDataArray & _segs, const Vec2f & _through, const Vec2f & _by);

Error arcBy(SegmentDataArray & _segs, const Vec2f & _to, bool _bClockwise = true);

Error arcHelper(SegmentDataArray & _segs,
                Float _extentDeg,
                const Vec2f & _direction,
                const Vec2f & _to,
                const Vec2f & _center,
                const Mat32f * _transform);
} // namespace segments

class STICK_API Path : public Item
{
    template <class T>
    friend class SegmentT;
    template <class T>
    friend class CurveT;
    friend class RenderInterface;
    friend struct detail::BooleanOperations;

  public:
    Path(stick::Allocator & _alloc, Document * _document, const char * _name);

    // absolute post-script style drawing commands
    void addPoint(const Vec2f & _to);

    void cubicCurveTo(const Vec2f & _handleOne, const Vec2f & _handleTwo, const Vec2f & _to);

    void quadraticCurveTo(const Vec2f & _handle, const Vec2f & _to);

    void curveTo(const Vec2f & _through, const Vec2f & _to, Float _parameter = 0.5);

    Error arcTo(const Vec2f & _through, const Vec2f & _to);

    Error arcTo(const Vec2f & _to, bool _bClockwise = true);

    Error arcTo(
        const Vec2f & _to, const Vec2f & _radii, Float _rotation, bool _bClockwise, bool _bLarge);

    // relative post-script style drawing commands
    void cubicCurveBy(const Vec2f & _handleOne, const Vec2f & _handleTwo, const Vec2f & _by);

    void quadraticCurveBy(const Vec2f & _handle, const Vec2f & _by);

    void curveBy(const Vec2f & _through, const Vec2f & _by, Float _parameter = 0.5);

    Error arcBy(const Vec2f & _through, const Vec2f & _by);

    Error arcBy(const Vec2f & _to, bool _bClockwise = true);

    void closePath();

    // these functins will replace any existing segments with the respective shape
    // these functions mainly exist so that you can reuse an existing path instead of having to create a new one.
    // the functions return *this.
    Path & makeEllipse(const Vec2f & _center, const Vec2f & _size);

    Path & makeCircle(const Vec2f & _center, Float _radius);

    Path & makeRectangle(const Vec2f & _from, const Vec2f & _to);

    Path & makeRoundedRectangle(const Vec2f & _min, const Vec2f & _max, const Vec2f & _radius);

    // TODO: Add the different smoothing versions / algorithms from more recent paper.js versions
    void smooth(Smoothing _type = Smoothing::Asymmetric, bool _bSmoothChildren = false);

    void smooth(Int64 _from, Int64 _to, Smoothing _type = Smoothing::Asymmetric);

    void simplify(Float _tolerance = 2.5);

    void addSegment(const Vec2f & _point, const Vec2f & _handleIn, const Vec2f & _handleOut);

    void addSegments(const SegmentData * _segments, Size _count);

    void swapSegments(SegmentDataArray & _segments, bool _bClose);

    Segment insertSegment(Size _index, const SegmentData & _seg);

    void insertSegments(Size _index, const SegmentData * _segments, Size _count);

    void removeSegment(Size _index);

    void removeSegments(Size _from);

    void removeSegments(Size _from, Size _to);

    void removeSegments();

    Vec2f positionAt(Float _offset) const;

    Vec2f normalAt(Float _offset) const;

    Vec2f tangentAt(Float _offset) const;

    Float curvatureAt(Float _offset) const;

    Float angleAt(Float _offset) const;

    void reverse();

    void setClockwise(bool _b);

    Curve curve(Size _index);

    const Curve curve(Size _index) const;

    Segment segment(Size _index);

    const Segment segment(Size _index) const;

    Size curveCount() const;

    Size segmentCount() const;

    // NOTE: This function is slightly different from paper.js
    // The paper.js version takes a maxDistance and spaces the resulting
    // segments evenly based on that max distance. This version takes a minDistance
    // which the segments try to keep at least, and spaces the segments non linearly
    // to match the original path as closesly as possible (i.e. linear parts of the path,
    // are subdivided only very little while curvy areas are subdivided a lot)
    void flatten(Float _angleTolerance = 0.25,
                 bool _bFlattenChildren = false,
                 Float _minDistance = 0.0,
                 Size _maxRecursion = 32);

    void flattenRegular(Float _maxDistance, bool _bFlattenChildren = false);

    struct OffsetAndSampleCount
    {
        Float offset;
        Size sampleCount;
    };

    OffsetAndSampleCount regularOffsetAndSampleCount(Float _maxDistance);

    Float regularOffset(Float _maxDistance);

    Path * splitAt(Float _offset);

    Path * splitAt(CurveLocation _loc);

    Path * slice(Float _from, Float _to) const;

    Path * slice(CurveLocation _from, CurveLocation _to) const;

    CurveLocation closestCurveLocation(const Vec2f & _point, Float & _outDistance) const;

    CurveLocation closestCurveLocation(const Vec2f & _point) const;

    CurveLocation curveLocationAt(Float _offset) const;

    void peaks(stick::DynamicArray<CurveLocation> & _peaks) const;

    void extrema(stick::DynamicArray<CurveLocation> & _extrema) const;

    stick::DynamicArray<CurveLocation> extrema() const;

    Float length() const;

    Float area() const;

    bool isClosed() const;

    bool isPolygon() const;

    bool isClockwise() const;

    bool contains(const Vec2f & _p) const;

    Path * clone() const final;

    SegmentView segments();

    CurveView curves();

    ConstSegmentView segments() const;

    ConstCurveView curves() const;

    const SegmentDataArray & segmentData() const;

    const CurveDataArray & curveData() const;

    IntersectionArray intersections() const;

    IntersectionArray intersections(const Path * _other) const;

    // called from Renderer
    bool cleanDirtyGeometry();

  private:
    bool canAddChild(Item * _e) const final;

    void addedChild(Item * _e) final;

    Segment createSegment(const Vec2f & _pos, const Vec2f & _handleIn, const Vec2f & _handleOut);

    void transformChanged(bool _bCalledFromParent) final;

    void intersectionsImpl(const Path * _other, IntersectionArray & _outIntersections) const;

    void rebuildCurves();

    stick::Maybe<Rect> computeFillBounds(const Mat32f * _transform, Float _padding) const;

    stick::Maybe<Rect> computeHandleBounds(const Mat32f * _transform) const;

    stick::Maybe<Rect> computeStrokeBounds(const Mat32f * _transform) const;

    stick::Maybe<Rect> computeBounds(const Mat32f * _transform, BoundsType _type) const final;

    Error arcHelper(Float _extentDeg,
                    Segment _segment,
                    const Vec2f & _direction,
                    const Vec2f & _to,
                    const Vec2f & _center,
                    const Mat32f * _transform);

    void applyTransform(const Mat32f & _transform, bool _bMarkParentsBoundsDirty) final;

    // helper to transform a segment without marking the curves dirty.
    void applyTransformToSegment(Size _index, const Mat32f & _transform);

    void markGeometryDirty(bool _bMarkLengthDirty, bool _bMarkParentsBoundsDirty = true);

    void appendedSegments(Size _count);

    SegmentDataArray m_segmentData;
    mutable CurveDataArray m_curveData;
    bool m_bIsClosed;

    // for hit testing
    mutable detail::MonoCurveLoopArray m_monoCurves;

    // rendering related
    mutable stick::Maybe<Float> m_length;
    bool m_bGeometryDirty;
};

template <class PT>
SegmentT<PT>::SegmentT(PT * _path, Size _idx) : m_path(_path), m_index(_idx)
{
}

template <class PT>
template <class OPT>
SegmentT<PT>::SegmentT(const SegmentT<OPT> & _other) :
    m_path(_other.m_path),
    m_index(_other.m_index)
{
}

template <class PT>
void SegmentT<PT>::setPosition(const Vec2f & _pos)
{
    auto delta = _pos - m_path->m_segmentData[m_index].position;
    m_path->m_segmentData[m_index].position = _pos;
    m_path->m_segmentData[m_index].handleIn += delta;
    m_path->m_segmentData[m_index].handleOut += delta;
    segmentChanged();
}

template <class PT>
void SegmentT<PT>::setHandleIn(const Vec2f & _pos)
{
    m_path->m_segmentData[m_index].handleIn = _pos;
    segmentChanged();
}

template <class PT>
void SegmentT<PT>::setHandleOut(const Vec2f & _pos)
{
    m_path->m_segmentData[m_index].handleOut = _pos;
    segmentChanged();
}

template <class PT>
const Vec2f & SegmentT<PT>::position() const
{
    return m_path->m_segmentData[m_index].position;
}

template <class PT>
Vec2f SegmentT<PT>::handleIn() const
{
    return m_path->m_segmentData[m_index].handleIn - m_path->m_segmentData[m_index].position;
}

template <class PT>
Vec2f SegmentT<PT>::handleOut() const
{
    return m_path->m_segmentData[m_index].handleOut - m_path->m_segmentData[m_index].position;
}

template <class PT>
const Vec2f & SegmentT<PT>::handleInAbsolute() const
{
    // return position() + handleIn();
    return m_path->m_segmentData[m_index].handleIn;
}

template <class PT>
const Vec2f & SegmentT<PT>::handleOutAbsolute() const
{
    // return position() + handleOut();
    return m_path->m_segmentData[m_index].handleOut;
}

template <class PT>
CurveT<PT> SegmentT<PT>::curveIn() const
{
    if (m_path->m_segmentData.count() > 1)
    {
        if (m_index == 0 && m_path->isClosed())
            return Curve(m_path, m_path->m_segmentData.count() - 1);
        else if (m_index > 0)
            return Curve(m_path, m_index - 1);
    }
    return Curve();
}

template <class PT>
CurveT<PT> SegmentT<PT>::curveOut() const
{
    if (m_path->m_segmentData.count() > 1 &&
        (m_index < m_path->m_segmentData.count() - 1 || m_path->isClosed()))
    {
        return Curve(m_path, m_index);
    }
    return Curve();
}

template <class PT>
bool SegmentT<PT>::isLinear() const
{
    handleIn();
    if (crunch::isClose(handleIn(), crunch::Vec2f(0.0), detail::PaperConstants::tolerance()) &&
        crunch::isClose(handleOut(), crunch::Vec2f(0.0), detail::PaperConstants::tolerance()))
        return true;

    return false;
}

template <class PT>
void SegmentT<PT>::remove()
{
    STICK_ASSERT(m_path);
    m_path->removeSegment(m_index);
}

template <class PT>
void SegmentT<PT>::transform(const Mat32f & _transform)
{
    STICK_ASSERT(m_path);
    m_path->applyTransformToSegment(m_index, _transform);

    // //mark the affected curves dirty
    // Curve ci = curveIn();
    // Curve co = curveOut();
    // if (ci)
    //     ci.markDirty();
    // if (co)
    //     co.markDirty();

    segmentChanged();
}

template <class PT>
void SegmentT<PT>::segmentChanged()
{
    auto ci = curveIn();
    auto co = curveOut();
    if (ci)
        ci.markDirty();
    if (co)
        co.markDirty();
}

template <class PT>
SegmentT<PT>::operator bool() const
{
    return m_path;
}

template <class PT>
Size SegmentT<PT>::index() const
{
    return m_index;
}

template <class PT>
CurveT<PT>::CurveT() : m_path(nullptr)
{
}

template <class PT>
CurveT<PT>::CurveT(PT * _path, Size _index) : m_path(_path), m_index(_index)
{
}

template <class PT>
template <class OPT>
CurveT<PT>::CurveT(const CurveT<OPT> & _other) : m_path(_other.m_path), m_index(_other.m_index)
{
}

template <class PT>
PT * CurveT<PT>::path() const
{
    return m_path;
}

template <class PT>
void CurveT<PT>::setPositionOne(const Vec2f & _vec)
{
    segmentOne().setPosition(_vec);
}

template <class PT>
void CurveT<PT>::setHandleOne(const Vec2f & _vec)
{
    segmentOne().setHandleOut(_vec);
}

template <class PT>
void CurveT<PT>::setPositionTwo(const Vec2f & _vec)
{
    segmentTwo().setPosition(_vec);
}

template <class PT>
void CurveT<PT>::setHandleTwo(const Vec2f & _vec)
{
    segmentTwo().setHandleIn(_vec);
}

template <class PT>
SegmentT<PT> CurveT<PT>::segmentOne() const
{
    return m_path->segment(m_index);
}

template <class PT>
SegmentT<PT> CurveT<PT>::segmentTwo() const
{
    return m_path->segment((m_index + 1) % m_path->m_segmentData.count());
}

template <class PT>
const Vec2f & CurveT<PT>::positionOne() const
{
    return segmentOne().position();
}

template <class PT>
const Vec2f & CurveT<PT>::positionTwo() const
{
    return segmentTwo().position();
}

template <class PT>
Vec2f CurveT<PT>::handleOne() const
{
    return segmentOne().handleOut();
}

template <class PT>
const Vec2f & CurveT<PT>::handleOneAbsolute() const
{
    return segmentOne().handleOutAbsolute();
}

template <class PT>
Vec2f CurveT<PT>::handleTwo() const
{
    return segmentTwo().handleIn();
}

template <class PT>
const Vec2f & CurveT<PT>::handleTwoAbsolute() const
{
    return segmentTwo().handleInAbsolute();
}

template <class PT>
Vec2f CurveT<PT>::positionAt(Float _offset) const
{
    return bezier().positionAt(parameterAtOffset(_offset));
}

template <class PT>
Vec2f CurveT<PT>::normalAt(Float _offset) const
{
    return bezier().normalAt(parameterAtOffset(_offset));
}

template <class PT>
Vec2f CurveT<PT>::tangentAt(Float _offset) const
{
    return bezier().tangentAt(parameterAtOffset(_offset));
}

template <class PT>
Float CurveT<PT>::curvatureAt(Float _offset) const
{
    return bezier().curvatureAt(parameterAtOffset(_offset));
}

template <class PT>
Float CurveT<PT>::angleAt(Float _offset) const
{
    return bezier().angleAt(parameterAtOffset(_offset));
}

template <class PT>
stick::Maybe<CurveT<PT>> CurveT<PT>::divideAt(Float _offset)
{
    return divideAtParameter(parameterAtOffset(_offset));
}

template <class PT>
Vec2f CurveT<PT>::positionAtParameter(Float _t) const
{
    return bezier().positionAt(_t);
}

template <class PT>
Vec2f CurveT<PT>::normalAtParameter(Float _t) const
{
    return bezier().normalAt(_t);
}

template <class PT>
Vec2f CurveT<PT>::tangentAtParameter(Float _t) const
{
    return bezier().tangentAt(_t);
}

template <class PT>
Float CurveT<PT>::curvatureAtParameter(Float _t) const
{
    return bezier().curvatureAt(_t);
}

template <class PT>
Float CurveT<PT>::angleAtParameter(Float _t) const
{
    return bezier().angleAt(_t);
}

template <class PT>
stick::Maybe<CurveT<PT>> CurveT<PT>::divideAtParameter(Float _t)
{
    STICK_ASSERT(m_path);

    // return empty maybe if _t is out of range
    if (_t >= 1 || _t <= 0)
        return stick::Maybe<CurveT>();

    // split the bezier
    auto splitResult = bezier().subdivide(_t);

    // adjust the exiting segments of this curve
    segmentOne().setHandleOut(splitResult.first.handleOne());
    segmentTwo().setHandleIn(splitResult.second.handleTwo());
    segmentTwo().setPosition(splitResult.second.positionTwo());

    // create the new segment
    SegmentData seg = { splitResult.first.handleTwo(),
                        splitResult.first.positionTwo(),
                        splitResult.second.handleOne() };

    m_path->insertSegment(m_index + 1, seg);

    // Size sindex = m_index;

    // auto it = m_path->m_segmentData.begin() + bindex;
    // m_path->m_segmentData.insert(it, {splitResult.first.positionTwo(),
    //                                   splitResult.first.handleTwo() -
    //                                   splitResult.first.positionTwo(),
    //                                   splitResult.second.handleOne() -
    //                                   splitResult.second.positionOne(), sindex));

    // // create the new curve in the correct index
    // auto cit = sindex < m_path.curveArray().count() ? m_path.curveArray().begin() + sindex :
    // m_path.curveArray().end(); stick::Size sindex2 = cit == m_path.curveArray().end() ? 0 :
    // sindex + 1; Curve * c = m_path.document().allocator().create<Curve>(m_path, sindex, sindex2);
    // auto rit = m_path.curveArray().insert(cit, stick::UniquePtr<Curve>(c,
    // m_path.document().allocator()));

    // // update curve segment indices
    // auto & curves = m_path.curveArray();
    // if (sindex2 != 0)
    // {
    //     for (stick::Size i = m_segmentB + 1; i < curves.count(); ++i)
    //     {
    //         curves[i]->m_segmentA += 1;
    //         if (curves[i]->m_segmentB != 0 || i < curves.count() - 1)
    //             curves[i]->m_segmentB = curves[i]->m_segmentA + 1;
    //     }
    // }
    // else
    // {
    //     //only update the previous closing curves indices
    //     curves[curves.count() - 2]->m_segmentB = sindex;
    // }

    // //mark the affected curves dirty
    // m_segmentB = sindex;
    // markDirty();
    // c->markDirty();

    // return handle to the the new curve
    return CurveT(m_path, m_index + 1);
}

template <class PT>
Float CurveT<PT>::parameterAtOffset(Float _offset) const
{
    return bezier().parameterAtOffset(_offset);
}

template <class PT>
Float CurveT<PT>::closestParameter(const Vec2f & _point) const
{
    return bezier().closestParameter(_point);
}

template <class PT>
Float CurveT<PT>::closestParameter(const Vec2f & _point, Float & _outDistance) const
{
    return bezier().closestParameter(_point, _outDistance, 0, 1, 0);
}

template <class PT>
Float CurveT<PT>::lengthBetween(Float _tStart, Float _tEnd) const
{
    return bezier().lengthBetween(_tStart, _tEnd);
}

template <class PT>
Float CurveT<PT>::pathOffset() const
{
    // calculate the offset from the start to the curve location
    Float offset = 0;
    for (ConstCurve c : m_path->curves())
    {
        if (c.m_index != m_index)
            offset += c.length();
        else
            break;
    }

    return offset;
}

template <class PT>
CurveLocation CurveT<PT>::closestCurveLocation(const Vec2f & _point) const
{
    Float t = closestParameter(_point);
    return CurveLocation(*this, t, pathOffset() + lengthBetween(0, t));
}

template <class PT>
CurveLocation CurveT<PT>::curveLocationAt(Float _offset) const
{
    return CurveLocation(*this, parameterAtOffset(_offset), pathOffset() + _offset);
}

template <class PT>
CurveLocation CurveT<PT>::curveLocationAtParameter(Float _t) const
{
    return CurveLocation(*this, _t, pathOffset() + lengthBetween(0, _t));
}

template <class PT>
bool CurveT<PT>::isLinear() const
{
    return crunch::isClose(handleOne(), Vec2f(0)) && crunch::isClose(handleTwo(), Vec2f(0));
}

template <class PT>
bool CurveT<PT>::isStraight() const
{
    if (isLinear())
        return true;

    Vec2f line = positionTwo() - positionOne();

    // Zero-length line, with some handles defined.
    if (crunch::isClose(line, Vec2f(0)))
        return false;

    if (crunch::isCollinear(handleOne(), line) && crunch::isCollinear(handleTwo(), line))
    {
        // Collinear handles. Project them onto line to see if they are
        // within the line's range:
        Float d = crunch::dot(line, line);
        Float p1 = crunch::dot(line, handleOne()) / d;
        Float p2 = crunch::dot(line, handleTwo()) / d;

        return p1 >= 0 && p1 <= 1 && p2 <= 0 && p2 >= -1;
    }

    return false;
}

template <class PT>
bool CurveT<PT>::isArc() const
{
    if (crunch::isOrthogonal(handleOne(), handleTwo(), detail::PaperConstants::tolerance()))
    {
        Line aLine = Line::fromPoints(positionOne(), handleOneAbsolute());
        Line bLine = Line::fromPoints(positionTwo(), handleTwoAbsolute());

        auto result = crunch::intersect(aLine, bLine);
        if (result)
        {
            Vec2f corner = *result;
            static Float kappa = detail::PaperConstants::kappa();
            static Float epsilon = detail::PaperConstants::epsilon();

            if (crunch::isClose(
                    crunch::length(handleOne()) / crunch::length(corner - positionOne()) - kappa,
                    0.0f,
                    epsilon) &&
                crunch::isClose(
                    crunch::length(handleTwo()) / crunch::length(corner - positionTwo()) - kappa,
                    0.0f,
                    epsilon))
            {
                return true;
            }
        }
    }
    return false;
}

template <class PT>
bool CurveT<PT>::isOrthogonal(const CurveT & _other) const
{
    if (isLinear() && _other.isLinear())
    {
        if (crunch::isOrthogonal(positionOne() - positionTwo(),
                                 _other.positionOne() - _other.positionTwo(),
                                 detail::PaperConstants::tolerance()))
            return true;
    }
    return false;
}

template <class PT>
bool CurveT<PT>::isCollinear(const CurveT & _other) const
{
    if (isLinear() && _other.isLinear())
    {
        if (crunch::isCollinear(positionOne() - positionTwo(),
                                _other.positionOne() - _other.positionTwo(),
                                detail::PaperConstants::tolerance()))
            return true;
    }
    return false;
}

template <class PT>
Float CurveT<PT>::length() const
{
    auto & cd = m_path->m_curveData[m_index];
    if (!cd.length)
    {
        cd.length = bezier().length();
    }
    return *cd.length;
}

template <class PT>
Float CurveT<PT>::area() const
{
    return bezier().area();
}

template <class PT>
const Rect & CurveT<PT>::bounds() const
{
    auto & cd = m_path->m_curveData[m_index];
    if (!cd.bounds)
        cd.bounds = bezier().bounds();
    return *cd.bounds;
}

template <class PT>
Rect CurveT<PT>::bounds(Float _padding) const
{
    return bezier().bounds(_padding);
}

template <class PT>
void CurveT<PT>::markDirty()
{
    STICK_ASSERT(m_path);
    auto & cd = m_path->m_curveData[m_index];
    cd.length.reset();
    cd.bounds.reset();
    cd.bezier.reset();
}

template <class PT>
const Bezier & CurveT<PT>::bezier() const
{
    STICK_ASSERT(m_path);
    auto & cd = m_path->m_curveData[m_index];
    if (!cd.bezier)
        cd.bezier = Bezier(positionOne(), handleOneAbsolute(), handleTwoAbsolute(), positionTwo());
    return *cd.bezier;
}

template <class PT>
void CurveT<PT>::peaks(stick::DynamicArray<Float> & _peaks) const
{
    auto peaks = bezier().peaks();
    for (stick::Int32 i = 0; i < peaks.count; ++i)
        _peaks.append(peaks.values[i]);
}

template <class PT>
void CurveT<PT>::extrema(stick::DynamicArray<Float> & _extrema) const
{
    auto ex = bezier().extrema2D();
    for (stick::Int32 i = 0; i < ex.count; ++i)
        _extrema.append(ex.values[i]);
}

template <class PT>
CurveT<PT>::operator bool() const
{
    return m_path;
}

template <class PT>
Size CurveT<PT>::index() const
{
    return m_index;
}
} // namespace paper

#endif // PAPER_PATH_HPP
