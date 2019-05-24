#include <Paper2/BinFormat/BinFormatExport.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Stick/Serialize.hpp>

namespace paper
{
namespace binfmt
{
using namespace stick;

using Serializer = SerializerT<LittleEndianPolicy, ContainerWriter<DynamicArray<UInt8>>>;

using ExportedGradients = stick::DynamicArray<const BaseGradient *>;

struct ExportSession
{
    ExportedGradients exportedGradients;
};

//@TODO: We could squeze all of the enum types into UInt8 or UInt16 I am sure...

template <class S>
void serialize(S & _s, const Mat32f & _mat)
{
    // on platforms that are 4 byte aligned for float and little endian, we can most likely just
    // copy the whole thing in one go (optimization)
    for (Size i = 0; i < 3; ++i)
    {
        auto & col = _mat[i];
        for (Size j = 0; j < 2; ++j)
            _s.write(col[j]);
    }
}

template <class S>
void serialize(S & _s, const Vec2f & _vec)
{
    // on platforms that are 4 byte aligned for float, we can most likely just copy the whole thing
    // in one go (optimization)
    _s.write(_vec.x);
    _s.write(_vec.y);
}

template <class S>
void serialize(S & _s, const ColorRGBA & _col)
{
    // on platforms that are 4 byte aligned for float, we can most likely just copy the whole thing
    // in one go (optimization)
    _s.write(_col.r);
    _s.write(_col.g);
    _s.write(_col.b);
    _s.write(_col.a);
}

template <class S>
void exportPaint(S & _serializer, const Paint & _paint, ExportSession & _session)
{
    _serializer.write(true);
    if (_paint.is<ColorRGBA>())
    {
        _serializer.write((UInt8)0);
        serialize(_serializer, _paint.get<ColorRGBA>());
    }
    else if (_paint.is<LinearGradientPtr>() || _paint.is<RadialGradientPtr>())
    {
        const BaseGradient * bgrad =
            _paint.is<LinearGradientPtr>()
                ? (const BaseGradient *)_paint.get<LinearGradientPtr>().get()
                : (const BaseGradient *)_paint.get<RadialGradientPtr>().get();

        _serializer.write(_paint.is<LinearGradientPtr>() ? (UInt8)1 : (UInt8)2);

        // check if the gradient was allready serialized (i.e. if its shared between multiple items)
        auto it = stick::find(
            _session.exportedGradients.begin(), _session.exportedGradients.end(), bgrad);
        if (it == _session.exportedGradients.end())
        {
            printf("FIRST FOUND GRAD\n");
            _session.exportedGradients.append(bgrad);
            _serializer.write((UInt32)_session.exportedGradients.count() - 1);
        }
        else
        {
            printf("FOUND GRAD\n");
            auto dist = std::distance(_session.exportedGradients.begin(), it);
            printf("DIST %lu\n", dist);
            _serializer.write((UInt32)dist);
        }

        // _serializer.write(_paint.is<LinearGradientPtr>() ? (UInt8)1 : (UInt8)2);
        // _serializer.write((UInt64)bgrad->type());
        // serialize(bgrad->origin());
        // serialize(bgrad->destination());
    }
}

template <class S>
void recursivelyExportItem(const Item * _item,
                           S & _serializer,
                           SegmentDataArray & _segs,
                           ExportSession & _session)
{
    _serializer.write((UInt64)_item->itemType());

    // item type specific section
    if (_item->itemType() == ItemType::Document)
    {
        const Document * doc = static_cast<const Document *>(_item);
        _serializer.write(doc->width());
        _serializer.write(doc->height());
    }
    else if (_item->itemType() == ItemType::Group)
    {
        const Group * grp = static_cast<const Group *>(_item);
        _serializer.write(grp->isClipped());
    }
    else if (_item->itemType() == ItemType::Path)
    {
        const Path * path = static_cast<const Path *>(_item);
        _serializer.write(path->isClosed());
        _serializer.write(_segs.count());        // start segment
        _serializer.write(path->segmentCount()); // segment count
        _segs.append(path->segmentData().begin(), path->segmentData().end());
    }
    else if (_item->itemType() == ItemType::Symbol)
    {
        //@TODO
    }
    else
    {
        STICK_ASSERT(false);
    }

    // item attributes
    _serializer.write(_item->name());
    _serializer.write(_item->isVisible());
    _serializer.write(_item->hasTransform());

    //@TODO: we could shorten the following stuff with a macro or template helper.
    // Macro will most likely be nicer in this case.
    if (_item->hasTransform())
        serialize(_serializer, _item->transform());
    _serializer.write(_item->hasPivot());
    if (_item->hasPivot())
        serialize(_serializer, _item->pivot());

    if (_item->style()->hasFill())
        exportPaint(_serializer, _item->fill(), _session);
    else
        _serializer.write(false);

    if (_item->style()->hasStroke())
        exportPaint(_serializer, _item->stroke(), _session);
    else
        _serializer.write(false);

    printf("B\n");
    _serializer.write(_item->style()->hasStrokeWidth());
    if (_item->style()->hasStrokeWidth())
        _serializer.write(_item->strokeWidth());
    _serializer.write(_item->style()->hasStrokeJoin());
    if (_item->style()->hasStrokeJoin())
        _serializer.write((UInt64)_item->strokeJoin());
    _serializer.write(_item->style()->hasStrokeCap());
    if (_item->style()->hasStrokeCap())
        _serializer.write((UInt64)_item->strokeCap());
    _serializer.write(_item->style()->hasScaleStroke());
    if (_item->style()->hasScaleStroke())
        _serializer.write(_item->scaleStroke());
    _serializer.write(_item->style()->hasMiterLimit());
    if (_item->style()->hasMiterLimit())
        _serializer.write(_item->miterLimit());

    printf("C\n");
    const auto & da = _item->dashArray();
    Size daCount = da.count();
    _serializer.write(daCount);
    for (Size i = 0; i < daCount; ++i)
        _serializer.write(da[i]);

    printf("D\n");
    _serializer.write(_item->style()->hasDashOffset());
    if (_item->style()->hasDashOffset())
        _serializer.write(_item->dashOffset());
    _serializer.write(_item->style()->hasWindingRule());
    if (_item->style()->hasWindingRule())
        _serializer.write((UInt64)_item->windingRule());

    printf("E\n");
    // children
    _serializer.write(_item->children().count());
    for (const Item * child : _item->children())
        recursivelyExportItem(child, _serializer, _segs, _session);
}

Error exportItem(const Item * _item, DynamicArray<UInt8> & _outBytes)
{
    static_assert(alignof(Float) == 4,
                  "binary import/export is only supported on platforms where float is 4-byte "
                  "aligned for now.");

    MemorySerializerLE tmp(_item->document()->allocator());

    tmp.reserve(1024); // random guess for now
    SegmentDataArray segmentData(_item->document()->allocator());

    // 01. hierarchy
    tmp.write("hr");
    ExportSession session;
    recursivelyExportItem(_item, tmp, segmentData, session);

    // 02. segment data
    printf("OFF %lu\n", tmp.storage().data.count());
    Size segmentDataOff = tmp.position();
    tmp.write("sd");
    tmp.write(segmentData.count());

    //@TODO: if host platform is little endian and float 4-byte aligned, copy all segments at
    // once i.e. tmp.write((const UInt8 *)&segmentData[0], sizeof(SegmentData) *
    // segmentData.count());
    for (const SegmentData & seg : segmentData)
    {
        serialize(tmp, seg.handleIn);
        serialize(tmp, seg.position);
        serialize(tmp, seg.handleOut);
    }

    // 03. gradient/paint data
    Size paintDataOff = tmp.position();
    tmp.write("pd");
    tmp.write(session.exportedGradients.count());

    printf("EXPORTING GRADS %lu %lu\n", paintDataOff, session.exportedGradients.count());
    for (const BaseGradient * grad : session.exportedGradients)
    {
        tmp.write((UInt64)grad->type());
        serialize(tmp, grad->origin());
        serialize(tmp, grad->destination());

        if(grad->type() == GradientType::Radial)
        {
            const RadialGradient * rg = static_cast<const RadialGradient*>(grad);
            const auto & fpo = rg->focalPointOffset();
            tmp.write((bool)fpo);
            if(fpo)
                serialize(tmp, *fpo);
            const auto & ratio = rg->ratio();
            tmp.write((bool)ratio);
            if(ratio)
                tmp.write(*ratio);
        }

        tmp.write((UInt64)grad->stops().count());
        for(const ColorStop & stop : grad->stops())
        {
            serialize(tmp, stop.color);
            tmp.write(stop.offset);
        }
    }

    Serializer os(_outBytes);
    os.reserve(tmp.position() + 24);

    //@TODO Write Paper and file format version as soon as we have those
    // header
    os.write("paper");
    os.write((UInt32)0); // future file format version

    Size headerSize = os.positionAfterWritingBytes(sizeof(Size) * 2);
    os.write(segmentDataOff + headerSize);
    os.write(paintDataOff + headerSize);
    os.write(tmp.storage().dataPtr(), tmp.storage().byteCount());

    return Error();
}

} // namespace binfmt
} // namespace paper