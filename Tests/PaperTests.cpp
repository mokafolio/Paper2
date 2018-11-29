//#include <Paper/Components.hpp>
#include <Paper2/Document.hpp>
#include <Paper2/Group.hpp>
#include <Paper2/Path.hpp>
#include <Crunch/StringConversion.hpp>
#include <Stick/Test.hpp>
// #include <Paper/Private/ContainerView.hpp>

using namespace stick;
using namespace paper;
using namespace crunch;

// clang format fails to usefully format the test macro stuff
// so we turn it off for now :/
// clang-format off
const Suite spec[] =
{
    SUITE("DOM Tests")
    {
        Document doc;

        Group * grp = doc.createGroup("Group");
        EXPECT(doc.children().count() == 1);
        EXPECT(grp->name() == "Group");
        EXPECT(grp->parent() == &doc);
        EXPECT(grp->document() == &doc);

        Group * grp2 = doc.createGroup();
        EXPECT(grp2->parent() == &doc);
        EXPECT(doc.children().count() == 2);
        grp->addChild(grp2);
        EXPECT(grp2->parent() == grp);
        EXPECT(doc.children().count() == 1);

        Group * grp3 = doc.createGroup("Group3");
        Group * grp4 = doc.createGroup("Group4");
        grp->addChild(grp4);
        grp3->insertBelow(grp2);
        EXPECT(grp->children().count() == 3);
        EXPECT(grp->children()[0] == grp3);
        EXPECT(grp->children()[1] == grp2);
        EXPECT(grp->children()[2] == grp4);
        grp3->insertAbove(grp4);
        EXPECT(grp->children().count() == 3);
        EXPECT(grp->children()[0] == grp2);
        EXPECT(grp->children()[1] == grp4);
        EXPECT(grp->children()[2] == grp3);
        EXPECT(grp3->parent() == grp);

        grp2->insertAbove(grp4);
        EXPECT(grp->children()[0] == grp4);
        EXPECT(grp->children()[1] == grp2);
        EXPECT(grp->children()[2] == grp3);
        grp2->sendToFront();
        EXPECT(grp->children()[0] == grp4);
        EXPECT(grp->children()[1] == grp3);
        EXPECT(grp->children()[2] == grp2);
        grp2->sendToBack();
        EXPECT(grp->children()[0] == grp2);
        EXPECT(grp->children()[1] == grp4);
        EXPECT(grp->children()[2] == grp3);

        grp2->insertAbove(grp);
        EXPECT(grp->children().count() == 2);
        EXPECT(doc.children().count() == 2);
        EXPECT(grp2->parent() == &doc);
        grp2->remove();
        EXPECT(doc.children().count() == 1);
    },
    SUITE("Basic Path Tests")
    {
        Document doc;

        Path * p = doc.createPath();
        p->addPoint(Vec2f(100.0f, 30.0f));
        p->addPoint(Vec2f(200.0f, 30.0f));
        EXPECT(p->segmentData().count() == 2);
        EXPECT(p->segmentData()[0].position == Vec2f(100.0f, 30.0f));
        EXPECT(p->segmentData()[1].position == Vec2f(200.0f, 30.0f));
        EXPECT(p->isPolygon());

        p->addSegment(Vec2f(150.0f, 150.0f), Vec2f(-5.0f, -3.0f), Vec2f(5.0f, 3.0f));
        EXPECT(p->segmentData().count() == 3);
        EXPECT(p->segmentData()[2].position == Vec2f(150.0f, 150.0f));
        EXPECT(p->segmentData()[2].handleIn == Vec2f(145.0f, 147.0f));
        EXPECT(p->segmentData()[2].handleOut == Vec2f(155.0f, 153.0f));

        printf("CC %lu\n", p->curveData().count());
        EXPECT(p->curveData().count() == 2);
        EXPECT(!p->isPolygon());
        EXPECT(!p->isClosed());

        stick::DynamicArray<Vec2f> expectedCurves =
        {
            Vec2f(100.0f, 30.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(200.0f, 30.0f),
            Vec2f(200.0f, 30.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(-5.0f, -3.0f),
            Vec2f(150.0f, 150.0f)
        };
        Size i = 0;
        for (auto c : p->curves())
        {
            EXPECT(c.positionOne() == expectedCurves[i++]);
            EXPECT(c.handleOne() == expectedCurves[i++]);
            EXPECT(c.handleTwo() == expectedCurves[i++]);
            EXPECT(c.positionTwo() == expectedCurves[i++]);
        }

        p->closePath();
        EXPECT(p->isClosed());
        EXPECT(p->curves().count() == 3);
        EXPECT(p->curves().last().positionOne() == Vec2f(150.0f, 150.0f));
        EXPECT(p->curves().last().handleOne() == Vec2f(5.0f, 3.0f));
        EXPECT(p->curves().last().positionTwo() == Vec2f(100.0f, 30.0f));

        // test insertion
        p->insertSegment(1, SegmentData{Vec2f(100, 75.0), Vec2f(100, 75.0), Vec2f(100, 75.0)});
        EXPECT(p->segmentData().count() == 4);
        EXPECT(p->segmentData()[0].position == Vec2f(100.0f, 30.0f));
        EXPECT(p->segmentData()[1].position == Vec2f(100.0f, 75.0f));
        EXPECT(p->segmentData()[2].position == Vec2f(200.0f, 30.0f));

        for(auto seg : p->segmentData())
            printf("SEG %f %f\n", seg.position.x, seg.position.y);

        EXPECT(p->curves().count() == 4);
        stick::DynamicArray<Vec2f> expectedCurves2 =
        {
            Vec2f(100.0f, 30.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(100.0f, 75.0f),
            Vec2f(100.0f, 75.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(200.0f, 30.0f),
            Vec2f(200.0f, 30.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(-5.0f, -3.0f),
            Vec2f(150.0f, 150.0f),
            Vec2f(150.0f, 150.0f),
            Vec2f(5.0f, 3.0f),
            Vec2f(0.0f, 0.0f),
            Vec2f(100.0f, 30.0f)
        };
        i = 0;
        for (auto c : p->curves())
        {
            printf("I %i\n", i);
            EXPECT(c.positionOne() == expectedCurves2[i++]);
            EXPECT(c.handleOne() == expectedCurves2[i++]);
            EXPECT(c.handleTwo() == expectedCurves2[i++]);
            EXPECT(c.positionTwo() == expectedCurves2[i++]);
        }
    },
    SUITE("Attribute Tests")
    {
        Document doc;
        Path * child = doc.createPath();
        EXPECT(!child->hasFill());
        EXPECT(!child->hasStroke());
        child->setFill(ColorRGBA(1.0f, 0.5f, 0.3f, 1.0f));
        child->setStroke(ColorRGBA(1.0f, 0.0f, 0.75f, 1.0f));
        EXPECT(child->fill().get<ColorRGBA>() == ColorRGBA(1.0f, 0.5f, 0.3f, 1.0f));
        EXPECT(child->stroke().get<ColorRGBA>() == ColorRGBA(1.0f, 0.0f, 0.75f, 1.0f));
        EXPECT(child->hasFill());
        EXPECT(child->hasStroke());
        Group * grp = doc.createGroup();
        grp->addChild(child);
        grp->setFill(ColorRGBA(0.34f, 0.25f, 1.0f, 0.5f));
        EXPECT(grp->fill().is<ColorRGBA>());
        EXPECT(grp->fill().get<ColorRGBA>() == ColorRGBA(0.34f, 0.25f, 1.0f, 0.5f));
        EXPECT(child->fill().get<ColorRGBA>() == ColorRGBA(0.34f, 0.25f, 1.0f, 0.5f));
        child->removeFill();
        EXPECT(child->hasFill() && child->fill().is<NoPaint>());
        grp->removeFill();
        EXPECT(!child->hasFill());
        EXPECT(grp->hasFill() && grp->fill().is<NoPaint>());
    },
    SUITE("Path Length Tests")
    {
        Document doc;
        Path * p = doc.createPath();
        p->addPoint(Vec2f(0.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 200.0f));
        EXPECT(isClose(p->length(), 400.0f));

        Float rad = 100;
        Path * p2 = doc.createCircle(Vec2f(0.0f, 0.0f), rad);
        Float circumference = Constants<Float>::pi() * rad * 2;
        printf("%f %f\n", p2->length(), circumference);
        EXPECT(isClose(p2->length(), circumference, 0.1f));
    },
    SUITE("Path Orientation Tests")
    {
        Document doc;
        Path * p = doc.createPath();
        p->addPoint(Vec2f(0.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 200.0f));
        p->addPoint(Vec2f(0.0f, 200.0f));
        EXPECT(p->isClockwise());
        p->reverse();
        EXPECT(!p->isClockwise());
    },
    SUITE("Path Bounds Tests")
    {
        Document doc;
        Path * p = doc.createPath();
        p->addPoint(Vec2f(0.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 0.0f));
        p->addPoint(Vec2f(200.0f, 100.0f));

        const Rect & bounds = p->bounds();
        EXPECT(isClose(bounds.min(), Vec2f(0.0f)));
        EXPECT(isClose(bounds.width(), 200.0f));
        EXPECT(isClose(bounds.height(), 100.0f));

        p->addPoint(Vec2f(200.0f, 200.0f));
        const Rect & bounds2 = p->bounds();
        EXPECT(isClose(bounds2.min(), Vec2f(0.0f)));
        EXPECT(isClose(bounds2.width(), 200.0f));
        EXPECT(isClose(bounds2.height(), 200.0f));

        p->closePath();
        p->setStroke(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
        p->setStrokeWidth(20.0);
        p->setStrokeJoin(StrokeJoin::Round);
        EXPECT(p->position() == Vec2f(100.0, 100.0));
        EXPECT(p->pivot() == Vec2f(100.0, 100.0));

        const Rect & strokeBounds = p->strokeBounds();
        EXPECT(isClose(strokeBounds.min(), Vec2f(-10.0f)));
        EXPECT(isClose(strokeBounds.width(), 220.0f));
        EXPECT(isClose(strokeBounds.height(), 220.0f));
    },
    SUITE("Transformed Path Bounds Tests")
    {
        Document doc;
        Path * p = doc.createPath();
        p->addPoint(Vec2f(0.0f, 0.0f));
        p->addPoint(Vec2f(100.0f, 0.0f));
        p->addPoint(Vec2f(100.0f, 100.0f));
        p->addPoint(Vec2f(0.0f, 100.0f));
        p->closePath();
        EXPECT(isClose(p->position(), Vec2f(50.0f, 50.0f)));

        p->translateTransform(Vec2f(100, 150));
        const Rect & bounds = p->bounds();
        EXPECT(isClose(bounds.min(), Vec2f(100.0f, 150.0f)));
        EXPECT(isClose(bounds.width(), 100.0f));
        EXPECT(isClose(bounds.height(), 100.0f));

        float diagonal = sqrt(100.0f * 100.0f + 100.0f * 100.0f);
        p->rotateTransform(Constants<Float>::pi() * 0.25);
        const Rect & bounds2 = p->bounds();
        printf("POS %f %f\n", p->position().x, p->position().y);
        printf("MIN %f %f\n", bounds2.min().x, bounds2.min().y);
        EXPECT(isClose(p->position(), Vec2f(150.0f, 200.0f)));
        EXPECT(isClose(bounds2.min(), Vec2f(150.0f, 200.0f) - Vec2f(diagonal * 0.5)));
        EXPECT(isClose(bounds2.width(), diagonal));
        EXPECT(isClose(bounds2.height(), diagonal));

        p->scaleTransform(2.0f);
        const Rect & bounds4 = p->bounds();
        EXPECT(isClose(bounds4.width(), diagonal * 2));
        EXPECT(isClose(bounds4.height(), diagonal * 2));

        p->setStroke(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
        p->setStrokeWidth(20.0);
        p->setStrokeJoin(StrokeJoin::Round);
        p->setStrokeCap(StrokeCap::Round);
        const Rect & bounds5 = p->strokeBounds();
        printf("w %f h %f d %f\n", bounds5.width(), bounds5.height(), diagonal);
        EXPECT(isClose(bounds5.width(), diagonal * 2 + 40.0f, 0.00001f));
        EXPECT(isClose(bounds5.height(), diagonal * 2 + 40.0f, 0.00001f));
    },
    SUITE("Clone Tests")
    {
        Document doc;
        Group * grp = doc.createGroup("grp");
        Path * p = doc.createPath("yessaa");
        p->addPoint(Vec2f(100.0f, 30.0f));
        p->addPoint(Vec2f(200.0f, 30.0f));
        p->setStroke(ColorRGBA(1.0f, 0.5f, 0.75f, 0.75f));
        p->setStrokeCap(StrokeCap::Square);
        grp->addChild(p);
        grp->setFill(ColorRGBA(0.25f, 0.33f, 0.44f, 1.0f));
        grp->setStrokeWidth(10.0);
        grp->setStrokeJoin(StrokeJoin::Round);

        // printf("WOOOP\n");
        Path * p2 = p->clone();
        EXPECT(p2->name() == "yessaa");
        EXPECT(p2->stroke().get<ColorRGBA>() == ColorRGBA(1.0f, 0.5f, 0.75f, 0.75f));
        EXPECT(p2->parent() == grp);
        EXPECT(p2->segmentData().count() == 2);
        EXPECT(p2->curveData().count() == 1);
        EXPECT(p2->segmentData()[0].position == Vec2f(100.0f, 30.0f));
        EXPECT(p2->segmentData()[1].position == Vec2f(200.0f, 30.0f));
        p2->setName("p2");

        EXPECT(p->strokeBounds() == p2->strokeBounds());
        EXPECT(p->bounds() == p2->bounds());

        Group * grp2 = grp->clone();
        EXPECT(grp2->name() == "grp");
        EXPECT(grp2->children().count() == 2);
        EXPECT(grp2->parent() == &doc);
        EXPECT(grp2->children()[0]->name() == "yessaa");
        EXPECT(grp2->children()[1]->name() == "p2");

        EXPECT(doc.children().count() == 2);
        EXPECT(doc.children()[0] == grp);
        EXPECT(doc.children()[1] == grp2);

        EXPECT(grp2->strokeBounds() == grp->strokeBounds());
        EXPECT(grp2->bounds() == grp->bounds());
    }
// SUITE("SVG Export Tests")
// {
//     //TODO: Turn this into an actual test
//     Document doc = createDocument();
//     doc.translateTransform(Vec2f(100, 200));
//     doc.scaleTransform(3.0);
//     Path c = doc.createCircle(Vec2f(100, 100), 10);
//     c.setFill(ColorRGBA(1.0, 1.0, 0.0, 1.0));
//     auto res = doc.exportSVG();
//     printf("%s\n", res.ensure().cString());
// },
// SUITE("SVG Import Tests")
// {
//     {
//         Document doc = createDocument();
//         String svg = "<svg width='100px' height='50px'><path d='M10 20 L100 20 100 120
//         Z'/></svg>"; printf("SVG:\n%s\n", svg.cString()); auto svgdata = doc.parseSVG(svg);
//         EXPECT(svgdata.width() == 100);
//         EXPECT(svgdata.height() == 50);
//         EXPECT(svgdata.group().isValid());
//         EXPECT(svgdata.group().children().count() == 1);
//         EXPECT(Item(svgdata.group().children()[0]).itemType() == EntityType::Path);
//         Path p = reinterpretEntity<Path>(svgdata.group().children()[0]);
//         EXPECT(p.segmentArray().count() == 3);
//         EXPECT(isClose(p.segmentArray()[0]->position(), Vec2f(10, 20)));
//         EXPECT(isClose(p.segmentArray()[1]->position(), Vec2f(100, 20)));
//         EXPECT(isClose(p.segmentArray()[2]->position(), Vec2f(100, 120)));
//         EXPECT(p.isClosed());
//     }
//     {
//         //TODO: test transforms on the group element
//         Document doc = createDocument();
//         String svg = "<svg><g transform='translate(10, 10) rotate(30)'><path d='M10 20 L100
//         20'/><path d='M-30 30.0e4 L100 20'/></g></svg>"; printf("SVG:\n%s\n", svg.cString());
//         auto svgdata = doc.parseSVG(svg);
//         EXPECT(!svgdata.error());
//         EXPECT(svgdata.group().children().count() == 1);
//         EXPECT(Item(svgdata.group().children()[0]).itemType() == EntityType::Group);
//         Group grp = reinterpretEntity<Group>(svgdata.group().children()[0]);
//         EXPECT(grp.children().count() == 2);
//     }
//     {
//         //test basic colors and attribute import
//         Document doc = createDocument();
//         String svg = "<svg><path d='M10 20 L100 20' fill='red' style='stroke: #333; stroke-width:
//         2px'/><circle cx='100' cy='200' r='20' fill='#4286f4' fill-rule='nonzero' stroke='black'
//         stroke-miterlimit='33.5' stroke-dasharray='1, 2,3 4 5' stroke-dashoffset='20.33'
//         vector-effect='non-scaling-stroke' stroke-linejoin='miter'
//         stroke-linecap='round'/></svg>"; printf("SVG:\n%s\n", svg.cString()); auto svgdata =
//         doc.parseSVG(svg); Path p = reinterpretEntity<Path>(svgdata.group().children()[0]);
//         EXPECT(p.fill().get<ColorRGBA>() == ColorRGBA(1, 0, 0, 1));
//         auto s = p.stroke().get<ColorRGBA>();
//         EXPECT(isClose(s.r, 51.0f / 255.0f) && isClose(s.g, 51.0f / 255.0f) && isClose(s.b, 51.0f
//         / 255.0f)); EXPECT(p.strokeWidth() == 2.0f); Path p2 =
//         reinterpretEntity<Path>(svgdata.group().children()[1]); auto & c2 =
//         p2.fill().get<ColorRGBA>(); EXPECT(isClose(c2.r, 66.0f / 255.0f) && isClose(c2.g, 134.0f
//         / 255.0f) && isClose(c2.b, 244.0f / 255.0f)); EXPECT(p2.stroke().get<ColorRGBA>() ==
//         ColorRGBA(0, 0, 0, 1)); EXPECT(p2.isScalingStroke() == false);
//         EXPECT(isClose(p2.miterLimit(), 33.5f));
//         EXPECT(isClose(p2.dashOffset(), 20.33f));
//         EXPECT(p2.windingRule() == WindingRule::NonZero);
//         printf("DA COUNT %lu\n", p2.dashArray().count());
//         EXPECT(p2.dashArray().count() == 5);
//         EXPECT(p2.dashArray()[0] == 1);
//         EXPECT(p2.dashArray()[1] == 2);
//         EXPECT(p2.dashArray()[2] == 3);
//         EXPECT(p2.dashArray()[3] == 4);
//         EXPECT(p2.dashArray()[4] == 5);
//         EXPECT(p2.strokeCap() == StrokeCap::Round);
//         EXPECT(p2.strokeJoin() == StrokeJoin::Miter);
//     }
// },
// SUITE("Basic Intersection Tests")
// {
//     Document doc = createDocument();
//     Path circle = doc.createCircle(Vec2f(100, 100), 100);
//     auto isecs = circle.intersections();
//     EXPECT(isecs.count() == 0);
//     // printf("DA COUNT %lu\n", isecs.count());
//     // for (auto & isec : isecs)
//     //     printf("ISEC %f %f\n", isec.position.x, isec.position.y);

//     Path line = doc.createPath();
//     line.addPoint(Vec2f(-100, 100));
//     line.addPoint(Vec2f(300, 100));
//     auto isecs2 = line.intersections(circle);
//     printf("DA COUNT %lu\n", isecs2.count());
//     EXPECT(isecs2.count() == 2);

//     Path selfIntersectingLinear = doc.createPath();
//     selfIntersectingLinear.addPoint(Vec2f(0, 0));
//     selfIntersectingLinear.addPoint(Vec2f(100, 0));
//     selfIntersectingLinear.addPoint(Vec2f(50, 100));
//     selfIntersectingLinear.addPoint(Vec2f(50, -100));
//     auto isecs3 = selfIntersectingLinear.intersections();
//     EXPECT(isecs3.count() == 1);
//     EXPECT(crunch::isClose(isecs3[0].position, Vec2f(50, 0)));

//     Path selfIntersecting = doc.createPath();
//     selfIntersecting.addPoint(Vec2f(100, 100));
//     selfIntersecting.arcTo(Vec2f(200, 100));
//     selfIntersecting.arcTo(Vec2f(200, 0));

//     auto isecs4 = selfIntersecting.intersections();
//     EXPECT(isecs4.count() == 1);
//     EXPECT(crunch::isClose(isecs4[0].position, Vec2f(150, 50)));

//     Path a = doc.createPath();
//     a.addPoint(Vec2f(100, 100));
//     a.arcTo(Vec2f(200, 100));

//     Path b = doc.createPath();
//     b.addPoint(Vec2f(200, 100));
//     b.arcTo(Vec2f(200, 0));

//     auto isecs5 = a.intersections(b);
//     EXPECT(isecs5.count() == 2);
//     EXPECT(crunch::isClose(isecs5[0].position, Vec2f(150, 50)));
//     EXPECT(crunch::isClose(isecs5[1].position, Vec2f(200, 100)));

//     auto isecs6 = b.intersections(a);
//     EXPECT(isecs6.count() == 2);
//     EXPECT(crunch::isClose(isecs6[0].position, Vec2f(200, 100)));
//     EXPECT(crunch::isClose(isecs6[1].position, Vec2f(150, 50)));
// }
};
// clang-format on

int main(int _argc, const char * _args[])
{
    return runTests(spec, _argc, _args);
}
