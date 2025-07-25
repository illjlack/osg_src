/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/


#include <osgUtil/LineSegmentIntersector>

#include <osg/Geometry>
#include <osg/Notify>
#include <osg/io_utils>
#include <osg/TriangleFunctor>
#include <osg/KdTree>
#include <osg/Timer>
#include <osg/TexMat>
#include <osg/TemplatePrimitiveFunctor>

using namespace osgUtil;

namespace LineSegmentIntersectorUtils
{

struct Settings
{
    Settings() :
        _lineSegIntersector(0),
        _iv(0),
        _drawable(0),
        _limitOneIntersection(false) {}

    osgUtil::LineSegmentIntersector* _lineSegIntersector;   // 线段相交器指针
    osgUtil::IntersectionVisitor*   _iv;                   // 相交访问者指针
    osg::Drawable*                  _drawable;             // 可绘制对象指针
    osg::ref_ptr<osg::Vec3Array>    _vertices;             // 顶点数组
    bool                            _limitOneIntersection; // 限制一个相交点标志
};

template<typename Vec3, typename value_type>
struct IntersectFunctor
{
    Settings* _settings;                    // 设置参数

    unsigned int            _primitiveIndex;    // 图元索引
    Vec3                    _start;            // 线段起点
    Vec3                    _end;              // 线段终点

    typedef std::pair< Vec3, Vec3> StartEnd;
    typedef std::vector< StartEnd > StartEndStack;

    StartEndStack _startEndStack;               // 起点终点堆栈

    Vec3        _d;                            // 方向向量
    value_type  _length;                       // 线段长度
    value_type  _inverse_length;               // 长度倒数

    Vec3        _d_invX;                       // X方向的倒数向量
    Vec3        _d_invY;                       // Y方向的倒数向量
    Vec3        _d_invZ;                       // Z方向的倒数向量

    bool        _hit;                          // 是否击中标志

    IntersectFunctor():
        _primitiveIndex(0),
        _length(0.0),
        _inverse_length(0.0),
        _hit(false)
    {
    }


    void set(const osg::Vec3d& s, const osg::Vec3d& e, Settings* settings)
    {
        _settings = settings;

        _start = s;
        _end = e;

        _startEndStack.push_back(StartEnd(_start,_end));

        _d = e - s;
        _length = _d.length();
        _inverse_length = (_length!=0.0) ? 1.0/_length : 0.0;
        _d *= _inverse_length;

        _d_invX = _d.x()!=0.0 ? _d/_d.x() : Vec3(0.0,0.0,0.0);
        _d_invY = _d.y()!=0.0 ? _d/_d.y() : Vec3(0.0,0.0,0.0);
        _d_invZ = _d.z()!=0.0 ? _d/_d.z() : Vec3(0.0,0.0,0.0);
    }

    bool enter(const osg::BoundingBox& bb)
    {
        StartEnd startend = _startEndStack.back();
        Vec3& s = startend.first;
        Vec3& e = startend.second;

        //return true;

        //if (!bb.valid()) return true;

        // 将s和e与包围盒的xMin到xMax范围进行比较
        if (s.x()<=e.x())
        {

            // 完全在外部的线段的平凡拒绝
            if (e.x()<bb.xMin()) return false;
            if (s.x()>bb.xMax()) return false;

            if (s.x()<bb.xMin())
            {
                // 将s裁剪到xMin
                s = s+_d_invX*(bb.xMin()-s.x());
            }

            if (e.x()>bb.xMax())
            {
                // 将e裁剪到xMax
                e = s+_d_invX*(bb.xMax()-s.x());
            }
        }
        else
        {
            if (s.x()<bb.xMin()) return false;
            if (e.x()>bb.xMax()) return false;

            if (e.x()<bb.xMin())
            {
                // 将s裁剪到xMin
                e = s+_d_invX*(bb.xMin()-s.x());
            }

            if (s.x()>bb.xMax())
            {
                // 将e裁剪到xMax
                s = s+_d_invX*(bb.xMax()-s.x());
            }
        }

        // 将s和e与包围盒的yMin到yMax范围进行比较
        if (s.y()<=e.y())
        {

            // 完全在外部的线段的平凡拒绝
            if (e.y()<bb.yMin()) return false;
            if (s.y()>bb.yMax()) return false;

            if (s.y()<bb.yMin())
            {
                // 将s裁剪到yMin
                s = s+_d_invY*(bb.yMin()-s.y());
            }

            if (e.y()>bb.yMax())
            {
                // 将e裁剪到yMax
                e = s+_d_invY*(bb.yMax()-s.y());
            }
        }
        else
        {
            if (s.y()<bb.yMin()) return false;
            if (e.y()>bb.yMax()) return false;

            if (e.y()<bb.yMin())
            {
                // 将s裁剪到yMin
                e = s+_d_invY*(bb.yMin()-s.y());
            }

            if (s.y()>bb.yMax())
            {
                // 将e裁剪到yMax
                s = s+_d_invY*(bb.yMax()-s.y());
            }
        }

        // 将s和e与包围盒的zMin到zMax范围进行比较
        if (s.z()<=e.z())
        {

            // 完全在外部的线段的平凡拒绝
            if (e.z()<bb.zMin()) return false;
            if (s.z()>bb.zMax()) return false;

            if (s.z()<bb.zMin())
            {
                // 将s裁剪到zMin
                s = s+_d_invZ*(bb.zMin()-s.z());
            }

            if (e.z()>bb.zMax())
            {
                // 将e裁剪到zMax
                e = s+_d_invZ*(bb.zMax()-s.z());
            }
        }
        else
        {
            if (s.z()<bb.zMin()) return false;
            if (e.z()>bb.zMax()) return false;

            if (e.z()<bb.zMin())
            {
                // 将s裁剪到zMin
                e = s+_d_invZ*(bb.zMin()-s.z());
            }

            if (s.z()>bb.zMax())
            {
                // 将e裁剪到zMax
                s = s+_d_invZ*(bb.zMax()-s.z());
            }
        }

        // OSG_NOTICE<<"clampped segment "<<s<<" "<<e<<std::endl;
        _startEndStack.push_back(startend);

        return true;
    }

    void leave()
    {
        // OSG_NOTICE<<"leave() "<<_startEndStack.size()<<std::endl;
        _startEndStack.pop_back();
    }

    void intersect(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        // 使用Möller-Trumbore算法进行射线-三角形相交检测
        Vec3 T = _start - v0;
        Vec3 E2 = v2 - v0;
        Vec3 E1 = v1 - v0;

        Vec3 P =  _d ^ E2;

        value_type det = P * E1;

        value_type r,r0,r1,r2;

        const value_type epsilon = 1e-10;
        if (det>epsilon)
        {
            value_type u = (P*T);
            if (u<0.0 || u>det) return;

            osg::Vec3 Q = T ^ E1;
            value_type v = (Q*_d);
            if (v<0.0 || v>det) return;

            if ((u+v)> det) return;

            value_type inv_det = 1.0/det;
            value_type t = (Q*E2)*inv_det;
            if (t<0.0 || t>_length) return;

            u *= inv_det;
            v *= inv_det;

            r0 = 1.0-u-v;
            r1 = u;
            r2 = v;
            r = t * _inverse_length;
        }
        else if (det<-epsilon)
        {
            value_type u = (P*T);
            if (u>0.0 || u<det) return;

            Vec3 Q = T ^ E1;
            value_type v = (Q*_d);
            if (v>0.0 || v<det) return;

            if ((u+v) < det) return;

            value_type inv_det = 1.0/det;
            value_type t = (Q*E2)*inv_det;
            if (t<0.0 || t>_length) return;

            u *= inv_det;
            v *= inv_det;

            r0 = 1.0-u-v;
            r1 = u;
            r2 = v;
            r = t * _inverse_length;
        }
        else
        {
            return;
        }

        // 将比例重新映射到LineSegment的范围内
        const osg::Vec3d& lsStart = _settings->_lineSegIntersector->getStart();
        const osg::Vec3d& lsEnd = _settings->_lineSegIntersector->getEnd();
        double remap_ratio =  ((_start - lsStart).length() + r*_length)/(lsEnd - lsStart).length();

        Vec3 in = lsStart*(1.0 - remap_ratio) + lsEnd*remap_ratio; // == v0*r0 + v1*r1 + v2*r2;
        Vec3 normal = E1^E2;
        normal.normalize();

        LineSegmentIntersector::Intersection hit;
        hit.ratio = remap_ratio;
        hit.matrix = _settings->_iv->getModelMatrix();
        hit.nodePath = _settings->_iv->getNodePath();
        hit.drawable = _settings->_drawable;
        hit.primitiveIndex = _primitiveIndex;

        hit.localIntersectionPoint = in;
        hit.localIntersectionNormal = normal;

        if (_settings->_vertices.valid())
        {
            const osg::Vec3* first = &(_settings->_vertices->front());
            hit.indexList.reserve(3);
            hit.ratioList.reserve(3);

            if (r0!=0.0f)
            {
                hit.indexList.push_back(&v0-first);
                hit.ratioList.push_back(r0);
            }

            if (r1!=0.0f)
            {
                hit.indexList.push_back(&v1-first);
                hit.ratioList.push_back(r1);
            }

            if (r2!=0.0f)
            {
                hit.indexList.push_back(&v2-first);
                hit.ratioList.push_back(r2);
            }
        }

        _settings->_lineSegIntersector->insertIntersection(hit);
        _hit = true;
    }

    // 处理线段（单个顶点）
    void operator()(const osg::Vec3&, bool /*treatVertexDataAsTemporary*/)
    {
        ++_primitiveIndex;
    }

    // 处理线段（两个顶点）
    void operator()(const osg::Vec3&, const osg::Vec3&, bool /*treatVertexDataAsTemporary*/)
    {
        ++_primitiveIndex;
    }

    // 处理三角形
    void operator()(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2, bool /*treatVertexDataAsTemporary*/)
    {
        intersect(v0,v1,v2);
        ++_primitiveIndex;
    }

    // 处理四边形（分解为两个三角形）
    void operator()(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3, bool /*treatVertexDataAsTemporary*/)
    {
        intersect(v0,v1,v3);
        intersect(v1,v2,v3);
        ++_primitiveIndex;
    }

    // 处理索引三角形
    void intersect(const osg::Vec3Array*, int , unsigned int)
    {
    }

    void intersect(const osg::Vec3Array*, int, unsigned int)
    {
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0, unsigned int p1, unsigned int p2)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        _primitiveIndex = primitiveIndex;

        intersect((*vertices)[p0], (*vertices)[p1], (*vertices)[p2]);
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0, unsigned int p1, unsigned int p2, unsigned int p3)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        _primitiveIndex = primitiveIndex;

        intersect((*vertices)[p0], (*vertices)[p1], (*vertices)[p3]);
        intersect((*vertices)[p1], (*vertices)[p2], (*vertices)[p3]);
    }
};

} // namespace LineSegmentIntersectorUtils

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  LineSegmentIntersector - 线段相交器
//

LineSegmentIntersector::LineSegmentIntersector(const osg::Vec3d& start, const osg::Vec3d& end):
    _parent(0),
    _start(start),
    _end(end)
{
}

LineSegmentIntersector::LineSegmentIntersector(CoordinateFrame cf, const osg::Vec3d& start, const osg::Vec3d& end,
                                               LineSegmentIntersector* parent, osgUtil::Intersector::IntersectionLimit intersectionLimit):
    Intersector(cf, intersectionLimit),
    _parent(parent),
    _start(start),
    _end(end)
{
}

LineSegmentIntersector::LineSegmentIntersector(CoordinateFrame cf, double x, double y):
    Intersector(cf),
    _parent(0)
{
    switch(cf)
    {
        case WINDOW : _start.set(x,y,0.0); _end.set(x,y,1.0); break;
        case PROJECTION : _start.set(x,y,-1.0); _end.set(x,y,1.0); break;
        case VIEW : _start.set(x,y,0.0); _end.set(x,y,1.0); break;
        case MODEL : _start.set(x,y,0.0); _end.set(x,y,1.0); break;
    }
}

Intersector* LineSegmentIntersector::clone(osgUtil::IntersectionVisitor& iv)
{
    if (_coordinateFrame==MODEL && iv.getModelMatrix()==0)
    {
        osg::ref_ptr<LineSegmentIntersector> lsi = new LineSegmentIntersector(_start, _end);
        lsi->_parent = this;
        lsi->_intersectionLimit = this->_intersectionLimit;
        lsi->setPrecisionHint(getPrecisionHint());
        return lsi.release();
    }

    // 计算将此相交器从其坐标框架转换到局部MODEL坐标框架的矩阵
    // 场景图中的几何体将始终位于MODEL坐标框架中
    osg::Matrix matrix(getTransformation(iv, _coordinateFrame));

    osg::ref_ptr<LineSegmentIntersector> lsi = new LineSegmentIntersector(_start * matrix, _end * matrix);
    lsi->_parent = this;
    lsi->_intersectionLimit = this->_intersectionLimit;
    lsi->setPrecisionHint(getPrecisionHint());
    return lsi.release();
}

osg::Matrix LineSegmentIntersector::getTransformation(IntersectionVisitor& iv, CoordinateFrame cf)
{
    osg::Matrix matrix;
    switch (cf)
    {
        case(WINDOW):
            if (iv.getWindowMatrix()) matrix.preMult( *iv.getWindowMatrix() );
            if (iv.getProjectionMatrix()) matrix.preMult( *iv.getProjectionMatrix() );
            if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
            if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
            break;
        case(PROJECTION):
            if (iv.getProjectionMatrix()) matrix.preMult( *iv.getProjectionMatrix() );
            if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
            if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
            break;
        case(VIEW):
            if (iv.getViewMatrix()) matrix.preMult( *iv.getViewMatrix() );
            if (iv.getModelMatrix()) matrix.preMult( *iv.getModelMatrix() );
            break;
        case(MODEL):
            if (iv.getModelMatrix()) matrix = *iv.getModelMatrix();
            break;
    }

    osg::Matrix inverse;
    inverse.invert(matrix);
    return inverse;
}

bool LineSegmentIntersector::enter(const osg::Node& node)
{
    if (reachedLimit()) return false;
    return !node.isCullingActive() || intersects( node.getBound() );
}

void LineSegmentIntersector::leave()
{
    // 什么也不做
}

void LineSegmentIntersector::intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable)
{
    if (reachedLimit()) return;

    osg::Vec3d s(_start), e(_end);
    if ( drawable->isCullingActive() && !intersectAndClip( s, e, drawable->getBoundingBox() ) ) return;

    if (iv.getDoDummyTraversal()) return;

    intersect(iv, drawable, s, e);
}

void LineSegmentIntersector::intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable,
                                       const osg::Vec3d& s, const osg::Vec3d& e)
{
    if (reachedLimit()) return;

    LineSegmentIntersectorUtils::Settings settings;
    settings._lineSegIntersector = this;
    settings._iv = &iv;
    settings._drawable = drawable;
    settings._limitOneIntersection = (_intersectionLimit == LIMIT_ONE_PER_DRAWABLE || _intersectionLimit == LIMIT_ONE);

    osg::Geometry* geometry = drawable->asGeometry();
    if (geometry)
    {
        settings._vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    }

    osg::KdTree* kdTree = iv.getUseKdTreeWhenAvailable() ? dynamic_cast<osg::KdTree*>(drawable->getShape()) : 0;

    if (getPrecisionHint()==USE_DOUBLE_CALCULATIONS)
    {
        osg::TemplatePrimitiveFunctor<LineSegmentIntersectorUtils::IntersectFunctor<osg::Vec3d, double> > intersector;
        intersector.set(s,e, &settings);

        if (kdTree) kdTree->intersect(intersector, kdTree->getNode(0));
        else drawable->accept(intersector);
    }
    else
    {
        osg::TemplatePrimitiveFunctor<LineSegmentIntersectorUtils::IntersectFunctor<osg::Vec3f, float> > intersector;
        intersector.set(s,e, &settings);

        if (kdTree) kdTree->intersect(intersector, kdTree->getNode(0));
        else drawable->accept(intersector);
    }
}

void LineSegmentIntersector::reset()
{
    Intersector::reset();

    _intersections.clear();
}

bool LineSegmentIntersector::intersects(const osg::BoundingSphere& bs)
{
    // 如果包围球无效，则返回true，基于无效球体还未定义的假设
    if (!bs.valid()) return true;

    osg::Vec3d sm = _start - bs._center;
    double c = sm.length2()-bs._radius*bs._radius;
    if (c<0.0) return true;

    osg::Vec3d se = _end-_start;
    double a = se.length2();
    double b = (sm*se)*2.0;
    double d = b*b-4.0*a*c;

    if (d<0.0) return false;

    d = sqrt(d);

    double div = 1.0/(2.0*a);

    double r1 = (-b-d)*div;
    double r2 = (-b+d)*div;

    if (r1<=0.0 && r2<=0.0) return false;

    if (r1>=1.0 && r2>=1.0) return false;

    if (_intersectionLimit == LIMIT_NEAREST && !getIntersections().empty())
    {
        double ratio = (sm.length() - bs._radius) / sqrt(a);
        if (ratio >= getIntersections().begin()->ratio) return false;
    }

    // 通过了所有拒绝测试，所以线段必须与包围球相交，返回true
    return true;
}

bool LineSegmentIntersector::intersectAndClip(osg::Vec3d& s, osg::Vec3d& e,const osg::BoundingBox& bbInput)
{
    osg::Vec3d bb_min(bbInput._min);
    osg::Vec3d bb_max(bbInput._max);

    double epsilon = 1e-5;

    // 将s和e与包围盒的xMin到xMax范围进行比较
    if (s.x()<=e.x())
    {
        // 完全在外部的线段的平凡拒绝
        if (e.x()<bb_min.x()) return false;
        if (s.x()>bb_max.x()) return false;

        if (s.x()<bb_min.x())
        {
            // 将s裁剪到xMin
            double r = (bb_min.x()-s.x())/(e.x()-s.x()) - epsilon;
            if (r>0.0) s = s + (e-s)*r;
        }

        if (e.x()>bb_max.x())
        {
            // 将e裁剪到xMax
            double r = (bb_max.x()-s.x())/(e.x()-s.x()) + epsilon;
            if (r<1.0) e = s+(e-s)*r;
        }
    }
    else
    {
        if (s.x()<bb_min.x()) return false;
        if (e.x()>bb_max.x()) return false;

        if (e.x()<bb_min.x())
        {
            // 将e裁剪到xMin
            double r = (bb_min.x()-e.x())/(s.x()-e.x()) - epsilon;
            if (r>0.0) e = e + (s-e)*r;
        }

        if (s.x()>bb_max.x())
        {
            // 将s裁剪到xMax
            double r = (bb_max.x()-e.x())/(s.x()-e.x()) + epsilon;
            if (r<1.0) s = e + (s-e)*r;
        }
    }

    // 将s和e与包围盒的yMin到yMax范围进行比较
    if (s.y()<=e.y())
    {
        // 完全在外部的线段的平凡拒绝
        if (e.y()<bb_min.y()) return false;
        if (s.y()>bb_max.y()) return false;

        if (s.y()<bb_min.y())
        {
            // 将s裁剪到yMin
            double r = (bb_min.y()-s.y())/(e.y()-s.y()) - epsilon;
            if (r>0.0) s = s + (e-s)*r;
        }

        if (e.y()>bb_max.y())
        {
            // 将e裁剪到yMax
            double r = (bb_max.y()-s.y())/(e.y()-s.y()) + epsilon;
            if (r<1.0) e = s+(e-s)*r;
        }
    }
    else
    {
        if (s.y()<bb_min.y()) return false;
        if (e.y()>bb_max.y()) return false;

        if (e.y()<bb_min.y())
        {
            // 将e裁剪到yMin
            double r = (bb_min.y()-e.y())/(s.y()-e.y()) - epsilon;
            if (r>0.0) e = e + (s-e)*r;
        }

        if (s.y()>bb_max.y())
        {
            // 将s裁剪到yMax
            double r = (bb_max.y()-e.y())/(s.y()-e.y()) + epsilon;
            if (r<1.0) s = e + (s-e)*r;
        }
    }

    // 将s和e与包围盒的zMin到zMax范围进行比较
    if (s.z()<=e.z())
    {
        // 完全在外部的线段的平凡拒绝
        if (e.z()<bb_min.z()) return false;
        if (s.z()>bb_max.z()) return false;

        if (s.z()<bb_min.z())
        {
            // 将s裁剪到zMin
            double r = (bb_min.z()-s.z())/(e.z()-s.z()) - epsilon;
            if (r>0.0) s = s + (e-s)*r;
        }

        if (e.z()>bb_max.z())
        {
            // 将e裁剪到zMax
            double r = (bb_max.z()-s.z())/(e.z()-s.z()) + epsilon;
            if (r<1.0) e = s+(e-s)*r;
        }
    }
    else
    {
        if (s.z()<bb_min.z()) return false;
        if (e.z()>bb_max.z()) return false;

        if (e.z()<bb_min.z())
        {
            // 将e裁剪到zMin
            double r = (bb_min.z()-e.z())/(s.z()-e.z()) - epsilon;
            if (r>0.0) e = e + (s-e)*r;
        }

        if (s.z()>bb_max.z())
        {
            // 将s裁剪到zMax
            double r = (bb_max.z()-e.z())/(s.z()-e.z()) + epsilon;
            if (r<1.0) s = e + (s-e)*r;
        }
    }

    // OSG_NOTICE<<"clampped segment "<<s<<" "<<e<<std::endl;

    // if (s==e) return false;

    return true;
}

osg::Texture* LineSegmentIntersector::Intersection::getTextureLookUp(osg::Vec3& tc) const
{
    osg::Geometry* geometry = drawable.valid() ? drawable->asGeometry() : 0;
    osg::Vec3Array* vertices = geometry ? dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : 0;

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

            osg::Array* texcoords = (geometry->getNumTexCoordArrays()>0) ? geometry->getTexCoordArray(0) : 0;
            osg::FloatArray* texcoords_FloatArray = dynamic_cast<osg::FloatArray*>(texcoords);
            osg::Vec2Array* texcoords_Vec2Array = dynamic_cast<osg::Vec2Array*>(texcoords);
            osg::Vec3Array* texcoords_Vec3Array = dynamic_cast<osg::Vec3Array*>(texcoords);
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
                const osg::Vec2& tc1 = (*texcoords_Vec2Array)[i1];
                const osg::Vec2& tc2 = (*texcoords_Vec2Array)[i2];
                const osg::Vec2& tc3 = (*texcoords_Vec2Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
            }
            else if (texcoords_Vec3Array)
            {
                // 我们有纹理坐标数组，现在可以计算相交点处的最终纹理坐标
                const osg::Vec3& tc1 = (*texcoords_Vec3Array)[i1];
                const osg::Vec3& tc2 = (*texcoords_Vec3Array)[i2];
                const osg::Vec3& tc3 = (*texcoords_Vec3Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
                tc.z() = tc1.z()*r1 + tc2.z()*r2 + tc3.z()*r3;
            }
            else
            {
                return 0;
            }
        }

        const osg::TexMat* activeTexMat = 0;
        const osg::Texture* activeTexture = 0;

        if (drawable->getStateSet())
        {
            const osg::TexMat* texMat = dynamic_cast<osg::TexMat*>(drawable->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXMAT));
            if (texMat) activeTexMat = texMat;

            const osg::Texture* texture = dynamic_cast<osg::Texture*>(drawable->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));
            if (texture) activeTexture = texture;
        }

        for(osg::NodePath::const_reverse_iterator itr = nodePath.rbegin();
            itr != nodePath.rend() && (!activeTexMat || !activeTexture);
            ++itr)
        {
            const osg::Node* node = *itr;
            if (node->getStateSet())
            {
                if (!activeTexMat)
                {
                    const osg::TexMat* texMat = dynamic_cast<const osg::TexMat*>(node->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXMAT));
                    if (texMat) activeTexMat = texMat;
                }

                if (!activeTexture)
                {
                    const osg::Texture* texture = dynamic_cast<const osg::Texture*>(node->getStateSet()->getTextureAttribute(0,osg::StateAttribute::TEXTURE));
                    if (texture) activeTexture = texture;
                }
            }
        }

        if (activeTexMat)
        {
            osg::Vec4 tc_transformed = osg::Vec4(tc.x(), tc.y(), tc.z() ,0.0f) * activeTexMat->getMatrix();
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

        return const_cast<osg::Texture*>(activeTexture);

    }
    return 0;
} 