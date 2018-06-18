#include <Paper2/Document.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Symbol.hpp>

namespace paper
{
    using namespace stick;

    Document::Document(Allocator & _alloc) :
        Item(_alloc, this, ItemType::Document, "Paper Document"),
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
        static Float s_kappa = detail::PaperConstants::kappa();
        // static Vec2f s_unitSegments[12] = { Vec2f(1, 0), Vec2f(0, -s_kappa), Vec2f(0, s_kappa),
        //                                     Vec2f(0, 1), Vec2f(s_kappa, 0), Vec2f(-s_kappa, 0),
        //                                     Vec2f(-1, 0), Vec2f(0, s_kappa), Vec2f(0, -s_kappa),
        //                                     Vec2f(0, -1), Vec2f(-s_kappa, 0), Vec2f(s_kappa, 0)
        //                                   };

        //Original paper values, don't conform with SVG though :(
        static SegmentData s_unitSegments[4] =
        {
            {Vec2f(0, s_kappa), Vec2f(-1, 0), Vec2f(0, -s_kappa)},
            {Vec2f(-s_kappa, 0), Vec2f(0, -1), Vec2f(s_kappa, 0)},
            {Vec2f(0, -s_kappa), Vec2f(1, 0), Vec2f(0, s_kappa)},
            {Vec2f(s_kappa, 0), Vec2f(0, 1), Vec2f(-s_kappa, 0)}
        };

        Path * ret = createPath(_name);

        Vec2f rad = _size * 0.5;
        SegmentData segs[4];
        for (Int32 i = 0; i < 4; ++i)
        {
            segs[i] =
            {
                s_unitSegments[i].handleIn * rad,
                s_unitSegments[i].position * rad + _center,
                s_unitSegments[i].handleOut * rad
            };
        }
        ret->addSegments(segs, 4);
        ret->closePath();

        return ret;
    }

    Path * Document::createCircle(const Vec2f & _center, Float _radius, const char * _name)
    {
        return createEllipse(_center, Vec2f(_radius) * 2.0f, _name);
    }

    Path * Document::createRectangle(const Vec2f & _from, const Vec2f & _to, const char * _name)
    {
        Path * ret = createPath(_name);

        SegmentData segs[4] =
        {
            {Vec2f(0), Vec2f(_to.x, _from.y), Vec2f(0)},
            {Vec2f(0), _to, Vec2f(0)},
            {Vec2f(0), Vec2f(_from.x, _to.y), Vec2f(0)},
            {Vec2f(0), _from, Vec2f(0)},
        };

        ret->addSegments(segs, 4);
        ret->closePath();

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
        auto it = stick::findIf(m_itemStorage.begin(), m_itemStorage.end(), [_e](const ItemUniquePtr & _item)
        {
            return _item.get() == _e;
        });

        if (it != m_itemStorage.end())
            m_itemStorage.remove(it);
    }
}
