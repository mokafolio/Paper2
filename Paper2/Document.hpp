#ifndef PAPER_DOCUMENT_HPP
#define PAPER_DOCUMENT_HPP

#include <Paper2/Item.hpp>
#include <Paper2/SVG/SVGImportResult.hpp>
#include <Stick/UniquePtr.hpp>

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
    Document(const char * _name = "Paper Document",
             stick::Allocator & _alloc = stick::defaultAllocator());

    ~Document() = default;

    Path * createPath(const char * _name = "");

    Path * createEllipse(const Vec2f & _center, const Vec2f & _size, const char * _name = "");

    Path * createCircle(const Vec2f & _center, Float _radius, const char * _name = "");

    Path * createRectangle(const Vec2f & _from, const Vec2f & _to, const char * _name = "");

    Path * createRoundedRectangle(const Vec2f & _min,
                                  const Vec2f & _max,
                                  const Vec2f & _radius,
                                  const char * _name = "");

    Group * createGroup(const char * _name = "");

    Symbol * createSymbol(Item * _item, const char * _name = "");

    void setSize(Float _width, Float _height);

    Float width() const;

    Float height() const;

    const Vec2f & size() const;

    svg::SVGImportResult parseSVG(const stick::String & _svg, stick::Size _dpi = 96);

    svg::SVGImportResult loadSVG(const stick::String & _uri, stick::Size _dpi = 96);

    stick::Allocator & allocator() const;

  private:
    // documents can't be cloned for now
    Document * clone() const final;

    bool canAddChild(Item * _e) const final;

    void destroyItem(Item * _e);

    ItemUniquePtrArray m_itemStorage;
    Vec2f m_size;
};
} // namespace paper

#endif // PAPER_DOCUMENT_HPP
