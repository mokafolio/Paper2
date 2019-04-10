#ifndef PAPER_BINFORMAT_BINFORMATEXPORT_HPP
#define PAPER_BINFORMAT_BINFORMATEXPORT_HPP

#include <Paper2/BasicTypes.hpp>
#include <Stick/Result.hpp>

namespace paper
{
class Item;
class Document;

namespace binfmt
{

STICK_LOCAL stick::Result<Item *> import(Document & _doc,
                                         const stick::UInt8 * _data,
                                         Size _byteCount);

} // namespace binfmt
} // namespace paper

#endif // PAPER_BINFORMAT_BINFORMATEXPORT_HPP
