#ifndef PAPER_STYLE_HPP
#define PAPER_STYLE_HPP

#include <Paper2/Paint.hpp>
#include <Stick/Maybe.hpp>
#include <Stick/SharedPtr.hpp>

namespace paper
{

class Item;
using ItemPtrArray = stick::DynamicArray<Item *>;

struct STICK_API ResolvedStyle
{
    Paint fill;
    Paint stroke;
    Float strokeWidth;
    StrokeJoin strokeJoin;
    StrokeCap strokeCap;
    bool scaleStroke;
    Float miterLimit;
    DashArray dashArray;
    Float dashOffset;
    WindingRule windingRule;
};

class Style;
using StylePtr = stick::SharedPtr<Style>;

class STICK_API Style
{
    friend class Item;

    public:

    void setStrokeJoin(StrokeJoin _join);
    void setStrokeCap(StrokeCap _cap);
    void setMiterLimit(Float _limit);
    void setStrokeWidth(Float _width);
    void setStroke(const stick::String & _svgName);
    void setStroke(const Paint & _paint);
    void setDashArray(const DashArray & _arr);
    void setDashOffset(Float _f);
    void setScaleStroke(bool _b);
    void setFill(const stick::String & _svgName);
    void setFill(const Paint & _paint);
    void setWindingRule(WindingRule _rule);

    StrokeJoin strokeJoin() const;
    StrokeCap strokeCap() const;
    Float miterLimit() const;
    Float strokeWidth() const;
    const DashArray & dashArray() const;
    Float dashOffset() const;
    WindingRule windingRule() const;
    bool scaleStroke() const;
    Paint fill() const;
    Paint stroke() const;

    bool hasStroke() const;
    bool hasFill() const;
    bool hasScaleStroke() const;
    bool hasMiterLimit() const;
    bool hasWindingRule() const;
    bool hasDashOffset() const;
    bool hasDashArray() const;
    bool hasStrokeWidth() const;
    bool hasStrokeCap() const;
    bool hasStrokeJoin() const;

    StylePtr clone(Item * _item = nullptr) const;

private:

    void itemRemovedStyle(Item * _item);
    void itemAddedStyle(Item * _item);

    stick::Maybe<Paint> m_fill;
    stick::Maybe<Paint> m_stroke;
    stick::Maybe<Float> m_strokeWidth;
    stick::Maybe<StrokeJoin> m_strokeJoin;
    stick::Maybe<StrokeCap> m_strokeCap;
    stick::Maybe<bool> m_scaleStroke;
    stick::Maybe<Float> m_miterLimit;
    stick::Maybe<DashArray> m_dashArray;
    stick::Maybe<Float> m_dashOffset;
    stick::Maybe<WindingRule> m_windingRule;

    ItemPtrArray m_items;
};

} // namespace paper

#endif // PAPER_STYLE_HPP
