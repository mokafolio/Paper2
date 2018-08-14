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

        virtual void setViewport(Float _x, Float _y,
                                 Float _widthInPixels, Float _heightInPixels) = 0;

        virtual void setProjection(const Mat4f & _projection) = 0;

        Document * document();


    protected:

        //these have to be implemented
        virtual Error drawPath(Path * _path, const Mat32f & _transform) = 0;
        virtual Error beginClipping(Path * _clippingPath, const Mat32f & _transform) = 0;
        virtual Error endClipping() = 0;

        //these can be implemented
        virtual Error prepareDrawing() { return Error(); }
        virtual Error finishDrawing() { return Error(); }

        Error drawChildren(Item * _item, const Mat32f * _transform, bool _bSkipFirst);
        Error drawItem(Item * _item, const Mat32f * _transform);

        Document * m_document;
    };
}

#endif //PAPER_RENDERINTERFACE_HPP
