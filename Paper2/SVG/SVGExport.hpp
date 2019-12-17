#ifndef PAPER_SVG_SVGEXPORT_HPP
#define PAPER_SVG_SVGEXPORT_HPP

#include <Paper2/BasicTypes.hpp>
#include <Stick/Result.hpp>

namespace paper
{
class Item;

namespace svg
{
//@TODO: As soon as text stuff is supported, do something similar to this:
//https://github.com/BTBurke/svg-embed-font
// to embed fonts inside the exported svgs (possibly optional, lets figure that out once we get
// there).
STICK_LOCAL stick::TextResult exportItem(const Item * _item,
                                         stick::Allocator & _alloc,
                                         bool _bMatchShapes);
} // namespace svg
} // namespace paper

#endif // PAPER_SVG_SVGEXPORT_HPP
