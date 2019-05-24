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
        {                                                                                          \
            it->markStrokeBoundsDirty(true);                                                       \
            it->m_bStyleDirty = true;                                                              \
        }                                                                                          \
    } while (false)

#define PROPERTY_SETTER(name, val)                                                                 \
    do                                                                                             \
    {                                                                                              \
        m_##name = val;                                                                            \
        for (Item * it : m_items)                                                                  \
            it->m_bStyleDirty = true;                                                              \
    } while (false)

namespace paper
{
using namespace stick;

Style::Style(stick::Allocator & _alloc) : m_items(_alloc)
{
}

Style::Style(stick::Allocator & _alloc, const ResolvedStyle & _resolved) :
    m_fill(_resolved.fill),
    m_stroke(_resolved.stroke),
    m_strokeWidth(_resolved.strokeWidth),
    m_strokeJoin(_resolved.strokeJoin),
    m_strokeCap(_resolved.strokeCap),
    m_scaleStroke(_resolved.scaleStroke),
    m_miterLimit(_resolved.miterLimit),
    m_dashOffset(_resolved.dashOffset),
    m_windingRule(_resolved.windingRule),
    m_items(_alloc)
{
    if (_resolved.dashArray.count())
    {
        m_dashArray = DashArray(_alloc);
        m_dashArray->append(_resolved.dashArray.begin(), _resolved.dashArray.end());
    }
}

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
    // for now we simply mark the stroke bounds dirty for all items using this style, if it is
    // set through the style. (when set through the item, it only marks bounds geometry dirty if
    // necessary)
    STROKE_PROPERTY_SETTER(stroke, _paint);
}

void Style::setDashArray(const DashArray & _arr)
{
    PROPERTY_SETTER(dashArray, _arr);
}

void Style::setDashOffset(Float _f)
{
    PROPERTY_SETTER(dashOffset, _f);
}

void Style::setScaleStroke(bool _b)
{
    PROPERTY_SETTER(scaleStroke, _b);
}

void Style::setFill(const String & _svgName)
{
    setFill(crunch::svgColor<ColorRGBA>(_svgName));
}

void Style::setFill(const Paint & _paint)
{
    PROPERTY_SETTER(fill, _paint);
}

void Style::setWindingRule(WindingRule _rule)
{
    PROPERTY_SETTER(windingRule, _rule);
}

StrokeJoin Style::strokeJoin() const
{
    PROPERTY_GETTER(strokeJoin, defaultStrokeJoin());
}

StrokeCap Style::strokeCap() const
{
    PROPERTY_GETTER(strokeCap, defaultStrokeCap());
}

Float Style::miterLimit() const
{
    PROPERTY_GETTER(miterLimit, defaultMiterLimit());
}

Float Style::strokeWidth() const
{
    PROPERTY_GETTER(strokeWidth, defaultStrokeWidth());
}

const DashArray & Style::dashArray() const
{
    PROPERTY_GETTER(dashArray, defaultDashArray());
}

Float Style::dashOffset() const
{
    PROPERTY_GETTER(dashOffset, defaultDashOffset());
}

WindingRule Style::windingRule() const
{
    PROPERTY_GETTER(windingRule, defaultWindingRule());
}

bool Style::scaleStroke() const
{
    PROPERTY_GETTER(scaleStroke, defaultScaleStroke());
}

Paint Style::fill() const
{
    PROPERTY_GETTER(fill, defaultFill());
}

Paint Style::stroke() const
{
    PROPERTY_GETTER(stroke, defaultStroke());
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

StrokeJoin Style::defaultStrokeJoin()
{
    return StrokeJoin::Bevel;
}

StrokeCap Style::defaultStrokeCap()
{
    return StrokeCap::Butt;
}

Float Style::defaultMiterLimit()
{
    return 4.0f;
}

Float Style::defaultStrokeWidth()
{
    return 1.0f;
}

Float Style::defaultDashOffset()
{
    return 0.0f;
}

const DashArray & Style::defaultDashArray()
{
    static DashArray s_default;
    return s_default;
}

WindingRule Style::defaultWindingRule()
{
    return WindingRule::EvenOdd;
}

bool Style::defaultScaleStroke()
{
    return true;
}

Paint Style::defaultFill()
{
    return NoPaint();
}

Paint Style::defaultStroke()
{
    return NoPaint();
}

} // namespace paper
