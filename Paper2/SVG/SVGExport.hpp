#ifndef PAPER_SVG_SVGEXPORT_HPP
#define PAPER_SVG_SVGEXPORT_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
    class Item;

    namespace svg
    {
        STICK_LOCAL Error exportItem(const Item * _item, String & _outString, bool _bMatchShapes);
    }
}

#endif //PAPER_SVG_SVGEXPORT_HPP
