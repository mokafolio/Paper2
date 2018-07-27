#include <Paper2/Private/Shape.hpp>
#include <Paper2/Path.hpp>
#include <Crunch/CommonFunc.hpp>

namespace paper
{
    namespace detail
    {
        Shape::Shape() :
            m_type(ShapeType::None)
        {

        }

        Shape::Shape(const Path * _path) :
            m_type(ShapeType::None)
        {
            ConstCurveView curves = _path->curves();
            ConstSegmentView segments = _path->segments();
            if (curves.count() == 4 &&
                    curves[0].isArc() &&
                    curves[1].isArc() &&
                    curves[2].isArc() &&
                    curves[3].isArc())
            {
                if (crunch::isClose(crunch::length(segments[0].position() - segments[2].position()) -
                                    crunch::length(segments[1].position() - segments[3].position()), (Float)0, PaperConstants::epsilon()))
                {
                    m_type = ShapeType::Circle;
                    m_data.circle.position = segments[2].position() + (segments[0].position() - segments[2].position()) * 0.5;
                    m_data.circle.radius = crunch::distance(segments[0].position(), segments[2].position()) * 0.5;
                }
                else
                {
                    m_type = ShapeType::Ellipse;
                    m_data.circle.position = segments[2].position() + (segments[0].position() - segments[2].position()) * 0.5;
                    m_data.ellipse.size = Vec2f(crunch::distance(segments[0].position(), segments[2].position()),
                                                crunch::distance(segments[1].position(), segments[3].position()));
                }
            }
            else if (_path->isPolygon() &&
                     curves.count() == 4 &&
                     curves[0].isCollinear(curves[2]) &&
                     curves[1].isCollinear(curves[3]) &&
                     curves[1].isOrthogonal(curves[0]))
            {
                m_type = ShapeType::Rectangle;
                // m_data.circle.position = _path.localBounds().center();
                Float w = segments[0].position().x - segments[3].position().x;
                Float h = segments[2].position().y - segments[3].position().y;

                m_data.rectangle.position = Vec2f(segments[3].position().x + w * 0.5,
                                                  segments[3].position().y + h * 0.5);

                m_data.rectangle.size = Vec2f(w, h);
                m_data.rectangle.cornerRadius = Vec2f(0);
            }
            else if (curves.count() == 8 &&
                     curves[1].isArc() &&
                     curves[3].isArc() &&
                     curves[5].isArc() &&
                     curves[7].isArc() &&
                     curves[0].isCollinear(curves[4]) &&
                     curves[2].isCollinear(curves[6]))
            {

                //rounded rect
                m_type = ShapeType::Rectangle;

                m_data.rectangle.position = _path->bounds().center();
                m_data.rectangle.size = Vec2f(crunch::distance(segments[7].position(),
                                              segments[2].position()),
                                              crunch::distance(segments[0].position(),
                                                      segments[5].position()));
                m_data.rectangle.cornerRadius = (m_data.rectangle.size - Vec2f(crunch::distance(segments[0].position(),
                                                 segments[1].position()),
                                                 crunch::distance(segments[2].position(),
                                                         segments[3].position()))) * 0.5;

            }

        }

        ShapeType Shape::shapeType() const
        {
            return m_type;
        }

        const Shape::Circle & Shape::circle() const
        {
            return m_data.circle;
        }

        const Shape::Ellipse & Shape::ellipse() const
        {
            return m_data.ellipse;
        }

        const Shape::Rectangle & Shape::rectangle() const
        {
            return m_data.rectangle;
        }
    }
}
