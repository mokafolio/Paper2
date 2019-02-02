#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/RenderInterface.hpp>
#include <Paper2/Symbol.hpp>
// #include <Paper/PlacedSymbol.hpp>
// #include <Crunch/StringConversion.hpp>

namespace paper
{
RenderInterface::RenderInterface()
{
}

RenderInterface::RenderInterface(RenderInterface && _other) :
    m_document(std::move(_other.m_document))
{
}

RenderInterface::~RenderInterface()
{
}

Error RenderInterface::draw()
{
    STICK_ASSERT(m_document);
    Error ret = prepareDrawing();
    if (ret)
        return ret;
    ret = drawChildren(m_document, nullptr, false, nullptr, 0);
    if (ret)
        return ret;
    ret = finishDrawing();
    return ret;
}

Error RenderInterface::drawChildren(
    Item * _item, const Mat32f * _transform, bool _bSkipFirst, Symbol * _symbol, Size _depth)
{
    Error err;
    Mat32f tmp;
    auto start = _item->children().begin();
    auto it = start + _bSkipFirst;
    for (; it != _item->children().end(); ++it)
    {
        if (_transform)
            tmp = *_transform * (*it)->transform();

        err = drawItem(
            (*it), _transform ? &tmp : nullptr, _symbol, _depth + std::distance(start, it));
        if (err)
            return err;
    }
    return err;
}

Error RenderInterface::drawItem(Item * _item,
                                const Mat32f * _transform,
                                Symbol * _symbol,
                                Size _depth)
{
    Error ret;

    if (!_item->isVisible())
        return ret;

    if (_item->itemType() == ItemType::Group)
    {
        Group * grp = static_cast<Group *>(_item);

        if (grp->isClipped())
        {
            Path * mask = nullptr;
            Item * transformItem =
                nullptr; // the item that provides the transform (i.e. needed for symbols)
            const auto & c2 = grp->children();
            if (c2.first()->itemType() == ItemType::Path)
            {
                mask = static_cast<Path *>(c2.first());
                transformItem = mask;
            }
            else if (c2.first()->itemType() == ItemType::Symbol)
            {
                Symbol * s = static_cast<Symbol *>(c2.first());
                if (s->item()->itemType() == ItemType::Path)
                {
                    mask = static_cast<Path *>(s->item());
                    transformItem = s;
                }
            }
            STICK_ASSERT(mask && transformItem);

            Mat32f tmp2;
            if (_transform)
            {
                tmp2 = *_transform * transformItem->transform();
            }
            ret = beginClipping(
                mask, _transform ? tmp2 : transformItem->absoluteTransform(), _symbol, _depth);
            if (ret)
                return ret;

            ret = drawChildren(grp, _transform, true, _symbol, _depth);

            if (ret)
                return ret;

            ret = endClipping();
        }
        else
            drawChildren(grp, _transform, false, _symbol, _depth);
    }
    else if (_item->itemType() == ItemType::Path)
    {
        Path * p = static_cast<Path *>(_item);
        if (p->segmentData().count() > 1)
            ret = drawPath(p, _transform ? *_transform : p->absoluteTransform(), _symbol, _depth);
    }
    else if (_item->itemType() == ItemType::Symbol)
    {
        Symbol * s = static_cast<Symbol *>(_item);
        ret = drawItem(s->item(), _transform ? _transform : &s->absoluteTransform(), _symbol ? _symbol : s, 0);
    }
    return ret;
}

Document * RenderInterface::document()
{
    return m_document;
}
} // namespace paper
