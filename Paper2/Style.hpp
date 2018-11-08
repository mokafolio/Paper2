#ifndef PAPER_STYLE_HPP
#define PAPER_STYLE_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
class Document;

class Style;
using StylePtr = stick::SharedPtr<Style>;

class STICK_API Style
{
    friend class Document;

  public:
    
    StylePtr clone() const;

    void setStrokeJoin(StrokeJoin _join);

    void setStrokeCap(StrokeCap _cap);

    void setMiterLimit(Float _limit);

    void setStrokeWidth(Float _width);

    void setStroke(const ColorRGBA & _color);

    void setStroke(const stick::String & _svgName);

    void setStroke(const LinearGradientPtr & _grad);

    void setStroke(const RadialGradientPtr & _grad);

    void setDashArray(const DashArray & _arr);

    void setDashOffset(Float _f);

    void setScaleStroke(bool _b);

    void removeStroke();

    void setFill(const ColorRGBA & _color);

    void setFill(const stick::String & _svgName);

    void setFill(const LinearGradientPtr & _grad);

    void setFill(const RadialGradientPtr & _grad);

    void removeFill();

    void setFillPaintTransform(const Mat32f & _transform);

    void setStrokePaintTransform(const Mat32f & _transform);

    void removeFillPaintTransform();

    void removeStrokePaintTransform();

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

  private:
    Style();
    Style(const Style &) = default;
    Style(Style &&) = default;
    Style & operator(const Style &) = default;
    Style & operator(Style &&) = default;

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
};

} // namespace paper

#endif // PAPER_STYLE_HPP
