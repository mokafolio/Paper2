#include <Paper2/Tarp/TarpRenderer.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Group.hpp>

#include <GL/gl3w.h>

#define TARP_IMPLEMENTATION_OPENGL
#include <Tarp/Tarp.h>

namespace paper
{
    namespace tarp
    {
        using namespace stick;

        using tpSegmentArray =  stick::DynamicArray<tpSegment>;

        namespace detail
        {
            struct TarpStuff
            {
                tpContext ctx;
                //@TODO: allow to pass in an allocator for this
                tpSegmentArray tmpSegmentBuffer;
            };

            struct TarpRenderData : public RenderData
            {
                TarpRenderData(tpPath _path = tpPathInvalidHandle()) :
                    path(_path)
                {
                }

                ~TarpRenderData()
                {
                    tpPathDestroy(path);
                }

                RenderDataUniquePtr clone() const
                {
                    //@TODO: Proper allocator
                    return makeUnique<TarpRenderData>(defaultAllocator(), tpPathClone(path));
                }

                tpPath path;
            };

            struct TarpGradientData : public RenderData
            {
                TarpGradientData(tpGradient _grad = tpGradientInvalidHandle()) :
                    gradient(_grad)
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
        }

        struct Gl3wInitializer
        {
            Gl3wInitializer() :
                bError(true)
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
        m_viewport(std::move(_other.m_viewport))
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
                             String::formatted("Could not init Tarp context: %s\n",
                                               tpErrorMessage()),
                             STICK_FILE, STICK_LINE);
            }

            this->m_document = &_doc;

            return Error();
        }

        void TarpRenderer::setViewport(Float _x, Float _y, Float _widthInPixels, Float _heightInPixels)
        {
            m_viewport = Rect(_x, _y, _x + _widthInPixels, _y + _heightInPixels);
        }

        void TarpRenderer::setProjection(const Mat4f & _projection)
        {
            tpSetProjection(m_tarp->ctx, (const tpMat4 *)&_projection);
        }

        static detail::TarpRenderData & ensureRenderData(Path * _path)
        {
            if (!_path->renderData())
                _path->setRenderData(makeUnique<detail::TarpRenderData>(defaultAllocator(), tpPathCreate()));
            return *static_cast<detail::TarpRenderData *>(_path->renderData());
        }

        static detail::TarpGradientData & ensureRenderData(LinearGradientPtr _gradient)
        {
            if (!_gradient->renderData())
                _gradient->setRenderData(makeUnique<detail::TarpGradientData>(defaultAllocator(), tpGradientCreateLinear(0, 0, 0, 0)));
            return *static_cast<detail::TarpGradientData *>(_gradient->renderData());
        }

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
                    _tmpData.append((tpSegment)
                    {
                        {hi.x, hi.y},
                        {seg.position().x, seg.position().y},
                        {ho.x, ho.y}
                    });
                }
            }
            else
            {
                //tarp does not support per contour transforms, so we need to bring child paths segments
                //to path space before adding it as a contour!
                for (Size i = 0; i < _path->segmentData().count(); ++i)
                {
                    ConstSegment seg = _path->segment(i);
                    Vec2f hi = *_transform * seg.handleInAbsolute();
                    Vec2f pos = *_transform * seg.position();
                    Vec2f ho = *_transform * seg.handleOutAbsolute();
                    _tmpData.append((tpSegment)
                    {
                        {hi.x, hi.y},
                        {pos.x, pos.y},
                        {ho.x, ho.y}
                    });
                }
            }
        }

        static void recursivelyUpdateTarpPath(tpSegmentArray & _tmpData,
                                              Path * _path,
                                              tpPath _tarpPath,
                                              const Mat32f * _transform,
                                              UInt32 & _contourIndex)
        {
            if (_path->cleanDirtyGeometry())
            {
                toTarpSegments(_tmpData, _path, _transform);
                tpPathSetContour(_tarpPath, _contourIndex, &_tmpData[0], _tmpData.count(), (tpBool)_path->isClosed());
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

                recursivelyUpdateTarpPath(_tmpData, static_cast<Path *>(c), _tarpPath, t, _contourIndex);
            }
        }

        static void updateTarpPath(tpSegmentArray & _tmpData, Path * _path, tpPath _tarpPath, const Mat32f * _transform)
        {
            UInt32 contourIndex = 0;
            recursivelyUpdateTarpPath(_tmpData, _path, _tarpPath, _transform, contourIndex);

            // remove contours that are not used anymore
            if (contourIndex < tpPathContourCount(_tarpPath))
            {
                for (Size i = tpPathContourCount(_tarpPath) - 1; i >= contourIndex; --i)
                    tpPathRemoveContour(_tarpPath, i);
            }

            //potentially update the stroke/fill transforms
            if (_path->hasfillPaintTransform() && _path->cleanDirtyfillPaintTransform())
                tpPathSetFillPaintTransform(_tarpPath, (tpTransform *)&_path->fillPaintTransform());

            if (_path->hasStrokePaintTransform() && _path->cleanDirtystrokePaintTransform())
                tpPathSetStrokePaintTransform(_tarpPath, (tpTransform *)&_path->strokePaintTransform());
        }

        static detail::TarpGradientData & updateTarpGradient(BaseGradient & _grad)
        {
            if (!_grad.renderData())
                _grad.setRenderData(makeUnique<detail::TarpGradientData>(defaultAllocator(),
                                    _grad.type() == GradientType::Linear ? tpGradientCreateLinear(0, 0, 0, 0) : tpGradientCreateRadialSymmetric(0, 0, 0)));

            detail::TarpGradientData & gd = *static_cast<detail::TarpGradientData *>(_grad.renderData());

            if (_grad.cleanDirtyPositions())
            {
                tpGradientSetPositions(gd.gradient, _grad.origin().x, _grad.origin().y, _grad.destination().x, _grad.destination().y);
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
                    tpGradientAddColorStop(gd.gradient, stop.color.r, stop.color.g, stop.color.b, stop.color.a, stop.offset);
                }
            }
            return gd;
        }

        //ugly template helper using pointer to member
        //so we don't have to write the code for setting strok and fill twice
        template<class M, class M2>
        static void setPaint(tpStyle * _style, M _styleMember, Path * _path, M2 _pathMember)
        {
            if ((_path->*_pathMember)().template is<ColorRGBA>())
            {
                ColorRGBA & col = (_path->*_pathMember)().template get<ColorRGBA>();
                _style->*_styleMember = tpPaintMakeColor(col.r, col.g, col.b, col.a);
            }
            else if ((_path->*_pathMember)().template is<LinearGradientPtr>() ||
                     (_path->*_pathMember)().template is<RadialGradientPtr>())
            {
                SharedPtr<BaseGradient> ptr;
                if ((_path->*_pathMember)().template is<LinearGradientPtr>())
                    ptr = (_path->*_pathMember)().template get<LinearGradientPtr>();
                else
                    ptr = (_path->*_pathMember)().template get<RadialGradientPtr>();

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

        Error TarpRenderer::drawPath(Path * _path, const Mat32f & _transform)
        {
            detail::TarpRenderData & rd = ensureRenderData(_path);
            tpStyle style = tpStyleMake();
            style.fillRule = _path->windingRule() == WindingRule::NonZero ? kTpFillRuleNonZero : kTpFillRuleEvenOdd;

            setPaint(&style, &tpStyle::fill, _path, &Path::fill);
            setPaint(&style, &tpStyle::stroke, _path, &Path::stroke);

            if (style.stroke.type != kTpPaintTypeNone && _path->strokeWidth())
            {
                style.strokeWidth = _path->strokeWidth();
                style.miterLimit = _path->miterLimit();
                style.scaleStroke = (tpBool)_path->scaleStroke();

                switch (_path->strokeJoin())
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

                switch (_path->strokeCap())
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

                auto & da = _path->dashArray();
                if (da.count())
                {
                    style.dashArray = &da[0];
                    style.dashCount = da.count();
                    style.dashOffset = _path->dashOffset();
                }
            }

            updateTarpPath(m_tarp->tmpSegmentBuffer, _path, rd.path, nullptr);
            tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);

            tpBool err = tpDrawPath(m_tarp->ctx, rd.path, &style);

            if (err) return Error(ec::InvalidOperation, "Failed to draw tarp path", STICK_FILE, STICK_LINE);
            return Error();
        }

        Error TarpRenderer::beginClipping(Path * _clippingPath, const Mat32f & _transform)
        {
            detail::TarpRenderData & rd = ensureRenderData(_clippingPath);

            updateTarpPath(m_tarp->tmpSegmentBuffer, _clippingPath, rd.path, nullptr);

            tpSetTransform(m_tarp->ctx, (tpTransform *)&_transform);
            tpBool err = tpBeginClipping(m_tarp->ctx, rd.path);
            if (err) return Error(ec::InvalidOperation, "Failed to draw tarp clip path", STICK_FILE, STICK_LINE);
            return Error();
        }

        Error TarpRenderer::endClipping()
        {
            tpBool err = tpEndClipping(m_tarp->ctx);
            if (err) return Error(ec::InvalidOperation, "Failed to draw tarp clip path", STICK_FILE, STICK_LINE);
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
    }
}
