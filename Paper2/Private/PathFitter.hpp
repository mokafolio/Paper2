#ifndef PAPER_PRIVATE_PATHFITTER_HPP
#define PAPER_PRIVATE_PATHFITTER_HPP

#include <Paper2/Path.hpp>

namespace paper
{
    namespace detail
    {
        // From paper.js source
        // An Algorithm for Automatically Fitting Digitized Curves
        // by Philip J. Schneider
        // from "Graphics Gems", Academic Press, 1990
        // Modifications and optimisations of original algorithm by Juerg Lehni.

        class PathFitter
        {
        public:

            struct MaxError
            {
                stick::Float64 value;
                Size index;
            };

            using PositionArray = stick::DynamicArray<Vec2f>;


            PathFitter(Path * _p, stick::Float64 _error, bool _bIgnoreClosed);

            void fit();

            void fitCubic(Size _first, Size _last, const Vec2f & _tan1, const Vec2f & _tan2);

            void addCurve(const Vec2f & _pointOne, const Vec2f & _handleOne,
                          const Vec2f & _handleTwo, const Vec2f & _pointTwo);

            Bezier generateBezier(Size _first, Size _last,
                                  const stick::DynamicArray<stick::Float64> & _uPrime,
                                  const Vec2f & _tan1, const Vec2f & _tan2);

            // Evaluate a Bezier curve at a particular parameter value
            Vec2f evaluate(Int32 _degree, const Bezier & _curve, stick::Float64 _t);

            // Given set of points and their parameterization, try to find
            // a better parameterization.
            bool reparameterize(Size _first, Size _last,
                                stick::DynamicArray<stick::Float64> & _u, const Bezier & _curve);

            // Use Newton-Raphson iteration to find better root.
            stick::Float64 findRoot(const Bezier & _curve, const Vec2f & _point, stick::Float64 _u);

            // Assign parameter values to digitized points
            // using relative distances between points.
            void chordLengthParameterize(Size _first, Size _last,
                                         stick::DynamicArray<stick::Float64> & _outResult);

            MaxError findMaxError(Size _first, Size _last,
                                  const Bezier & _curve, const stick::DynamicArray<stick::Float64> & _u);

        private:

            Path * m_path;
            Float m_error;
            bool m_bIgnoreClosed;
            SegmentDataArray m_newSegments;
            PositionArray m_positions;
        };
    }
}

#endif //PAPER_PRIVATE_PATHFITTER_HPP
