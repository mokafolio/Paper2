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

template <class T>
struct SharedResourceExport
{
    using ResourceArray = stick::DynamicArray<const T *>;

    template <class S>
    void write(const T * _res, S & _serializer)
    {
        auto it = stick::find(m_exported.begin(), m_exported.end(), _res);
        if (it == m_exported.end())
        {
            m_exported.append(_res);
            _serializer.write((UInt32)m_exported.count() - 1);
            printf("FIRST FOUND ITEM %lu\n", m_exported.count() - 1);
        }
        else
        {
            printf("FOUND ITEM\n");
            auto dist = std::distance(m_exported.begin(), it);
            printf("DIST %lu\n", dist);
            _serializer.write((UInt32)dist);
        }
    }

    ResourceArray m_exported;
};

using Serializer = SerializerT<LittleEndianPolicy, ContainerWriter<DynamicArray<UInt8>>>;
using ExportedGradients = stick::DynamicArray<const BaseGradient *>;

struct ExportSession
{
    // ExportedGradients exportedGradients;
    SharedResourceExport<BaseGradient> gradientExport;
    SharedResourceExport<Style> styleExport;
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
    if (_paint.is<NoPaint>())
        _serializer.write(false);
    else
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
            _session.gradientExport.write(bgrad, _serializer);
        }
    }
}

// template <class S>
// void exportStyle(S & _serializer, const StylePtr & _style, ExportSession & _session)
// {

// }

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

    // const Style & s = _item->style();

    //@TODO: we could shorten the following stuff with a macro or template helper.
    // Macro will most likely be nicer in this case.
    if (_item->hasTransform())
        serialize(_serializer, _item->transform());
    _serializer.write(_item->hasPivot());
    if (_item->hasPivot())
        serialize(_serializer, _item->pivot());

    _session.styleExport.write(_item->stylePtr().get(), _serializer);

    // if (_item->style()->hasFill())
    //     exportPaint(_serializer, _item->fill(), _session);
    // else
    //     _serializer.write(false);

    // if (_item->style()->hasStroke())
    //     exportPaint(_serializer, _item->stroke(), _session);
    // else
    //     _serializer.write(false);

    // exportPaint(_serializer, s.fill(), _session);
    // exportPaint(_serializer, s.stroke(), _session);

    // printf("B\n");

    // _serializer.write(_item->style()->hasStrokeWidth());
    // if (_item->style()->hasStrokeWidth())
    //     _serializer.write(_item->strokeWidth());
    // _serializer.write(_item->style()->hasStrokeJoin());
    // if (_item->style()->hasStrokeJoin())
    //     _serializer.write((UInt64)_item->strokeJoin());
    // _serializer.write(_item->style()->hasStrokeCap());
    // if (_item->style()->hasStrokeCap())
    //     _serializer.write((UInt64)_item->strokeCap());
    // _serializer.write(_item->style()->hasScaleStroke());
    // if (_item->style()->hasScaleStroke())
    //     _serializer.write(_item->scaleStroke());
    // _serializer.write(_item->style()->hasMiterLimit());
    // if (_item->style()->hasMiterLimit())
    //     _serializer.write(_item->miterLimit());

    // _serializer.write(true);
    // _serializer.write(s.strokeWidth());
    // _serializer.write(true);
    // _serializer.write((UInt64)s.strokeJoin());
    // _serializer.write(true);
    // _serializer.write((UInt64)s.strokeCap());
    // _serializer.write(true);
    // _serializer.write(s.scaleStroke());
    // _serializer.write(true);
    // _serializer.write(s.miterLimit());

    // printf("C\n");
    // const auto & da = s.dashArray();
    // Size daCount = da.count();
    // _serializer.write(daCount);
    // for (Size i = 0; i < daCount; ++i)
    //     _serializer.write(da[i]);

    // printf("D\n");
    // // _serializer.write(_item->style()->hasDashOffset());
    // // if (_item->style()->hasDashOffset())
    // //     _serializer.write(_item->dashOffset());
    // // _serializer.write(_item->style()->hasWindingRule());
    // // if (_item->style()->hasWindingRule())
    // //     _serializer.write((UInt64)_item->windingRule());
    // _serializer.write(true);
    // _serializer.write(s.dashOffset());
    // _serializer.write(true);
    // _serializer.write((UInt64)s.windingRule());

    // printf("E\n");
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
    tmp.write("hierarchy");
    ExportSession session;
    recursivelyExportItem(_item, tmp, segmentData, session);

    // 02. segment data
    printf("seg OFF %lu\n", tmp.storage().data.count());
    Size segmentDataOff = tmp.position();
    tmp.write("segmentData");
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

    // 03. Style data
    Size styleDataOff = tmp.position();
    tmp.write("styleData");
    tmp.write(session.styleExport.m_exported.count());

    printf("EXPORTING STYLES %lu\n", session.styleExport.m_exported.count());
    for (const Style * s : session.styleExport.m_exported)
    {
        exportPaint(tmp, s->fill(), session);
        exportPaint(tmp, s->stroke(), session);

        tmp.write(s->strokeWidth());
        tmp.write((UInt64)s->strokeJoin());
        tmp.write((UInt64)s->strokeCap());
        tmp.write(s->scaleStroke());
        tmp.write(s->miterLimit());

        const auto & da = s->dashArray();
        Size daCount = da.count();
        tmp.write(daCount);
        for (Size i = 0; i < daCount; ++i)
            tmp.write(da[i]);

        tmp.write(s->dashOffset());
        tmp.write((UInt64)s->windingRule());
    }

    // 04. gradient/paint data
    Size paintDataOff = tmp.position();
    tmp.write("paintData");
    tmp.write(session.gradientExport.m_exported.count());

    printf("EXPORTING GRADS %lu %lu\n", paintDataOff, session.gradientExport.m_exported.count());
    for (const BaseGradient * grad : session.gradientExport.m_exported)
    {
        tmp.write((UInt64)grad->type());
        serialize(tmp, grad->origin());
        serialize(tmp, grad->destination());

        if (grad->type() == GradientType::Radial)
        {
            const RadialGradient * rg = static_cast<const RadialGradient *>(grad);
            const auto & fpo = rg->focalPointOffset();
            tmp.write((bool)fpo);
            if (fpo)
                serialize(tmp, *fpo);
            const auto & ratio = rg->ratio();
            tmp.write((bool)ratio);
            if (ratio)
                tmp.write(*ratio);
        }

        tmp.write((UInt64)grad->stops().count());
        for (const ColorStop & stop : grad->stops())
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

    Size headerSize = os.positionAfterWritingBytes(sizeof(Size) * 3);

    printf("FINALS %lu %lu %lu\n", segmentDataOff + headerSize, styleDataOff + headerSize, paintDataOff + headerSize);
    os.write(segmentDataOff + headerSize);
    os.write(styleDataOff + headerSize);
    os.write(paintDataOff + headerSize);
    os.write(tmp.storage().dataPtr(), tmp.storage().byteCount());

    return Error();
}

} // namespace binfmt
} // namespace paper
