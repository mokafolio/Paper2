#include <Paper2/Paint.hpp>

namespace paper
{
using namespace stick;

BaseGradient::BaseGradient(const Vec2f & _from, const Vec2f & _to, GradientType _type) :
    m_type(_type),
    m_origin(_from),
    m_destination(_to),
    m_bStopsDirty(true),
    m_bPositionsDirty(true)
{
}

BaseGradient::BaseGradient(const BaseGradient & _other) :
    m_origin(_other.m_origin),
    m_destination(_other.m_destination),
    m_bStopsDirty(true),
    m_bPositionsDirty(true)
{
    if (_other.m_renderData)
        m_renderData = _other.m_renderData->clone();
}

BaseGradient::~BaseGradient()
{
}

void BaseGradient::setOrigin(const Vec2f & _position)
{
    m_origin = _position;
    markPositionsDirty();
}

void BaseGradient::setDestination(const Vec2f & _position)
{
    m_destination = _position;
    markPositionsDirty();
}

void BaseGradient::setOriginAndDestination(const Vec2f & _orig, const Vec2f & _dest)
{
    m_origin = _orig;
    m_destination = _dest;
    markPositionsDirty();
}

void BaseGradient::addStop(const ColorRGBA & _color, Float _offset)
{
    m_stops.append({_color, _offset});
    markStopsDirty();
}

const Vec2f & BaseGradient::origin() const
{
    return m_origin;
}

const Vec2f & BaseGradient::destination() const
{
    return m_destination;
}

const ColorStopArray & BaseGradient::stops() const
{
    return m_stops;
}

GradientType BaseGradient::type() const
{
    return m_type;
}

void BaseGradient::markStopsDirty()
{
    m_bStopsDirty = true;
}

void BaseGradient::markPositionsDirty()
{
    m_bPositionsDirty = true;
}

bool BaseGradient::cleanDirtyPositions()
{
    bool ret = m_bPositionsDirty;
    m_bPositionsDirty = false;
    return ret;
}

bool BaseGradient::cleanDirtyStops()
{
    bool ret = m_bStopsDirty;
    m_bStopsDirty = false;
    return ret;
}

RenderData * BaseGradient::renderData()
{
    return m_renderData.get();
}

void BaseGradient::setRenderData(RenderDataUniquePtr _ptr)
{
    m_renderData = std::move(_ptr);
}

LinearGradient::LinearGradient(const Vec2f & _from, const Vec2f & _to) :
    BaseGradient(_from, _to, GradientType::Linear)
{
}

LinearGradient::LinearGradient(const LinearGradient & _other) : BaseGradient(_other)
{
}

RadialGradient::RadialGradient(const Vec2f & _from, const Vec2f & _to) :
    BaseGradient(_from, _to, GradientType::Radial)
{
}

RadialGradient::RadialGradient(const RadialGradient & _other) : BaseGradient(_other)
{
}

void RadialGradient::setFocalPointOffset(const Vec2f & _focalPoint)
{
    m_focalPointOffset = _focalPoint;
    markPositionsDirty();
}

void RadialGradient::setRatio(Float _ratio)
{
    m_ratio = _ratio;
    markPositionsDirty();
}

const Maybe<Vec2f> & RadialGradient::focalPointOffset() const
{
    return m_focalPointOffset;
}

const Maybe<Float> & RadialGradient::ratio() const
{
    return m_ratio;
}

LinearGradientPtr createLinearGradient(const Vec2f & _from, const Vec2f & _to)
{
    return makeShared<LinearGradient>(_from, _to);
}

RadialGradientPtr createRadialGradient(const Vec2f & _from, const Vec2f & _to)
{
    return makeShared<RadialGradient>(_from, _to);
}
} // namespace paper
