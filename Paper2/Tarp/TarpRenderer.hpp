#ifndef PAPER_TARP_TARPRENDERER_HPP
#define PAPER_TARP_TARPRENDERER_HPP

#include <Paper2/RenderInterface.hpp>
#include <Stick/UniquePtr.hpp>

namespace paper
{
namespace tarp
{
namespace detail
{
struct TarpStuff;
};

class STICK_API TarpRenderer : public RenderInterface
{
  public:
    TarpRenderer();

    TarpRenderer(TarpRenderer && _other);

    ~TarpRenderer();

    Error init(Document & _doc) final;

    void setViewport(Float _x, Float _y, Float _widthInPixels, Float _heightInPixels) final;

    void setProjection(const Mat4f & _projection) final;

  private:
    Error drawPath(Path * _path, const Mat32f & _transform, Symbol * _symbol, Size _depth) final;

    Error beginClipping(Path * _clippingPath, const Mat32f & _transform, Symbol * _symbol, Size _depth) final;

    Error endClipping() final;

    Error prepareDrawing() final;

    Error finishDrawing() final;

    stick::UniquePtr<detail::TarpStuff> m_tarp;
    Rect m_viewport;
};
} // namespace tarp
} // namespace paper

#endif // PAPER_TARP_TARPRENDERER_HPP

//@TODO: Ideas for future rendering improvements:
// 1. Render all consecutive static paths onto one layer.
// 2. Render animated paths onto their own layers.
// 3. Composite all layers.
//=======================================================
