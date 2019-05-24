#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Paper2/Private/Shape.hpp>
#include <Paper2/SVG/SVGExport.hpp>

#include <Stick/StringConversion.hpp>

#include <pugixml.hpp>

namespace paper
{
namespace svg
{
using namespace stick;

//@TODO: Make sure all string allocations use the session allocator.
//@TODO: Add some sort of style stack that also takes svg defaults into account to avoid setting
// redundant style information on svg nodes

struct UsedGradient
{
    const BaseGradient * gradient;
    String id;
};

using UsedGradientArray = stick::DynamicArray<UsedGradient>;

struct ExportSession
{
    Allocator * allocator;
    pugi::xml_node rootNode;
    pugi::xml_node defsNode;
    UsedGradientArray gradients;
    Size clipID;
};

struct StringWriter : pugi::xml_writer
{
    StringWriter(Allocator & _alloc) : result(_alloc)
    {
    }

    void write(const void * data, size_t size) final
    {
        result.append(static_cast<const char *>(data), size);
    }

    String result;
};

static pugi::xml_node ensureDefsNode(ExportSession & _session)
{
    if (!_session.defsNode)
        _session.defsNode = _session.rootNode.prepend_child("defs");
    return _session.defsNode;
}

static String colorToHexCSSString(const ColorRGBA & _color, Allocator & _alloc = defaultAllocator())
{
    UInt32 r =
        crunch::min(static_cast<UInt32>(255),
                    crunch::max(static_cast<UInt32>(0), static_cast<UInt32>(_color.r * 255)));
    UInt32 g =
        crunch::min(static_cast<UInt32>(255),
                    crunch::max(static_cast<UInt32>(0), static_cast<UInt32>(_color.g * 255)));
    UInt32 b =
        crunch::min(static_cast<UInt32>(255),
                    crunch::max(static_cast<UInt32>(0), static_cast<UInt32>(_color.b * 255)));

    return String::formatted(_alloc, "#%02x%02x%02x", r, g, b);
}

// static String colorToHexCSSString(const ColorRGBA & _color, Allocator & _alloc =
// defaultAllocator())
// {
//     return String::formatted(_alloc, "rgb(%i, %i, %i)", (Int32)(_color.r * 255), (Int32)(_color.g
//     * 255), (Int32)(_color.b * 255));
// }

static void setTransform(const Item * _item, pugi::xml_node _node)
{
    if (_item->hasTransform())
    {
        auto & tm = _item->transform();
        _node.append_attribute("transform") = String::formatted("matrix(%f, %f, %f, %f, %f, %f)",
                                                                tm.element(0, 0),
                                                                tm.element(0, 1),
                                                                tm.element(1, 0),
                                                                tm.element(1, 1),
                                                                tm.element(2, 0),
                                                                tm.element(2, 1))
                                                  .cString();
    }
}

static void addCurveToPathData(ConstCurve _curve, String & _currentData, bool _bApplyTransform)
{
    if (_curve.isLinear())
    {
        Vec2f stp = _curve.segmentTwo().position();
        Vec2f sop = _curve.segmentOne().position();

        // this happens if the curve is part of a compound path
        if (_bApplyTransform)
        {
            auto transform = _curve.path()->transform();
            stp = transform * stp;
            sop = transform * sop;
        }

        // relative line to
        Vec2f tmp = stp - sop;
        _currentData.appendFormatted(" l%.4f,%.4f", tmp.x, tmp.y);
    }
    else
    {
        Vec2f stp = _curve.segmentTwo().position();
        Vec2f sop = _curve.segmentOne().position();
        Vec2f ho = _curve.handleOneAbsolute();
        Vec2f ht = _curve.handleTwoAbsolute();

        // this happens if the curve is part of a compound path
        if (_bApplyTransform)
        {
            auto transform = _curve.path()->transform();
            stp = transform * stp;
            sop = transform * sop;
            ho = transform * ho;
            ht = transform * ht;
        }

        // relative curve to
        Vec2f a = ho - sop;
        Vec2f b = ht - sop;
        Vec2f c = stp - sop;
        _currentData.appendFormatted(
            " c%.4f,%.4f,%.4f,%.4f,%.4f,%.4f", a.x, a.y, b.x, b.y, c.x, c.y);
    }
}

static void addPathToPathData(const Path * _path, String & _currentData, bool _bIsCompoundPath)
{
    ConstCurveView curves = _path->curves();
    if (curves.count())
    {
        // absolute move to
        Vec2f to = curves[0].segmentOne().position();
        bool bApplyTransform = false;
        // for compound paths we need to transform the segment vertices directly
        if (_path->hasTransform() && _bIsCompoundPath)
        {
            bApplyTransform = true;
            to = _path->transform() * to;
        }
        _currentData.appendFormatted("M%.4f,%.4f", to.x, to.y);

        for (ConstCurve c : curves)
            addCurveToPathData(c, _currentData, bApplyTransform);
    }
}

static void recursivelyAddPathToPathData(const Path * _path, String & _currentData, bool _bFirst)
{
    addPathToPathData(_path, _currentData, _bFirst ? false : true);
    for (Size i = 0; i < _path->children().count(); ++i)
    {
        if (i > 0)
            _currentData.append(" ");
        recursivelyAddPathToPathData(static_cast<const Path *>(_path->children()[i]), _currentData, false);
    }
}

template <class Getter, class Getter2, class Getter3>
static void addPaintToStyle(Getter _getter,
                            Getter2 _hasPaintTransformGetter,
                            Getter3 _paintTransformGetter,
                            const char * _attributeName,
                            const Item * _item,
                            pugi::xml_node _node,
                            ExportSession & _session)
{
    if ((_item->*_getter)().template is<ColorRGBA>())
    {
        const ColorRGBA & color = (_item->*_getter)().template get<ColorRGBA>();
        _node.append_attribute(_attributeName) = colorToHexCSSString(color).cString();

        if (color.a < 1.0)
            _node.append_attribute(String::concat(_attributeName, "-opacity").cString()) = color.a;
    }
    else if (_item->fill().template is<LinearGradientPtr>() ||
             _item->fill().template is<RadialGradientPtr>())
    {
        const BaseGradient * grad =
            (_item->*_getter)().template is<LinearGradientPtr>()
                ? static_cast<const BaseGradient *>(
                      (_item->*_getter)().template get<LinearGradientPtr>().get())
                : static_cast<const BaseGradient *>(
                      (_item->*_getter)().template get<RadialGradientPtr>().get());

        // auto it = stick::findIf(_session.gradients.begin(), _session.gradients.end(),
        // [grad](const UsedGradient & _grad)
        // {
        //     return _grad.gradient == grad;
        // });

        Size matches = 0;
        UsedGradient * match = nullptr;
        for (auto & g : _session.gradients)
        {
            if (g.gradient == grad)
            {
                match = &g;
                ++matches;
            }
        }
        if (matches == 0 || matches > 1 || (_item->*_hasPaintTransformGetter)())
        {
            auto defsNode = ensureDefsNode(_session);

            pugi::xml_node gradNode(nullptr);

            bool bTransform = (_item->*_hasPaintTransformGetter)();
            Vec2f origin =
                bTransform ? (_item->*_paintTransformGetter)() * grad->origin() : grad->origin();
            Vec2f dest = bTransform ? (_item->*_paintTransformGetter)() * grad->destination()
                                    : grad->destination();

            if ((_item->*_getter)().template is<LinearGradientPtr>())
            {
                gradNode = defsNode.append_child("linearGradient");
                gradNode.append_attribute("x1") = origin.x;
                gradNode.append_attribute("y1") = origin.y;
                gradNode.append_attribute("x2") = dest.x;
                gradNode.append_attribute("y2") = dest.y;
            }
            else
            {
                const RadialGradient * rgrad = static_cast<const RadialGradient *>(grad);
                Float32 rad = crunch::distance(origin, dest);
                gradNode = defsNode.append_child("radialGradient");
                gradNode.append_attribute("cx") = origin.x;
                gradNode.append_attribute("cy") = origin.y;
                gradNode.append_attribute("r") = rad;
                if (const auto & mfo = rgrad->focalPointOffset())
                {
                    gradNode.append_attribute("fx") = origin.x + (*mfo).x;
                    gradNode.append_attribute("fy") = origin.y + (*mfo).y;
                }

                //@TODO: SVG radial gradients are somewhat limited. In order to properly support
                // elliptical gradients when the ratio is not 1.0, we need to most likely create a
                // gradient transform :/

                // if (const auto & mr = rgrad->ratio())
                // {
                //     gradNode.append_attribute("fr") = rad * (*mr);
                // }
            }

            gradNode.append_attribute("gradientUnits") = "userSpaceOnUse";

            UsedGradient ug = { grad, String::formatted("grad-%lu", _session.gradients.count()) };
            gradNode.append_attribute("id") = ug.id.cString();

            for (const auto & stop : grad->stops())
            {
                pugi::xml_node sn = gradNode.append_child("stop");
                sn.append_attribute("offset") =
                    String::formatted("%i%%", (int)(stop.offset * 100.0f)).cString();
                sn.append_attribute("stop-color") = colorToHexCSSString(stop.color).cString();
                if (stop.color.a != 1)
                    sn.append_attribute("stop-opacity") = stop.color.a;
            }

            _node.append_attribute(_attributeName) = String::concat("url(#", ug.id, ")").cString();

            _session.gradients.append(ug);
        }
        else if (matches == 1)
        {
            STICK_ASSERT(match);
            _node.append_attribute(_attributeName) =
                String::concat("url(#", match->id.cString(), ")").cString();
        }
    }
}

static void addStyle(const Item * _item,
                     pugi::xml_node _node,
                     ExportSession & _session,
                     bool _bIsClipPath = false)
{
    // We dont export the name for now: items with the same name will break valid SVG as the name
    // has to be unique in that case. Additionally valid svg names can't be arbitrary strings. I
    // don't think there is a good way of dealing with this so we don't for now.
    /*if(_item.name().length())
    {
        _node.addChild("id", _item.name(), ValueHint::XMLAttribute);
    }*/

    if (_bIsClipPath || !_item->fill().is<NoPaint>() || _item->children().count())
    {
        if (_item->windingRule() == WindingRule::NonZero)
            _node.append_attribute("fill-rule") = "nonzero";
        else
            _node.append_attribute("fill-rule") = "evenodd";
    }

    if (_bIsClipPath)
        return;

    if (!_item->isVisible())
        _node.append_attribute("visibility") = "hidden";

    // fill related things
    if (_item->style()->hasFill() && _item->fill().is<NoPaint>())
        _node.append_attribute("fill") = "none";
    else
        addPaintToStyle(&Item::fill,
                        &Item::hasfillPaintTransform,
                        &Item::fillPaintTransform,
                        "fill",
                        _item,
                        _node,
                        _session);

    // stroke related things
    if (!_item->style()->hasStroke())
        _node.append_attribute("stroke") = "none";
    else
        addPaintToStyle(&Item::stroke,
                        &Item::hasStrokePaintTransform,
                        &Item::strokePaintTransform,
                        "stroke",
                        _item,
                        _node,
                        _session);

    if (_item->style()->hasStroke() || _item->children().count())
    {
        if (_item->style()->hasStrokeWidth())
            _node.append_attribute("stroke-width") = _item->strokeWidth();

        if (_item->style()->hasStrokeCap())
        {
            switch (_item->strokeCap())
            {
            case StrokeCap::Butt:
                _node.append_attribute("stroke-linecap") = "butt";
                break;
            case StrokeCap::Square:
                _node.append_attribute("stroke-linecap") = "square";
                break;
            case StrokeCap::Round:
                _node.append_attribute("stroke-linecap") = "round";
                break;
            }
        }
        if (_item->style()->hasStrokeJoin())
        {
            switch (_item->strokeJoin())
            {
            case StrokeJoin::Bevel:
                _node.append_attribute("stroke-linejoin") = "bevel";
                break;
            case StrokeJoin::Miter:
                _node.append_attribute("stroke-linejoin") = "miter";
                break;
            case StrokeJoin::Round:
                _node.append_attribute("stroke-linejoin") = "round";
                break;
            }
        }

        if (_item->style()->hasMiterLimit())
            _node.append_attribute("stroke-miterlimit") = _item->miterLimit();

        if (_item->style()->hasDashArray() && _item->dashArray().count())
        {
            String dashString;
            auto it = _item->dashArray().begin();
            for (; it != _item->dashArray().end(); ++it)
            {
                if (it != _item->dashArray().begin())
                    dashString.appendFormatted(", %.4f", *it);
                else
                    dashString.appendFormatted("%.4f", *it);
            }
            _node.append_attribute("stroke-dasharray") = dashString.cString();
        }

        if (_item->style()->hasDashOffset())
            _node.append_attribute("stroke-dashoffset") = _item->dashOffset();

        if (_item->style()->hasScaleStroke() && !_item->scaleStroke())
            _node.append_attribute("vector-effect") = "non-scaling-stroke";
    }
}

static pugi::xml_node addItemToXml(const Item * _item,
                                   pugi::xml_node _parent,
                                   ExportSession & _session,
                                   bool _bMatchShapes);

static pugi::xml_node addDocument(const Document * _doc,
                                  pugi::xml_node _parent,
                                  ExportSession & _session,
                                  bool _bMatchShapes)
{
    pugi::xml_node svg = _parent.append_child("svg");
    svg.append_attribute("xmlns") = "http://www.w3.org/2000/svg";
    svg.append_attribute("xmlns:xlink") = "http://www.w3.org/1999/xlink";
    svg.append_attribute("width") = _doc->width();
    svg.append_attribute("height") = _doc->height();
    // _session.defsNode = svg.append_child("defs");
    _session.rootNode = svg;

    pugi::xml_node parent = svg;
    if (_doc->hasTransform())
    {
        parent = svg.append_child("g");
        setTransform(_doc, parent);
    }

    for (const Item * child : _doc->children())
        addItemToXml(child, parent, _session, _bMatchShapes);

    return svg;
}

static pugi::xml_node exportCurveData(const Path * _path, pugi::xml_node _parent)
{
    if (_path->isPolygon())
    {
        if (_path->segmentCount() > 2)
        {
            String points;
            for (Size i = 0; i < _path->segmentCount(); ++i)
            {
                auto & segData = _path->segmentData()[i];
                if (i != _path->segmentCount() - 1)
                    points.appendFormatted("%.4f,%.4f ", segData.position.x, segData.position.y);
                else
                    points.appendFormatted("%.4f,%.4f", segData.position.x, segData.position.y);
            }

            pugi::xml_node node = _parent.append_child(_path->isClosed() ? "polygon" : "polyline");
            node.append_attribute("points") = points.cString();
            return node;
        }
        else if (_path->segmentCount() == 2)
        {
            pugi::xml_node node = _parent.append_child("line");
            node.append_attribute("x1") = _path->segmentData()[0].position.x;
            node.append_attribute("y1") = _path->segmentData()[0].position.y;
            node.append_attribute("x2") = _path->segmentData()[1].position.x;
            node.append_attribute("y2") = _path->segmentData()[1].position.y;
            return node;
        }
    }
    else
    {
        String curveString;
        addPathToPathData(_path, curveString, false);
        pugi::xml_node node = _parent.append_child("path");
        node.append_attribute("d") = curveString.cString();
        return node;
    }

    return pugi::xml_node();
}

static pugi::xml_node addPath(const Path * _path,
                              pugi::xml_node _node,
                              bool _bMatchShapes,
                              bool _bIsClippingPath = false)
{
    pugi::xml_node ret;
    if (_path->children().count())
    {
        String pathsString;
        recursivelyAddPathToPathData(_path, pathsString, true);
        ret = _node.append_child("path");
        ret.append_attribute("d") = pathsString.cString();
    }
    else
    {
        if (_bMatchShapes)
        {
            // this does shape matching
            detail::Shape shape(_path);
            detail::ShapeType shapeType = shape.shapeType();
            if (shapeType == detail::ShapeType::Circle)
            {
                ret = _node.append_child("circle");
                ret.append_attribute("cx") = shape.circle().position.x;
                ret.append_attribute("cy") = shape.circle().position.y;
                ret.append_attribute("r") = shape.circle().radius;
            }
            else if (shapeType == detail::ShapeType::Ellipse)
            {
                ret = _node.append_child("ellipse");
                ret.append_attribute("cx") = shape.ellipse().position.x;
                ret.append_attribute("cy") = shape.ellipse().position.y;
                ret.append_attribute("rx") = shape.ellipse().size.x * 0.5f;
                ret.append_attribute("ry") = shape.ellipse().size.y * 0.5f;
            }
            else if (shapeType == detail::ShapeType::Rectangle)
            {
                ret = _node.append_child("rect");
                ret.append_attribute("x") =
                    shape.rectangle().position.x - shape.rectangle().size.x * 0.5f;
                ret.append_attribute("y") =
                    shape.rectangle().position.y - shape.rectangle().size.y * 0.5f;
                ret.append_attribute("width") = shape.rectangle().size.x;
                ret.append_attribute("height") = shape.rectangle().size.y;

                if (shape.rectangle().cornerRadius.x != 0)
                    ret.append_attribute("rx") = shape.rectangle().cornerRadius.x;

                if (shape.rectangle().cornerRadius.y != 0)
                    ret.append_attribute("ry") = shape.rectangle().cornerRadius.y;
            }
            else
            {
                ret = exportCurveData(_path, _node);
            }
        }
        else
        {
            ret = exportCurveData(_path, _node);
        }
    }

    setTransform(_path, ret);
    return ret;
}

static pugi::xml_node addGroup(const Group * _group,
                               pugi::xml_node _parent,
                               ExportSession & _session,
                               bool _bMatchShapes)
{
    if (!_group->children().count())
        return pugi::xml_node();

    pugi::xml_node grp = _parent.append_child("g");
    auto it = _group->children().begin();
    if (_group->isClipped())
    {
        String idString = String::formatted("clip-%lu", _session.clipID);

        auto defsNode = ensureDefsNode(_session);

        pugi::xml_node clip = defsNode.append_child("clipPath");
        clip.append_attribute("id") = idString.cString();
        pugi::xml_node cp = addPath(static_cast<const Path *>(*it), clip, _bMatchShapes, true);
        addStyle(*it, cp, _session, true); // only for winding rule

        grp.append_attribute("clip-path") = String::concat("url(#", idString, ")").cString();
        ++it;
        ++_session.clipID;
    }
    for (; it != _group->children().end(); ++it)
        addItemToXml(*it, grp, _session, _bMatchShapes);

    setTransform(_group, grp);

    return grp;
}

static pugi::xml_node addItemToXml(const Item * _item,
                                   pugi::xml_node _parent,
                                   ExportSession & _session,
                                   bool _bMatchShapes)
{
    pugi::xml_node node;
    switch (_item->itemType())
    {
    case ItemType::Document:
        node = addDocument(static_cast<const Document *>(_item), _parent, _session, _bMatchShapes);
        break;
    case ItemType::Path:
        node = addPath(static_cast<const Path *>(_item), _parent, _bMatchShapes);
        break;
    case ItemType::Group:
        node = addGroup(static_cast<const Group *>(_item), _parent, _session, _bMatchShapes);
        break;
    default:
        break;
    }

    if (node && _item->itemType() != ItemType::Document)
        addStyle(_item, node, _session);
    return node;
}

TextResult exportItem(const Item * _item, Allocator & _alloc, bool _bMatchShapes)
{
    pugi::xml_document xml;
    StringWriter writer(_alloc);
    ExportSession session{ &_alloc, xml, pugi::xml_node(nullptr), UsedGradientArray(_alloc), 0 };
    addItemToXml(_item, xml, session, _bMatchShapes);
    xml.print(writer);
    return writer.result;
}
} // namespace svg
} // namespace paper
