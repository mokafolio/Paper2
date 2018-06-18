#ifndef PAPER_DOCUMENT_HPP
#define PAPER_DOCUMENT_HPP

#include <Stick/UniquePtr.hpp>
#include <Paper2/Item.hpp>

namespace paper
{
    class Path;
    class Group;
    class Symbol;

    using ItemUniquePtr = stick::UniquePtr<Item>;
    using ItemUniquePtrArray = stick::DynamicArray<ItemUniquePtr>;

    class STICK_API Document : public Item
    {
        friend class Item;

    public:


        Document(stick::Allocator & _alloc = stick::defaultAllocator());

        ~Document() = default;


        Path * createPath(const char * _name = nullptr);

        Path * createEllipse(const Vec2f & _center, const Vec2f & _size, const char * _name = nullptr);

        Path * createCircle(const Vec2f & _center, Float _radius, const char * _name = nullptr);

        Path * createRectangle(const Vec2f & _from, const Vec2f & _to, const char * _name = nullptr);

        Group * createGroup(const char * _name = nullptr);

        Symbol * createSymbol(Item * _item, const char * _name = nullptr);

        void setSize(Float _width, Float _height);

        Float width() const;

        Float height() const;

        const Vec2f & size() const;

        stick::Allocator & allocator() const;


    private:

        // documents can't be cloned for now
        Document * clone() const final;

        bool canAddChild(Item * _e) const final;

        void destroyItem(Item * _e);

        ItemUniquePtrArray m_itemStorage;
        Vec2f m_size;
    };
}

#endif //PAPER_DOCUMENT_HPP
