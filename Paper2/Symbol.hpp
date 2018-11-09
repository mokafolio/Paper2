#ifndef PAPER_SYMBOL_HPP
#define PAPER_SYMBOL_HPP

#include <Paper2/Item.hpp>

namespace paper
{
class Document;

class STICK_API Symbol : public Item
{
    friend class Item;
    friend class Document;

  public:
    Symbol(stick::Allocator & _alloc, Document * _document, const char * _name);

    Item * item();

    const Mat32f & absoluteTransform() const final;

    Symbol * clone() const;

    // called from Renderer
    Size version() const;

  private:
    stick::Maybe<Rect> computeBounds(const Mat32f * _transform, BoundsType _type) const final;

    // called from Document
    void setItem(Item * _item);

    Item * m_item;
    // since a symbol can reference multiple things that need to be rendered (i.e.
    // if it's referencing a group) a simple dirty flag does not suffice.
    Size m_version;
};
} // namespace paper

#endif // PAPER_SYMBOL_HPP
