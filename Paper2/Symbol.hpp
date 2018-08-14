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

    private:

        stick::Maybe<Rect> computeBounds(const Mat32f * _transform, BoundsType _type) const final;

        //called from Document
        void setItem(Item * _item);


        Item * m_item;
    };
}

#endif //PAPER_SYMBOL_HPP
