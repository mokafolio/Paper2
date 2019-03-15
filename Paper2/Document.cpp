#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Symbol.hpp>

#include <Paper2/SVG/SVGImport.hpp>

#include <Stick/FileUtilities.hpp>

namespace paper
{
using namespace stick;

Document::Document(const char * _name, Allocator & _alloc) :
    Item(_alloc, this, ItemType::Document, _name),
    m_itemStorage(_alloc),
    m_size(0)
{
}

Path * Document::createPath(const char * _name)
{
    m_itemStorage.append(makeUnique<Path>(allocator(), allocator(), this, _name));
    this->addChild(m_itemStorage.last().get());
    return static_cast<Path *>(m_itemStorage.last().get());
}

Path * Document::createEllipse(const Vec2f & _center, const Vec2f & _size, const char * _name)
{
    Path * ret = createPath(_name);
    ret->makeEllipse(_center, _size);
    return ret;
}

Path * Document::createCircle(const Vec2f & _center, Float _radius, const char * _name)
{
    Path * ret = createPath(_name);
    ret->makeCircle(_center, _radius);
    return ret;
}

Path * Document::createRectangle(const Vec2f & _from, const Vec2f & _to, const char * _name)
{
    Path * ret = createPath(_name);
    ret->makeRectangle(_from, _to);
    return ret;
}

Path * Document::createRoundedRectangle(const Vec2f & _min,
                                        const Vec2f & _max,
                                        const Vec2f & _radius,
                                        const char * _name)
{
    Path * ret = createPath(_name);
    ret->makeRoundedRectangle(_min, _max, _radius);
    return ret;
}

Group * Document::createGroup(const char * _name)
{
    m_itemStorage.append(makeUnique<Group>(allocator(), allocator(), this, _name));
    this->addChild(m_itemStorage.last().get());
    return static_cast<Group *>(m_itemStorage.last().get());
}

Symbol * Document::createSymbol(Item * _item, const char * _name)
{
    m_itemStorage.append(makeUnique<Symbol>(allocator(), allocator(), this, _name));
    Symbol * s = static_cast<Symbol *>(m_itemStorage.last().get());
    s->setItem(_item);
    this->addChild(s);
    return s;
}

void Document::setSize(Float _width, Float _height)
{
    m_size = Vec2f(_width, _height);
}

Float Document::width() const
{
    return m_size.x;
}

Float Document::height() const
{
    return m_size.y;
}

const Vec2f & Document::size() const
{
    return m_size;
}

Allocator & Document::allocator() const
{
    return m_itemStorage.allocator();
}

Document * Document::clone() const
{
    STICK_ASSERT(false);
    return nullptr;
}

bool Document::canAddChild(Item * _e) const
{
    return _e->itemType() != ItemType::Document;
}

void Document::destroyItem(Item * _e)
{
    auto it = stick::findIf(m_itemStorage.begin(),
                            m_itemStorage.end(),
                            [_e](const ItemUniquePtr & _item) { return _item.get() == _e; });

    if (it != m_itemStorage.end())
        m_itemStorage.remove(it);
}

svg::SVGImportResult Document::parseSVG(const String & _svg, Size _dpi)
{
    svg::SVGImport import;
    return import.parse(*this, _svg, _dpi);
}

svg::SVGImportResult Document::loadSVG(const String & _uri, Size _dpi)
{
    auto result = loadTextFile(_uri);
    if (!result)
        return result.error();
    return parseSVG(result.get(), _dpi);
}

} // namespace paper
