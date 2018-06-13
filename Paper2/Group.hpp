#ifndef PAPER_GROUP_HPP
#define PAPER_GROUP_HPP

#include <Paper2/Item.hpp>

namespace paper
{
    class Document;

    class STICK_API Group : public Item
    {
    public:

        Group(stick::Allocator & _alloc, Document * _document, const char * _name);

        void setClipped(bool _b);
        
        bool isClipped() const;

        Group * clone() const final;

    private:
        
        bool canAddChild(Item * _e) const final;

        stick::Maybe<Rect> computeBounds(const Mat32f * _transform, BoundsType _type) const final;


        bool m_bIsClipped;
    };
}

#endif //PAPER_GROUP_HPP
