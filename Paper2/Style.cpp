#include <Paper2/Item.hpp>
#include <Paper2/Style.hpp>

#define PROPERTY_GETTER(name, def)                                                                 \
    do                                                                                             \
    {                                                                                              \
        if (m_##name)                                                                              \
        {                                                                                          \
            return *m_##name;                                                                      \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            return def;                                                                            \
        }                                                                                          \
    } while (false)

#define STROKE_PROPERTY_SETTER(name, val)                                                          \
    do                                                                                             \
    {                                                                                              \
        m_##name = val;                                                                            \
        for (Item * it : m_items)                                                                  \
            it->markStrokeBoundsDirty(true);                                                       \
    } while (false)

namespace paper
{
using namespace stick;

// Style::Style()
// {
// }

// Style::~Style()
// {
// }

void Style::setStrokeJoin(StrokeJoin _join)
{
    STROKE_PROPERTY_SETTER(strokeJoin, _join);
}

void Style::setStrokeCap(StrokeCap _cap)
{
    STROKE_PROPERTY_SETTER(strokeCap, _cap);
}

void Style::setMiterLimit(Float _limit)
{
    STROKE_PROPERTY_SETTER(miterLimit, _limit);
}

void Style::setStrokeWidth(Float _width)
{
    STROKE_PROPERTY_SETTER(strokeWidth, _width);
}

void Style::setStroke(const String & _svgName)
{
    setStroke(crunch::svgColor<ColorRGBA>(_svgName));
}

void Style::setStroke(const Paint & _paint)
{
    bool bps = hasStroke();
    m_stroke = _paint;
    if (!bps)
    {
        for (Item * it : m_items)
            it->markStrokeBoundsDirty(true);
    }
}

void Style::setDashArray(const DashArray & _arr)
{
    m_dashArray = _arr;
}

void Style::setDashOffset(Float _f)
{
    m_dashOffset = _f;
}

void Style::setScaleStroke(bool _b)
{
    m_scaleStroke = _b;
}

void Style::setFill(const String & _svgName)
{
    setFill(crunch::svgColor<ColorRGBA>(_svgName));
}

void Style::setFill(const Paint & _paint)
{
    m_fill = _paint;
}

void Style::setWindingRule(WindingRule _rule)
{
    m_windingRule = _rule;
}

StrokeJoin Style::strokeJoin() const
{
    PROPERTY_GETTER(strokeJoin, StrokeJoin::Bevel);
}

StrokeCap Style::strokeCap() const
{
    PROPERTY_GETTER(strokeCap, StrokeCap::Butt);
}

Float Style::miterLimit() const
{
    PROPERTY_GETTER(miterLimit, 4);
}

Float Style::strokeWidth() const
{
    PROPERTY_GETTER(strokeWidth, 1.0);
}

const DashArray & Style::dashArray() const
{
    static DashArray s_default;
    PROPERTY_GETTER(dashArray, s_default);
}

Float Style::dashOffset() const
{
    PROPERTY_GETTER(dashOffset, 0.0);
}

WindingRule Style::windingRule() const
{
    PROPERTY_GETTER(windingRule, WindingRule::EvenOdd);
}

bool Style::scaleStroke() const
{
    PROPERTY_GETTER(scaleStroke, true);
}

Paint Style::fill() const
{
    PROPERTY_GETTER(fill, NoPaint());
}

Paint Style::stroke() const
{
    PROPERTY_GETTER(stroke, NoPaint());
}

bool Style::hasStroke() const
{
    return (bool)m_stroke;
}

bool Style::hasFill() const
{
    return (bool)m_fill;
}

bool Style::hasScaleStroke() const
{
    return (bool)m_scaleStroke;
}

bool Style::hasMiterLimit() const
{
    return (bool)m_miterLimit;
}

bool Style::hasWindingRule() const
{
    return (bool)m_windingRule;
}

bool Style::hasDashOffset() const
{
    return (bool)m_dashOffset;
}

bool Style::hasDashArray() const
{
    return (bool)m_dashArray;
}

bool Style::hasStrokeWidth() const
{
    return (bool)m_strokeWidth;
}

bool Style::hasStrokeCap() const
{
    return (bool)m_strokeCap;
}

bool Style::hasStrokeJoin() const
{
    return (bool)m_strokeJoin;
}

StylePtr Style::clone(Item * _item) const
{
    auto ret = stick::makeShared<Style>(*this);
    if (_item)
        ret->itemAddedStyle(_item);
    return ret;
}

void Style::itemRemovedStyle(Item * _item)
{
    auto it = stick::find(m_items.begin(), m_items.end(), _item);
    STICK_ASSERT(it != m_items.end());
    m_items.remove(it);
}

void Style::itemAddedStyle(Item * _item)
{
    m_items.append(_item);
}

} // namespace paper
