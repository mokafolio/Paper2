#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>

namespace paper
{
using namespace stick;

Group::Group(Allocator & _alloc, Document * _document, const char * _name) :
    Item(_alloc, _document, ItemType::Group, _name),
    m_bIsClipped(false)
{
}

void Group::setClipped(bool _b)
{
    m_bIsClipped = _b;
}

bool Group::isClipped() const
{
    return m_bIsClipped;
}

Group * Group::clone() const
{
    Group * ret = m_document->createGroup(m_name.cString() ? m_name.cString() : "");
    ret->m_bIsClipped = m_bIsClipped;

    // clone properties and children
    cloneItemTo(ret);

    return ret;
}

bool Group::canAddChild(Item * _e) const
{
    return _e->itemType() != ItemType::Document;
}

Maybe<Rect> Group::computeBounds(const Mat32f * _transform, BoundsType _type) const
{
    //@TODO: I am not sure if we actually need to do anything special for clipped groups....
    if (m_bIsClipped)
    {
        if (m_children.count())
        {
            Mat32f tmp = _transform ? *_transform * m_children.first()->transform() : Mat32f();
            // Maybe<Rect> maskBounds = m_children.first()->computeBounds(_transform ? &tmp :
            // nullptr, _type); if (maskBounds)
            // {
            //     Maybe<Rect> childBounds = mergeWithChildrenBounds(Maybe<Rect>(), _transform,
            //     _type, true); if (childBounds)
            //     {
            //         printf("RETURNING DA INTERSECTED BOUNDS %f %f %f %f\n", childBounds->min().x,
            //         childBounds->min().y, maskBounds->min().x, maskBounds->min().y); return
            //         Rect(crunch::max(childBounds->min(), maskBounds->min()),
            //                     crunch::min(childBounds->max(), maskBounds->max()));
            //     }
            // }
            // return Maybe<Rect>();
            return m_children.first()->computeBounds(_transform ? &tmp : nullptr, _type);
        }
    }
    return mergeWithChildrenBounds(Maybe<Rect>(), _transform, _type);
}
} // namespace paper
