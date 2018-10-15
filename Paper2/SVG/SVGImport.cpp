#include <Stick/DynamicArray.hpp>
#include <Stick/HashMap.hpp>
#include <Stick/Result.hpp>
#include <Stick/URI.hpp>

#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/SVG/SVGImport.hpp>
#include <Paper2/SVG/SVGImportResult.hpp>
#include <pugixml.hpp>

namespace paper
{
namespace svg
{

using namespace stick;
using namespace crunch;

namespace detail
{
const char * toCString(const char * _begin, const char * _end)
{
    return _begin;
}
} // namespace detail

template <class InputIter>
class StringViewT
{
  public:
    StringViewT()
    {
    }

    StringViewT(const StringViewT &) = default;
    StringViewT(StringViewT &&) = default;
    StringViewT & operator=(const StringViewT &) = default;
    StringViewT & operator=(StringViewT &&) = default;

    StringViewT(InputIter _begin, InputIter _end) : m_begin(_begin), m_end(_end)
    {
    }

    InputIter begin() const
    {
        return m_begin;
    }

    InputIter end() const
    {
        return m_end;
    }

    bool operator==(const char * _str) const
    {
        return std::strncmp(
                   m_begin,
                   _str,
                   std::min((Size)std::strlen(_str), (Size)std::distance(m_begin, m_end))) == 0;
    }

    bool operator!=(const char * _str) const
    {
        return !(*this == _str);
    }

    const char * cString() const
    {
        return detail::toCString(m_begin, m_end);
    }

  private:
    InputIter m_begin;
    InputIter m_end;
};

template <class T>
StringViewT<T> makeStringView(T _begin, T _end)
{
    return StringViewT<T>(_begin, _end);
}

StringViewT<const char *> makeStringView(const char * _str)
{
    return StringViewT<const char *>(_str, _str + std::strlen(_str));
}

namespace detail
{
static bool isCommand(const char _c)
{
    return _c == 'M' || _c == 'm' || _c == 'L' || _c == 'l' || _c == 'H' || _c == 'h' ||
           _c == 'V' || _c == 'v' || _c == 'C' || _c == 'c' || _c == 'S' || _c == 's' ||
           _c == 'Q' || _c == 'q' || _c == 'T' || _c == 't' || _c == 'A' || _c == 'a' ||
           _c == 'Z' || _c == 'z';
}

static String::ConstIter skipWhitespaceAndCommas(String::ConstIter _it, String::ConstIter _end)
{
    while (_it != _end && (std::isspace(*_it) || *_it == ','))
        ++_it;
    return _it;
}

template <class F>
static String::ConstIter parseNumbers(String::ConstIter _it,
                                      String::ConstIter _end,
                                      F _endCondition,
                                      DynamicArray<Float> & _outNumbers)
{
    _outNumbers.clear();
    _it = skipWhitespaceAndCommas(_it, _end);

    Float value;
    while (_it != _end)
    {
        std::sscanf(_it, "%f", &value);
        _outNumbers.append(value);

        // skip the sign part
        if (*_it == '+' || *_it == '-')
            ++_it;
        // skip integer part
        while (_it != _end && std::isdigit(*_it))
            ++_it;
        // skip fractional part
        if (*_it == '.')
        {
            ++_it;
            while (_it != _end && std::isdigit(*_it))
                ++_it;
        }
        // skip exponent
        if (*_it == 'E' || *_it == 'e')
        {
            ++_it;
            if (*_it == '+' || *_it == '-')
                ++_it;

            while (_it != _end && std::isdigit(*_it))
                ++_it;
        }
        _it = skipWhitespaceAndCommas(_it, _end);
        if (_endCondition(*_it))
            break;
    }

    // return advanceToNextCommand(_it, _end);
    return _it;
}
enum class TransformAction
{
    Matrix,
    Translate,
    Scale,
    Rotate,
    SkewX,
    SkewY,
    None
};

static Mat32f parseTransform(stick::String::ConstIter _it, stick::String::ConstIter _end)
{
    Mat32f ret = Mat32f::identity();
    TransformAction action = TransformAction::None;

    //@TODO: Use document allocator
    DynamicArray<Float> numbers;
    numbers.reserve(64);

    while (_it != _end)
    {
        Mat32f tmp;
        if (std::strncmp(_it, "matrix", 6) == 0)
        {
            action = TransformAction::Matrix;
        }
        else if (std::strncmp(_it, "translate", 9) == 0)
        {
            action = TransformAction::Translate;
        }
        else if (std::strncmp(_it, "scale", 5) == 0)
        {
            action = TransformAction::Scale;
        }
        else if (std::strncmp(_it, "rotate", 6) == 0)
        {
            action = TransformAction::Rotate;
        }
        else if (std::strncmp(_it, "skewX", 5) == 0)
        {
            action = TransformAction::SkewX;
        }
        else if (std::strncmp(_it, "skewY", 5) == 0)
        {
            action = TransformAction::SkewY;
        }
        else
        {
            ++_it;
            continue;
        }

        // advance to the opening bracket
        while (_it != _end && *_it != '(')
            ++_it;
        ++_it;
        _it = parseNumbers(_it, _end, [](char _c) { return _c == ')'; }, numbers);
        if (_it != _end)
            ++_it; // skip ')'

        if (action == TransformAction::Matrix && numbers.count() == 6)
        {
            tmp = Mat32f(Vec2f(numbers[0], numbers[1]),
                         Vec2f(numbers[2], numbers[3]),
                         Vec2f(numbers[4], numbers[5]));
        }
        else if (action == TransformAction::Translate && numbers.count() >= 1)
        {
            tmp = Mat32f::translation(Vec2f(numbers[0], numbers.count() < 2 ? 0.0 : numbers[1]));
        }
        else if (action == TransformAction::Scale && numbers.count() >= 1)
        {
            tmp = Mat32f::scaling(Vec2f(numbers[0], numbers.count() < 2 ? numbers[0] : numbers[1]));
        }
        else if (action == TransformAction::Rotate && numbers.count() >= 1)
        {
            if (numbers.count() == 3)
            {
                Vec2f center = Vec2f(numbers[1], numbers[2]);
                tmp = Mat32f::translation(center);
                tmp.rotate(toRadians(numbers[0]));
                tmp.translate(-center);
            }
            else
            {
                tmp = Mat32f::rotation(toRadians(numbers[0]));
            }
        }
        else if (action == TransformAction::SkewX && numbers.count() == 1)
        {
            tmp = Mat32f::skewMatrix(Vec2f(toRadians(numbers[0]), 0));
        }
        else if (action == TransformAction::SkewY && numbers.count() == 1)
        {
            tmp = Mat32f::skewMatrix(Vec2f(0, toRadians(numbers[0])));
        }
        else
        {
            continue;
        }

        // multiply with the current matrix (open gl style, right to left)
        ret = ret * tmp;
    }
    return ret;
}

static stick::Maybe<ColorRGB> parseColor(String::ConstIter _begin, String::ConstIter _end)
{
    _begin = skipWhitespaceAndCommas(_begin, _end);
    if (*_begin == '#')
    {
        ++_begin; // skip the pound sign
        UInt32 hexCountPerChannel = _end - _begin <= 4 ? 1 : 2;
        if (hexCountPerChannel == 2)
        {
            String tmp(_begin, _begin + 2);
            auto r = std::strtoul(tmp.cString(), NULL, 16);
            tmp = String(_begin + 2, _begin + 4);
            auto g = std::strtoul(tmp.cString(), NULL, 16);
            tmp = String(_begin + 4, _begin + 6);
            auto b = std::strtoul(tmp.cString(), NULL, 16);
            return ColorRGB(r / 255.0, g / 255.0, b / 255.0);
        }
        else
        {
            String tmp = String::concat(_begin[0], _begin[0]);
            auto r = std::strtoul(tmp.cString(), NULL, 16);
            tmp = String::concat(_begin[1], _begin[1]);
            auto g = std::strtoul(tmp.cString(), NULL, 16);
            tmp = String::concat(_begin[2], _begin[2]);
            auto b = std::strtoul(tmp.cString(), NULL, 16);
            return ColorRGB(r / 255.0, g / 255.0, b / 255.0);
        }
    }
    else if (std::strncmp(_begin, "rgb", 3) == 0)
    {
        // code adjusted from nanosvg:
        // https://github.com/memononen/nanosvg/blob/master/src/nanosvg.h
        Int32 r = -1, g = -1, b = -1;
        char s1[32] = "", s2[32] = "";
        Int32 n = std::sscanf(_begin + 4, "%d%[%%, \t]%d%[%%, \t]%d", &r, s1, &g, s2, &b);
        STICK_ASSERT(n == 5);
        if (std::strchr(s1, '%'))
            return ColorRGB(r / 100.0, g / 100.0, b / 100.0);
        else
            return ColorRGB(r / 255.0, g / 255.0, b / 255.0);
    }
    else
    {
        // is this a named svg color?
        //@TODO: Check if it actually is an svg color
        auto col = crunch::svgColor<ColorRGB>(String(_begin, _end));
        return crunch::svgColor<ColorRGB>(String(_begin, _end));
    }

    return Maybe<ColorRGB>();
}

static StringViewT<String::ConstIter> parseURL(String::ConstIter _begin, String::ConstIter _end)
{
    while (_begin != _end && *_begin != '(')
        ++_begin;
    while (_begin != _end && *_begin != '#')
        ++_begin;
    ++_begin;
    auto begin = _begin;
    while (_begin != _end && *_begin != ')')
        ++_begin;
    return StringViewT<String::ConstIter>(begin, _begin);
};

template <class Functor>
static bool findXMLAttrCB(pugi::xml_node _node, const char * _name, Item * _item, Functor _cb)
{
    if (auto attr = _node.attribute(_name))
    {
        _cb(_item, attr);
        return true;
    }
    return false;
}

static Vec2f reflect(const Vec2f & _position, const Vec2f & _around)
{
    return _around + (_around - _position);
}
} // namespace detail

enum class SVGUnits
{
    EM,
    EX,
    PX,
    PT,
    PC,
    CM,
    MM,
    IN,
    Percent,
    User
};

struct STICK_LOCAL SVGAttributes
{
    ColorRGBA fillColor;
    WindingRule windingRule;
    ColorRGBA strokeColor;
    Float strokeWidth;
    StrokeCap strokeCap;
    StrokeJoin strokeJoin;
    bool bScalingStroke;
    Float miterLimit;
    DynamicArray<Float> dashArray;
    Float dashOffset;
    // since we dont support text yet, we only care
    // about font size for em/ex calculations
    Float fontSize;
};

struct SVGCoordinate
{
    SVGUnits units;
    Float value;
};

struct SVGView
{
    Rect rectangle;
    Mat32f viewBoxTransform;
};

class SVGImportSession
{
  public:
    SVGImportResult parse(Document & _doc, const String & _svg, Size _dpi)
    {
        // m_dpi = _dpi;
        // m_namedItems.clear();

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer(_svg.ptr(), _svg.length());

        if (result)
        {
            Float x, y, w, h;
            if (auto val = doc.attribute("x"))
                x = val.as_float();
            if (auto val = doc.attribute("y"))
                y = val.as_float();
            if (auto val = doc.attribute("width"))
                w = val.as_float();
            if (auto val = doc.attribute("height"))
                h = val.as_float();

            Error err;
            Group * grp = static_cast<Group *>(recursivelyImportNode(doc, doc, err));

            // remove all tmp items
            for (auto & item : m_tmpItems)
                item->remove();
            m_tmpItems.clear();

            return SVGImportResult(grp, w, h, err);
        }
        else
        {
            return SVGImportResult(
                Error(stick::ec::ParseFailed,
                      String::concat("Could not parse xml document: ", result.description()),
                      STICK_FILE,
                      STICK_LINE));
        }
    }

    Item * recursivelyImportNode(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        Item * item = nullptr;
        if (std::strcmp(_node.name(), "svg") == 0)
        {
            item = importGroup(_node, _rootNode, true, _error);
        }
        else if (std::strcmp(_node.name(), "g") == 0)
        {
            item = importGroup(_node, _rootNode, false, _error);
        }
        else if (std::strcmp(_node.name(), "rect") == 0)
        {
            item = importRectangle(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "circle") == 0)
        {
            item = importCircle(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "ellipse") == 0)
        {
            item = importEllipse(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "line") == 0)
        {
            item = importLine(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "polyline") == 0)
        {
            item = importPolyline(_node, _rootNode, false, _error);
        }
        else if (std::strcmp(_node.name(), "polygon") == 0)
        {
            item = importPolyline(_node, _rootNode, true, _error);
        }
        else if (std::strcmp(_node.name(), "path") == 0)
        {
            item = importPath(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "clipPath") == 0)
        {
            item = importClipPath(_node, _rootNode, _error);
        }
        else if (std::strcmp(_node.name(), "defs") == 0)
        {
            item = importGroup(_node, _rootNode, false, _error);
            if (!_error)
                m_tmpItems.append(item);
        }
        // else if (_node.name() == "symbol")
        // {
        //     item = importGroup(_node, _error);
        // }
        else
        {
            //?
        }
        return item;
    }

    Group * importGroup(pugi::xml_node _node,
                        pugi::xml_node _rootNode,
                        bool _bSVGNode,
                        Error & _error)
    {
        Group * grp = m_document->createGroup();

        // establish a new view based on the provided x,y,width,height (needed for viewbox
        // calculation)
        if (_bSVGNode)
        {
            Float x, y, w, h;
            auto mx = _node.attribute("x");
            auto my = _node.attribute("y");
            auto mw = _node.attribute("width");
            auto mh = _node.attribute("height");
            if (mw && mh)
            {
                w = mw.as_float();
                h = mh.as_float();
                x = mx ? mx.as_float() : 0;
                y = my ? my.as_float() : 0;
                m_viewStack.append(Rect(x, y, x + w, y + h));
            }
        }
        pushAttributes(_node, _rootNode, grp);
        for (auto & child : _node)
        {
            Item * item = recursivelyImportNode(child, _rootNode, _error);
            if (_error)
                break;
            else
                grp->addChild(item);

            // set the default fill if none is inherited
            if (item->itemType() == ItemType::Path && !item->hasFill())
                item->setFill(ColorRGBA(0, 0, 0, 1));
        }

        detail::findXMLAttrCB(_node, "viewBox", grp, [&](Item * _it, pugi::xml_attribute _attr) {
            // if no view is esablished, ignore viewbox
            if (m_viewStack.count())
            {
                // TODO: take preserveAspectRatio attribute into account (argh NOOOOOOOOOO)
                const Rect & r = m_viewStack.last();

                stick::DynamicArray<Float> numbers(m_document->allocator());
                numbers.reserve(4);

                detail::parseNumbers(_attr.value(),
                                     _attr.value() + std::strlen(_attr.value()),
                                     [](char _c) { return false; },
                                     numbers);
                Mat32f viewTransform = Mat32f::identity();
                if (m_viewStack.count() > 1)
                    viewTransform.translate(r.min());
                Vec2f scale(r.width() / numbers[2], r.height() / numbers[3]);
                viewTransform.scale(scale);
                _it->setTransform(viewTransform);

                Path * mask =
                    m_document->createRectangle(Vec2f(0, 0), r.size() * (Vec2f(1.0) / scale));
                mask->insertBelow(grp->children().first());
                grp->setClipped(true);
            }
        });

        if (_bSVGNode)
            m_viewStack.removeLast();

        popAttributes();
        return grp;
    }

    Path * importClipPath(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        Path * ret = m_document->createPath();
        for (auto & child : _node)
        {
            Item * item = recursivelyImportNode(child, _rootNode, _error);
            if (_error)
                break;
            // only add paths as children, ignore the rest (there should be no rest though,
            // safety first :))
            if (item->itemType() == ItemType::Path)
                ret->addChild(static_cast<Path *>(item));
        }
        pushAttributes(_node, _rootNode, ret);
        popAttributes();
        m_tmpItems.append(ret);
        return ret;
    }

    Path * importPath(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        if (auto attr = _node.attribute("d"))
        {
            const char * str = attr.value();
            Path * p = m_document->createPath();
            pushAttributes(_node, _rootNode, p);
            parsePathData(*m_document, p, str);
            popAttributes();
            return p;
        }
        else
        {
            _error =
                Error(ec::ParseFailed, "SVG path is missing d attribute", STICK_FILE, STICK_LINE);
            return nullptr;
        }
    }

    Path * importPolyline(pugi::xml_node _node,
                          pugi::xml_node _rootNode,
                          bool _bIsPolygon,
                          Error & _error)
    {
        auto mpoints = _node.attribute("points");
        if (mpoints)
        {
            DynamicArray<Float> numbers(m_document->allocator());
            numbers.reserve(64);
            detail::parseNumbers(mpoints.value(),
                                 mpoints.value() + std::strlen(mpoints.value()),
                                 [](char) { return false; },
                                 numbers);

            SegmentDataArray segs(m_document->allocator());
            segs.reserve(numbers.count() / 2);
            for (Size i = 0; i < numbers.count(); i += 2)
            {
                segments::addPoint(segs, Vec2f(numbers[i], numbers[i + 1]));
            }

            Path * ret = m_document->createPath();
            ret->swapSegments(segs, _bIsPolygon);

            pushAttributes(_node, _rootNode, ret);
            popAttributes();
            return ret;
        }
        else
        {
            _error = Error(ec::ParseFailed,
                           "SVG polyline/polygon is missing points attribute",
                           STICK_FILE,
                           STICK_LINE);
        }
        return nullptr;
    }

    Path * importCircle(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        auto mcx = _node.attribute("cx");
        auto mcy = _node.attribute("cy");
        auto mr = _node.attribute("r");
        if (mr)
        {
            Float x = mcx ? coordinatePixels(mcx.value()) : 0;
            Float y = mcy ? coordinatePixels(mcx.value()) : 0;
            Path * ret = m_document->createCircle(Vec2f(x, y), coordinatePixels(mr.value()));
            pushAttributes(_node, _rootNode, ret);
            popAttributes();
            return ret;
        }
        else
        {
            _error = Error(
                ec::ParseFailed, "SVG circle has to provide cx, cy and r", STICK_FILE, STICK_LINE);
        }
        return nullptr;
    }

    Path * importEllipse(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        auto mcx = _node.attribute("cx");
        auto mcy = _node.attribute("cy");
        auto mrx = _node.attribute("rx");
        auto mry = _node.attribute("ry");
        if (mrx && mry)
        {
            Float x = mcx ? coordinatePixels(mcx.value()) : 0;
            Float y = mcy ? coordinatePixels(mcy.value()) : 0;
            Path * ret = m_document->createEllipse(
                Vec2f(x, y),
                Vec2f(coordinatePixels(mrx.value()) * 2, coordinatePixels(mry.value()) * 2));
            pushAttributes(_node, _rootNode, ret);
            popAttributes();
            return ret;
        }
        else
        {
            _error = Error(ec::ParseFailed,
                           "SVG ellipse has to provide cx, cy, rx and ry",
                           STICK_FILE,
                           STICK_LINE);
        }
        return nullptr;
    }

    Path * importRectangle(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        auto mx = _node.attribute("x");
        auto my = _node.attribute("y");
        auto mw = _node.attribute("width");
        auto mh = _node.attribute("height");

        if (mx && my && mw && mh)
        {
            auto mrx = _node.attribute("rx");
            auto mry = _node.attribute("ry");

            Float x = coordinatePixels(mx.value());
            Float y = coordinatePixels(my.value());
            Path * ret;
            if (mrx || mry)
            {
                Float rx = mrx ? coordinatePixels(mrx.value()) : coordinatePixels(mry.value());
                Float ry = mry ? coordinatePixels(mry.value()) : rx;
                ret = m_document->createRoundedRectangle(
                    Vec2f(x, y),
                    Vec2f(x, y) + Vec2f(coordinatePixels(mw.value()), coordinatePixels(mh.value())),
                    Vec2f(rx, ry));
            }
            else
            {
                ret =
                    m_document->createRectangle(Vec2f(x, y),
                                                Vec2f(x, y) + Vec2f(coordinatePixels(mw.value()),
                                                                    coordinatePixels(mh.value())));
            }
            pushAttributes(_node, _rootNode, ret);
            popAttributes();
            return ret;
        }
        else
        {
            _error = Error(ec::ParseFailed,
                           "SVG rect missing x, y, width or height attribute",
                           STICK_FILE,
                           STICK_LINE);
        }

        return nullptr;
    }

    Path * importLine(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
        auto mx1 = _node.attribute("x1");
        auto my1 = _node.attribute("y1");
        auto mx2 = _node.attribute("x2");
        auto my2 = _node.attribute("y2");
        if (mx1 && my1 && mx2 && my2)
        {
            Float x1 = coordinatePixels(mx1.value());
            Float y1 = coordinatePixels(my1.value());
            Float x2 = coordinatePixels(mx2.value());
            Float y2 = coordinatePixels(my2.value());

            SegmentDataArray segs(m_document->allocator());
            segs.reserve(2);
            segments::addPoint(segs, Vec2f(x1, y1));
            segments::addPoint(segs, Vec2f(x2, y2));
            Path * ret = m_document->createPath();
            ret->swapSegments(segs, false);
            pushAttributes(_node, _rootNode, ret);
            popAttributes();
            return ret;
        }
        else
        {
            _error = Error(ec::ParseFailed,
                           "SVG line missing x1, y1, x2 or y1 attribute",
                           STICK_FILE,
                           STICK_LINE);
        }
        return nullptr;
    }

    static void parsePathData(Document & _doc, Path * _path, const String & _data)
    {
        DynamicArray<Float> numbers(_doc.allocator());
        auto end = _data.end();
        auto it = detail::skipWhitespaceAndCommas(_data.begin(), end);
        Path * currentPath = _path;
        Vec2f last;
        Vec2f lastHandle;
        SegmentDataArray segments(_doc.allocator());
        segments.reserve(64);
        //@TODO:Parse path data into SegmentData and use addSegments instead to avoid a lot of
        // overhead
        do
        {
            char cmd = *it;
            // auto tend = advanceToNextCommand(it + 1, end);
            ++it;
            it = detail::parseNumbers(
                it, end, [](char _c) { return detail::isCommand(_c); }, numbers);

            // STICK_ASSERT(it == tend);
            if (cmd == 'M' || cmd == 'm')
            {
                if (segments.count())
                {
                    currentPath->swapSegments(segments, false);
                    segments.reserve(64);
                    Path * tmp = _doc.createPath();
                    currentPath->addChild(tmp);
                    currentPath = tmp;
                }

                bool bRelative = cmd == 'm';
                for (int i = 0; i < numbers.count(); i += 2)
                {
                    if (bRelative)
                        last = last + Vec2f(numbers[i], numbers[i + 1]);
                    else
                        last = Vec2f(numbers[i], numbers[i + 1]);
                    segments::addPoint(segments, last);
                }
                lastHandle = last;
            }
            else if (cmd == 'L' || cmd == 'l')
            {
                bool bRelative = cmd == 'l';
                for (int i = 0; i < numbers.count(); i += 2)
                {
                    if (bRelative)
                        last = last + Vec2f(numbers[i], numbers[i + 1]);
                    else
                        last = Vec2f(numbers[i], numbers[i + 1]);
                    segments::addPoint(segments, last);
                }
                lastHandle = last;
            }
            else if (cmd == 'H' || cmd == 'h' || cmd == 'V' || cmd == 'v')
            {
                bool bRelative = cmd == 'h' || cmd == 'v';
                bool bVert = cmd == 'V' || cmd == 'v';
                for (int i = 0; i < numbers.count(); ++i)
                {
                    if (bVert)
                    {
                        if (bRelative)
                            last.y = last.y + numbers[i];
                        else
                            last.y = numbers[i];
                    }
                    else
                    {
                        if (bRelative)
                            last.x = last.x + numbers[i];
                        else
                            last.x = numbers[i];
                    }
                    segments::addPoint(segments, last);
                }
                lastHandle = last;
            }
            else if (cmd == 'C' || cmd == 'c')
            {
                bool bRelative = cmd == 'c';
                Vec2f start = last;
                for (int i = 0; i < numbers.count(); i += 6)
                {
                    if (!bRelative)
                    {
                        last = Vec2f(numbers[i + 4], numbers[i + 5]);
                        lastHandle = Vec2f(numbers[i + 2], numbers[i + 3]);
                        segments::cubicCurveTo(
                            segments, Vec2f(numbers[i], numbers[i + 1]), lastHandle, last);
                    }
                    else
                    {
                        last = start + Vec2f(numbers[i + 4], numbers[i + 5]);
                        lastHandle = Vec2f(start.x + numbers[i + 2], start.y + numbers[i + 3]);
                        segments::cubicCurveTo(
                            segments,
                            Vec2f(start.x + numbers[i], start.y + numbers[i + 1]),
                            lastHandle,
                            last);
                    }
                }
            }
            else if (cmd == 'S' || cmd == 's')
            {
                bool bRelative = cmd == 's';
                Vec2f start = last;
                for (int i = 0; i < numbers.count(); i += 4)
                {

                    Vec2f nextLast, nextHandle;
                    if (!bRelative)
                    {
                        nextLast = Vec2f(numbers[i + 2], numbers[i + 3]);
                        nextHandle = Vec2f(numbers[i], numbers[i + 1]);
                    }
                    else
                    {
                        nextLast = start + Vec2f(numbers[i + 2], numbers[i + 3]);
                        nextHandle = start + Vec2f(numbers[i], numbers[i + 1]);
                    }
                    segments::cubicCurveTo(
                        segments, detail::reflect(lastHandle, last), nextHandle, nextLast);
                    lastHandle = nextHandle;
                    last = nextLast;
                }
            }
            else if (cmd == 'Q' || cmd == 'q')
            {
                bool bRelative = cmd == 'q';
                Vec2f start = last;
                for (int i = 0; i < numbers.count(); i += 4)
                {
                    if (!bRelative)
                    {
                        last = Vec2f(numbers[i + 2], numbers[i + 3]);
                        lastHandle = Vec2f(numbers[i], numbers[i + 1]);
                    }
                    else
                    {
                        last = start + Vec2f(numbers[i + 2], numbers[i + 3]);
                        lastHandle = start + Vec2f(numbers[i], numbers[i + 1]);
                    }
                    segments::quadraticCurveTo(segments, lastHandle, last);
                }
            }
            else if (cmd == 'T' || cmd == 't')
            {
                bool bRelative = cmd == 't';
                Vec2f start = last;
                for (int i = 0; i < numbers.count(); i += 2)
                {
                    Vec2f nextLast = !bRelative ? Vec2f(numbers[i], numbers[i + 1])
                                                : start + Vec2f(numbers[i], numbers[i + 1]);
                    lastHandle = detail::reflect(lastHandle, last);
                    segments::quadraticCurveTo(segments, lastHandle, nextLast);
                    last = nextLast;
                }
            }
            else if (cmd == 'A' || cmd == 'a')
            {
                bool bRelative = cmd == 'a';
                for (int i = 0; i < numbers.count(); i += 7)
                {
                    last = !bRelative ? Vec2f(numbers[i + 5], numbers[i + 6])
                                      : last + Vec2f(numbers[i + 5], numbers[i + 6]);

                    Float rads = crunch::toRadians(numbers[i + 2]);
                    segments::arcTo(segments,
                                    last,
                                    Vec2f(numbers[i], numbers[i + 1]),
                                    crunch::toRadians(numbers[i + 2]),
                                    (bool)numbers[i + 4],
                                    (bool)numbers[i + 3]);
                    lastHandle = currentPath->segmentData().last().handleOut;
                }
            }
            else if (cmd == 'Z' || cmd == 'z')
            {
                currentPath->swapSegments(segments, true);
                last = currentPath->segmentData().last().position;
                lastHandle = currentPath->segmentData().last().handleOut;
            }
            else
            {
                //@TODO?
            }
        } while (it != end);
    }

    Float toPixels(Float _value, SVGUnits _units, Float _start = 0.0, Float _length = 1.0)
    {
        switch (_units)
        {
        case SVGUnits::PX:
        case SVGUnits::User:
        default:
            return _value;
        case SVGUnits::PT:
            return _value / (Float)72.0 * m_dpi;
        case SVGUnits::PC:
            return _value / (Float)6.0 * m_dpi;
        case SVGUnits::EM:
            return _value / (Float)6.0 * m_dpi;
        case SVGUnits::EX:
            return _value / (Float)6.0 * m_dpi;
        case SVGUnits::CM:
            return _value / (Float)2.54 * m_dpi;
        case SVGUnits::MM:
            return _value / (Float)25.4 * m_dpi;
        case SVGUnits::IN:
            return _value * m_dpi;
        case SVGUnits::Percent:
            return _start + _value / 100.0 * _length;
        }
    }

    SVGCoordinate parseCoordinate(const char * _str)
    {
        SVGCoordinate ret;
        char units[32] = "";
        std::sscanf(_str, "%f%s", &ret.value, units);

        if (units[0] == 'e' && units[1] == 'm')
            ret.units = SVGUnits::EM;
        else if (units[0] == 'e' && units[1] == 'x')
            ret.units = SVGUnits::EX;
        else if (units[0] == 'p' && units[1] == 'x')
            ret.units = SVGUnits::PX;
        else if (units[0] == 'p' && units[1] == 't')
            ret.units = SVGUnits::PT;
        else if (units[0] == 'p' && units[1] == 'c')
            ret.units = SVGUnits::PC;
        else if (units[0] == 'c' && units[1] == 'm')
            ret.units = SVGUnits::CM;
        else if (units[0] == 'm' && units[1] == 'm')
            ret.units = SVGUnits::MM;
        else if (units[0] == 'i' && units[1] == 'n')
            ret.units = SVGUnits::IN;
        else if (units[0] == '%')
            ret.units = SVGUnits::Percent;
        else
            ret.units = SVGUnits::User;

        return ret;
    }

    Float coordinatePixels(const char * _str, Float _start = 0.0, Float _length = 1.0)
    {
        auto coord = parseCoordinate(_str);
        return toPixels(coord.value, coord.units, _start, _length);
    }

    template <class SVA, class SVB>
    void parseAttribute(SVA _name, SVB _value, SVGAttributes & _attributes, Item * _item)
    {
        if (_name == "style")
        {
            parseStyle(_value, _attributes, _item);
        }
        else if (_name == "fill")
        {
            if (_value == "none")
            {
                _item->removeFill();
            }
            else
            {
                auto col = detail::parseColor(_value.begin(), _value.end());
                if (col)
                {
                    auto c = toRGBA(*col);
                    // we need to get the current alpha, as that might have been set allready
                    c.a = _attributes.fillColor.a;
                    _item->setFill(c);
                    _attributes.fillColor = c;
                }
            }
        }
        else if (_name == "fill-opacity")
        {
            _attributes.fillColor.a = std::atof(_value.cString());
            if (auto mf = _item->fill().maybe<ColorRGBA>())
            {
                _item->setFill(ColorRGBA((*mf).r, (*mf).g, (*mf).b, (*mf).a));
            }
        }
        else if (_name == "fill-rule")
        {
            if (_value == "nonzero")
                _attributes.windingRule = WindingRule::NonZero;
            else if (_value == "evenodd")
                _attributes.windingRule = WindingRule::EvenOdd;
            _item->setWindingRule(_attributes.windingRule);
        }
        else if (_name == "stroke")
        {
            if (_value == "none")
            {
                _item->removeStroke();
            }
            else
            {
                auto col = detail::parseColor(_value.begin(), _value.end());
                if (col)
                {
                    auto c = toRGBA(*col);
                    // we need to get the current alpha, as that might have been set allready
                    c.a = _attributes.strokeColor.a;
                    _item->setStroke(c);
                    _attributes.strokeColor = c;
                }
            }
        }
        else if (_name == "stroke-opacity")
        {
            _attributes.strokeColor.a = std::atof(_value.cString());
            if (auto mf = _item->stroke().maybe<ColorRGBA>())
            {
                _item->setStroke(ColorRGBA((*mf).r, (*mf).g, (*mf).b, _attributes.strokeColor.a));
            }
        }
        else if (_name == "stroke-width")
        {
            _attributes.strokeWidth = coordinatePixels(_value.cString());
            _item->setStrokeWidth(_attributes.strokeWidth);
        }
        else if (_name == "stroke-linecap")
        {
            if (_value == "butt")
                _attributes.strokeCap = StrokeCap::Butt;
            else if (_value == "round")
                _attributes.strokeCap = StrokeCap::Round;
            else if (_value == "square")
                _attributes.strokeCap = StrokeCap::Square;

            _item->setStrokeCap(_attributes.strokeCap);
        }
        else if (_name == "stroke-linejoin")
        {
            if (_value == "miter")
                _attributes.strokeJoin = StrokeJoin::Miter;
            else if (_value == "round")
                _attributes.strokeJoin = StrokeJoin::Round;
            else if (_value == "bevel")
                _attributes.strokeJoin = StrokeJoin::Bevel;

            _item->setStrokeJoin(_attributes.strokeJoin);
        }
        else if (_name == "stroke-miterlimit")
        {
            _attributes.miterLimit = coordinatePixels(_value.cString());
            _item->setMiterLimit(_attributes.miterLimit);
        }
        else if (_name == "vector-effect")
        {
            _attributes.bScalingStroke = _value != "non-scaling-stroke";
            _item->setScaleStroke(_attributes.bScalingStroke);
        }
        else if (_name == "stroke-dasharray")
        {
            _attributes.dashArray.clear();

            auto it = _value.begin();
            auto end = _value.end();
            it = detail::skipWhitespaceAndCommas(it, end);

            // handle none case
            if (it != end && *it != 'n')
            {
                while (it != end)
                {
                    // TODO take percentage start and length into account and pass it to toPixels
                    _attributes.dashArray.append(coordinatePixels(it));
                    while (it != end && !std::isspace(*it) && *it != ',')
                        ++it;
                    it = detail::skipWhitespaceAndCommas(it, end);
                }
            }

            _item->setDashArray(_attributes.dashArray);
        }
        else if (_name == "stroke-dashoffset")
        {
            _attributes.dashOffset = coordinatePixels(_value.cString());
            _item->setDashOffset(_attributes.dashOffset);
        }
        else if (_name == "font-size")
        {
            _attributes.fontSize = coordinatePixels(_value.cString());
        }
        else if (_name == "transform")
        {
            _item->setTransform(detail::parseTransform(_value.begin(), _value.end()));
        }
        else if (_name == "id")
        {
            //@TODO: Correct allocators
            _item->setName(String(_value.begin(), _value.end()));
            m_namedItems.insert(String(_value.begin(), _value.end()), _item);
        }
    }

    template <class SV>
    void parseStyle(SV _style, SVGAttributes & _attributes, Item * _item)
    {
        auto b = _style.begin();
        auto e = _style.end();
        SV left, right;
        while (b != e)
        {
            while (b != e && std::isspace(*b))
                ++b;
            auto ls = b;
            while (b != e && *b != ':')
                ++b;
            left = SV(ls, b);
            ++b;
            while (b != e && std::isspace(*b))
                ++b;
            ls = b;
            while (b != e && *b != ';')
                ++b;
            right = SV(ls, b);
            parseAttribute(left, right, _attributes, _item);
            if (b != e)
                ++b;
        }
    }

    void pushAttributes(pugi::xml_node _node, pugi::xml_node _rootNode, Item * _item)
    {
        SVGAttributes attr;
        if (m_attributeStack.count())
            attr = m_attributeStack.last();

        for (auto it = _node.attributes_begin(); it != _node.attributes_end(); ++it)
            parseAttribute(makeStringView(it->name()), makeStringView(it->value()), attr, _item);

        m_attributeStack.append(attr);
    }

    void popAttributes()
    {
        m_attributeStack.removeLast();
    }

    Document * m_document;
    Size m_dpi;
    DynamicArray<SVGAttributes> m_attributeStack;
    DynamicArray<Rect> m_viewStack;
    // for items that are only temporary in the dom and should
    // be removed at the end of the import (i.e. clipping masks, defs nodes etc.)
    DynamicArray<Item *> m_tmpItems;
    HashMap<String, Item *> m_namedItems;
};

SVGImport::SVGImport()
{
}

SVGImportResult SVGImport::parse(Document & _doc, const String & _svg, Size _dpi)
{
    SVGImportSession session;
    return session.parse(_doc, _svg, _dpi);
}

} // namespace svg
} // namespace paper