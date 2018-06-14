#include <Paper2/Document.hpp>
#include <Paper2/Symbol.hpp>
#include <Crunch/MatrixFunc.hpp>

#define PROPERTY_GETTER(name, def) \
do \
{ \
    if(m_ ## name) \
    { \
        return *m_ ## name; \
    } \
    else if(m_parent) \
    { \
        return m_parent->name(); \
    }\
    else \
    { \
        return def; \
    } \
} \
while(false)

#define PROPERTY_SETTER(name, val) \
do \
{ \
    m_ ## name = val; \
    for(Item * child : m_children) \
        child->recursivelyResetProperty(&Item::m_ ## name); \
} \
while(false)

namespace paper
{
    using namespace stick;

    Item::Item(Allocator & _alloc, Document * _document, ItemType _type, const char * _name) :
        m_document(_document),
        m_type(_type),
        m_parent(nullptr),
        m_children(_alloc),
        m_bVisible(true),
        m_fill(NoPaint()),
        m_stroke(NoPaint()),
        m_name(_alloc)
    {
        if (_name)
            m_name.append(_name);
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
        //nothing to do by default
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

    void Item::setPosition(const Vec2f & _position)
    {
        translateTransform(_position - pivot());
    }

    void Item::setPivot(const Vec2f & _pivot)
    {
        m_pivot = _pivot;
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
        Mat32f mat = Mat32f::translation(-_center);
        mat.scale(_scale);
        mat.translate(_center);
        transform(mat);
    }

    void Item::rotateTransform(Float _radians)
    {
        rotateTransform(_radians, pivot());
    }

    void Item::rotateTransform(Float _radians, const Vec2f & _point)
    {
        Mat32f mat = Mat32f::translation(-_point);
        mat.rotate(_radians);
        mat.translate(_point);
        transform(mat);
    }

    void Item::skewTransform(const Vec2f & _angles)
    {
        skewTransform(_angles, pivot());
    }

    void Item::skewTransform(const Vec2f & _angles, const Vec2f & _center)
    {
        Mat32f mat = Mat32f::translation(-_center);
        mat.skew(_angles);
        mat.translate(_center);
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
        Mat32f mat = Mat32f::translation(-_center);
        mat.scale(_scale);
        mat.translate(_center);
        applyTransform(mat, true);
    }

    void Item::rotate(Float _radians)
    {
        rotate(_radians, pivot());
    }

    void Item::rotate(Float _radians, const Vec2f & _center)
    {
        Mat32f mat = Mat32f::translation(-_center);
        mat.rotate(_radians);
        mat.translate(_center);
        applyTransform(mat, true);
    }

    void Item::skew(const Vec2f & _angles)
    {
        skew(_angles, pivot());
    }

    void Item::skew(const Vec2f & _angles, const Vec2f & _center)
    {
        Mat32f mat = Mat32f::translation(-_center);
        mat.skew(_angles);
        mat.translate(_center);
        applyTransform(mat, true);
    }

    void Item::applyTransform(const Mat32f & _transform, bool _bMarkParentsBoundsDirty)
    {
        //by default an item is emtpy and has nothing to apply the transform to...
        applyTransformToChildren(_transform);
    }

    void Item::applyTransformToChildren(const Mat32f & _transform)
    {
        for (Item * c : m_children)
            c->applyTransform(_transform, false);
    }

    const Mat32f & Item::transform() const
    {
        static Mat32f s_proxy = Mat32f::identity();
        if (m_transform) return *m_transform;
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

    Float Item::rotation() const
    {
        return decomposeIfNeeded(m_decomposedTransform, transform()).rotation;
    }

    const Vec2f & Item::translation() const
    {
        return transform()[2];
    }

    Item::Decomposed & Item::decomposeIfNeeded(stick::Maybe<Decomposed> & _dec, const Mat32f & _transform) const
    {
        if (!_dec)
        {
            Decomposed dec;
            Vec2f trans; //we ignore it
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

    bool Item::isTransformed() const
    {
        if (hasTransform())
            return true;
        else if (m_parent)
            return m_parent->isTransformed();

        return false;
    }

    void Item::setStrokeJoin(StrokeJoin _join)
    {
        PROPERTY_SETTER(strokeJoin, _join);
    }

    void Item::setStrokeCap(StrokeCap _cap)
    {
        PROPERTY_SETTER(strokeCap, _cap);
    }

    void Item::setMiterLimit(Float _limit)
    {
        PROPERTY_SETTER(miterLimit, _limit);
    }

    void Item::setStrokeWidth(Float _width)
    {
        PROPERTY_SETTER(strokeWidth, _width);
    }

    void Item::setStroke(const ColorRGBA & _color)
    {
        PROPERTY_SETTER(stroke, _color);
    }

    void Item::setStroke(const String & _svgName)
    {
        PROPERTY_SETTER(stroke, crunch::svgColor<ColorRGBA>(_svgName));
    }

    void Item::setStroke(const LinearGradientPtr & _grad)
    {
        PROPERTY_SETTER(stroke, _grad);
    }

    void Item::setStroke(const RadialGradientPtr & _grad)
    {
        PROPERTY_SETTER(stroke, _grad);
    }

    void Item::setDashArray(const DashArray & _arr)
    {
        PROPERTY_SETTER(dashArray, _arr);
    }

    void Item::setDashOffset(Float _f)
    {
        PROPERTY_SETTER(dashOffset, _f);
    }

    void Item::setScaleStroke(bool _b)
    {
        PROPERTY_SETTER(scaleStroke, _b);
    }

    void Item::removeStroke()
    {
        PROPERTY_SETTER(stroke, NoPaint());
    }

    void Item::setFill(const ColorRGBA & _color)
    {
        PROPERTY_SETTER(fill, _color);
    }

    void Item::setFill(const stick::String & _svgName)
    {
        PROPERTY_SETTER(fill, crunch::svgColor<ColorRGBA>(_svgName));
    }

    void Item::setFill(const LinearGradientPtr & _grad)
    {
        PROPERTY_SETTER(fill, _grad);
    }

    void Item::setFill(const RadialGradientPtr & _grad)
    {
        PROPERTY_SETTER(fill, _grad);
    }

    void Item::removeFill()
    {
        PROPERTY_SETTER(fill, NoPaint());
    }

    void Item::setWindingRule(WindingRule _rule)
    {
        m_windingRule = _rule;
    }

    StrokeJoin Item::strokeJoin() const
    {
        PROPERTY_GETTER(strokeJoin, StrokeJoin::Bevel);
    }

    StrokeCap Item::strokeCap() const
    {
        PROPERTY_GETTER(strokeCap, StrokeCap::Butt);
    }

    Float Item::miterLimit() const
    {
        PROPERTY_GETTER(miterLimit, 4);
    }

    Float Item::strokeWidth() const
    {
        PROPERTY_GETTER(strokeWidth, 1.0);
    }

    const DashArray & Item::dashArray() const
    {
        static DashArray s_default;
        PROPERTY_GETTER(dashArray, s_default);
    }

    Float Item::dashOffset() const
    {
        PROPERTY_GETTER(dashOffset, 0.0);
    }

    WindingRule Item::windingRule() const
    {
        PROPERTY_GETTER(windingRule, WindingRule::EvenOdd);
    }

    bool Item::scaleStroke() const
    {
        PROPERTY_GETTER(scaleStroke, true);
    }

    Paint Item::fill() const
    {
        PROPERTY_GETTER(fill, ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
    }

    Paint Item::stroke() const
    {
        PROPERTY_GETTER(stroke, ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f));
    }

    bool Item::hasStroke() const
    {
        return !stroke().is<NoPaint>();
    }

    bool Item::hasFill() const
    {
        return !fill().is<NoPaint>();
    }

    Document * Item::document()
    {
        return m_document;
    }

    const Document * Item::document() const
    {
        return m_document;
    }

    ItemType Item::itemType() const
    {
        return m_type;
    }

    Maybe<Rect> Item::mergeWithChildrenBounds(
        const Maybe<Rect> & _bounds,
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
                //if a custom transform was passed along, we need to compute the absolute
                //transform for the child and pass it along.
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
        for (Item * child : m_children)
            child->markAbsoluteTransformDirty();
    }

    void Item::markBoundsDirty(bool _bNotifyParent)
    {
        markStrokeBoundsDirty(false);
        markFillBoundsDirty(false);
        if (_bNotifyParent && m_parent)
            m_parent->markBoundsDirty(true);
    }

    void Item::markStrokeBoundsDirty(bool _bNotifyParent)
    {
        m_strokeBounds.reset();
        if (_bNotifyParent && m_parent)
            m_parent->markStrokeBoundsDirty(true);
    }

    void Item::markFillBoundsDirty(bool _bNotifyParent)
    {
        m_fillBounds.reset();
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
        return Maybe<Rect>();
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
        _item->m_pivot = m_pivot;
        _item->m_fill = m_fill;
        _item->m_stroke = m_stroke;
        _item->m_strokeWidth = m_strokeWidth;
        _item->m_strokeJoin = m_strokeJoin;
        _item->m_strokeCap = m_strokeCap;
        _item->m_scaleStroke = m_scaleStroke;
        _item->m_miterLimit = m_miterLimit;
        _item->m_dashArray = m_dashArray;
        _item->m_dashOffset = m_dashOffset;
        _item->m_windingRule = m_windingRule;
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
}
