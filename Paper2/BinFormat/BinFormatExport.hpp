#ifndef PAPER_BINFORMAT_BINFORMATEXPORT_HPP
#define PAPER_BINFORMAT_BINFORMATEXPORT_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
class Item;

namespace binfmt
{

// the binary format saves in 4-byte aligned little endian.
STICK_LOCAL Error exportItem(const Item * _item, stick::DynamicArray<stick::UInt8> & _outBytes);

} // namespace binfmt
} // namespace paper

#endif // PAPER_BINFORMAT_BINFORMATEXPORT_HPP
