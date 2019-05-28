#include <Stick/DynamicArray.hpp>
#include <Stick/FixedArray.hpp>
#include <Stick/HashMap.hpp>
#include <Stick/Result.hpp>
#include <Stick/URI.hpp>

#include <Crunch/MatrixFunc.hpp>

#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/SVG/SVGImport.hpp>
#include <Paper2/SVG/SVGImportResult.hpp>
#include <Paper2/Symbol.hpp>

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
        Size len = std::distance(m_begin, m_end);
        if (len != std::strlen(_str))
            return false;
        return std::strncmp(m_begin, _str, len) == 0;
    }

    bool operator!=(const char * _str) const
    {
        return !(*this == _str);
    }

    const char * cString() const
    {
        return detail::toCString(m_begin, m_end);
    }

    String string() const
    {
        return String(begin(), end());
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

template <class T>
static std::pair<StringViewT<T>, StringViewT<T>> parsePreserveAspectRatio(T _begin, T _end)
{
    // skip whitespace
    while (_begin != _end && *_begin == ' ')
        ++_begin;
    T aStart = _begin;
    while (_begin != _end && *_begin != ' ')
        ++_begin;

    StringViewT<T> a(aStart, _begin);
    T bStart = ++_begin;
    while (_begin != _end && *_begin != ' ')
        ++_begin;
    return std::make_pair(a, StringViewT<T>(bStart, _begin));
}

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
    ColorRGBA fillColor = ColorRGBA(0, 0, 0, 1);
    ColorRGBA strokeColor = ColorRGBA(0, 0, 0, 1);
    WindingRule windingRule = WindingRule::NonZero;
    // since we dont support text yet, we only care
    // about font size for em/ex calculations
    Float fontSize = 16;
    Float strokeWidth;
    StrokeCap strokeCap;
    StrokeJoin strokeJoin;
    bool bScalingStroke;
    Float miterLimit;
    DynamicArray<Float> dashArray;
    Float dashOffset;
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

// to differentiate between slight differences in between these types of SVG groups
// used to trigger specific behavior in importGroup.
enum class SVGGroupType
{
    Group,
    SVG,
    Symbol,
    Defs
};

class STICK_API SVGImportSession
{
  public:
    SVGImportResult parse(Document & _doc, const String & _svg, Size _dpi)
    {
        printf("START PARSE\n");
        m_document = &_doc;
        m_dpi = _dpi;
        m_namedItems.clear();
        m_viewportStack.clear();
        m_rootNode = pugi::xml_node();

        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer(_svg.ptr(), _svg.length());

        if (result)
        {
            m_rootNode = doc.first_child();
            m_viewportStack.append({ 0, 0, _doc.width(), _doc.height() });
            auto vp = parseViewport(m_rootNode, { SVGUnits::Percent, 100 });

            Error err;
            Group * ret = m_document->createGroup();
            m_hiddenGroup = m_document->createGroup("HiddenSVGGroup");
            m_hiddenGroup->setVisible(false);
            ret->addChild(m_hiddenGroup);
            Group * grp = static_cast<Group *>(recursivelyImportNode(doc.first_child(), err));

            // recursivelySetDefaultFill(grp);

            ret->addChild(grp);
            return SVGImportResult(ret, vp[1], vp[2], err);
        }
        else
        {
            return SVGImportResult(
                Error(stick::ec::ParseFailed,
                      String::concat("Could not parse xml document: ", result.description()),
                      STICK_FILE,
                      STICK_LINE));
        }
        printf("END PARSE\n");
    }

    // void recursivelySetDefaultFill(Item * _item)
    // {
    //     for (Item * child : _item->children())
    //     {
    //         if (child->itemType() == ItemType::Path)
    //         {
    //             // set the default fill if there is none directly set on the child
    //             if (!child->style()->hasFill())
    //                 child->setFill(ColorRGBA(0, 0, 0, 1));
    //         }
    //         else if (child->itemType() == ItemType::Group)
    //         {
    //             // only recurse into the group if there is no fill set on it
    //             if (!child->style()->hasFill())
    //                 recursivelySetDefaultFill(child);
    //         }
    //     }
    // }

    Item * recursivelyImportNode(pugi::xml_node _node, Error & _error)
    {
        // first check if the item was allready imported by name
        auto it = m_namedItems.end();
        if (auto idAttr = _node.attribute("id"))
            it = m_namedItems.find(idAttr.value());

        if (it != m_namedItems.end())
            return it->value;

        return recursivelyImportNodeImpl(_node, _error);
    }

    Item * recursivelyImportNodeImpl(pugi::xml_node _node, Error & _error)
    {
        printf("recursivelyImportNode %s\n", _node.name());
        Item * item = nullptr;

        if (std::strcmp(_node.name(), "svg") == 0)
        {
            item = importGroup(_node, SVGGroupType::SVG, _error);
        }
        else if (std::strcmp(_node.name(), "symbol") == 0)
        {
            item = importGroup(_node, SVGGroupType::Symbol, _error);
        }
        else if (std::strcmp(_node.name(), "g") == 0)
        {
            item = importGroup(_node, SVGGroupType::Group, _error);
        }
        else if (std::strcmp(_node.name(), "rect") == 0)
        {
            item = importRectangle(_node, _error);
        }
        else if (std::strcmp(_node.name(), "circle") == 0)
        {
            item = importCircle(_node, _error);
        }
        else if (std::strcmp(_node.name(), "ellipse") == 0)
        {
            item = importEllipse(_node, _error);
        }
        else if (std::strcmp(_node.name(), "line") == 0)
        {
            item = importLine(_node, _error);
        }
        else if (std::strcmp(_node.name(), "polyline") == 0)
        {
            item = importPolyline(_node, false, _error);
        }
        else if (std::strcmp(_node.name(), "polygon") == 0)
        {
            item = importPolyline(_node, true, _error);
        }
        else if (std::strcmp(_node.name(), "path") == 0)
        {
            item = importPath(_node, _error);
        }
        else if (std::strcmp(_node.name(), "clipPath") == 0)
        {
            item = importClipPath(_node, _error);
        }
        else if (std::strcmp(_node.name(), "defs") == 0)
        {
            item = importGroup(_node, SVGGroupType::Defs, _error);
            // if (!_error)
            //     m_tmpItems.append(item);
        }
        else if (std::strcmp(_node.name(), "use") == 0)
        {
            item = importUse(_node, _error);
        }
        else
        {
            //?
        }

        if (item)
        {
            // accomodate for SVG's default winding rule (which is NonZero, we defaults to
            // EvenOdd)
            // if (item->itemType() == ItemType::Path)
            // {
            //     if (!item->style()->hasWindingRule())
            //         item->setWindingRule(WindingRule::NonZero);
            // }

            // // we take care of the clip-path / viewBox (as it might induce clipping, too)
            // attribute
            // // after parsing finished, as it might have us nest the item in a group that needs to
            // be
            // // returned instead of the item
            // detail::findXMLAttrCB(_node, "clip-path", item, [&](Item * _it, pugi::xml_node
            // _child) {
            //     auto url =
            //         detail::parseURL(_child.valueString().begin(), _child.valueString().end());

            //     //@TODO: hmmm any way to avoid this string copy without introducing StringViews
            //     to Stick? Don't think so :/ String str(url.begin(), url.end()); auto it =
            //     m_namedItems.find(str); Path * mask;
            //     // check if we allready imported the clip path
            //     if (it != m_namedItems.end())
            //     {
            //         mask = brick::reinterpretEntity<Path>(it->value);
            //     }
            //     else
            //     {
            //         // if not, find it in the document and import it
            //         auto maybe = _rootNode.find([&str](const Shrub & _s) {
            //             for (auto & _c : _s)
            //             {
            //                 if (_c.valueHint() == ValueHint::XMLAttribute && _c.name() == "id" &&
            //                     _c.valueString() == str)
            //                     return true;
            //             }
            //             return false;
            //         });

            //         if (maybe)
            //         {
            //             Error err;
            //             mask = importClipPath(*maybe, _rootNode, err);
            //             // TODO: HANDLE ERROR?
            //         }
            //     }

            //     if (mask.isValid())
            //     {
            //         if (_it.itemType() == EntityType::Group)
            //         {
            //             Group grp = brick::reinterpretEntity<Group>(_it);
            //             grp.setClipped(true);
            //             if (grp.children().count())
            //             {
            //                 mask.clone().insertBelow(grp.children().first());
            //             }
            //             else
            //             {
            //                 grp.addChild(mask.clone());
            //             }
            //         }
            //         else
            //         {
            //             Group grp = m_document->createGroup();
            //             grp.addChild(mask.clone());
            //             grp.addChild(_it);
            //             grp.setClipped(true);
            //             item = grp;
            //         }
            //     }
            //     else
            //     {
            //         // WARNING? ERROR?
            //     }
            // });
        }
        return item;
    }

    FixedArray<Float, 4> parseViewport(pugi::xml_node _node, SVGCoordinate _defaultHW)
    {
        auto mx = _node.attribute("x");
        auto my = _node.attribute("y");
        auto mw = _node.attribute("width");
        auto mh = _node.attribute("height");

        Float x, y, w, h;
        Vec2f cvv = currentViewVertical();
        Vec2f cvh = currentViewHorizontal();

        w = coordinatePixels(mw ? mw.value() : "auto", cvh.x, cvh.y, _defaultHW);
        h = coordinatePixels(mh ? mh.value() : "auto", cvv.x, cvv.y, _defaultHW);
        x = mx ? coordinatePixels(mx.value(), cvh.x, cvh.y) : 0;
        y = my ? coordinatePixels(my.value(), cvv.x, cvv.y) : 0;

        return { x, y, w, h };
    }

    void parseViewBox(pugi::xml_node _node, Item * _item, Path * _mask)
    {
        // parse the attributes that need to be parsed in a certain order first...
        detail::findXMLAttrCB(_node, "viewBox", _item, [&](Item * _it, pugi::xml_attribute _attr) {
            printf("PARSING VIEWBOX\n");
            STICK_ASSERT(_it->itemType() == ItemType::Group);
            stick::DynamicArray<Float> numbers(m_document->allocator());
            numbers.reserve(4);

            detail::parseNumbers(_attr.value(),
                                 _attr.value() + std::strlen(_attr.value()),
                                 [](char _c) { return false; },
                                 numbers);

            printf("PARSING VIEWBOX2\n");
            // @TODO: take preserveAspectRatio attribute into account (argh NOOOOOOOOOO)
            const char * pav = "xMidYMid meet";
            if (auto pan = _node.attribute("preserveAspectRatio"))
                pav = pan.value();

            auto pair = detail::parsePreserveAspectRatio(pav, pav + std::strlen(pav));
            printf("DA STRINGS %s B %s\n",
                   pair.first.string().cString(),
                   pair.second.string().cString());

            const Rect & r = m_viewportStack.last();
            Rect r2(numbers[0], numbers[1], numbers[0] + numbers[2], numbers[1] + numbers[3]);
            Mat32f viewTransform = Mat32f::identity();

            if (pair.second == "meet")
            {
                Float sf = r.width() / r2.width();
                if (r2.height() * sf > r.height())
                    sf = r.height() / r2.height();

                printf("SCALE FACT %f\n", sf);
                viewTransform.scale(sf);
            }
            else if (pair.second == "slice")
            {
                Float sf = r.width() / r2.width();
                if (r2.height() * sf < r.height())
                    sf = r.height() / r2.height();

                printf("SCALE FACT %f\n", sf);
                viewTransform.scale(sf);
            }
            else
            {
                printf("DA NOOONE\n");
                Vec2f scale(r.width() / r2.width(), r.height() / r2.height());
                viewTransform.scale(scale);
            }

            // viewTransform.translate(-r2.min() * scaleFact);

            // we do the translations in their respective spaces as thats easier to wrap ma head
            // around...
            Mat32f translation = Mat32f::translation(-r2.min());

            if (pair.first == "none")
            {
                printf("PAAA\n");
                viewTransform.translate(r.min());
            }
            else if (pair.first == "xMidYMid")
            {
                viewTransform.translate(r.center());
                translation.translate(-r2.size() * 0.5);
            }
            else if (pair.first == "xMinYMid")
            {
                viewTransform.translate(r.min().x, r.center().y);
                translation.translate(r2.size() * Vec2f(0.0, -0.5));
            }
            else if (pair.first == "xMaxYMid")
            {
                viewTransform.translate(r.max().x, r.center().y);
                translation.translate(r2.size() * Vec2f(-1.0, -0.5));
            }
            else if (pair.first == "xMidYMin")
            {
                viewTransform.translate(r.center().x, r.min().y);
                translation.translate(r2.size() * Vec2f(-0.5, 0.0));
            }
            else if (pair.first == "xMidYMax")
            {
                viewTransform.translate(r.center().x, r.max().y);
                translation.translate(r2.size() * Vec2f(-0.5, -1.0));
            }
            else if (pair.first == "xMinYMin")
            {
                viewTransform.translate(r.min());
                // translation.translate(r2.size() * Vec2f(-0.5, -1.0));
            }
            else if (pair.first == "xMinYMax")
            {
                viewTransform.translate(r.min().x, r.max().y);
                translation.translate(r2.size() * Vec2f(0, -1.0));
            }
            else if (pair.first == "xMaxYMin")
            {
                viewTransform.translate(r.max().x, r.min().y);
                translation.translate(r2.size() * Vec2f(-1.0, 0.0));
            }
            else if (pair.first == "xMaxYMax")
            {
                viewTransform.translate(r.max());
                translation.translate(r2.size() * Vec2f(-1.0, -1.0));
            }

            //...and then just multiply them to get the final transform
            viewTransform = viewTransform * translation;
            _it->transform(viewTransform);

            if (_mask)
            {
                _mask->transform(inverse(viewTransform));
            }

            m_viewportStack.append(r2);
        });
    }

    Group * importGroup(pugi::xml_node _node, SVGGroupType _type, Error & _error)
    {
        printf("IMPORT GROUP\n");
        Group * grp = m_document->createGroup();
        Path * mask = nullptr;
        // establish a new view based on the provided x,y,width,height (needed for viewbox
        // calculation)
        if (_type == SVGGroupType::SVG || _type == SVGGroupType::Symbol)
        {
            auto vp = parseViewport(_node, SVGCoordinate{ SVGUnits::Percent, 100 });
            m_viewportStack.append(Rect(vp[0], vp[1], vp[0] + vp[2], vp[1] + vp[3]));
            mask = m_document->createRectangle(
                Vec2f(vp[0], vp[1]), Vec2f(vp[0] + vp[2], vp[1] + vp[3]), "SVG Viewport Mask");
            grp->addChild(mask);
            grp->setClipped(true);
            parseViewBox(_node, grp, mask);
        }
        pushAttributes(_node, grp);
        for (auto & child : _node)
        {
            Item * item = recursivelyImportNode(child, _error);
            if (_error)
                break;

            if (item)
                grp->addChild(item);
        }

        popAttributes();
        printf("IMPORT GROUP END\n");
        if (_type == SVGGroupType::Defs || _type == SVGGroupType::Symbol)
        {
            m_hiddenGroup->addChild(grp);
            return nullptr;
        }

        if (_type == SVGGroupType::SVG || _type == SVGGroupType::Symbol)
            m_viewportStack.removeLast();

        return grp;
    }

    Path * importClipPath(pugi::xml_node _node, Error & _error)
    {
        printf("IMPORT DA CLIP\n");
        Path * ret = m_document->createPath();
        for (auto & child : _node)
        {
            Item * item = recursivelyImportNode(child, _error);
            if (_error)
                break;
            // only add paths as children, ignore the rest (there should be no rest though,
            // safety first :))
            if (item && item->itemType() == ItemType::Path)
                ret->addChild(static_cast<Path *>(item));
        }
        pushAttributes(_node, ret);
        popAttributes();
        m_tmpItems.append(ret);
        return ret;
    }

    Path * importPath(pugi::xml_node _node, Error & _error)
    {
        if (auto attr = _node.attribute("d"))
        {
            const char * str = attr.value();
            Path * p = m_document->createPath();
            pushAttributes(_node, p);
            parsePathData(*m_document, p, str);
            popAttributes();
            printf("IMPORT PATH %lu\n", p->segmentCount());
            return p;
        }
        return nullptr;
    }

    Path * importPolyline(pugi::xml_node _node, bool _bIsPolygon, Error & _error)
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

            pushAttributes(_node, ret);
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

    Path * importCircle(pugi::xml_node _node, Error & _error)
    {
        auto mcx = _node.attribute("cx");
        auto mcy = _node.attribute("cy");
        auto mr = _node.attribute("r");
        if (mr)
        {
            Vec2f hStartAndLength = currentViewHorizontal();
            Vec2f vStartAndLength = currentViewVertical();
            Float x = mcx ? coordinatePixels(mcx.value(),
                                             hStartAndLength.x,
                                             hStartAndLength.y,
                                             SVGCoordinate{ SVGUnits::PX, 0.0 })
                          : 0;
            Float y = mcy ? coordinatePixels(mcy.value(),
                                             vStartAndLength.x,
                                             vStartAndLength.y,
                                             SVGCoordinate{ SVGUnits::PX, 0.0 })
                          : 0;
            printf("IMPORT CIRCLE %f %f %f\n", x, y, mr.as_float());
            Path * ret = m_document->createCircle(
                Vec2f(x, y),
                coordinatePixels(
                    mr.value(), 0, currentViewLength(), SVGCoordinate{ SVGUnits::PX, 0.0 }));
            pushAttributes(_node, ret);
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

    Path * importEllipse(pugi::xml_node _node, Error & _error)
    {
        auto mcx = _node.attribute("cx");
        auto mcy = _node.attribute("cy");
        auto mrx = _node.attribute("rx");
        auto mry = _node.attribute("ry");
        if (mrx && mry)
        {
            Vec2f hStartAndLength = currentViewHorizontal();
            Vec2f vStartAndLength = currentViewVertical();
            Float x = mcx ? coordinatePixels(mcx.value(),
                                             hStartAndLength.x,
                                             hStartAndLength.y,
                                             SVGCoordinate{ SVGUnits::PX, 0.0 })
                          : 0;
            Float y = mcy ? coordinatePixels(mcy.value(),
                                             vStartAndLength.x,
                                             vStartAndLength.y,
                                             SVGCoordinate{ SVGUnits::PX, 0.0 })
                          : 0;
            Path * ret = m_document->createEllipse(
                Vec2f(x, y),
                Vec2f(coordinatePixels(
                          mrx.value(), 0, currentViewLength(), SVGCoordinate{ SVGUnits::PX, 0.0 }) *
                          2,
                      coordinatePixels(
                          mry.value(), 0, currentViewLength(), SVGCoordinate{ SVGUnits::PX, 0.0 }) *
                          2));
            pushAttributes(_node, ret);
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

    Path * importRectangle(pugi::xml_node _node, Error & _error)
    {
        auto mx = _node.attribute("x");
        auto my = _node.attribute("y");
        auto mw = _node.attribute("width");
        auto mh = _node.attribute("height");

        if (mx && my && mw && mh)
        {
            auto mrx = _node.attribute("rx");
            auto mry = _node.attribute("ry");

            Vec2f hStartAndLength = currentViewHorizontal();
            Vec2f vStartAndLength = currentViewVertical();
            Float x = coordinatePixels(mx.value(),
                                       hStartAndLength.x,
                                       hStartAndLength.y,
                                       SVGCoordinate{ SVGUnits::PX, 0.0 });
            Float y = coordinatePixels(my.value(),
                                       vStartAndLength.x,
                                       vStartAndLength.y,
                                       SVGCoordinate{ SVGUnits::PX, 0.0 });
            Float w = coordinatePixels(mw.value(),
                                       hStartAndLength.x,
                                       hStartAndLength.y,
                                       SVGCoordinate{ SVGUnits::Percent, 100.0 });
            Float h = coordinatePixels(mh.value(),
                                       vStartAndLength.x,
                                       vStartAndLength.y,
                                       SVGCoordinate{ SVGUnits::Percent, 100.0 });
            Path * ret;
            if (mrx || mry)
            {
                Float rx = mrx ? coordinatePixels(mrx.value(),
                                                  hStartAndLength.x,
                                                  hStartAndLength.y,
                                                  SVGCoordinate{ SVGUnits::PX, 0.0 })
                               : coordinatePixels(mry.value(),
                                                  vStartAndLength.x,
                                                  vStartAndLength.y,
                                                  SVGCoordinate{ SVGUnits::PX, 0.0 });
                Float ry = mry ? coordinatePixels(mry.value(),
                                                  vStartAndLength.x,
                                                  vStartAndLength.y,
                                                  SVGCoordinate{ SVGUnits::PX, 0.0 })
                               : rx;
                ret = m_document->createRoundedRectangle(
                    Vec2f(x, y), Vec2f(x, y) + Vec2f(w, h), Vec2f(rx, ry));
            }
            else
            {
                ret = m_document->createRectangle(Vec2f(x, y), Vec2f(x, y) + Vec2f(w, h));
            }
            pushAttributes(_node, ret);
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

    Path * importLine(pugi::xml_node _node, Error & _error)
    {
        auto mx1 = _node.attribute("x1");
        auto my1 = _node.attribute("y1");
        auto mx2 = _node.attribute("x2");
        auto my2 = _node.attribute("y2");
        if (mx1 && my1 && mx2 && my2)
        {
            Vec2f hStartAndLength = currentViewHorizontal();
            Vec2f vStartAndLength = currentViewVertical();

            Float x1 = coordinatePixels(mx1.value(),
                                        hStartAndLength.x,
                                        hStartAndLength.y,
                                        SVGCoordinate{ SVGUnits::PX, 0.0 });
            Float y1 = coordinatePixels(my1.value(),
                                        vStartAndLength.x,
                                        vStartAndLength.y,
                                        SVGCoordinate{ SVGUnits::PX, 0.0 });
            Float x2 = coordinatePixels(mx2.value(),
                                        hStartAndLength.x,
                                        hStartAndLength.y,
                                        SVGCoordinate{ SVGUnits::PX, 0.0 });
            Float y2 = coordinatePixels(my2.value(),
                                        vStartAndLength.x,
                                        vStartAndLength.y,
                                        SVGCoordinate{ SVGUnits::PX, 0.0 });

            SegmentDataArray segs(m_document->allocator());
            segs.reserve(2);
            segments::addPoint(segs, Vec2f(x1, y1));
            segments::addPoint(segs, Vec2f(x2, y2));
            Path * ret = m_document->createPath();
            ret->swapSegments(segs, false);
            pushAttributes(_node, ret);
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

    Symbol * importUse(pugi::xml_node _node, Error & _error)
    {
        pugi::xml_attribute href = _node.attribute("xlink:href");
        href = href ? href : _node.attribute("href");
        if (!href)
        {
            _error = Error(ec::ParseFailed,
                           "Svg use node is missing href or xlink:href attribute",
                           STICK_FILE,
                           STICK_LINE);
            return nullptr;
        }

        const char * name = href.value();
        while (std::isspace(*name) || *name == '#')
            ++name;

        Item * item = namedItem(name, _error);

        if (_error)
            return nullptr;

        Symbol * ret = m_document->createSymbol(item);

        // stick::Maybe<Mat32f> transform = inverse(item->absoluteTransform());
        stick::Maybe<Mat32f> transform;
        auto xa = _node.attribute("x");
        auto ya = _node.attribute("y");
        auto wa = _node.attribute("width");
        auto ha = _node.attribute("height");
        Vec2f hStartAndLength = currentViewHorizontal();
        Vec2f vStartAndLength = currentViewVertical();
        if (wa || ha)
        {
            Float w = wa ? coordinatePixels(wa.value(),
                                            hStartAndLength.x,
                                            hStartAndLength.y,
                                            SVGCoordinate{ SVGUnits::PX, 0.0 })
                         : item->bounds().width();
            Float h = ha ? coordinatePixels(ha.value(),
                                            vStartAndLength.x,
                                            vStartAndLength.y,
                                            SVGCoordinate{ SVGUnits::PX, 0.0 })
                         : item->bounds().height();
            transform = Mat32f::scaling(w / item->bounds().width(), h / item->bounds().height());
        }
        if (xa || ya)
        {
            printf("SETTING POSITION\n");
            Float x = xa ? coordinatePixels(xa.value(),
                                            hStartAndLength.x,
                                            hStartAndLength.y,
                                            SVGCoordinate{ SVGUnits::PX, 0.0 })
                         : 0;
            Float y = ya ? coordinatePixels(ya.value(),
                                            vStartAndLength.x,
                                            vStartAndLength.y,
                                            SVGCoordinate{ SVGUnits::PX, 0.0 })
                         : 0;
            printf("X Y %f %f\n", x, y);
            if (transform)
                (*transform).translate(x, y);
            else
                transform = Mat32f::translation(x, y);
        }

        printf("IMPORT USSSE\n");
        if (transform)
            ret->setTransform(*transform);

        pushAttributes(_node, ret);
        popAttributes();

        return ret;
    }

    Item * namedItem(const char * _name, Error & _error)
    {
        auto it = m_namedItems.find(_name);
        if (it != m_namedItems.end())
            return it->value;

        auto node = m_rootNode.find_child([_name](pugi::xml_node _node) {
            if (auto attr = _node.attribute("id"))
                return std::strcmp(attr.value(), _name) == 0;
            return false;
        });

        if (node)
            return recursivelyImportNodeImpl(node, _error);

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
        bool bCloseCurrentPath = false;
        SegmentDataArray segments(_doc.allocator());
        segments.reserve(32);

        // lambda makes this a lot nicer...
        auto finishCurrentContour = [&](bool _bCreateNewPath) {
            if (segments.count())
            {
                currentPath->swapSegments(segments, bCloseCurrentPath);

                if (_bCreateNewPath)
                {
                    segments.reserve(32);
                    bCloseCurrentPath = false;
                    Path * tmp = _doc.createPath();
                    currentPath->addChild(tmp);
                    currentPath = tmp;
                }
                else
                    currentPath = nullptr;
            }
        };

        do
        {
            char cmd = *it;
            // auto tend = advanceToNextCommand(it + 1, end);
            ++it;
            it = detail::parseNumbers(it, end, detail::isCommand, numbers);

            printf("CMD %c\n", cmd);
            for (auto num : numbers)
                printf("NUM %f\n", num);

            // STICK_ASSERT(it == tend);
            if (cmd == 'M' || cmd == 'm')
            {

                finishCurrentContour(true);

                bool bRelative = cmd == 'm';
                for (Size i = 0; i < numbers.count(); i += 2)
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
                for (Size i = 0; i < numbers.count(); i += 2)
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
                for (Size i = 0; i < numbers.count(); ++i)
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
                for (Size i = 0; i < numbers.count(); i += 6)
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
                for (Size i = 0; i < numbers.count(); i += 4)
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
                for (Size i = 0; i < numbers.count(); i += 4)
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
                for (Size i = 0; i < numbers.count(); i += 2)
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
                printf("DO DA ARC %lu\n", numbers.count());
                for (Size i = 0; i < numbers.count(); i += 7)
                {
                    last = !bRelative ? Vec2f(numbers[i + 5], numbers[i + 6])
                                      : last + Vec2f(numbers[i + 5], numbers[i + 6]);

                    printf("WOOOOOP %f %f %f %f %f %f %f\n",
                           numbers[i],
                           numbers[i + 1],
                           numbers[i + 2],
                           numbers[i + 3],
                           numbers[i + 4],
                           numbers[i + 5],
                           numbers[i + 6]);
                    segments::arcTo(segments,
                                    last,
                                    Vec2f(numbers[i], numbers[i + 1]),
                                    crunch::toRadians(numbers[i + 2]),
                                    (bool)numbers[i + 4],
                                    (bool)numbers[i + 3]);
                    lastHandle = segments.last().handleOut;
                }
            }
            else if (cmd == 'Z' || cmd == 'z')
            {
                bCloseCurrentPath = true;
                // currentPath->swapSegments(segments, true);
                // last = currentPath->segmentData().last().position;
                // lastHandle = currentPath->segmentData().last().handleOut;
            }
            else
            {
                //@TODO?
            }
        } while (it != end);

        finishCurrentContour(false);
    }

    Float toPixels(Float _value, SVGUnits _units, Float _start = 0.0, Float _length = 1.0)
    {
        Float fontSize = m_attributeStack.count() ? m_attributeStack.last().fontSize : 16;
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
            return _value * fontSize;
        case SVGUnits::EX:
            return _value * fontSize *
                   0.5; // since we don't load fonts (yet), we pick the default x-height of 0.5
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

    Float coordinatePixels(
        const char * _str,
        Float _start,
        Float _length,
        stick::Maybe<SVGCoordinate> _defaultValue = stick::Maybe<SVGCoordinate>())
    {
        if (std::strcmp(_str, "auto") == 0)
        {
            STICK_ASSERT(_defaultValue);
            return toPixels((*_defaultValue).value, (*_defaultValue).units, _start, _length);
        }
        auto coord = parseCoordinate(_str);
        printf("coord.value %f %f %f\n", coord.value, _start, _length);
        return toPixels(coord.value, coord.units, _start, _length);
    }

    Vec2f currentViewVertical() const
    {
        if (!m_viewportStack.count())
            return (Vec2f(0, 1));

        return Vec2f(m_viewportStack.last().min().y, m_viewportStack.last().height());
    }

    Vec2f currentViewHorizontal() const
    {
        if (!m_viewportStack.count())
            return (Vec2f(0, 1));

        return Vec2f(m_viewportStack.last().min().x, m_viewportStack.last().width());
    }

    Float currentViewLength() const
    {
        if (!m_viewportStack.count())
            return 1.0;

        Float w = m_viewportStack.last().width();
        Float h = m_viewportStack.last().height();
        printf("DA FOOOOCKING %lu %f %f\n", m_viewportStack.count(), w, h);
        return std::sqrt(w * w + h * h) / std::sqrt(2.0f);
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
                    printf("PARSED COLOR: %f %f %f\n", (*col).r, (*col).g, (*col).b);
                    auto c = toRGBA(*col);
                    // we need to get the current alpha, as that might have been set allready
                    c.a = _attributes.fillColor.a;
                    printf("CA %f\n", c.a);
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
                    printf("PARSED COLOR: %f %f %f\n", (*col).r, (*col).g, (*col).b);
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
            _attributes.strokeWidth = coordinatePixels(
                _value.cString(), 0, currentViewLength(), SVGCoordinate{ SVGUnits::PX, 1.0 });
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
            _attributes.miterLimit = std::max(1.0, std::atof(_value.cString()));
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
                    _attributes.dashArray.append(coordinatePixels(it, 0, currentViewLength()));
                    while (it != end && !std::isspace(*it) && *it != ',')
                        ++it;
                    it = detail::skipWhitespaceAndCommas(it, end);
                }
            }

            _item->setDashArray(_attributes.dashArray);
        }
        else if (_name == "stroke-dashoffset")
        {
            _attributes.dashOffset = coordinatePixels(
                _value.cString(), 0, currentViewLength(), SVGCoordinate{ SVGUnits::PX, 0.0 });
            _item->setDashOffset(_attributes.dashOffset);
        }
        else if (_name == "font-size")
        {
            _attributes.fontSize = coordinatePixels(_value.cString(), 0, currentViewLength());
            printf("GOT FONT SIZE %f\n", _attributes.fontSize);
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

    void pushAttributes(pugi::xml_node _node, Item * _item)
    {
        SVGAttributes attr;

        //@TODO: Is this needed???!?!?!
        // if (m_attributeStack.count())
        //     attr = m_attributeStack.last();

        // parse the attributes that need to be parsed in a certain order first...

        //...parse the remaining attributes
        for (auto it = _node.attributes_begin(); it != _node.attributes_end(); ++it)
            parseAttribute(makeStringView(it->name()), makeStringView(it->value()), attr, _item);

        m_attributeStack.append(attr);
    }

    void popAttributes()
    {
        m_attributeStack.removeLast();
    }

    Document * m_document;
    pugi::xml_node m_rootNode;
    Size m_dpi;
    DynamicArray<SVGAttributes> m_attributeStack;
    DynamicArray<Rect> m_viewportStack;
    // for items that are only temporary in the dom and should
    // be removed at the end of the import (i.e. clipping masks, defs nodes etc.)
    DynamicArray<Item *> m_tmpItems;
    HashMap<String, Item *> m_namedItems;
    Group * m_hiddenGroup; // group holding defs and symbol nodes, not affected by the svg's
                           // transform/viewBox etc.
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