#ifndef PAPER_ITEM_HPP
#define PAPER_ITEM_HPP

#include <Paper2/Constants.hpp>
#include <Paper2/Paint.hpp>
#include <Stick/Result.hpp>

namespace paper
{
class Document;
class Symbol;

class Item;
using ItemPtrArray = stick::DynamicArray<Item *>;

class STICK_API Item
{
    friend class RenderInterface;
    friend class Symbol;
    friend class Group;

  public:
    enum class BoundsType
    {
        Fill,
        Stroke,
        Handle
    };

    Item(stick::Allocator & _alloc, Document * _document, ItemType _type, const char * _name);

    virtual ~Item();

    bool addChild(Item * _e);

    bool insertAbove(const Item * _e);

    bool insertBelow(const Item * _e);

    bool sendToFront();

    bool sendToBack();

    void remove();

    bool removeChild(Item * _item);

    void removeChildren();

    void reverseChildren();

    Item * findChild(const String & _name) const;

    const ItemPtrArray & children() const;

    const String & name() const;

    Item * parent() const;

    void setPosition(const Vec2f & _position);

    void setPivot(const Vec2f & _pivot);

    void removePivot();

    void setVisible(bool _b);

    void setName(const stick::String & _name);

    // transformation things

    void setTransform(const Mat32f & _transform);

    void removeTransform();

    void translateTransform(Float _x, Float _y);

    void translateTransform(const Vec2f & _translation);

    void scaleTransform(Float _scale);

    void scaleTransform(Float _scaleX, Float _scaleY);

    void scaleTransform(const Vec2f & _scale);

    void scaleTransform(const Vec2f & _scale, const Vec2f & _center);

    void rotateTransform(Float _radians);

    void rotateTransform(Float _radians, const Vec2f & _point);

    void skewTransform(const Vec2f & _angles);

    void skewTransform(const Vec2f & _angles, const Vec2f & _center);

    void transform(const Mat32f & _transform);

    void translate(Float _x, Float _y);

    void translate(const Vec2f & _translation);

    void scale(Float _scale);

    void scale(Float _scaleX, Float _scaleY);

    void scale(const Vec2f & _scale);

    void scale(const Vec2f & _scale, const Vec2f & _center);

    void rotate(Float _radians);

    void rotate(Float _radians, const Vec2f & _point);

    void skew(const Vec2f & _angles);

    void skew(const Vec2f & _angles, const Vec2f & _center);

    virtual void applyTransform(const Mat32f & _transform, bool _bMarkParentsBoundsDirty);

    const Mat32f & transform() const;

    virtual const Mat32f & absoluteTransform() const;

    Float rotation() const;

    const Vec2f & translation() const;

    const Vec2f & scaling() const;

    Float absoluteRotation() const;

    const Vec2f & absoluteTranslation() const;

    const Vec2f & absoluteScaling() const;

    const Rect & bounds() const;

    const Rect & handleBounds() const;

    const Rect & strokeBounds() const;

    Vec2f position() const;

    Vec2f pivot() const;

    bool isVisible() const;

    // checks if there is a transform active on this item
    bool hasTransform() const;

    // checks if there is any transform in the document hierarchy that affects this item
    bool isTransformed() const;

    const Mat32f & fillPaintTransform() const;

    const Mat32f & strokePaintTransform() const;

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

    bool hasfillPaintTransform() const;

    bool hasStrokePaintTransform() const;
    

    virtual Item * clone() const = 0;

    Document * document() const;

    ItemType itemType() const;

    stick::TextResult exportSVG() const;

    Error saveSVG(const String & _uri) const;


    // mainly for internal/rendering use
    RenderData * renderData();

    void setRenderData(RenderDataUniquePtr _ptr);

    bool cleanDirtyfillPaintTransform();

    bool cleanDirtystrokePaintTransform();


    //@TODO: debug functions to print the hierarchy
    void hierarchyString(String & _outputString, Size _indent = 0) const;


  protected:
    struct Decomposed
    {
        Float rotation;
        Vec2f scaling;
        Vec2f skew;
    };

    bool insertHelper(const Item * _e, bool _bAbove);

    void removeHelper(bool _bRemoveFromParent);

    virtual void transformChanged(bool _bCalledFromParent);

    void removeFromParent();

    void markAbsoluteTransformDirty();

    void markBoundsDirty(bool _bNotifyParent);

    void markStrokeBoundsDirty(bool _bNotifyParent);

    void markFillBoundsDirty(bool _bNotifyParent);

    stick::Maybe<Rect> mergeWithChildrenBounds(const stick::Maybe<Rect> & _bounds,
                                               const Mat32f * _transform,
                                               BoundsType _type,
                                               bool _bSkipFirstChild = false) const;

    void cloneItemTo(Item * _item) const;

    void applyTransformToChildrenAndPivot(const Mat32f & _transform);

    virtual bool canAddChild(Item * _e) const;

    virtual void addedChild(Item * _e);

    // overwritten by the actual Item types to compute the bounds. _transform will only be passed
    // along if it is different from the items transform (i.e. for Symbols). This seesm a little
    // stupid but allows to use the existing, cached matrix stack for the default case.
    virtual stick::Maybe<Rect> computeBounds(const Mat32f * _transform, BoundsType _type) const;

    static Rect noBounds();

    Decomposed & decomposeIfNeeded(stick::Maybe<Decomposed> & _dec,
                                   const Mat32f & _transform) const;

    // helper to recursively reset a property Maybe (using pointer to member)
    template <class Member>
    void recursivelyResetProperty(Member _member)
    {
        (this->*_member).reset();
        for (Item * c : m_children)
            c->recursivelyResetProperty(_member);
    }

    // basics
    mutable Document * m_document;
    ItemType m_type;
    Item * m_parent;
    ItemPtrArray m_children;
    String m_name;
    bool m_bVisible;

    // transform related
    stick::Maybe<Mat32f> m_transform;
    mutable stick::Maybe<Mat32f> m_absoluteTransform;
    mutable stick::Maybe<Decomposed> m_decomposedTransform;
    mutable stick::Maybe<Decomposed> m_absoluteDecomposedTransform;
    stick::Maybe<Vec2f> m_pivot;
    stick::DynamicArray<Symbol *> m_symbols;
    stick::Maybe<Mat32f> m_fillPaintTransform;
    stick::Maybe<Mat32f> m_strokePaintTransform;
    mutable bool m_fillPaintTransformDirty;
    mutable bool m_strokePaintTransformDirty;

    // style
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

    // bounds / length
    mutable stick::Maybe<Rect> m_fillBounds;
    mutable stick::Maybe<Rect> m_strokeBounds;
    mutable stick::Maybe<Rect> m_handleBounds;

    // rendering related
    RenderDataUniquePtr m_renderData;
};
}; // namespace paper

#endif // PAPER_ITEM_HPP
