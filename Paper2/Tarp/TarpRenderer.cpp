#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Symbol.hpp>
#include <Paper2/Tarp/TarpRenderer.hpp>

#include <GL/gl3w.h>

#define TARP_IMPLEMENTATION_OPENGL
#include <Tarp/Tarp.h>

namespace paper
{
namespace tarp
{
using namespace stick;

using tpSegmentArray = stick::DynamicArray<tpSegment>;

namespace detail
{
struct TarpStuff
{
    tpContext ctx;
    //@TODO: allow to pass in an allocator for this
    tpSegmentArray tmpSegmentBuffer;
};

struct TarpPathData : public RenderData
{
    TarpPathData(tpPath _path = tpPathInvalidHandle()) : path(_path)
    {
    }

    ~TarpPathData()
    {
        tpPathDestroy(path);
    }

    RenderDataUniquePtr clone() const
    {
        //@TODO: Proper allocator
        return makeUnique<TarpPathData>(defaultAllocator(), tpPathClone(path));
    }

    tpPath path;
};

struct TarpSymbolData : public RenderData
{
    struct RefItem
    {
        Item * lastItem;
        Size lastSymbolVersion;
        tpRenderCache cache;
    };

    TarpSymbolData()
    {
    }

    ~TarpSymbolData()
    {
        for (auto & ri : caches)
            tpRenderCacheDestroy(ri.cache);
    }

    tpRenderCache addItem(Item * _item, Size _idx, Size _version)
    {
        Size lastSize = caches.count();
        if (_idx >= lastSize)
        {
            caches.resize(_idx + 1);
            for (Size i = lastSize; i <= _idx; ++i)
            {
                caches[i].lastSymbolVersion = -1;
                caches[i].lastItem = nullptr;
                caches[i].cache = tpRenderCacheCreate();
            }
        }

        bool bNeedsUpdate =
            _item != caches[_idx].lastItem || caches[_idx].lastSymbolVersion != _version;
        caches[_idx].lastItem = _item;
        caches[_idx].lastSymbolVersion = _version;

        return bNeedsUpdate ? caches[_idx].cache : tpRenderCacheInvalidHandle();
    }

    RenderDataUniquePtr clone() const
    {
        //@TODO: not cloning is the right thing to do here?
        return RenderDataUniquePtr();
    }

    DynamicArray<RefItem> caches;
};

struct TarpGradientData : public RenderData
{
    TarpGradientData(tpGradient _grad = tpGradientInvalidHandle()) : gradient(_grad)
    {
    }

    ~TarpGradientData()
    {
        tpGradientDestroy(gradient);
    }

    RenderDataUniquePtr clone() const
    {
        //@TODO: Proper allocator
        return makeUnique<TarpGradientData>(defaultAllocator(), tpGradientClone(gradient));
    }

    tpGradient gradient;
};
} // namespace detail

struct Gl3wInitializer
{
    Gl3wInitializer() : bError(true)
    {
        bError = gl3wInit();
    }

    bool bError;
};

static bool ensureGl3w()
{
    static Gl3wInitializer s_initializer;
    STICK_ASSERT(!s_initializer.bError);
    return s_initializer.bError;
}

TarpRenderer::TarpRenderer()
{
}

TarpRenderer::TarpRenderer(TarpRenderer && _other) :
    m_tarp(std::move(_other.m_tarp)),
    m_viewport(std::move(_other.m_viewport)),
    m_transform(std::move(_other.m_transform)),
    m_transformID(std::move(_other.m_transformID))
{
}

TarpRenderer::~TarpRenderer()
{
    if (m_tarp)
        tpContextDestroy(m_tarp->ctx);
}

Error TarpRenderer::init(Document & _doc)
{
    if (ensureGl3w())
    {
        //@TODO: Better error code
        return Error(ec::InvalidOperation, "Could not initialize opengl", STICK_FILE, STICK_LINE);
    }

    m_tarp = makeUnique<detail::TarpStuff>();

    m_tarp->ctx = tpContextCreate();
    if (!tpContextIsValidHandle(m_tarp->ctx))
    {
        return Error(ec::InvalidOperation,
                     String::formatted("Could not init Tarp context: %s\n", tpErrorMessage()),
                     STICK_FILE,
                     STICK_LINE);
    }

    this->m_document = &_doc;
    m_transform = Mat32f::identity();
    m_transformID = 0;

    return Error();
}

void TarpRenderer::setViewport(Float _x, Float _y, Float _widthInPixels, Float _heightInPixels)
{
    m_viewport = Rect(_x, _y, _x + _widthInPixels, _y + _heightInPixels);
}

void TarpRenderer::setProjection(const Mat4f & _proj)
{
    tpSetProjection(m_tarp->ctx, (const tpMat4 *)_proj.ptr());
}

void TarpRenderer::setTransform(const Mat32f & _transform)
{
    m_transform = _transform;
    ++m_transformID;
}

void TarpRenderer::setDefaultProjection()
{
    tpSetDefaultProjection(m_tarp->ctx, m_document->width(), m_document->height());
}

static detail::TarpPathData & ensureRenderData(Path * _path)
{
    if (!_path->renderData())
        _path->setRenderData(makeUnique<detail::TarpPathData>(defaultAllocator(), tpPathCreate()));
    return *static_cast<detail::TarpPathData *>(_path->renderData());
}

static detail::TarpSymbolData & ensureRenderData(Symbol * _symbol)
{
    if (!_symbol->renderData())
        _symbol->setRenderData(makeUnique<detail::TarpSymbolData>(defaultAllocator()));
    return *static_cast<detail::TarpSymbolData *>(_symbol->renderData());
}

// static detail::TarpGradientData & ensureRenderData(LinearGradientPtr _gradient)
// {
//     if (!_gradient->renderData())
//         _gradient->setRenderData(makeUnique<detail::TarpGradientData>(
//             defaultAllocator(), tpGradientCreateLinear(0, 0, 0, 0)));
//     return *static_cast<detail::TarpGradientData *>(_gradient->renderData());
// }

static void toTarpSegments(tpSegmentArray & _tmpData, Path * _path, const Mat32f * _transform)
{
    _tmpData.clear();
    if (!_transform)
    {
        for (Size i = 0; i < _path->segmentData().count(); ++i)
        {
            ConstSegment seg = _path->segment(i);
            Vec2f hi = seg.handleInAbsolute();
            Vec2f ho = seg.handleOutAbsolute();
            _tmpData.append((tpSegment){
                { hi.x, hi.y }, { seg.position().x, seg.position().y }, { ho.x, ho.y } });
        }
    }
    else
    {
        // tarp does not support per contour transforms, so we need to bring child paths segments
        // to path space before adding it as a contour!
        for (Size i = 0; i < _path->segmentData().count(); ++i)
        {
            ConstSegment seg = _path->segment(i);
            Vec2f hi = *_transform * seg.handleInAbsolute();
            Vec2f pos = *_transform * seg.position();
            Vec2f ho = *_transform * seg.handleOutAbsolute();
            _tmpData.append((tpSegment){ { hi.x, hi.y }, { pos.x, pos.y }, { ho.x, ho.y } });
        }
    }
}

static void recursivelyUpdateTarpPath(tpSegmentArray & _tmpData,
                                      Path * _path,
                                      tpPath _tarpPath,
                                      const Mat32f * _transform,
                                      UInt32 & _contourIndex,
                                      bool _bContoursDirty)
{
    if (_path->cleanDirtyGeometry() || (_contourIndex > 0 && _bContoursDirty))
    {
        toTarpSegments(_tmpData, _path, _transform);
        tpPathSetContour(
            _tarpPath, _contourIndex, &_tmpData[0], _tmpData.count(), (tpBool)_path->isClosed());
    }

    _contourIndex += 1;

    for (Item * c : _path->children())
    {
        const Mat32f * t = _transform;
        Mat32f tmp;
        if (c->hasTransform())
        {
            if (_transform)
            {
                tmp = *_transform * c->transform();
                t = &tmp;
            }
            else
            {
                t = &c->transform();
            }
        }

        recursivelyUpdateTarpPath(_tmpData, static_cast<Path *>(c), _tarpPath, t, _contourIndex, _bContoursDirty);
    }
}

static void updateTarpPath(tpSegmentArray & _tmpData,
                           Path * _path,
                           tpPath _tarpPath,
                           const Mat32f * _transform)
{
    UInt32 contourIndex = 0;
    recursivelyUpdateTarpPath(_tmpData, _path, _tarpPath, _transform, contourIndex, _path->cleanDirtyContours());

    // remove contours that are not used anymore
    if (contourIndex < tpPathContourCount(_tarpPath))
    {
        for (Size i = tpPathContourCount(_tarpPath) - 1; i >= contourIndex; --i)
            tpPathRemoveContour(_tarpPath, i);
    }

    // printf("CONT C %i\n", tpPathContourCount(_tarpPath));

    // potentially update the stroke/fill transforms
    if (_path->hasfillPaintTransform() && _path->cleanDirtyFillPaintTransform())
        tpPathSetFillPaintTransform(_tarpPath, (tpTransform *)&_path->fillPaintTransform());

    if (_path->hasStrokePaintTransform() && _path->cleanDirtyStrokePaintTransform())
        tpPathSetStrokePaintTransform(_tarpPath, (tpTransform *)&_path->strokePaintTransform());
}

static detail::TarpGradientData & updateTarpGradient(BaseGradient & _grad)
{
    if (!_grad.renderData())
        _grad.setRenderData(makeUnique<detail::TarpGradientData>(
            defaultAllocator(),
            _grad.type() == GradientType::Linear ? tpGradientCreateLinear(0, 0, 0, 0)
                                                 : tpGradientCreateRadialSymmetric(0, 0, 0)));

    detail::TarpGradientData & gd = *static_cast<detail::TarpGradientData *>(_grad.renderData());

    if (_grad.cleanDirtyPositions())
    {
        tpGradientSetPositions(gd.gradient,
                               _grad.origin().x,
                               _grad.origin().y,
                               _grad.destination().x,
                               _grad.destination().y);
        if (_grad.type() == GradientType::Radial)
        {
            RadialGradient & rg = static_cast<RadialGradient &>(_grad);
            if (auto & fo = rg.focalPointOffset())
                tpGradientSetFocalPointOffset(gd.gradient, (*fo).x, (*fo).y);
            if (auto & r = rg.ratio())
                tpGradientSetRatio(gd.gradient, *r);
        }
    }

    if (_grad.cleanDirtyStops())
    {
        tpGradientClearColorStops(gd.gradient);
        for (auto & stop : _grad.stops())
        {
            tpGradientAddColorStop(
                gd.gradient, stop.color.r, stop.color.g, stop.color.b, stop.color.a, stop.offset);
        }
    }
    return gd;
}

// ugly template helper using pointer to member
// so we don't have to write the code for setting stroke and fill twice
template <class M>
static void setPaint(tpStyle * _style, M _styleMember, const Paint & _paint)
{
    if (_paint.is<ColorRGBA>())
    {
        const ColorRGBA & col = _paint.get<ColorRGBA>();
        _style->*_styleMember = tpPaintMakeColor(col.r, col.g, col.b, col.a);
    }
    else if (_paint.is<LinearGradientPtr>() ||
             _paint.is<RadialGradientPtr>())
    {
        SharedPtr<BaseGradient> ptr;
        if (_paint.is<LinearGradientPtr>())
            ptr = _paint.get<LinearGradientPtr>();
        else
            ptr = _paint.get<RadialGradientPtr>();

        if (ptr)
        {
            detail::TarpGradientData & gd = updateTarpGradient(*ptr);
            _style->*_styleMember = tpPaintMakeGradient(gd.gradient);
        }
    }
    else
    {
        (_style->*_styleMember).type = kTpPaintTypeNone;
    }
}

// helper to pick the correct style between a potential symbol and its path
// template <class Ret>
// Ret property(Item * _prio,
//              Item * _other,
//              bool (Item::*_haser)() const,
//              Ret (Item::*_getter)() const)
// {
//     if (!_other || (_prio && (_prio->*_haser)()))
//         return (_prio->*_getter)();

//     return (_other->*_getter)();
// }

// static tpStyle makeStyle(Path * _path, Symbol * _symbol, bool _bPrioritizeSymbol = false)
// {
//     Item *prio, *other;
//     if (_bPrioritizeSymbol)
//     {
//         prio = _symbol;
//         other = _path;
//     }
//     else
//     {
//         prio = _path;
//         other = _symbol;
//     }

//     STICK_ASSERT(prio || other);

//     tpStyle style = tpStyleMake();
//     style.fillRule =
//         property(prio, other, &Item::hasWindingRule, &Item::windingRule) == WindingRule::NonZero
//             ? kTpFillRuleNonZero
//             : kTpFillRuleEvenOdd;

//     if (!other || (prio && prio->hasFill()))
//         setPaint(&style, &tpStyle::fill, prio, &Item::fill);
//     else
//         setPaint(&style, &tpStyle::fill, other, &Item::fill);

//     if (!other || (prio && prio->hasStroke()))
//         setPaint(&style, &tpStyle::stroke, prio, &Item::stroke);
//     else
//         setPaint(&style, &tpStyle::stroke, other, &Item::stroke);

//     Float sw = property(prio, other, &Item::hasStrokeWidth, &Item::strokeWidth);
//     if (style.stroke.type != kTpPaintTypeNone && sw)
//     {
//         style.strokeWidth = sw;
//         style.miterLimit = property(prio, other, &Item::hasMiterLimit, &Item::miterLimit);
//         style.scaleStroke =
//             (tpBool)property(_path, _symbol, &Item::hasScaleStroke, &Item::scaleStroke);

//         switch (property(prio, other, &Item::hasStrokeJoin, &Item::strokeJoin))
//         {
//         case StrokeJoin::Round:
//             style.strokeJoin = kTpStrokeJoinRound;
//             break;
//         case StrokeJoin::Miter:
//             style.strokeJoin = kTpStrokeJoinMiter;
//             break;
//         case StrokeJoin::Bevel:
//         default:
//             style.strokeJoin = kTpStrokeJoinBevel;
//             break;
//         }

//         switch (property(prio, other, &Item::hasStrokeCap, &Item::strokeCap))
//         {
//         case StrokeCap::Round:
//             style.strokeCap = kTpStrokeCapRound;
//             break;
//         case StrokeCap::Square:
//             style.strokeCap = kTpStrokeCapSquare;
//             break;
//         case StrokeCap::Butt:
//         default:
//             style.strokeCap = kTpStrokeCapButt;
//             break;
//         }

//         auto & da =
//             !other || (prio && prio->hasDashArray()) ? prio->dashArray() : other->dashArray();
//         if (da.count())
//         {
//             style.dashArray = &da[0];
//             style.dashCount = da.count();
//             style.dashOffset = property(prio, other, &Item::hasDashOffset, &Item::dashOffset);
//         }
//     }

//     return style;
// }

Error TarpRenderer::drawPath(Path * _path, const Mat32f & _transform, Symbol * _symbol, Size _depth)
{
    detail::TarpPathData & rd = ensureRenderData(_path);

    const StyleData * rs;
    StyleData tmp;

    // if(_symbol)
    // {
    //     STICK_ASSERT(_path == _symbol->item());
    //     tmp = _symbol->resolveStyle();
    //     rs = &tmp;
    // }
    // else
        rs = &_path->style().data();

    tpStyle style = tpStyleMake();
    style.fillRule =
        rs->windingRule == WindingRule::NonZero ? kTpFillRuleNonZero : kTpFillRuleEvenOdd;

    setPaint(&style, &tpStyle::fill, rs->fill);
    setPaint(&style, &tpStyle::stroke, rs->stroke);

    if (style.stroke.type != kTpPaintTypeNone && rs->strokeWidth)
    {
        style.strokeWidth = rs->strokeWidth;
        style.miterLimit = rs->miterLimit;
        style.scaleStroke = (tpBool)rs->scaleStroke;

        switch (rs->strokeJoin)
        {
        case StrokeJoin::Round:
            style.strokeJoin = kTpStrokeJoinRound;
            break;
        case StrokeJoin::Miter:
            style.strokeJoin = kTpStrokeJoinMiter;
            break;
        case StrokeJoin::Bevel:
        default:
            style.strokeJoin = kTpStrokeJoinBevel;
            break;
        }

        switch (rs->strokeCap)
        {
        case StrokeCap::Round:
            style.strokeCap = kTpStrokeCapRound;
            break;
        case StrokeCap::Square:
            style.strokeCap = kTpStrokeCapSquare;
            break;
        case StrokeCap::Butt:
        default:
            style.strokeCap = kTpStrokeCapButt;
            break;
        }

        auto & da = rs->dashArray;
        if (da.count())
        {
            style.dashArray = &da[0];
            style.dashCount = da.count();
            style.dashOffset = rs->dashOffset;
        }
    }

    // tpStyle style = makeStyle(_path, _symbol);
    // style = makeStyle(_path, nullptr);
    updateTarpPath(m_tarp->tmpSegmentBuffer, _path, rd.path, nullptr);

    tpBool err = tpFalse;
    //@TODO: Draw the path if there is no other style on the symbol and if the path is only
    // translated
    //@TODO: As of now we don't generate a separate rendercache if the symbol is a group (as then we
    // know the path being drawn right now is part of that group). In that case we just draw the
    // path
    // instead (which will recache the internal tarp cache which is suboptimal) as it's an easy
    // working solution for now. In the future we could have a symbol manage multiple renderCaches
    // (one for each item in its sub hierarchy). But that will make things a lot more complex :(
    if (!_symbol /* || _symbol->item()->itemType() == ItemType::Group*/)
    {
        if (!_path->renderTransform() || _path->lastRenderTransformID() != m_transformID)
            _path->setRenderTransform(m_transform * _transform, m_transformID);

        /* @TODO: only set the transform if it actually changed compared to the last draw call */
        tpSetTransform(m_tarp->ctx, (tpTransform *)&(*_path->renderTransform()));
        err = tpDrawPath(m_tarp->ctx, rd.path, &style);
    }
    else
    {
        //@TODO: Make proper transform/render transform update for symbol.
        detail::TarpSymbolData & sd = ensureRenderData(_symbol);
        tpRenderCache cache = sd.addItem(_path, _depth, _symbol->version());
        if (tpRenderCacheIsValidHandle(cache))
        {
            tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);
            err = tpCachePath(m_tarp->ctx, rd.path, &style, cache);
        }
        if (err)
            return Error(ec::InvalidOperation,
                         "Failed to cache tarp path for symbol",
                         STICK_FILE,
                         STICK_LINE);

        err = tpDrawRenderCache(m_tarp->ctx, sd.caches[_depth].cache);
    }

    if (err)
        return Error(ec::InvalidOperation, "Failed to draw tarp path", STICK_FILE, STICK_LINE);

    return Error();
}

Error TarpRenderer::beginClipping(Path * _clippingPath,
                                  const Mat32f & _transform,
                                  Symbol * _symbol,
                                  Size _depth)
{
    detail::TarpPathData & rd = ensureRenderData(_clippingPath);

    updateTarpPath(m_tarp->tmpSegmentBuffer, _clippingPath, rd.path, nullptr);

    tpBool err = tpFalse;
    if (!_symbol)
    {
        tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);
        err = tpBeginClippingWithFillRule(m_tarp->ctx,
                                          rd.path,
                                          _clippingPath->windingRule() == WindingRule::NonZero
                                              ? kTpFillRuleNonZero
                                              : kTpFillRuleEvenOdd);
    }
    else
    {
        detail::TarpSymbolData & sd = ensureRenderData(_symbol);
        tpRenderCache cache = sd.addItem(_clippingPath, _depth, _symbol->version());
        if (tpRenderCacheIsValidHandle(cache))
        {
            tpStyle clippingStyle = tpStyleMake();
            clippingStyle.stroke.type = kTpPaintTypeNone;
            clippingStyle.fillRule = _clippingPath->windingRule() == WindingRule::NonZero
                                         ? kTpFillRuleNonZero
                                         : kTpFillRuleEvenOdd;

            tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);
            err = tpCachePath(m_tarp->ctx, rd.path, &clippingStyle, cache);
        }
        if (err)
            return Error(ec::InvalidOperation,
                         "Failed to cache tarp clipping path for symbol",
                         STICK_FILE,
                         STICK_LINE);

        err = tpBeginClippingFromRenderCache(m_tarp->ctx, sd.caches[_depth].cache);
    }

    if (err)
        return Error(ec::InvalidOperation, "Failed to draw tarp clip path", STICK_FILE, STICK_LINE);
    return Error();
}

Error TarpRenderer::endClipping()
{
    tpBool err = tpEndClipping(m_tarp->ctx);
    if (err)
        return Error(ec::InvalidOperation, "Failed to draw tarp clip path", STICK_FILE, STICK_LINE);
    return Error();
}

Error TarpRenderer::prepareDrawing()
{
    glViewport(m_viewport.min().x, m_viewport.min().y, m_viewport.width(), m_viewport.height());
    tpPrepareDrawing(m_tarp->ctx);

    return Error();
}

Error TarpRenderer::finishDrawing()
{
    tpFinishDrawing(m_tarp->ctx);
    return Error();
}

static void recursivelyFindContourIdx(Path * _root, Path * _target, Size * _outIdx)
{
    for (auto * child : _root->children())
    {
        (*_outIdx)++;
        if (child == _target)
            return;
        recursivelyFindContourIdx(static_cast<Path *>(child), _target, _outIdx);
    }
}

void TarpRenderer::flattenedPathVertices(Path * _path,
                                         Vec2f ** _outPtr,
                                         Size * _outCount,
                                         const Mat32f & _transform)
{
    // as a path can be a contour of a tarp path (i.e. if it is the child of another path),
    // we need to find the root path first.
    Path * root = _path;
    while (root->parent() && root->parent()->itemType() == ItemType::Path)
        root = static_cast<Path *>(root->parent());

    // hmm we should most likely cache the contour index somewhere...this seems kinda BS
    Size cidx = 0;
    if (root != _path)
        recursivelyFindContourIdx(root, _path, &cidx);

    detail::TarpPathData & rd = ensureRenderData(root);
    updateTarpPath(m_tarp->tmpSegmentBuffer, root, rd.path, nullptr);

    int outCount;
    tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);
    tpPathFlattenedContour(m_tarp->ctx, rd.path, cidx, (tpVec2 **)_outPtr, &outCount);
    *_outCount = outCount;
}

} // namespace tarp
} // namespace paper
