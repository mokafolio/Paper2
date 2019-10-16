#ifndef PAPER_STYLE_HPP
#define PAPER_STYLE_HPP

#include <Paper2/Paint.hpp>
#include <Stick/Maybe.hpp>
#include <Stick/SharedPtr.hpp>

#include <Tink/Tink.hpp>

namespace paper
{

class Item;
using ItemPtrArray = stick::DynamicArray<Item *>;

struct STICK_API StyleData
{
    StyleData();
    StyleData(const StyleData &) = default;
    StyleData(StyleData &&) = default;
    StyleData & operator = (StyleData &&) = default;
    StyleData & operator = (const StyleData &) = default;

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

    //text related stuff
    tink::FontPtr font;
    tink::TextDirection textDirection;
    Float fontSize;
    Float fontTracking;
    Float fontLeading;
};

class Style;
using StylePtr = stick::SharedPtr<Style>;

class STICK_API Style
{
    friend class Item;

  public:

    Style(stick::Allocator & _alloc);
    Style(const Style &);
    Style(Style &&) = default;
    Style(stick::Allocator & _alloc, const StyleData & _data);
    ~Style() = default;

    Style & operator = (Style &&) = default;
    Style & operator = (const StyleData & _data);

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

    void setFont(const tink::FontPtr & _fnt);
    void setTextDirection(tink::TextDirection _dir);
    void setFontSize(Float _size);
    void setFontTracking(Float _val);
    void setFontLeading(Float _val);


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
    const StyleData & data() const;

    tink::FontPtr font() const;
    tink::TextDirection textDirection() const;
    Float fontSize() const;
    Float fontTracking() const;
    Float fontLeading() const;

    StylePtr clone(Item * _item = nullptr) const;

    static StrokeJoin defaultStrokeJoin();
    static StrokeCap defaultStrokeCap();
    static Float defaultMiterLimit();
    static Float defaultStrokeWidth();
    static Float defaultDashOffset();
    static const DashArray & defaultDashArray();
    static WindingRule defaultWindingRule();
    static bool defaultScaleStroke();
    static Paint defaultFill();
    static Paint defaultStroke();  
    static tink::FontPtr defaultFont();
    static tink::TextDirection defautTextDirection();
    static Float defaultFontSize();
    static Float defaultFontTracking();
    static Float defaultFontLeading();

  private:
    void itemRemovedStyle(Item * _item);
    void itemAddedStyle(Item * _item);

    StyleData m_data;
    ItemPtrArray m_items;
};

STICK_API bool strokeBoundsDifferent(const StyleData & _a, const StyleData & _b);

} // namespace paper

#endif // PAPER_STYLE_HPP
