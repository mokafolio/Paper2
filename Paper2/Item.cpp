#include <Crunch/MatrixFunc.hpp>
#include <Paper2/BinFormat/BinFormatExport.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/SVG/SVGExport.hpp>
#include <Paper2/Symbol.hpp>
#include <Stick/FileUtilities.hpp>

#define PROPERTY_GETTER(name, def)                                                                 \
    do                                                                                             \
    {                                                                                              \
        if (m_style->m_##name)                                                                     \
        {                                                                                          \
            return *m_style->m_##name;                                                             \
        }                                                                                          \
        else if (m_parent)                                                                         \
        {                                                                                          \
            return m_parent->name();                                                               \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            return def;                                                                            \
        }                                                                                          \
    } while (false)

#define PROPERTY_SETTER(name, val)                                                                 \
    do                                                                                             \
    {                                                                                              \
        StylePtr & s = getOrCloneStyle();                                                          \
        s->name(val);                                                                              \
        for (Item * child : m_children)                                                            \
            child->name(val);                                                                      \
    } while (false)

namespace paper
{
using namespace stick;

Item::Item(Allocator & _alloc, Document * _document, ItemType _type, const char * _name) :
    m_document(_document),
    m_type(_type),
    m_parent(nullptr),
    m_children(_alloc),
    // m_fill(NoPaint()),
    // m_stroke(NoPaint()),
    m_name(_alloc),
    m_bVisible(true),
    m_lastRenderTransformID(-1),
    m_fillPaintTransformDirty(false),
    m_strokePaintTransformDirty(false)
{
    m_name.append(_name);

    if (m_type != ItemType::Document)
        setStyle(m_document->defaultStyle());
}

Item::~Item()
{
}

bool Item::addChild(Item * _e)
{
    if (canAddChild(_e))
    {
        _e->removeFromParent();
        _e->markAbsoluteTransformDirty();

        m_children.append(_e);
        markBoundsDirty(true);
        _e->m_parent = this;

        addedChild(_e);

        return true;
    }
    return false;
}

Item * Item::findChild(const String & _name) const
{
    for (Item * child : m_children)
    {
        if (child->name() == _name)
            return child;
    }
    return nullptr;
}

Item * Item::child(Size _idx) const
{
    if (_idx > m_children.count() - 1)
        return nullptr;

    return m_children[_idx];
}

bool Item::insertAbove(const Item * _e)
{
    return insertHelper(_e, true);
}

bool Item::insertBelow(const Item * _e)
{
    return insertHelper(_e, false);
}

bool Item::insertHelper(const Item * _e, bool _bAbove)
{
    if (_e->m_parent && _e->m_parent->canAddChild(this))
    {
        removeFromParent();

        auto it = stick::find(_e->m_parent->m_children.begin(), _e->m_parent->m_children.end(), _e);
        STICK_ASSERT(it != _e->m_parent->m_children.end());
        _e->m_parent->m_children.insert(_bAbove ? it + 1 : it, this);
        m_parent = _e->m_parent;

        _e->m_parent->addedChild(this);
        return true;
    }
    return false;
}

bool Item::sendToFront()
{
    if (m_parent)
    {
        auto it = stick::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
        m_parent->m_children.remove(it);
        m_parent->m_children.append(this);
        return true;
    }

    return false;
}

bool Item::sendToBack()
{
    if (m_parent)
    {
        auto it = stick::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
        m_parent->m_children.remove(it);
        m_parent->m_children.insert(m_parent->m_children.begin(), this);
        return true;
    }
    return false;
}

void Item::remove()
{
    removeHelper(true);
}

bool Item::removeChild(Item * _item)
{
    auto it = stick::find(m_children.begin(), m_children.end(), _item);
    if (it != m_children.end())
    {
        m_children.remove(it);
        removedChild(_item);
        return true;
    }
    return false;
}

void Item::removeChildren()
{
    for (Item * child : m_children)
        child->removeHelper(false);
    m_children.clear();
}

void Item::removeHelper(bool _bRemoveFromParent)
{
    removeChildren();
    if (_bRemoveFromParent)
        removeFromParent();

    m_style->itemRemovedStyle(this);
    m_document->destroyItem(this);
}

void Item::reverseChildren()
{
    std::reverse(m_children.begin(), m_children.end());
}

bool Item::canAddChild(Item * _e) const
{
    return false;
}

void Item::addedChild(Item * _e)
{
    // nothing to do by default
}

void Item::removedChild(Item * _e)
{
    // nothing to do by default
}

void Item::removeFromParent()
{
    if (m_parent)
    {
        auto it = stick::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
        STICK_ASSERT(it != m_parent->m_children.end());
        m_parent->m_children.remove(it);
        m_parent->markBoundsDirty(true);
        m_parent = nullptr;
    }
}

const ItemPtrArray & Item::children() const
{
    return m_children;
}

const String & Item::name() const
{
    return m_name;
}

Item * Item::parent() const
{
    return m_parent;
}

Item * Item::nextSibling() const
{
    STICK_ASSERT(m_parent);
    auto it = stick::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
    STICK_ASSERT(it != m_parent->m_children.end());
    ++it;
    if (it != m_parent->m_children.end())
        return *it;
    return nullptr;
}

Item * Item::previousSibling() const
{
    STICK_ASSERT(m_parent);
    auto it = stick::find(m_parent->m_children.rbegin(), m_parent->m_children.rend(), this);
    STICK_ASSERT(it != m_parent->m_children.rend());
    ++it;
    if (it != m_parent->m_children.rend())
        return *it;
    return nullptr;
}

bool Item::isDescendant(const Item * _item) const
{
    const Item * parent = this;
    while ((parent = parent->m_parent))
    {
        if (parent == _item)
            return true;
    }
    return false;
}

bool Item::isAncestor(const Item * _item) const
{
    return _item ? _item->isDescendant(this) : false;
}

void Item::setPosition(const Vec2f & _position)
{
    translateTransform(_position - pivot());
}

void Item::setPivot(const Vec2f & _pivot)
{
    m_pivot = _pivot;
}

void Item::removePivot()
{
    m_pivot.reset();
}

void Item::setVisible(bool _b)
{
    m_bVisible = _b;
}

void Item::setName(const String & _name)
{
    m_name = _name;
}

void Item::setTransform(const Mat32f & _transform)
{
    m_transform = _transform;
    transformChanged(false);
}

void Item::removeTransform()
{
    m_transform.reset();
    transformChanged(false);
}

void Item::transformChanged(bool _bCalledFromParent)
{
    markBoundsDirty(!_bCalledFromParent);
    m_absoluteTransform.reset();
    m_renderTransform.reset();

    for (Symbol * s : m_symbols)
        s->markAbsoluteTransformDirty();

    for (Item * child : m_children)
        child->transformChanged(true);
}

void Item::translateTransform(Float _x, Float _y)
{
    translateTransform(Vec2f(_x, _y));
}

void Item::translateTransform(const Vec2f & _translation)
{
    transform(Mat32f::translation(_translation));
}

void Item::scaleTransform(Float _scale)
{
    scaleTransform(Vec2f(_scale));
}

void Item::scaleTransform(Float _scaleX, Float _scaleY)
{
    scaleTransform(Vec2f(_scaleX, _scaleY));
}

void Item::scaleTransform(const Vec2f & _scale)
{
    scaleTransform(_scale, pivot());
}

void Item::scaleTransform(const Vec2f & _scale, const Vec2f & _center)
{
    Mat32f mat = Mat32f::translation(_center);
    mat.scale(_scale);
    mat.translate(-_center);
    transform(mat);
}

void Item::rotateTransform(Float _radians)
{
    rotateTransform(_radians, pivot());
}

void Item::rotateTransform(Float _radians, const Vec2f & _point)
{
    Mat32f mat = Mat32f::translation(_point);
    mat.rotate(_radians);
    mat.translate(-_point);
    transform(mat);
}

void Item::skewTransform(const Vec2f & _angles)
{
    skewTransform(_angles, pivot());
}

void Item::skewTransform(const Vec2f & _angles, const Vec2f & _center)
{
    Mat32f mat = Mat32f::translation(_center);
    mat.skew(_angles);
    mat.translate(-_center);
    transform(mat);
}

void Item::transform(const Mat32f & _transform)
{
    setTransform(_transform * transform());
}

void Item::translate(Float _x, Float _y)
{
    translate(Vec2f(_x, _y));
}

void Item::translate(const Vec2f & _translation)
{
    applyTransform(Mat32f::translation(_translation), true);
}

void Item::scale(Float _scale)
{
    scale(Vec2f(_scale));
}

void Item::scale(Float _scaleX, Float _scaleY)
{
    scale(Vec2f(_scaleX, _scaleY));
}

void Item::scale(const Vec2f & _scale)
{
    scale(_scale, pivot());
}

void Item::scale(const Vec2f & _scale, const Vec2f & _center)
{
    auto center = _center - translation();
    Mat32f mat = Mat32f::translation(center);
    mat.scale(_scale);
    mat.translate(-center);
    applyTransform(mat, true);
}

void Item::rotate(Float _radians)
{
    rotate(_radians, pivot());
}

void Item::rotate(Float _radians, const Vec2f & _center)
{
    auto center = _center - translation();
    Mat32f mat = Mat32f::translation(center);
    mat.rotate(_radians);
    mat.translate(-center);
    applyTransform(mat, true);
}

void Item::skew(const Vec2f & _angles)
{
    skew(_angles, pivot());
}

void Item::skew(const Vec2f & _angles, const Vec2f & _center)
{
    auto center = _center - translation();
    Mat32f mat = Mat32f::translation(center);
    mat.skew(_angles);
    mat.translate(-center);
    applyTransform(mat, true);
}

void Item::applyTransform(const Mat32f & _transform, bool _bMarkParentsBoundsDirty)
{
    // by default an item is emtpy and has nothing to apply the transform to...
    applyTransformToChildrenAndPivot(_transform);

    //...but we mark the bounds dirty
    markBoundsDirty(_bMarkParentsBoundsDirty);
}

void Item::applyTransformToChildrenAndPivot(const Mat32f & _transform)
{
    if (m_pivot)
        m_pivot = _transform * *m_pivot;

    for (Item * c : m_children)
        c->applyTransform(_transform, false);
}

const Mat32f & Item::transform() const
{
    static Mat32f s_proxy = Mat32f::identity();
    if (m_transform)
        return *m_transform;
    return s_proxy;
}

const Mat32f & Item::absoluteTransform() const
{
    if (!m_absoluteTransform)
    {
        if (m_parent && m_transform)
            m_absoluteTransform = m_parent->absoluteTransform() * *m_transform;
        else if (m_parent)
            m_absoluteTransform = m_parent->absoluteTransform();
        else
            m_absoluteTransform = transform();
    }
    return *m_absoluteTransform;
}

void Item::setRenderTransform(const Mat32f & _trans, Size _lastRenderTransformID)
{
    m_renderTransform = _trans;
    m_lastRenderTransformID = _lastRenderTransformID;
}

const Maybe<Mat32f> & Item::renderTransform() const
{
    return m_renderTransform;
}

Size Item::lastRenderTransformID() const
{
    return m_lastRenderTransformID;
}

Float Item::rotation() const
{
    return decomposeIfNeeded(m_decomposedTransform, transform()).rotation;
}

const Vec2f & Item::translation() const
{
    return transform()[2];
}

Item::Decomposed & Item::decomposeIfNeeded(stick::Maybe<Decomposed> & _dec,
                                           const Mat32f & _transform) const
{
    if (!_dec)
    {
        Decomposed dec;
        Vec2f trans; // we ignore it
        crunch::decompose(_transform, trans, dec.rotation, dec.skew, dec.scaling);
        _dec = dec;
    }
    return *_dec;
}

const Vec2f & Item::scaling() const
{
    return decomposeIfNeeded(m_decomposedTransform, transform()).scaling;
}

Float Item::absoluteRotation() const
{
    return decomposeIfNeeded(m_absoluteDecomposedTransform, transform()).rotation;
}

const Vec2f & Item::absoluteTranslation() const
{
    return absoluteTransform()[2];
}

const Vec2f & Item::absoluteScaling() const
{
    return decomposeIfNeeded(m_absoluteDecomposedTransform, transform()).scaling;
}

const Rect & Item::bounds() const
{
    if (!m_fillBounds)
    {
        auto mb = computeBounds(nullptr, BoundsType::Fill);
        m_fillBounds = mb ? *mb : noBounds();
    }

    return *m_fillBounds;
}

const Rect & Item::handleBounds() const
{
    if (!m_handleBounds)
    {
        auto mb = computeBounds(nullptr, BoundsType::Handle);
        m_handleBounds = mb ? *mb : noBounds();
    }
    return *m_handleBounds;
}

const Rect & Item::strokeBounds() const
{
    if (!m_strokeBounds)
    {
        auto mb = computeBounds(nullptr, BoundsType::Stroke);
        m_strokeBounds = mb ? *mb : noBounds();
    }
    return *m_strokeBounds;
}

Vec2f Item::position() const
{
    return bounds().center();
}

Vec2f Item::pivot() const
{
    if (m_pivot)
        return *m_pivot;
    return position();
}

bool Item::isVisible() const
{
    return m_bVisible;
}

bool Item::hasTransform() const
{
    return (bool)m_transform;
}

bool Item::hasPivot() const
{
    return (bool)m_pivot;
}

bool Item::isTransformed() const
{
    if (hasTransform())
        return true;
    else if (m_parent)
        return m_parent->isTransformed();

    return false;
}

const Mat32f & Item::fillPaintTransform() const
{
    static Mat32f s_proxy = Mat32f::identity();
    if (m_fillPaintTransform)
        return *m_fillPaintTransform;
    return s_proxy;
}

const Mat32f & Item::strokePaintTransform() const
{
    static Mat32f s_proxy = Mat32f::identity();
    if (m_strokePaintTransform)
        return *m_strokePaintTransform;
    return s_proxy;
}

//@TODO: diff settings that affect stroke bounds and mark them dirty accordingly
void Item::setStyle(const StylePtr & _style)
{
    printf("SETTING STYLE\n");
    STICK_ASSERT(_style);
    if (m_style)
        m_style->itemRemovedStyle(this);

    printf("b\n");
    bool bDifferent = m_style ? strokeBoundsDifferent(m_style->m_data, _style->m_data) : true;
    m_style = _style;
    m_style->itemAddedStyle(this);

     printf("c\n");
    if (bDifferent)
        markStrokeBoundsDirty(true);

    for (Item * child : m_children)
        child->setStyle(_style);
     printf("d\n");
}

void Item::setStyle(const StyleData & _data)
{
    auto & styleptr = getOrCloneStyle();
    *styleptr = _data;
    for (Item * child : m_children)
        child->setStyle(styleptr);
}

void Item::setStrokeJoin(StrokeJoin _join)
{
    PROPERTY_SETTER(setStrokeJoin, _join);
    // markStrokeBoundsDirty(true);
}

void Item::setStrokeCap(StrokeCap _cap)
{
    PROPERTY_SETTER(setStrokeCap, _cap);
    // markStrokeBoundsDirty(true);
}

void Item::setMiterLimit(Float _limit)
{
    PROPERTY_SETTER(setMiterLimit, _limit);
    // markStrokeBoundsDirty(true);
}

void Item::setStrokeWidth(Float _width)
{
    PROPERTY_SETTER(setStrokeWidth, _width);
    // markStrokeBoundsDirty(true);
}

void Item::setStroke(const String & _svgName)
{
    setStroke(crunch::svgColor<ColorRGBA>(_svgName));
}

void Item::setStroke(const Paint & _paint)
{
    bool bps = stroke().is<NoPaint>();
    PROPERTY_SETTER(setStroke, _paint);
    if (!bps)
        markStrokeBoundsDirty(true);
}

void Item::setDashArray(const DashArray & _arr)
{
    PROPERTY_SETTER(setDashArray, _arr);
}

void Item::setDashOffset(Float _f)
{
    PROPERTY_SETTER(setDashOffset, _f);
}

void Item::setScaleStroke(bool _b)
{
    PROPERTY_SETTER(setScaleStroke, _b);
    // markStrokeBoundsDirty(true);
}

void Item::removeStroke()
{
    // PROPERTY_SETTER(setStroke, NoPaint());
    // markStrokeBoundsDirty(true);
    setStroke(NoPaint());
}

void Item::setFill(const stick::String & _svgName)
{
    setFill(crunch::svgColor<ColorRGBA>(_svgName));
}

void Item::setFill(const Paint & _paint)
{
    PROPERTY_SETTER(setFill, _paint);
}

void Item::removeFill()
{
    setFill(NoPaint());
}

void Item::setWindingRule(WindingRule _rule)
{
    PROPERTY_SETTER(setWindingRule, _rule);
}

void Item::setFillPaintTransform(const Mat32f & _transform)
{
    m_fillPaintTransform = _transform;
    m_fillPaintTransformDirty = true;
}

void Item::setStrokePaintTransform(const Mat32f & _transform)
{
    m_strokePaintTransform = _transform;
    m_strokePaintTransformDirty = true;
}

void Item::removeFillPaintTransform()
{
    m_fillPaintTransform.reset();
    m_fillPaintTransformDirty = true;
}

void Item::removeStrokePaintTransform()
{
    m_strokePaintTransform.reset();
    m_strokePaintTransformDirty = true;
}

StrokeJoin Item::strokeJoin() const
{
    // return resolvedStyle().strokeJoin;
    return m_style->strokeJoin();
}

StrokeCap Item::strokeCap() const
{
    // return resolvedStyle().strokeCap;
    return m_style->strokeCap();
}

Float Item::miterLimit() const
{
    // return resolvedStyle().miterLimit;
    return m_style->miterLimit();
}

Float Item::strokeWidth() const
{
    // return resolvedStyle().strokeWidth;
    return m_style->strokeWidth();
}

const DashArray & Item::dashArray() const
{
    // return resolvedStyle().dashArray;
    return m_style->dashArray();
}

Float Item::dashOffset() const
{
    // return resolvedStyle().dashOffset;
    return m_style->dashOffset();
}

WindingRule Item::windingRule() const
{
    // return resolvedStyle().windingRule;
    return m_style->windingRule();
}

bool Item::scaleStroke() const
{
    // return resolvedStyle().scaleStroke;
    return m_style->scaleStroke();
}

Paint Item::fill() const
{
    // return resolvedStyle().fill;
    return m_style->fill();
}

Paint Item::stroke() const
{
    // return resolvedStyle().stroke;
    return m_style->stroke();
}

// bool Item::isAffectedByFill() const
// {
//     if (m_style->hasFill())
//         return true;
//     if (m_parent)
//         return m_parent->isAffectedByFill();
//     return false;
// }

// bool Item::isAffectedByStroke() const
// {
//     if (m_style->hasStroke())
//         return true;
//     if (m_parent)
//         return m_parent->isAffectedByStroke();
//     return false;
// }

// bool Item::hasStroke() const
// {
//     // return !stroke().is<NoPaint>();
//     return m_style->hasStroke();
// }

// bool Item::hasFill() const
// {
//     // return !fill().is<NoPaint>();
//     return m_style->hasFill();
// }

// bool Item::hasScaleStroke() const
// {
//     return m_style->hasScaleStroke();
// }

// bool Item::hasMiterLimit() const
// {
//     return m_style->hasMiterLimit();
// }

// bool Item::hasWindingRule() const
// {
//     return m_style->hasWindingRule();
// }

// bool Item::hasDashOffset() const
// {
//     return m_style->hasDashOffset();
// }

// bool Item::hasDashArray() const
// {
//     return m_style->hasDashArray();
// }

// bool Item::hasStrokeWidth() const
// {
//     return m_style->hasStrokeWidth();
// }

// bool Item::hasStrokeCap() const
// {
//     return m_style->hasStrokeCap();
// }

// bool Item::hasStrokeJoin() const
// {
//     return m_style->hasStrokeJoin();
// }

bool Item::hasfillPaintTransform() const
{
    return (bool)m_fillPaintTransform;
}

bool Item::hasStrokePaintTransform() const
{
    return (bool)m_strokePaintTransform;
}

Document * Item::document() const
{
    return m_document;
}

ItemType Item::itemType() const
{
    return m_type;
}

Maybe<Rect> Item::mergeWithChildrenBounds(const Maybe<Rect> & _bounds,
                                          const Mat32f * _transform,
                                          BoundsType _type,
                                          bool _bSkipFirstChild) const
{
    Maybe<Rect> ret = _bounds;
    Mat32f tmpMat;
    auto it = m_children.begin() + _bSkipFirstChild;
    for (; it != m_children.end(); ++it)
    {
        Maybe<Rect> tmp;
        if (_transform)
        {
            // if a custom transform was passed along, we need to compute the absolute
            // transform for the child and pass it along.
            tmpMat = *_transform * (*it)->transform();
            tmp = (*it)->computeBounds(&tmpMat, _type);
        }
        else
            tmp = (*it)->computeBounds(nullptr, _type);

        if (tmp)
        {
            if (ret)
                ret = crunch::merge(*ret, *tmp);
            else
                ret = *tmp;
        }
    }
    return ret;
}

void Item::markAbsoluteTransformDirty()
{
    m_absoluteTransform.reset();
    m_renderTransform.reset();
    for (Item * child : m_children)
        child->markAbsoluteTransformDirty();
}

void Item::markBoundsDirty(bool _bNotifyParent)
{
    markStrokeBoundsDirty(_bNotifyParent);
    markFillBoundsDirty(_bNotifyParent);
}

void Item::markStrokeBoundsDirty(bool _bNotifyParent)
{
    m_strokeBounds.reset();
    if (_bNotifyParent && m_parent)
        m_parent->markStrokeBoundsDirty(true);
    //@TODO: Can this be removed?
    markSymbolsDirty();
}

void Item::markFillBoundsDirty(bool _bNotifyParent)
{
    m_fillBounds.reset();
    m_handleBounds.reset();
    if (_bNotifyParent && m_parent)
        m_parent->markFillBoundsDirty(true);
}

Rect Item::noBounds()
{
    //@TODO: or nan?
    return Rect(std::numeric_limits<Float>::infinity(),
                std::numeric_limits<Float>::infinity(),
                std::numeric_limits<Float>::infinity(),
                std::numeric_limits<Float>::infinity());
}

Maybe<Rect> Item::computeBounds(const Mat32f * _transform, BoundsType _type) const
{
    return mergeWithChildrenBounds(Maybe<Rect>(), _transform, _type, false);
}

void Item::cloneItemTo(Item * _item) const
{
    // @TODO: generate default protected copy constructors for all the items?
    // as most/all of the members are copy constructible that might be less
    // error prone. Then only parent and children would have to be touched during cloning.
    _item->m_document = m_document;
    _item->m_type = m_type;
    _item->m_bVisible = m_bVisible;
    _item->m_transform = m_transform;
    _item->m_absoluteTransform = m_absoluteTransform;
    _item->m_decomposedTransform = m_decomposedTransform;
    _item->m_absoluteDecomposedTransform = m_absoluteDecomposedTransform;
    _item->m_fillPaintTransform = m_fillPaintTransform;
    _item->m_strokePaintTransform = m_strokePaintTransform;
    _item->m_fillPaintTransformDirty = m_fillPaintTransformDirty;
    _item->m_strokePaintTransformDirty = m_strokePaintTransformDirty;
    _item->m_pivot = m_pivot;

    // _item->m_fill = m_fill;
    // _item->m_stroke = m_stroke;
    // _item->m_strokeWidth = m_strokeWidth;
    // _item->m_strokeJoin = m_strokeJoin;
    // _item->m_strokeCap = m_strokeCap;
    // _item->m_scaleStroke = m_scaleStroke;
    // _item->m_miterLimit = m_miterLimit;
    // _item->m_dashArray = m_dashArray;
    // _item->m_dashOffset = m_dashOffset;
    // _item->m_windingRule = m_windingRule;
    _item->setStyle(m_style);

    _item->m_fillBounds = m_fillBounds;
    _item->m_strokeBounds = m_strokeBounds;
    _item->m_handleBounds = m_handleBounds;

    if (m_renderData)
        _item->m_renderData = m_renderData->clone();

    for (Item * c : m_children)
        _item->addChild(c->clone());

    _item->insertAbove(this);
}

RenderData * Item::renderData()
{
    return m_renderData.get();
}

void Item::setRenderData(RenderDataUniquePtr _ptr)
{
    m_renderData = std::move(_ptr);
}

TextResult Item::exportSVG() const
{
    return svg::exportItem(this, m_document->allocator(), true);
}

Result<DynamicArray<UInt8>> Item::exportBinary() const
{
    DynamicArray<UInt8> ret(document()->allocator());
    Error err = binfmt::exportItem(this, ret);
    if (err)
        return err;
    return ret;
}

Error Item::saveSVG(const String & _uri) const
{
    auto res = exportSVG();
    if (!res)
        return res.error();
    return saveTextFile(res.get(), _uri);
}

bool Item::cleanDirtyFillPaintTransform()
{
    bool ret = m_fillPaintTransformDirty;
    m_fillPaintTransformDirty = false;
    return ret;
}

bool Item::cleanDirtyStrokePaintTransform()
{
    bool ret = m_strokePaintTransformDirty;
    m_strokePaintTransformDirty = false;
    return ret;
}

void Item::markSymbolsDirty() const
{
    for (Symbol * s : m_symbols)
        ++s->m_version;
}

void Item::hierarchyString(String & _outputString, Size _indent) const
{
    String indent;
    for (Size i = 0; i < _indent; ++i)
        indent.append("    ");

    switch (itemType())
    {
    case ItemType::Document:
        _outputString.append(AppendVariadicFlag(), indent, "Document ", name(), ":\n");
        break;
    case ItemType::Group:
        _outputString.append(AppendVariadicFlag(), indent, "Group ", name(), ":\n");
        break;
    case ItemType::Path:
        _outputString.append(AppendVariadicFlag(), indent, "Path ", name(), ":\n");
        break;
    case ItemType::Symbol:
        _outputString.append(AppendVariadicFlag(), indent, "Symbol ", name(), ":\n");
        break;
    default:
        break;
    }

    for (auto * child : children())
        child->hierarchyString(_outputString, _indent + 1);
}

HitTestSettings::HitTestSettings() : curveTolerance(2.5f), mode(HitTestFill | HitTestCurves)
{
}

bool HitTestSettings::testFill() const
{
    return (mode & HitTestFill) == HitTestFill;
}

bool HitTestSettings::testCurves() const
{
    return (mode & HitTestCurves) == HitTestCurves;
}

Maybe<HitTestResult> Item::hitTest(const Vec2f & _pos, const HitTestSettings & _settings) const
{
    HitTestResultArray tmp(m_children.allocator());
    performHitTest(_pos, _settings, false, nullptr, tmp);
    return tmp.count() ? tmp.first() : Maybe<HitTestResult>();
}

HitTestResultArray Item::hitTestAll(const Vec2f & _pos, const HitTestSettings & _settings) const
{
    HitTestResultArray tmp(m_children.allocator());
    performHitTest(_pos, _settings, true, nullptr, tmp);
    return tmp;
}

bool Item::hitTestChildren(const Vec2f & _pos,
                           const HitTestSettings & _settings,
                           bool _bMultiple,
                           const Mat32f * _transform,
                           HitTestResultArray & _outResults) const
{
    Size startCount = _outResults.count();
    Mat32f tmp;
    for (auto it = children().rbegin(); it != children().rend(); ++it)
    {
        if (_transform)
        {
            Mat32f tmp = *_transform * (*it)->transform();
            if ((*it)->performHitTest(_pos, _settings, _bMultiple, &tmp, _outResults) &&
                !_bMultiple)
                return true;
        }
        else if ((*it)->performHitTest(_pos, _settings, _bMultiple, nullptr, _outResults) &&
                 !_bMultiple)
            return true;
    }
    return startCount < _outResults.count();
}

bool Item::performHitTest(const Vec2f & _pos,
                          const HitTestSettings & _settings,
                          bool _bMultiple,
                          const Mat32f * _transform,
                          HitTestResultArray & _outResults) const
{
    return this->hitTestChildren(_pos, _settings, _bMultiple, _transform, _outResults);
}

ItemPtrArray Item::selectChildren(const Rect & _area)
{
    DynamicArray<Item *> ret(m_children.allocator());
    for (Item * child : m_children)
    {
        if (child->performSelectionTest(_area))
            ret.append(child);
    }
    return ret;
}

bool Item::performSelectionTest(const Rect & _rect) const
{
    for (auto it = children().rbegin(); it != children().rend(); ++it)
    {
        if ((*it)->performSelectionTest(_rect))
            return true;
    }
    return false;
}

StylePtr & Item::getOrCloneStyle()
{
    if (m_style.useCount() > 1)
    {
        m_style->itemRemovedStyle(this);
        m_style = m_style->clone(this);
    }
    return m_style;
}

// const ResolvedStyle & Item::resolvedStyle() const
// {
//     if (m_bStyleDirty)
//     {
//         m_bStyleDirty = false;
//         m_resolvedStyle = {
//             resolveStyleProperty(&Style::m_fill, Style::defaultFill()),
//             resolveStyleProperty(&Style::m_stroke, Style::defaultStroke()),
//             resolveStyleProperty(&Style::m_strokeWidth, Style::defaultStrokeWidth()),
//             resolveStyleProperty(&Style::m_strokeJoin, Style::defaultStrokeJoin()),
//             resolveStyleProperty(&Style::m_strokeCap, Style::defaultStrokeCap()),
//             resolveStyleProperty(&Style::m_scaleStroke, Style::defaultScaleStroke()),
//             resolveStyleProperty(&Style::m_miterLimit, Style::defaultMiterLimit()),
//             resolveStyleProperty(&Style::m_dashArray, Style::defaultDashArray()),
//             resolveStyleProperty(&Style::m_dashOffset, Style::defaultDashOffset()),
//             resolveStyleProperty(&Style::m_windingRule, Style::defaultWindingRule())
//         };
//     }

//     return m_resolvedStyle;
// }

const Style & Item::style() const
{
    return *m_style;
}

const StylePtr & Item::stylePtr() const
{
    return m_style;
}

} // namespace paper
