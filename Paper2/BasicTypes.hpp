#ifndef PAPER_BASICTYPES_HPP
#define PAPER_BASICTYPES_HPP

#include <Stick/DynamicArray.hpp>
#include <Stick/String.hpp>
#include <Stick/Error.hpp>
#include <Crunch/Vector2.hpp>
#include <Crunch/Line.hpp>
#include <Crunch/Matrix32.hpp>
#include <Crunch/Matrix4.hpp>
#include <Crunch/Rectangle.hpp>
#include <Crunch/Bezier.hpp>
#include <Crunch/Colors.hpp>

namespace paper
{
    using Error = stick::Error;
    using Size = stick::Size;
    using UInt32 = stick::UInt32;
    using Int32 = stick::Int32;
    using UInt64 = stick::UInt64;
    using Int64 = stick::Int64;
    using String = stick::String;
    using Float = stick::Float32;
    using Vec2f = crunch::Vector2<Float>;
    using Mat32f = crunch::Matrix32<Float>;
    using Mat4f = crunch::Matrix4<Float>;
    using Rect = crunch::Rectangle<Float>;
    using Bezier = crunch::BezierCubic<Vec2f>;
    using Line = crunch::Line<Vec2f>;
    using ColorRGB = crunch::ColorRGB;
    using ColorRGBA = crunch::ColorRGBA;
    using ColorHSB = crunch::ColorHSB;
    using ColorHSBA = crunch::ColorHSBA;
    using DashArray = stick::DynamicArray<Float>;
}

#endif //PAPER_BASICTYPES_HPP
