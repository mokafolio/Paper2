#include <Paper2/Document.hpp>
#include <Paper2/PDF/PDFExport.hpp>
#include <Stick/Map.hpp>
#include <Stick/Variant.hpp>

namespace paper
{
namespace pdf
{
using namespace stick;

enum class PDFValueType
{
    Bool,
    Number,
    String,
    Name,
    Array,
    Dict,
    Stream,
    Reference, // reference to another pdf object
    Null
};

struct PDFNull
{
};

struct PDFReference
{
    Size objIdx;
};

struct PDFStream
{
};
struct PDFValue
{
    using Storage = Variant<bool,
                            Float32,
                            String,
                            DynamicArray<PDFValue>,
                            Map<String, PDFValue>,
                            PDFStream,
                            PDFReference,
                            PDFNull>;

    PDFValue(const Storage & _val = PDFNull()) : storage(_val)
    {
    }

    PDFValueType type() const
    {
        if (storage.is<bool>())
            return PDFValueType::Bool;
        else if (storage.is<Float32>())
            return PDFValueType::Number;
        else if (storage.is<String>())
        {
            const auto & str = storage.get<String>();
            if (str.length() > 0 && str[0] == '/')
                return PDFValueType::Name;
            return PDFValueType::String;
        }
        else if (storage.is<DynamicArray<PDFValue>>())
            return PDFValueType::Array;
        else if (storage.is<Map<String, PDFValue>>())
            return PDFValueType::Dict;
        else if (storage.is<PDFStream>())
            return PDFValueType::Stream;
        else if (storage.is<PDFReference>())
            return PDFValueType::Reference;

        return PDFValueType::Null;
    }

    Storage storage;
};

struct PDFSession
{
    struct PDFObject
    {
        Size byteOff;
        Size byteCount;
    };

    using PDFObjectArray = DynamicArray<PDFObject>;

    PDFSession(Allocator & _alloc) : str(_alloc), tmp(_alloc), objects(_alloc)
    {
    }

    Size addObjectString(const char * _str)
    {
        Size len = std::strlen(_str);
        Size off = str.length();
        tmp.clear();
        tmp.appendFormatted("%lu 0 obj\n    ", objects.count() + 1);
        str.append(tmp.begin(), tmp.end());
        str.append(_str, _str + len);
        str.append("\nendobj\n");

        objects.append({ off, len + tmp.length() + 8 });
        return objects.count() - 1;
    }

    void finalize()
    {
        auto xrefOff = str.length();
        str.appendFormatted("xref\n0 %lu\n0000000000 65535 f\n", objects.count() + 1);
        for(auto & obj : objects)
        {
            str.appendFormatted("%010lu 00000 n\n", obj.byteOff);
        }
        str.appendFormatted("trailer <</Size %lu/Root 1 0 R>>", objects.count());
        str.appendFormatted("\nstartxref\n%lu\n%%%%EOF", xrefOff);
    }

    String str, tmp;
    PDFObjectArray objects;
};

Error exportDocument(const Document * _doc, ByteArray & _outBytes)
{
    // HPDF_Doc pdf;
    // HPDF_Page page;

    // //@TODO: proper haru error handling
    // pdf = HPDF_New(NULL, NULL);
    // if (!pdf)
    //     return Error(
    //         ec::ComposeFailed, "failed to create haru pdf document", STICK_FILE, STICK_LINE);

    // page = HPDF_AddPage(pdf);
    // HPDF_Page_SetWidth(page, _doc->width());
    // HPDF_Page_SetHeight(page, _doc->height());

    // HPDF_Free(pdf);

    // return Error();

    // PDFSession session(defaultAllocator());

    // String pdf;

    // pdf.append("%PDF-1.6\n");
    // pdf.append("1 0 obj <</Type /Catalog /Pages 2 0 R>>\nendobj\n");
    // pdf.append("2 0 obj <</Type /Pages /Kids [3 0 R] /Count 1>>\nendobj\n");
    // pdf.appendFormatted(
    //     "3 0 obj <</MediaBox [0 0 %i %i]>>\nendobj\n", _doc->width(), _doc->height());

    // Size xrefStart = pdf.length();
    // Size objCount = 4;
    // pdf.appendFormatted("xref\n%lu %lu\n", 0, objCount);

    // pdf.append("%%EOF");

    PDFSession session(defaultAllocator());
    session.str.append("%PDF-1.3\n");
    session.addObjectString("<</Type /Catalog /Pages 2 0 R>>");
    session.addObjectString(String::formatted("<</Type /Pages /Kids [3 0 R] /Count 1 /MediaBox [0 0 %i %i]>>", (Int32)_doc->width(), (Int32)_doc->height()).cString());
    session.addObjectString("<</Type /Page /Parent 2 0 R /Resources <<>> >>");
    session.finalize();

    printf("PDF:\n%s\n", session.str.cString());

    return Error();
}

} // namespace pdf
} // namespace paper