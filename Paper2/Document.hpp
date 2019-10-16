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
using TextWithAttributes = tink::TextWithAttributes2T<StylePtr>;

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

    Group * createTextOutlines(const char * _utf8,
                               const tink::FontAttributes & _attributes,
                               const Vec2f & _position);

    Group * createTextOutlines(const TextWithAttributes & _txt,
                               const Vec2f & _position,
                               Float _maxWidth = std::numeric_limits<Float>::max());

    Group * createGroup(const char * _name = "");

    Symbol * createSymbol(Item * _item, const char * _name = "");

    void setSize(Float _width, Float _height);

    Float width() const;

    Float height() const;

    const Vec2f & size() const;

    svg::SVGImportResult parseSVG(const stick::String & _svg, stick::Size _dpi = 96);

    stick::Result<Item *> parseBinary(const stick::UInt8 * _data, Size _byteCount);

    svg::SVGImportResult loadSVG(const stick::String & _uri, stick::Size _dpi = 96);

    stick::Allocator & allocator() const;

    const StylePtr & defaultStyle() const;

    StylePtr createStyle(const StyleData & _style = StyleData());

    LinearGradientPtr createLinearGradient(const Vec2f & _from, const Vec2f & _to);

    RadialGradientPtr createRadialGradient(const Vec2f & _from, const Vec2f & _to);

    stick::TextResult exportPDF() const; 

  private:
    // documents can't be cloned for now
    Document * clone() const final;

    bool canAddChild(Item * _e) const final;

    void destroyItem(Item * _e);

    stick::Allocator * m_alloc;
    ItemUniquePtrArray m_itemStorage;
    Vec2f m_size;
    StylePtr m_defaultStyle;
};
} // namespace paper

namespace tink
{
template <>
struct FontAttributeTraits2<paper::StylePtr>
{
    static FontPtr font(const paper::StylePtr & _style)
    {
        return _style->font();
    }

    static stick::Float32 size(const paper::StylePtr & _style)
    {
        return _style->fontSize();
    }

    static TextDirection direction(const paper::StylePtr & _style)
    {
        return _style->textDirection();
    }

    static stick::Float32 tracking(const paper::StylePtr & _style)
    {
        return _style->fontTracking();
    }

    static stick::Float32 leading(const paper::StylePtr & _style)
    {
        return _style->fontLeading();
    }
};
} // namespace tink

#endif // PAPER_DOCUMENT_HPP
