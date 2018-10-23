#include <Paper2/Document.hpp>
#include <Paper2/Symbol.hpp>

namespace paper
{
using namespace stick;

Symbol::Symbol(Allocator & _alloc, Document * _document, const char * _name) :
    Item(_alloc, _document, ItemType::Symbol, _name),
    m_item(nullptr)
{
}

void Symbol::setItem(Item * _item)
{
    STICK_ASSERT(_item->itemType() != ItemType::Document);
    m_item = _item;
}

Item * Symbol::item()
{
    return m_item;
}

Symbol * Symbol::clone() const
{
    Symbol * ret = m_document->createSymbol(m_item, m_name.cString() ? m_name.cString() : "");

    // clone properties and children
    cloneItemTo(ret);

    return ret;
}

const Mat32f & Symbol::absoluteTransform() const
{
    if (!m_absoluteTransform)
    {
        if (isTransformed())
            m_absoluteTransform = m_parent->absoluteTransform() * transform() * m_item->transform();
        else
            m_absoluteTransform = m_item->transform();
    }

    // printf("TRANS %f %f\n", (*m_absoluteTransform)[2].x, (*m_absoluteTransform)[2].y);
    return *m_absoluteTransform;
}

Maybe<Rect> Symbol::computeBounds(const Mat32f * _transform, BoundsType _type) const
{
    // if there is a transform on this symbol, we need to compute the bounds
    if (isTransformed())
        return m_item->computeBounds(&absoluteTransform(), _type);

    // otherwise we simply grab the bounds from the item as they might already be cached.
    switch (_type)
    {
    case BoundsType::Stroke:
        return m_item->strokeBounds();
        break;
    case BoundsType::Fill:
        return m_item->bounds();
        break;
    case BoundsType::Handle:
        return m_item->handleBounds();
        break;
    default:
        return Maybe<Rect>();
    }
}
} // namespace paper
