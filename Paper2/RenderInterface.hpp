#ifndef PAPER_RENDERINTERFACE_HPP
#define PAPER_RENDERINTERFACE_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
class Document;
class Path;
class Item;

class STICK_API RenderInterface
{
  public:
    RenderInterface();

    RenderInterface(RenderInterface && _other);

    virtual ~RenderInterface();

    virtual Error init(Document & _doc) = 0;

    Error draw();

    virtual void setViewport(Float _x, Float _y, Float _widthInPixels, Float _heightInPixels) = 0;
    
    // set the width and height of the render area in document units
    virtual void setSize(Float _width, Float _height) = 0;

    Document * document();

  protected:
    // these have to be implemented
    virtual Error drawPath(Path * _path, const Mat32f & _transform, Symbol * _symbol, Size _depth) = 0;
    virtual Error beginClipping(Path * _clippingPath, const Mat32f & _transform, Symbol * _symbol, Size _depth) = 0;
    virtual Error endClipping() = 0;

    // these can be implemented
    virtual Error prepareDrawing()
    {
        return Error();
    }
    virtual Error finishDrawing()
    {
        return Error();
    }

    Error drawChildren(Item * _item, const Mat32f * _transform, bool _bSkipFirst, Symbol * _symbol, Size _depth);
    Error drawItem(Item * _item, const Mat32f * _transform, Symbol * _symbol, Size _depth);

    Document * m_document;
};
} // namespace paper

#endif // PAPER_RENDERINTERFACE_HPP
