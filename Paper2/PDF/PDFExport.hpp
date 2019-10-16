#ifndef PAPER_PDF_PDFEXPORT_HPP
#define PAPER_PDF_PDFEXPORT_HPP

#include <Paper2/BasicTypes.hpp>
#include <Stick/FileUtilities.hpp>
#include <Stick/Result.hpp>

namespace paper
{
class Document;

namespace pdf
{
STICK_LOCAL stick::Error exportDocument(const Document * _item, stick::ByteArray & _outBytes);
}
} // namespace paper

#endif // PAPER_PDF_PDFEXPORT_HPP
