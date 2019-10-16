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
        m_data.name = val;                                                                         \
        for (Item * it : m_items)                                                                  \
        {                                                                                          \
            it->markStrokeBoundsDirty(true);                                                       \
        }                                                                                          \
    } while (false)

#define PROPERTY_SETTER(name, val)                                                                 \
    do                                                                                             \
    {                                                                                              \
        m_data.name = val;                                                                         \
    } while (false)

namespace paper
{
using namespace stick;

StyleData::StyleData()
    : fill(Style::defaultFill())
    , stroke(Style::defaultStroke())
    , strokeWidth(Style::defaultStrokeWidth())
    , strokeJoin(Style::defaultStrokeJoin())
    , strokeCap(Style::defaultStrokeCap())
    , scaleStroke(Style::defaultScaleStroke())
    , miterLimit(Style::defaultMiterLimit())
    , dashArray(Style::defaultDashArray())
    , dashOffset(Style::defaultDashOffset())
    , windingRule(Style::defaultWindingRule())
    , font(Style::defaultFont())
    , textDirection(Style::defautTextDirection())
    , fontSize(Style::defaultFontSize())
    , fontTracking(Style::defaultFontTracking())
    , fontLeading(Style::defaultFontLeading())
{
}

Style::Style(stick::Allocator & _alloc) : m_items(_alloc)
{
}

Style::Style(stick::Allocator & _alloc, const StyleData & _data) : m_data(_data), m_items(_alloc)
{
}

Style::Style(const Style & _other) : m_data(_other.m_data), m_items(_other.m_items.allocator())
{
}

Style & Style::operator=(const StyleData & _data)
{
    bool bDifferent = strokeBoundsDifferent(m_data, _data);
    m_data = _data;
    if (bDifferent)
    {
        for (Item * item : m_items)
            item->markStrokeBoundsDirty(true);
    }
    return *this;
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
    // PROPERTY_SETTER(dashArray, _arr);
    // m_data.dashArray = _arr;
    STROKE_PROPERTY_SETTER(dashArray, _arr);
}

void Style::setDashOffset(Float _f)
{
    // PROPERTY_SETTER(dashOffset, _f);
    // m_data.dashOffset = _f;
    STROKE_PROPERTY_SETTER(dashOffset, _f);
}

void Style::setScaleStroke(bool _b)
{
    STROKE_PROPERTY_SETTER(scaleStroke, _b);
}

void Style::setFill(const String & _svgName)
{
    setFill(crunch::svgColor<ColorRGBA>(_svgName));
}

void Style::setFill(const Paint & _paint)
{
    // PROPERTY_SETTER(fill, _paint);
    m_data.fill = _paint;
}

void Style::setWindingRule(WindingRule _rule)
{
    // PROPERTY_SETTER(windingRule, _rule);
    m_data.windingRule = _rule;
}

void Style::setFont(const tink::FontPtr & _fnt)
{
    m_data.font = _fnt;
}
void Style::setTextDirection(tink::TextDirection _dir)
{
    m_data.textDirection = _dir;
}

void Style::setFontSize(Float _size)
{
    m_data.fontSize = _size;
    m_data.fontLeading = _size * 1.2;
}

void Style::setFontTracking(Float _val)
{
    m_data.fontTracking = _val;
}

void Style::setFontLeading(Float _val)
{
    m_data.fontLeading = _val;
}

StrokeJoin Style::strokeJoin() const
{
    // PROPERTY_GETTER(strokeJoin, defaultStrokeJoin());
    return m_data.strokeJoin;
}

StrokeCap Style::strokeCap() const
{
    // PROPERTY_GETTER(strokeCap, defaultStrokeCap());
    return m_data.strokeCap;
}

Float Style::miterLimit() const
{
    // PROPERTY_GETTER(miterLimit, defaultMiterLimit());
    return m_data.miterLimit;
}

Float Style::strokeWidth() const
{
    // PROPERTY_GETTER(strokeWidth, defaultStrokeWidth());
    return m_data.strokeWidth;
}

const DashArray & Style::dashArray() const
{
    // PROPERTY_GETTER(dashArray, defaultDashArray());
    return m_data.dashArray;
}

Float Style::dashOffset() const
{
    // PROPERTY_GETTER(dashOffset, defaultDashOffset());
    return m_data.dashOffset;
}

WindingRule Style::windingRule() const
{
    // PROPERTY_GETTER(windingRule, defaultWindingRule());
    return m_data.windingRule;
}

bool Style::scaleStroke() const
{
    // PROPERTY_GETTER(scaleStroke, defaultScaleStroke());
    return m_data.scaleStroke;
}

Paint Style::fill() const
{
    // PROPERTY_GETTER(fill, defaultFill());
    return m_data.fill;
}

Paint Style::stroke() const
{
    // PROPERTY_GETTER(stroke, defaultStroke());
    return m_data.stroke;
}

tink::FontPtr Style::font() const
{
    return m_data.font;
}

tink::TextDirection Style::textDirection() const
{
    return m_data.textDirection;
}

Float Style::fontSize() const
{
    return m_data.fontSize;
}

Float Style::fontTracking() const
{
    return m_data.fontTracking;
}

Float Style::fontLeading() const
{
    return m_data.fontLeading;
}

const StyleData & Style::data() const
{
    return m_data;
}

StylePtr Style::clone(Item * _item) const
{
    auto ret = stick::makeShared<Style>(m_items.allocator(), *this);
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

tink::FontPtr Style::defaultFont()
{
    // @TODO: load a default font and return it here?
    return tink::FontPtr();
}

tink::TextDirection Style::defautTextDirection()
{
    return tink::TextDirection::Auto;
}

Float Style::defaultFontSize()
{
    return 16.0;
}

Float Style::defaultFontTracking()
{
    return 0.0;
}

Float Style::defaultFontLeading()
{
    return defaultFontSize() * 1.2;
}

bool strokeBoundsDifferent(const StyleData & _a, const StyleData & _b)
{
    if (_a.strokeWidth != _b.strokeWidth || _a.strokeJoin != _b.strokeJoin ||
        _a.strokeCap != _b.strokeCap || _a.miterLimit != _b.miterLimit ||
        _a.scaleStroke != _b.scaleStroke || (_a.stroke.is<NoPaint>() && !_b.stroke.is<NoPaint>()))
        return true;
    return false;
}

} // namespace paper
