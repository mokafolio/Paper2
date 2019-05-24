#include <Paper2/BinFormat/BinFormatImport.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Stick/Serialize.hpp>

namespace paper
{
namespace binfmt
{
using namespace stick;

using Deserializer = DeserializerT<LittleEndianPolicy, MemoryReader>;

struct ImportSession
{
    DynamicArray<SharedPtr<BaseGradient>> importedGradients;
};

Vec2f readVec2f(Deserializer & _ds)
{
    return Vec2f(_ds.readFloat32(), _ds.readFloat32());
}

ColorRGBA readColor(Deserializer & _ds)
{
    return ColorRGBA(_ds.readFloat32(), _ds.readFloat32(), _ds.readFloat32(), _ds.readFloat32());
}

//@TODO: This whole function could be a lot more concise
void importPaint(Item * _item, Deserializer & _ds, ImportSession & _session, bool _bStroke)
{
    if (_ds.readBool())
    {
        UInt8 type = _ds.readUInt8();
        if (type == 0)
        {
            if (_bStroke)
                _item->setStroke(readColor(_ds));
            else
                _item->setFill(readColor(_ds));
        }
        else if (type == 1 || type == 2)
        {
            UInt32 idx = _ds.readUInt32();
            STICK_ASSERT(
                idx < _session.importedGradients.count()); //@TODO: instead of assert, this should
                                                           // most likely be a bad file error
            const auto & grad = _session.importedGradients[idx];
            if (_bStroke)
            {
                if (grad->type() == GradientType::Linear)
                    _item->setStroke(
                        LinearGradientPtr(grad, static_cast<LinearGradient *>(grad.get())));
                else
                    _item->setStroke(
                        RadialGradientPtr(grad, static_cast<RadialGradient *>(grad.get())));
            }
            else
            {
                if (grad->type() == GradientType::Linear)
                    _item->setFill(
                        LinearGradientPtr(grad, static_cast<LinearGradient *>(grad.get())));
                else
                    _item->setFill(
                        RadialGradientPtr(grad, static_cast<RadialGradient *>(grad.get())));
            }
        }
        else
        {
            STICK_ASSERT(false);
        }
    }
}

//@TODO: proper error handling (might need some helpers/changes in Serilize in stick)
static Item * importItem(Document & _doc,
                         Item * _parent,
                         Deserializer & _ds,
                         ImportSession & _session,
                         const SegmentData * _segArray,
                         Size _segmentDataCount)
{
    printf("IMPORT ITEM\n");
    Item * ret = nullptr;

    ItemType type = (ItemType)_ds.readUInt64();

    // item type specific section
    if (type == ItemType::Document)
    {
        Float32 w = _ds.readFloat32();
        Float32 h = _ds.readFloat32();
        _doc.setSize(w, h);
        ret = &_doc;
    }
    else if (type == ItemType::Group)
    {
        Group * grp = _doc.createGroup();
        grp->setClipped(_ds.readBool());
        ret = grp;
    }
    else if (type == ItemType::Path)
    {
        Path * path = _doc.createPath();
        bool isClosed = _ds.readBool();
        Size firstSeg = _ds.readUInt64();
        Size segCount = _ds.readUInt64();

        path->addSegments(_segArray + firstSeg, segCount);

        if (isClosed)
            path->closePath();
        ret = path;
    }
    else if (type == ItemType::Symbol)
    {
        //@TODO
    }
    else
    {
        STICK_ASSERT(false);
    }

    STICK_ASSERT(ret);

    ret->setName(_ds.readCString());
    ret->setVisible(_ds.readBool());

    if (_ds.readBool())
    {
        Mat32f trans;
        for (Size i = 0; i < 3; ++i)
        {
            Vec2f col;
            for (Size j = 0; j < 2; ++j)
                col[j] = _ds.readFloat32();
            trans[i] = col;
        }
        ret->setTransform(trans);
    }

    if (_ds.readBool())
        ret->setPivot(Vec2f(_ds.readFloat32(), _ds.readFloat32()));

    importPaint(ret, _ds, _session, false);
    importPaint(ret, _ds, _session, true);

    // // stick::Maybe<Paint> m_fill;
    // // stick::Maybe<Paint> m_stroke;

    if (_ds.readBool())
        ret->setStrokeWidth(_ds.readFloat32());

    if (_ds.readBool())
        ret->setStrokeJoin((StrokeJoin)_ds.readUInt64());

    if (_ds.readBool())
        ret->setStrokeCap((StrokeCap)_ds.readUInt64());

    if (_ds.readBool())
        ret->setScaleStroke(_ds.readBool());

    if (_ds.readBool())
        ret->setMiterLimit(_ds.readFloat32());

    Size dashCount = _ds.readUInt64();
    if (dashCount)
    {
        DashArray arr(_doc.allocator());
        arr.resize(dashCount);
        for (Size i = 0; i < dashCount; ++i)
            arr[i] = _ds.readFloat32();
        ret->setDashArray(std::move(arr));
    }

    if (_ds.readBool())
        ret->setDashOffset(_ds.readFloat32());

    if (_ds.readBool())
        ret->setWindingRule((WindingRule)_ds.readUInt64());

    Size childCount = _ds.readUInt64();
    printf("CHILD COUNT %lu\n", childCount);

    for (Size i = 0; i < childCount; ++i)
        importItem(_doc, ret, _ds, _session, _segArray, _segmentDataCount);

    if (_parent)
        _parent->addChild(ret);

    return ret;
}

Result<Item *> import(Document & _doc, const UInt8 * _data, Size _byteCount)
{
    Deserializer ds(_data, _byteCount);
    ImportSession session;

    printf("A\n");

    // header
    const char * paper = ds.readCString();
    if (!paper || std::strcmp(paper, "paper") != 0)
        return Error(ec::InvalidOperation, "invalid header", STICK_FILE, STICK_LINE);

    printf("B %s\n", paper);

    UInt32 version = ds.readUInt32();
    UInt64 segmentDataOff = ds.readUInt64();
    UInt64 paintDataOff = ds.readUInt64();

    printf("VERSION %u SEGOFF %lu PDATAOFF %lu BC %lu\n", version, segmentDataOff, paintDataOff, _byteCount);

    Size hierarchyPos = ds.position();
    ds.setPosition(segmentDataOff);

    printf("C\n");

    const char * sr = ds.readCString();
    printf("sr %s\n", sr);
    if (!sr || std::strcmp(sr, "sd") != 0)
        return Error(ec::InvalidOperation, "bad file", STICK_FILE, STICK_LINE);

    Size segmentDataCount = ds.readUInt64();
    // @TODO: on platforms that are little endian and where float is 4 byte aligned, do something
    // along these lines: const SegmentData * segArr = reinterpret_cast<const SegmentData *>(_data +
    // ds.position());
    SegmentDataArray segs(_doc.allocator());
    segs.resize(segmentDataCount);
    for (Size i = 0; i < segmentDataCount; ++i)
    {
        segs[i].handleIn = Vec2f(ds.readFloat32(), ds.readFloat32());
        segs[i].position = Vec2f(ds.readFloat32(), ds.readFloat32());
        segs[i].handleOut = Vec2f(ds.readFloat32(), ds.readFloat32());
    }

    // paint/gradient data
    ds.setPosition(paintDataOff);
    sr = ds.readCString();
    printf("SRRRR %s\n", sr);
    if (!sr || std::strcmp(sr, "pd") != 0)
        return Error(ec::InvalidOperation, "bad file", STICK_FILE, STICK_LINE);

    Size gradCount = ds.readUInt64();
    for (Size i = 0; i < gradCount; ++i)
    {
        SharedPtr<BaseGradient> grad;
        GradientType type = (GradientType)ds.readUInt64();
        Vec2f origin = readVec2f(ds);
        Vec2f dest = readVec2f(ds);
        if (type == GradientType::Linear)
        {
            auto linearGrad = _doc.createLinearGradient(origin, dest);
            grad = linearGrad;
        }
        else if (type == GradientType::Radial)
        {
            auto radialGrad = _doc.createRadialGradient(origin, dest);
            grad = radialGrad;

            if (ds.readBool())
                radialGrad->setFocalPointOffset(readVec2f(ds));
            if (ds.readBool())
                radialGrad->setRatio(ds.readFloat32());
        }
        else
        {
            STICK_ASSERT(false); // error instead?
        }

        Size stopCount = ds.readUInt64();
        for (Size j = 0; j < stopCount; ++j)
            grad->addStop(readColor(ds), ds.readFloat32());

        session.importedGradients.append(grad);
    }

    printf("D\n");

    // hieararchy
    ds.setPosition(hierarchyPos);
    const char * hr = ds.readCString();
    if (!hr || std::strcmp(hr, "hr") != 0)
        return Error(ec::InvalidOperation, "bad file", STICK_FILE, STICK_LINE);

    printf("E\n");
    return importItem(_doc, nullptr, ds, session, &segs[0], segs.count());
}

} // namespace binfmt
} // namespace paper
