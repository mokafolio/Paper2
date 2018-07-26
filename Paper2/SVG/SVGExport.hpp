#ifndef PAPER_SVG_SVGEXPORT_HPP
#define PAPER_SVG_SVGEXPORT_HPP

#include <Paper2/BasicTypes.hpp>
#include <Stick/Result.hpp>

namespace paper
{
    class Item;

    namespace svg
    {
        STICK_LOCAL stick::TextResult exportItem(const Item * _item, stick::Allocator & _alloc, bool _bMatchShapes);
    }
}

#endif //PAPER_SVG_SVGEXPORT_HPP
