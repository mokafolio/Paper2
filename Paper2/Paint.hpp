#ifndef PAPER_PAINT_HPP
#define PAPER_PAINT_HPP

#include <Paper2/Constants.hpp>
#include <Paper2/RenderData.hpp>
#include <Stick/SharedPtr.hpp>

namespace paper
{
    struct STICK_API ColorStop
    {
        ColorRGBA color;
        Float offset;
    };

    using ColorStopArray = stick::DynamicArray<ColorStop>;

    class STICK_API BaseGradient
    {
    public:

        BaseGradient(const Vec2f & _from, const Vec2f & _to, GradientType _type);

        BaseGradient(const BaseGradient & _other);

        virtual ~BaseGradient();


        void setOrigin(const Vec2f & _position);

        void setDestination(const Vec2f & _position);

        void setOriginAndDestination(const Vec2f & _orig, const Vec2f & _dest);

        void addStop(const ColorRGBA & _color, Float _offset);


        const Vec2f & origin() const;

        const Vec2f & destination() const;

        const ColorStopArray & stops() const;

        GradientType type() const;


        void markStopsDirty();
        void markPositionsDirty();
        bool cleanDirtyPositions();
        bool cleanDirtyStops();

        RenderData * renderData();
        void setRenderData(RenderDataUniquePtr _ptr);

    protected:

        GradientType m_type;
        Vec2f m_origin;
        Vec2f m_destination;
        ColorStopArray m_stops;
        bool m_bStopsDirty;
        bool m_bPositionsDirty;
        RenderDataUniquePtr m_renderData;
    };

    class STICK_API LinearGradient : public BaseGradient
    {
    public:

        LinearGradient(const Vec2f & _from, const Vec2f & _to);

        LinearGradient(const LinearGradient & _other);
    };

    class STICK_API RadialGradient : public BaseGradient
    {
    public:

        RadialGradient(const Vec2f & _from, const Vec2f & _to);

        RadialGradient(const RadialGradient & _other);

        void setFocalPointOffset(const Vec2f & _focalPoint);

        void setRatio(Float _ratio);

        const stick::Maybe<Vec2f> & focalPointOffset() const;

        const stick::Maybe<Float> & ratio() const;

    private:

        stick::Maybe<Vec2f> m_focalPointOffset;
        stick::Maybe<Float> m_ratio;
    };

    using LinearGradientPtr = stick::SharedPtr<LinearGradient>;
    using RadialGradientPtr = stick::SharedPtr<RadialGradient>;

    struct STICK_API NoPaint {};
    using Paint = stick::Variant<NoPaint, ColorRGBA, LinearGradientPtr, RadialGradientPtr>;

    STICK_API LinearGradientPtr createLinearGradient(const Vec2f & _from, const Vec2f & _to);
    STICK_API RadialGradientPtr createRadialGradient(const Vec2f & _from, const Vec2f & _to);
}

#endif //PAPER_PAINT_HPP
