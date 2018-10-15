#ifndef PAPER_SVG_SVGIMPORT_HPP
#define PAPER_SVG_SVGIMPORT_HPP

#include <Paper2/BasicTypes.hpp>

namespace paper
{
namespace svg
{

class SVGImportResult;

class STICK_LOCAL SVGImport
{
  public:
    SVGImport();

    SVGImportResult parse(Document & _doc, const stick::String & _svg, stick::Size _dpi = 72);
};

} // namespace svg
} // namespace paper

#endif // PAPER_SVG_SVGIMPORT_HPP
