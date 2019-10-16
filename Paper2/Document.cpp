#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Symbol.hpp>

#include <Paper2/BinFormat/BinFormatImport.hpp>
#include <Paper2/SVG/SVGImport.hpp>
#include <Paper2/PDF/PDFExport.hpp>

#include <Stick/FileUtilities.hpp>

namespace paper
{
using namespace stick;

Document::Document(const char * _name, Allocator & _alloc)
    : Item(_alloc, this, ItemType::Document, _name)
    , m_alloc(&_alloc)
    , m_itemStorage(_alloc)
    , m_size(0)
{
    m_defaultStyle = createStyle();
    setStyle(m_defaultStyle);
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

// Group * Document::createTextOutlines(const char * _utf8,
//                                      const tink::FontAttributes & _attributes,
//                                      const Vec2f & _pos)
// {
//     tink::ShapedGlyphArray sga = tink::shape(_utf8, _attributes, tink::ShaperSettings(),
//     *m_alloc); Group * root = createGroup();

//     Vec2f p = _pos;
//     for (auto & g : sga)
//     {
//         //         printf("sgs[%i] %lu %f %f %f %f\n",
//         //        i,
//         //        sgs[i].glyph,
//         //        sgs[i].offsetX,
//         //        sgs[i].offsetY,
//         //        sgs[i].advanceX,
//         //        sgs[i].advanceY);

//         tink::FontVertexArray verts;
//         _attributes.font->glyphShape(g.glyph, _attributes.size, verts);

//         Path * currentPath = nullptr;
//         Path * glyphRoot = nullptr;
//         // Vec2f o(g.offsetX, g.offsetY);
//         for (auto & vert : verts)
//         {
//             printf("%f %f\n", vert.x, vert.y);
//             STICK_ASSERT(vert.cmd != tink::DrawCommand::Unknown);
//             if (vert.cmd == tink::DrawCommand::MoveTo)
//             {
//                 printf("move to\n");
//                 Path * parent = currentPath;
//                 currentPath = createPath();
//                 // currentPath->setFill("red");
//                 currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
//                 if (parent)
//                 {
//                     parent->closePath();
//                     printf("adding child\n");
//                     parent->addChild(currentPath);
//                 }
//                 else
//                     glyphRoot = currentPath;
//             }
//             else if (vert.cmd == tink::DrawCommand::LineTo)
//             {
//                 printf("line to\n");
//                 STICK_ASSERT(currentPath);
//                 currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
//             }
//             else if (vert.cmd == tink::DrawCommand::QuadraticCurveTo)
//             {
//                 printf("quadratic to\n");
//                 STICK_ASSERT(currentPath);
//                 Vec2f v(vert.x, -vert.y);
//                 currentPath->quadraticCurveTo(p + Vec2f(vert.cx, -vert.cy), p + v);
//             }
//             else if (vert.cmd == tink::DrawCommand::CubicCurveTo)
//             {
//                 printf("cubic to\n");
//                 STICK_ASSERT(currentPath);
//                 Vec2f v(vert.x, -vert.y);
//                 currentPath->cubicCurveTo(
//                     p + Vec2f(vert.cx, -vert.cy), p + Vec2f(vert.cx1, -vert.cy1), p + v);
//             }
//         }

//         if (glyphRoot)
//         {
//             currentPath->closePath();
//             printf("current path children %lu\n", currentPath->children().count());
//             root->addChild(glyphRoot);
//         }

//         p += Vec2f(g.advanceX, -g.advanceY);
//     }

//     return root;
// }

Group * Document::createTextOutlines(const TextWithAttributes & _txt,
                                     const Vec2f & _position,
                                     Float _maxWidth)
{

    // tink::UnicodeArray ucode;
    // tink::utf8::decode_range(_txt.text().begin(), _txt.text().end(), stick::backInserter(ucode));
    // printf("num codepoints %lu\n", ucode.count());

    // tink::UnicodeArray tmp;
    // tink::bidi(ucode.ptr(), ucode.count(), tmp, tink::TextDirection::Auto);
    // printf("num codepoints2 %lu\n", tmp.count());

    // printf("input txt %s\n", _txt.text().cString());
    // stick::String i2;
    // tink::utf8::encode_range(tmp.begin(), tmp.end(), stick::backInserter(i2));
    // printf("visual txt: %s\n", i2.cString());

    // tink::Typesetter setter(*m_alloc);
    // tink::TextLayout layout = setter.layout(_txt, _position.x, _position.y, _maxWidth);

    // Group * root = createGroup();
    // printf("line count %lu\n", layout.lines().count());
    // for (const auto & line : layout.lines())
    // {
    //     printf("runs %lu %lu\n", line.runStartIdx, line.runCount);
    //     for (const auto & run : layout.runs(line))
    //     {
    //         printf("attri %lu\n", run.attributesIndex);
    //         const StylePtr & style = _txt.attributes(run.attributesIndex);
    //         Group * grp = createGroup();
    //         for (const auto & g : layout.glyphs(run))
    //         {
    //             tink::FontVertexArray verts;
    //             printf("FONT SIZE %f\n", style->fontSize());
    //             style->font()->glyphShape(g.sg.glyph, style->fontSize(), verts);

    //             Vec2f p(g.x, g.y);
    //             Path * currentPath = nullptr;
    //             Path * glyphRoot = nullptr;
    //             // Vec2f o(g.offsetX, g.offsetY);
    //             for (auto & vert : verts)
    //             {
    //                 // printf("%f %f\n", vert.x, vert.y);
    //                 STICK_ASSERT(vert.cmd != tink::DrawCommand::Unknown);
    //                 if (vert.cmd == tink::DrawCommand::MoveTo)
    //                 {
    //                     // printf("move to\n");
    //                     Path * parent = currentPath;
    //                     currentPath = createPath();
    //                     // currentPath->setFill("red");
    //                     currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
    //                     if (parent)
    //                     {
    //                         parent->closePath();
    //                         // printf("adding child\n");
    //                         parent->addChild(currentPath);
    //                     }
    //                     else
    //                         glyphRoot = currentPath;
    //                 }
    //                 else if (vert.cmd == tink::DrawCommand::LineTo)
    //                 {
    //                     // printf("line to\n");
    //                     STICK_ASSERT(currentPath);
    //                     currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
    //                 }
    //                 else if (vert.cmd == tink::DrawCommand::QuadraticCurveTo)
    //                 {
    //                     // printf("quadratic to\n");
    //                     STICK_ASSERT(currentPath);
    //                     Vec2f v(vert.x, -vert.y);
    //                     currentPath->quadraticCurveTo(p + Vec2f(vert.cx, -vert.cy), p + v);
    //                 }
    //                 else if (vert.cmd == tink::DrawCommand::CubicCurveTo)
    //                 {
    //                     // printf("cubic to\n");
    //                     STICK_ASSERT(currentPath);
    //                     Vec2f v(vert.x, -vert.y);
    //                     currentPath->cubicCurveTo(
    //                         p + Vec2f(vert.cx, -vert.cy), p + Vec2f(vert.cx1, -vert.cy1), p + v);
    //                 }
    //             }

    //             if (glyphRoot)
    //             {
    //                 currentPath->closePath();
    //                 // printf("current path children %lu\n", currentPath->children().count());
    //                 grp->addChild(glyphRoot);
    //             }

    //             grp->setStyle(style);
    //         }
    //         root->addChild(grp);
    //     }
    // }
    // return root;

    tink::ParagraphLayout layout(*m_alloc);
    layout.setText(_txt, tink::TextDirection::Auto);

    Group * root = createGroup();

    while (auto * line = layout.nextLine(_maxWidth))
    {
        for (const auto & run : line->runs())
        {
            const StylePtr & style = _txt.attributes(run.attributesIndex());
            Group * grp = createGroup();
            for (const auto & g : run.glyphs())
            {
                tink::FontVertexArray verts;
                printf("FONT SIZE %f\n", style->fontSize());
                style->font()->glyphShape(g.sg.glyph, style->fontSize(), verts);

                Vec2f p = _position + Vec2f(g.x, g.y);
                Path * currentPath = nullptr;
                Path * glyphRoot = nullptr;
                // Vec2f o(g.offsetX, g.offsetY);
                for (auto & vert : verts)
                {
                    // printf("%f %f\n", vert.x, vert.y);
                    STICK_ASSERT(vert.cmd != tink::DrawCommand::Unknown);
                    if (vert.cmd == tink::DrawCommand::MoveTo)
                    {
                        // printf("move to\n");
                        Path * parent = currentPath;
                        currentPath = createPath();
                        // currentPath->setFill("red");
                        currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
                        if (parent)
                        {
                            parent->closePath();
                            // printf("adding child\n");
                            parent->addChild(currentPath);
                        }
                        else
                            glyphRoot = currentPath;
                    }
                    else if (vert.cmd == tink::DrawCommand::LineTo)
                    {
                        // printf("line to\n");
                        STICK_ASSERT(currentPath);
                        currentPath->addPoint(p + Vec2f(vert.x, -vert.y));
                    }
                    else if (vert.cmd == tink::DrawCommand::QuadraticCurveTo)
                    {
                        // printf("quadratic to\n");
                        STICK_ASSERT(currentPath);
                        Vec2f v(vert.x, -vert.y);
                        currentPath->quadraticCurveTo(p + Vec2f(vert.cx, -vert.cy), p + v);
                    }
                    else if (vert.cmd == tink::DrawCommand::CubicCurveTo)
                    {
                        // printf("cubic to\n");
                        STICK_ASSERT(currentPath);
                        Vec2f v(vert.x, -vert.y);
                        currentPath->cubicCurveTo(
                            p + Vec2f(vert.cx, -vert.cy), p + Vec2f(vert.cx1, -vert.cy1), p + v);
                    }
                }

                if (glyphRoot)
                {
                    currentPath->closePath();
                    // printf("current path children %lu\n", currentPath->children().count());
                    grp->addChild(glyphRoot);
                }

                grp->setStyle(style);
            }
            root->addChild(grp);
        }
    }

    return root;
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

Result<Item *> Document::parseBinary(const UInt8 * _data, Size _byteCount)
{
    return binfmt::import(*this, _data, _byteCount);
}

const StylePtr & Document::defaultStyle() const
{
    return m_defaultStyle;
}

StylePtr Document::createStyle(const StyleData & _style)
{
    return makeShared<Style>(*m_alloc, *m_alloc, _style);
}

LinearGradientPtr Document::createLinearGradient(const Vec2f & _from, const Vec2f & _to)
{
    return makeShared<LinearGradient>(*m_alloc, _from, _to);
}

RadialGradientPtr Document::createRadialGradient(const Vec2f & _from, const Vec2f & _to)
{
    return makeShared<RadialGradient>(*m_alloc, _from, _to);
}

TextResult Document::exportPDF() const
{
    ByteArray ba;
    auto err = pdf::exportDocument(this, ba);
    return err;
}

} // namespace paper
