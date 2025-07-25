/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/


#include <osgUtil/PolytopeIntersector>

#include <osg/Geometry>
#include <osg/KdTree>
#include <osg/Notify>
#include <osg/io_utils>
#include <osg/TemplatePrimitiveFunctor>

using namespace osgUtil;


namespace PolytopeIntersectorUtils
{

struct Settings : public osg::Referenced
{
    Settings() :
        _polytopeIntersector(0),
        _iv(0),
        _drawable(0),
        _limitOneIntersection(false),
        _primitiveMask( PolytopeIntersector::ALL_PRIMITIVES ) {}

    osgUtil::PolytopeIntersector*   _polytopeIntersector;  // 多面体相交器指针
    osgUtil::IntersectionVisitor*   _iv;                  // 相交访问者指针
    osg::Drawable*                  _drawable;            // 可绘制对象指针
    osg::ref_ptr<osg::Vec3Array>    _vertices;            // 顶点数组
    bool                            _limitOneIntersection; // 限制一个相交点标志
    unsigned int                    _primitiveMask;       // 图元掩码
};

template<typename Vec3>
struct IntersectFunctor
{
    typedef typename Vec3::value_type value_type;
    typedef Vec3 vec_type;

    typedef std::vector<vec_type> Vertices;
    Vertices                        src, dest;              // 源顶点和目标顶点数组

    osg::ref_ptr<Settings>          _settings;              // 设置参数
    unsigned int                    _primitiveIndex;        // 图元索引
    bool                            _hit;                   // 是否击中标志

    IntersectFunctor():
        _primitiveIndex(0),
        _hit(false)
    {
        src.reserve(10);
        dest.reserve(10);
    }

    bool enter(const osg::BoundingBox& bb)
    {
        if (_settings->_polytopeIntersector->getPolytope().contains(bb))
        {
            _settings->_polytopeIntersector->getPolytope().pushCurrentMask();

            return true;
        }
        else
        {
            return false;
        }
    }

    void leave()
    {
        _settings->_polytopeIntersector->getPolytope().popCurrentMask();
    }

    void addIntersection()
    {

        vec_type center(0.0,0.0,0.0);
        double maxDistance = -DBL_MAX;
        const osg::Plane& referencePlane = _settings->_polytopeIntersector->getReferencePlane();
        for(typename Vertices::iterator itr = src.begin();
            itr != src.end();
            ++itr)
        {
            center += *itr;
            double d = referencePlane.distance(*itr);
            if (d>maxDistance) maxDistance = d;

        }

        center /= value_type(src.size());

        PolytopeIntersector::Intersection intersection;
        intersection.primitiveIndex = _primitiveIndex;
        intersection.distance = referencePlane.distance(center);
        intersection.maxDistance = maxDistance;
        intersection.nodePath = _settings->_iv->getNodePath();
        intersection.drawable = _settings->_drawable;
        intersection.matrix = _settings->_iv->getModelMatrix();
        intersection.localIntersectionPoint = center;

        if (src.size()<PolytopeIntersector::Intersection::MaxNumIntesectionPoints) intersection.numIntersectionPoints = src.size();
        else intersection.numIntersectionPoints = PolytopeIntersector::Intersection::MaxNumIntesectionPoints;

        for(unsigned int i=0; i<intersection.numIntersectionPoints; ++i)
        {
            intersection.intersectionPoints[i] = src[i];
        }

        // OSG_NOTICE<<"intersection "<<src.size()<<" center="<<center<<std::endl;

        _settings->_polytopeIntersector->insertIntersection(intersection);
        _hit = true;

        // OSG_NOTICE<<"addIntersection() center="<<center<<std::endl;
    }

    bool contains()
    {
        const osg::Polytope& polytope = _settings->_polytopeIntersector->getPolytope();
        const osg::Polytope::PlaneList& planeList = polytope.getPlaneList();

        osg::Polytope::ClippingMask resultMask = polytope.getCurrentMask();
        if (!resultMask) return true;

        osg::Polytope::ClippingMask selector_mask = 0x1;

        for(osg::Polytope::PlaneList::const_iterator pitr = planeList.begin();
            pitr != planeList.end();
            ++pitr)
        {
            if (resultMask&selector_mask)
            {
                //OSG_NOTICE<<"Polytope::contains() Plane testing"<<std::endl;

                dest.clear();

                const osg::Plane& plane = *pitr;
                typename Vertices::iterator vitr = src.begin();

                vec_type* v_previous = &(*(vitr++));
                value_type d_previous = plane.distance(*v_previous);

                for(; vitr != src.end(); ++vitr)
                {
                    vec_type* v_current = &(*vitr);
                    value_type d_current = plane.distance(*v_current);

                    if (d_previous>=0.0)
                    {
                        dest.push_back(*v_previous);
                    }

                    if (d_previous*d_current<0.0)
                    {
                        // 边跨越平面，所以在它们之间插入顶点
                        value_type distance = d_previous-d_current;
                        value_type r_current = d_previous/distance;
                        vec_type v_new = (*v_previous)*(1.0-r_current) + (*v_current)*r_current;
                        dest.push_back(v_new);
                    }

                    d_previous = d_current;
                    v_previous = v_current;

                }

                if (d_previous>=0.0)
                {
                    dest.push_back(*v_previous);
                }

                if (dest.size()<=1)
                {
                    // OSG_NOTICE<<"Polytope::contains() All points on triangle culled, dest.size()="<<dest.size()<<std::endl;
                    return false;
                }

                dest.swap(src);
            }
            else
            {
                // OSG_NOTICE<<"Polytope::contains() Plane disabled"<<std::endl;
            }

            selector_mask <<= 1;
        }

        //OSG_NOTICE<<"Polytope::contains() triangle within Polytope, src.size()="<<src.size()<<std::endl;

        return true;
    }

    bool contains(const osg::Vec3f& v0)
    {
        if (_settings->_polytopeIntersector->getPolytope().contains(v0))
        {
            // 初始化要测试的顶点集合
            src.clear();
            src.push_back(v0);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool contains(const osg::Vec3f& v0, const osg::Vec3f& v1)
    {
        // 初始化要测试的顶点集合
        src.clear();
        src.push_back(v0);
        src.push_back(v1);

        return contains();
    }

    bool contains(const osg::Vec3f& v0, const osg::Vec3f& v1, const osg::Vec3f& v2)
    {
        // 初始化要测试的顶点集合
        src.clear();
        src.push_back(v0);
        src.push_back(v1);
        src.push_back(v2);
        src.push_back(v0);

        return contains();
    }

    bool contains(const osg::Vec3f& v0, const osg::Vec3f& v1, const osg::Vec3f& v2, const osg::Vec3f& v3)
    {
        // 初始化要测试的顶点集合
        src.clear();
        src.push_back(v0);
        src.push_back(v1);
        src.push_back(v2);
        src.push_back(v3);
        src.push_back(v0);

        return contains();
    }

    void operator()(const osg::Vec3& v0, bool /*treatVertexDataAsTemporary*/)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::POINT_PRIMITIVES)==0)
        {
          ++_primitiveIndex;
          return;
        }

        // 初始化要测试的顶点集合
        src.clear();

        const osg::Polytope& polytope = _settings->_polytopeIntersector->getPolytope();
        osg::Polytope::ClippingMask resultMask = polytope.getCurrentMask();
        if (resultMask)
        {
            osg::Polytope::ClippingMask selector_mask = 0x1;

            const osg::Polytope::PlaneList& planeList = polytope.getPlaneList();
            for(osg::Polytope::PlaneList::const_iterator pitr = planeList.begin();
                pitr != planeList.end();
                ++pitr)
            {
                if (resultMask&selector_mask)
                {
                    const osg::Plane& plane=*pitr;
                    double d1=plane.distance(v0);
                    if (d1<0.0)   // 点在外部
                    {
                      ++_primitiveIndex;
                      return;
                    }
                }
            }
        }

        src.push_back(v0);

        addIntersection();
        
        ++_primitiveIndex;
    }

    // 处理线段
    void operator()(const osg::Vec3& v0, const osg::Vec3& v1, bool /*treatVertexDataAsTemporary*/)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::LINE_PRIMITIVES)==0)
        {
          ++_primitiveIndex;
          return;
        }

        src.clear();
        src.push_back(v0);
        src.push_back(v1);
        src.push_back(v0);
        if (contains())
        {
            addIntersection();
        }
        ++_primitiveIndex;
    }

    // 处理三角形
    void operator()(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2, bool /*treatVertexDataAsTemporary*/)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::TRIANGLE_PRIMITIVES)==0)
        {
          ++_primitiveIndex;
          return;
        }

        src.clear();
        src.push_back(v0);
        src.push_back(v1);
        src.push_back(v2);
        src.push_back(v0);

        if (contains())
        {
            addIntersection();
        }
        ++_primitiveIndex;
    }

    void operator()(const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3, bool /*treatVertexDataAsTemporary*/)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::TRIANGLE_PRIMITIVES)==0)
        {
          ++_primitiveIndex;
          return;
        }

        src.clear();

        src.push_back(v0);
        src.push_back(v1);
        src.push_back(v2);
        src.push_back(v3);
        src.push_back(v0);

        if (contains())
        {
            addIntersection();
        }
        ++_primitiveIndex;
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::POINT_PRIMITIVES)==0) return;

        if (contains((*vertices)[p0]))
        {
            _primitiveIndex = primitiveIndex;

            addIntersection();
        }
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0, unsigned int p1)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::LINE_PRIMITIVES)==0) return;

        if (contains((*vertices)[p0], (*vertices)[p1]))
        {
            _primitiveIndex = primitiveIndex;

            addIntersection();
        }
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0, unsigned int p1, unsigned int p2)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::TRIANGLE_PRIMITIVES)==0) return;

        if (contains((*vertices)[p0], (*vertices)[p1], (*vertices)[p2]))
        {
            _primitiveIndex = primitiveIndex;

            addIntersection();
        }
    }

    void intersect(const osg::Vec3Array* vertices, int primitiveIndex, unsigned int p0, unsigned int p1, unsigned int p2, unsigned int p3)
    {
        if (_settings->_limitOneIntersection && _hit) return;

        if ((_settings->_primitiveMask&PolytopeIntersector::TRIANGLE_PRIMITIVES)==0) return;

        if (contains((*vertices)[p0], (*vertices)[p1], (*vertices)[p2], (*vertices)[p3]))
        {
            _primitiveIndex = primitiveIndex;

            addIntersection();
        }
    }


};

} // namespace  PolytopeIntersectorUtils


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PolytopeIntersector - 多面体相交器
//
PolytopeIntersector::PolytopeIntersector(const osg::Polytope& polytope):
    _parent(0),
    _polytope(polytope),
    _primitiveMask( ALL_PRIMITIVES )
{
    if (!_polytope.getPlaneList().empty())
    {
        _referencePlane = _polytope.getPlaneList().back();
    }
}

PolytopeIntersector::PolytopeIntersector(CoordinateFrame cf, const osg::Polytope& polytope):
    Intersector(cf),
    _parent(0),
    _polytope(polytope),
    _primitiveMask( ALL_PRIMITIVES )
{
    if (!_polytope.getPlaneList().empty())
    {
        _referencePlane = _polytope.getPlaneList().back();
    }
}

PolytopeIntersector::PolytopeIntersector(CoordinateFrame cf, double xMin, double yMin, double xMax, double yMax):
    Intersector(cf),
    _parent(0),
    _primitiveMask( ALL_PRIMITIVES )
{
    double zNear = 0.0;
    switch(cf)
    {
        case WINDOW : zNear = 0.0; break;
        case PROJECTION : zNear = 1.0; break;
        case VIEW : zNear = 0.0; break;
        case MODEL : zNear = 0.0; break;
    }

    _polytope.add(osg::Plane(1.0, 0.0, 0.0, -xMin));
    _polytope.add(osg::Plane(-1.0,0.0 ,0.0, xMax));
    _polytope.add(osg::Plane(0.0, 1.0, 0.0,-yMin));
    _polytope.add(osg::Plane(0.0,-1.0,0.0, yMax));
    _polytope.add(osg::Plane(0.0,0.0,1.0, zNear));

    _referencePlane = _polytope.getPlaneList().back();
}

Intersector* PolytopeIntersector::clone(osgUtil::IntersectionVisitor& iv)
{
    if (_coordinateFrame==MODEL && iv.getModelMatrix()==0)
    {
        osg::ref_ptr<PolytopeIntersector> pi = new PolytopeIntersector(_polytope);
        pi->_parent = this;
        pi->_intersectionLimit = this->_intersectionLimit;
        pi->_primitiveMask = this->_primitiveMask;
        pi->_referencePlane = this->_referencePlane;
        pi->setPrecisionHint(getPrecisionHint());
        return pi.release();
    }

    // 计算将此相交器从其坐标框架转换到局部MODEL坐标框架的矩阵
    // 场景图中的几何体将始终位于MODEL坐标框架中
    osg::Matrix matrix;
    switch (_coordinateFrame)
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

    osg::Polytope transformedPolytope;
    transformedPolytope.setAndTransformProvidingInverse(_polytope, matrix);

    osg::ref_ptr<PolytopeIntersector> pi = new PolytopeIntersector(transformedPolytope);
    pi->_parent = this;
    pi->_intersectionLimit = this->_intersectionLimit;
    pi->_primitiveMask = this->_primitiveMask;
    pi->_referencePlane = this->_referencePlane;
    pi->_referencePlane.transformProvidingInverse(matrix);
    pi->setPrecisionHint(getPrecisionHint());
    return pi.release();
}

bool PolytopeIntersector::enter(const osg::Node& node)
{
    if (reachedLimit()) return false;
    return !node.isCullingActive() || _polytope.contains( node.getBound() );
}


void PolytopeIntersector::leave()
{
    // 什么也不做
}



void PolytopeIntersector::intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable)
{
    if (reachedLimit()) return;

    if ( !_polytope.contains( drawable->getBoundingBox() ) ) return;

    osg::ref_ptr<PolytopeIntersectorUtils::Settings> settings = new PolytopeIntersectorUtils::Settings;
    settings->_polytopeIntersector = this;
    settings->_iv = &iv;
    settings->_drawable = drawable;
    settings->_limitOneIntersection = (_intersectionLimit == LIMIT_ONE_PER_DRAWABLE || _intersectionLimit == LIMIT_ONE);
    settings->_primitiveMask = _primitiveMask;

    osg::KdTree* kdTree = iv.getUseKdTreeWhenAvailable() ? dynamic_cast<osg::KdTree*>(drawable->getShape()) : 0;

    if (getPrecisionHint()==USE_DOUBLE_CALCULATIONS)
    {
        osg::TemplatePrimitiveFunctor<PolytopeIntersectorUtils::IntersectFunctor<osg::Vec3d> > intersector;
        intersector._settings = settings;

        if (kdTree) kdTree->intersect(intersector, kdTree->getNode(0));
        else drawable->accept(intersector);
    }
    else
    {
        osg::TemplatePrimitiveFunctor<PolytopeIntersectorUtils::IntersectFunctor<osg::Vec3f> > intersector;
        intersector._settings = settings;

        if (kdTree) kdTree->intersect(intersector, kdTree->getNode(0));
        else drawable->accept(intersector);
    }
}


void PolytopeIntersector::reset()
{
    Intersector::reset();

    _intersections.clear();
} 