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

class StringView
{
  public:
    StringView()
    {
    }

    StringView(String::ConstIter _begin, String::ConstIter _end) : m_begin(_begin), m_end(_end)
    {
    }

    String::ConstIter begin() const
    {
        return m_begin;
    }

    String::ConstIter end() const
    {
        return m_end;
    }

  private:
    String::ConstIter m_begin;
    String::ConstIter m_end;
};

static StringView parseURL(String::ConstIter _begin, String::ConstIter _end)
{
    while (_begin != _end && *_begin != '(')
        ++_begin;
    while (_begin != _end && *_begin != '#')
        ++_begin;
    ++_begin;
    auto begin = _begin;
    while (_begin != _end && *_begin != ')')
        ++_begin;
    return StringView(begin, _begin);
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
        Item * item;
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
    }

    Group * importGroup(pugi::xml_node _node,
                        pugi::xml_node _rootNode,
                        bool _bSVGNode,
                        Error & _error)
    {
    }

    Path * importClipPath(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
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
    }

    Path * importCircle(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
    }

    Path * importEllipse(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
    }

    Path * importRectangle(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
    }

    Path * importLine(pugi::xml_node _node, pugi::xml_node _rootNode, Error & _error)
    {
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

    void parseAttribute(pugi::xml_attribute _attr, SVGAttributes & _attributes, Item * _item)
    {
        if (std::strcmp(_attr.name(), "style") == 0)
        {
            // parseStyle(_value, _attr, _item);
        }
        else if (std::strcmp(_attr.name(), "fill") == 0)
        {
            if (std::strcmp(_attr.value(), "none") == 0)
            {
                _item->removeFill();
            }
            else
            {
                auto col =
                    detail::parseColor(_attr.value(), _attr.value() + std::strlen(_attr.value()));
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
        else if (std::strcmp(_attr.name(), "fill-opacity") == 0)
        {
            _attributes.fillColor.a = _attr.as_float();
            if (auto mf = _item->fill().maybe<ColorRGBA>())
            {
                _item->setFill(ColorRGBA((*mf).r, (*mf).g, (*mf).b, (*mf).a));
            }
        }
        else if (std::strcmp(_attr.name(), "fill-rule") == 0)
        {
            if (std::strcmp(_attr.value(), "nonzero") == 0)
                _attributes.windingRule = WindingRule::NonZero;
            else if (std::strcmp(_attr.value(), "evenodd") == 0)
                _attributes.windingRule = WindingRule::EvenOdd;
            _item->setWindingRule(_attributes.windingRule);
        }
        else if (std::strcmp(_attr.name(), "stroke") == 0)
        {
            if (std::strcmp(_attr.value(), "none") == 0)
            {
                _item->removeStroke();
            }
            else
            {
                auto col =
                    detail::parseColor(_attr.value(), _attr.value() + std::strlen(_attr.value()));
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
        else if (std::strcmp(_attr.name(), "stroke-opacity") == 0)
        {
            _attributes.strokeColor.a = _attr.as_float();
            if (auto mf = _item->stroke().maybe<ColorRGBA>())
            {
                _item->setStroke(ColorRGBA((*mf).r, (*mf).g, (*mf).b, _attributes.strokeColor.a));
            }
        }
        else if (std::strcmp(_attr.name(), "stroke-width") == 0)
        {
            _attributes.strokeWidth = coordinatePixels(_attr.value());
            _item->setStrokeWidth(_attributes.strokeWidth);
        }
        else if (std::strcmp(_attr.name(), "stroke-linecap") == 0)
        {
            if (std::strcmp(_attr.value(), "butt") == 0)
                _attributes.strokeCap = StrokeCap::Butt;
            else if (std::strcmp(_attr.value(), "round") == 0)
                _attributes.strokeCap = StrokeCap::Round;
            else if (std::strcmp(_attr.value(), "square") == 0)
                _attributes.strokeCap = StrokeCap::Square;

            _item->setStrokeCap(_attributes.strokeCap);
        }
        else if (std::strcmp(_attr.name(), "stroke-linejoin") == 0)
        {
            if (std::strcmp(_attr.value(), "miter") == 0)
                _attributes.strokeJoin = StrokeJoin::Miter;
            else if (std::strcmp(_attr.value(), "round") == 0)
                _attributes.strokeJoin = StrokeJoin::Round;
            else if (std::strcmp(_attr.value(), "bevel") == 0)
                _attributes.strokeJoin = StrokeJoin::Bevel;

            _item->setStrokeJoin(_attributes.strokeJoin);
        }
        else if (std::strcmp(_attr.name(), "stroke-miterlimit") == 0)
        {
            _attributes.miterLimit = coordinatePixels(_attr.value());
            _item->setMiterLimit(_attributes.miterLimit);
        }
        else if (std::strcmp(_attr.name(), "vector-effect") == 0)
        {
            _attributes.bScalingStroke = std::strcmp(_attr.value(), "non-scaling-stroke") != 0;
            _item->setScaleStroke(_attributes.bScalingStroke);
        }
        else if (std::strcmp(_attr.name(), "stroke-dasharray") == 0)
        {
            _attributes.dashArray.clear();

            auto it = _attr.value();
            auto end = _attr.value() + std::strlen(_attr.value());
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
        else if (std::strcmp(_attr.name(), "stroke-dashoffset") == 0)
        {
            _attributes.dashOffset = coordinatePixels(_attr.value());
            _item->setDashOffset(_attributes.dashOffset);
        }
        else if (std::strcmp(_attr.name(), "font-size") == 0)
        {
            _attributes.fontSize = coordinatePixels(_attr.value());
        }
        else if (std::strcmp(_attr.name(), "transform") == 0)
        {
            _item->setTransform(
                detail::parseTransform(_attr.value(), _attr.value() + std::strlen(_attr.value())));
        }
        else if (std::strcmp(_attr.name(), "id") == 0)
        {
            _item->setName(_attr.value());
            m_namedItems.insert(_attr.value(), _item);
        }
    }

    void parseStyle(const String & _style, SVGAttributes & _attr, Item * _item)
    {
    }

    void pushAttributes(pugi::xml_node _node, pugi::xml_node _rootNode, Item * _item)
    {
    }

    void popAttributes()
    {
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

} // namespace svg
} // namespace paper