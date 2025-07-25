/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/


#include <osgUtil/RayIntersector>
#include <osgUtil/LineSegmentIntersector>
#include <osg/KdTree>
#include <osg/Notify>
#include <osg/TexMat>
#include <limits>
#include <cmath>

using namespace osg;
using namespace osgUtil;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  RayIntersector - 射线相交器
//

RayIntersector::RayIntersector(CoordinateFrame cf, RayIntersector* parent,
                               Intersector::IntersectionLimit intersectionLimit) :
    Intersector(cf, intersectionLimit),
    _parent(parent)
{
    if (parent) setPrecisionHint(parent->getPrecisionHint());
}

RayIntersector::RayIntersector(const Vec3d& start, const Vec3d& direction) :
    Intersector(),
    _parent(0),
    _start(start),
    _direction(direction)
{
}

RayIntersector::RayIntersector(CoordinateFrame cf, const Vec3d& start, const Vec3d& direction,
                               RayIntersector* parent, Intersector::IntersectionLimit intersectionLimit) :
    Intersector(cf, intersectionLimit),
    _parent(parent),
    _start(start),
    _direction(direction)
{
    if (parent) setPrecisionHint(parent->getPrecisionHint());
}

RayIntersector::RayIntersector(CoordinateFrame cf, double x, double y) :
    Intersector(cf),
    _parent(0)
{
    switch(cf)
    {
        case WINDOW:     setStart(Vec3d(x,y,0.));  setDirection(Vec3d(0.,0.,1.)); break;
        case PROJECTION: setStart(Vec3d(x,y,-1.)); setDirection(Vec3d(0.,0.,1.)); break;
        case VIEW:       setStart(Vec3d(x,y,0.));  setDirection(Vec3d(0.,0.,1.)); break;
        case MODEL:      setStart(Vec3d(x,y,0.));  setDirection(Vec3d(0.,0.,1.)); break;
    }
}

Intersector* RayIntersector::clone(IntersectionVisitor& iv)
{
    if (_coordinateFrame==MODEL && iv.getModelMatrix()==0)
    {
        return new RayIntersector(MODEL, _start, _direction, this, _intersectionLimit);
    }

    Matrix matrix(LineSegmentIntersector::getTransformation(iv, _coordinateFrame));

    Vec3d newStart = _start * matrix;
    Vec4d tmp = Vec4d(_start + _direction, 1.) * matrix;
    Vec3d newEnd = Vec3d(tmp.x(), tmp.y(), tmp.z()) - (newStart * tmp.w());
    return new RayIntersector(MODEL, newStart, newEnd, this, _intersectionLimit);
}

bool RayIntersector::enter(const Node& node)
{
    if (reachedLimit()) return false;
    return !node.isCullingActive() || intersects( node.getBound() );
}

void RayIntersector::leave()
{
    // 什么也不做
}

void RayIntersector::reset()
{
    Intersector::reset();

    _intersections.clear();
}

void RayIntersector::intersect(IntersectionVisitor& iv, Drawable* drawable)
{
    // 检查是否已达到setIntersectionLimit()指定的限制？
    if (reachedLimit()) return;

    // 将射线裁剪为有限线段
    Vec3d s(_start), e;
    if (!intersectAndClip(s, _direction, e, drawable->getBoundingBox())) return;

    // 虚拟遍历
    if (iv.getDoDummyTraversal()) return;

    // 使用LineSegmentIntersector获取相交结果
    LineSegmentIntersector lsi(MODEL, s, e, NULL, _intersectionLimit);
    lsi.setPrecisionHint(getPrecisionHint());
    lsi.intersect(iv, drawable, s, e);

    // 从LineSegmentIntersector复制相交结果
    LineSegmentIntersector::Intersections intersections = lsi.getIntersections();
    if (!intersections.empty())
    {
        double preLength = (s - _start).length();
        double esLength = (e - s).length();

        for(LineSegmentIntersector::Intersections::iterator it = intersections.begin();
            it != intersections.end(); it++)
        {
            Intersection hit;
            hit.distance = preLength + it->ratio * esLength;
            hit.matrix = it->matrix;
            hit.nodePath = it->nodePath;
            hit.drawable = it->drawable;
            hit.primitiveIndex = it->primitiveIndex;

            hit.localIntersectionPoint = it->localIntersectionPoint;
            hit.localIntersectionNormal = it->localIntersectionNormal;

            hit.indexList = it->indexList;
            hit.ratioList = it->ratioList;

            insertIntersection(hit);
        }
    }
}

bool RayIntersector::intersects(const BoundingSphere& bs)
{
    // 如果包围球无效，则返回true，基于无效球体还未定义的假设
    if (!bs.valid()) return true;

    // 测试_start是否在包围球内部
    Vec3d sm = _start - bs._center;
    double c = sm.length2() - bs._radius * bs._radius;
    if (c<0.0) return true;

    // 求解二次方程
    double a = _direction.length2();
    double b = (sm * _direction) * 2.0;
    double d = b * b - 4.0 * a * c;

    // 如果d<0则无相交
    if (d<0.0) return false;

    // 计算二次方程的两个解
    d = sqrt(d);
    double div = 1.0/(2.0*a);
    double r1 = (-b-d)*div;
    double r2 = (-b+d)*div;

    // 如果两个相交点都在射线起点之前，返回false
    if (r1<=0.0 && r2<=0.0) return false;

    // 如果是LIMIT_NEAREST且包围球的最近点比已找到的相交更远，返回false
    if (_intersectionLimit == LIMIT_NEAREST && !getIntersections().empty())
    {
        double minDistance = sm.length() - bs._radius;
        if (minDistance >= getIntersections().begin()->distance) return false;
    }

    // 通过了所有拒绝测试，所以射线必须与包围球相交，返回true
    return true;
}

bool RayIntersector::intersectAndClip(Vec3d& s, const Vec3d& d, Vec3d& e, const BoundingBox& bbInput)
{
    // 包围盒的最小值和最大值
    Vec3d bb_min(bbInput._min);
    Vec3d bb_max(bbInput._max);

    // 通过epsilon扩展包围盒的范围以防止数值误差导致的遗漏
    const double epsilon = 1e-6;

    // 对包围盒的最小到最大范围的所有三个分量裁剪s
    for (int i=0; i<3; i++)
    {
        // 测试方向
        if (d[i] >= 0.)
        {
            // 完全在外部的线段的平凡拒绝
            if (s[i] > bb_max[i]) return false;

            if ((d[i] > epsilon) && (s[i] < bb_min[i]))
            {
                // 将s裁剪到xMin
                double t = (bb_min[i]-s[i])/d[i] - epsilon;
                if (t>0.0) s = s + d*t;
            }
        }
        else
        {
            // 完全在外部的线段的平凡拒绝
            if (s[i] < bb_min[i]) return false;

            if ((d[i] < -epsilon) && (s[i] > bb_max[i]))
            {
                // 将s裁剪到xMax
                double t = (bb_max[i]-s[i])/d[i] - epsilon;
                if (t>0.0) s = s + d*t;
            }
        }
    }

    // 裁剪射线终点的t值
    double end_t = std::numeric_limits<double>::infinity();

    // 通过包围盒裁剪射线得到终点
    // 注意：这不能在之前的循环中完成，因为起点s在移动
    for (int i=0; i<3; i++)
    {
        // 测试方向
        if (d[i] >= epsilon)
        {
            // 基于xMax计算end_t
            double t = (bb_max[i]-s[i])/d[i] + epsilon;
            if (t < end_t)
                end_t = t;
        }
        else if (d[i] <= -epsilon)
        {
            // 基于xMin计算end_t
            double t = (bb_min[i]-s[i])/d[i] + epsilon;
            if (t < end_t)
                end_t = t;
        }
    }

    // 如果我们无法钳制终点，返回false
    if (end_t==std::numeric_limits<double>::infinity()) return false;

    // 计算e
    e = s + d*end_t;

    return true;
}

Texture* RayIntersector::Intersection::getTextureLookUp(Vec3& tc) const
{
    Geometry* geometry = drawable.valid() ? drawable->asGeometry() : 0;
    Vec3Array* vertices = geometry ? dynamic_cast<Vec3Array*>(geometry->getVertexArray()) : 0;

    if (vertices)
    {
        if (indexList.size()==3 && ratioList.size()==3)
        {
            unsigned int i1 = indexList[0];
            unsigned int i2 = indexList[1];
            unsigned int i3 = indexList[2];

            float r1 = ratioList[0];
            float r2 = ratioList[1];
            float r3 = ratioList[2];

            Array* texcoords = (geometry->getNumTexCoordArrays()>0) ? geometry->getTexCoordArray(0) : 0;
            FloatArray* texcoords_FloatArray = dynamic_cast<FloatArray*>(texcoords);
            Vec2Array* texcoords_Vec2Array = dynamic_cast<Vec2Array*>(texcoords);
            Vec3Array* texcoords_Vec3Array = dynamic_cast<Vec3Array*>(texcoords);
            if (texcoords_FloatArray)
            {
                // 我们有纹理坐标数组，现在可以计算相交点处的最终纹理坐标
                float tc1 = (*texcoords_FloatArray)[i1];
                float tc2 = (*texcoords_FloatArray)[i2];
                float tc3 = (*texcoords_FloatArray)[i3];
                tc.x() = tc1*r1 + tc2*r2 + tc3*r3;
            }
            else if (texcoords_Vec2Array)
            {
                // 我们有纹理坐标数组，现在可以计算相交点处的最终纹理坐标
                const Vec2& tc1 = (*texcoords_Vec2Array)[i1];
                const Vec2& tc2 = (*texcoords_Vec2Array)[i2];
                const Vec2& tc3 = (*texcoords_Vec2Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
            }
            else if (texcoords_Vec3Array)
            {
                // 我们有纹理坐标数组，现在可以计算相交点处的最终纹理坐标
                const Vec3& tc1 = (*texcoords_Vec3Array)[i1];
                const Vec3& tc2 = (*texcoords_Vec3Array)[i2];
                const Vec3& tc3 = (*texcoords_Vec3Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
                tc.z() = tc1.z()*r1 + tc2.z()*r2 + tc3.z()*r3;
            }
            else
            {
                return 0;
            }
        }

        const TexMat* activeTexMat = 0;
        const Texture* activeTexture = 0;

        if (drawable->getStateSet())
        {
            const TexMat* texMat = dynamic_cast<TexMat*>(drawable->getStateSet()->getTextureAttribute(0,StateAttribute::TEXMAT));
            if (texMat) activeTexMat = texMat;

            const Texture* texture = dynamic_cast<Texture*>(drawable->getStateSet()->getTextureAttribute(0,StateAttribute::TEXTURE));
            if (texture) activeTexture = texture;
        }

        for(NodePath::const_reverse_iterator itr = nodePath.rbegin();
            itr != nodePath.rend() && (!activeTexMat || !activeTexture);
            ++itr)
            {
                const Node* node = *itr;
                if (node->getStateSet())
                {
                    if (!activeTexMat)
                    {
                        const TexMat* texMat = dynamic_cast<const TexMat*>(node->getStateSet()->getTextureAttribute(0,StateAttribute::TEXMAT));
                        if (texMat) activeTexMat = texMat;
                    }

                    if (!activeTexture)
                    {
                        const Texture* texture = dynamic_cast<const Texture*>(node->getStateSet()->getTextureAttribute(0,StateAttribute::TEXTURE));
                        if (texture) activeTexture = texture;
                    }
                }
            }

            if (activeTexMat)
            {
                Vec4 tc_transformed = Vec4(tc.x(), tc.y(), tc.z() ,0.0f) * activeTexMat->getMatrix();
                tc.x() = tc_transformed.x();
                tc.y() = tc_transformed.y();
                tc.z() = tc_transformed.z();

                if (activeTexture && activeTexMat->getScaleByTextureRectangleSize())
                {
                    tc.x() *= static_cast<float>(activeTexture->getTextureWidth());
                    tc.y() *= static_cast<float>(activeTexture->getTextureHeight());
                    tc.z() *= static_cast<float>(activeTexture->getTextureDepth());
                }
            }

            return const_cast<Texture*>(activeTexture);

    }
    return 0;
} 