/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/

#ifndef OSGUTIL_POLYTOPEINTERSECTOR
#define OSGUTIL_POLYTOPEINTERSECTOR 1

#include <osgUtil/IntersectionVisitor>

namespace osgUtil
{

/** 实现与场景图进行多面体相交的具体类。
  * 需要与IntersectionVisitor配合使用。 */
class OSGUTIL_EXPORT PolytopeIntersector : public Intersector
{
    public:

        /** 使用在MODEL坐标系中指定的多面体构造PolytopeIntersector。*/
        PolytopeIntersector(const osg::Polytope& polytope);

        /** 使用在指定坐标框架中指定的多面体构造PolytopeIntersector。*/
        PolytopeIntersector(CoordinateFrame cf, const osg::Polytope& polytope);

        /** 支持WINDOW或PROJECTION坐标拾取的便捷构造函数
          * 在WINDOW坐标系（裁剪空间立方体）中，创建一个五面多面体框，前面在0.0处，
          * 侧面围绕框xMin、yMin、xMax、yMax。
          * 在PROJECTION坐标系（裁剪空间立方体）中，创建一个五面多面体框，前面在-1处，
          * 侧面围绕框xMin、yMin、xMax、yMax。
          * 在VIEW和MODEL坐标系中，创建一个五面多面体框，前面在0.0处，
          * 侧面围绕框xMin、yMin、xMax、yMax。*/
        PolytopeIntersector(CoordinateFrame cf, double xMin, double yMin, double xMax, double yMax);

        /** 获取相交器使用的多面体。*/
        osg::Polytope& getPolytope() { return _polytope;}

        /** 获取相交器使用的常量多面体。*/
        const osg::Polytope& getPolytope() const { return _polytope;}


        typedef osg::Plane::Vec3_type Vec3_type;

        struct Intersection
        {
            Intersection():
                distance(0.0),
                maxDistance(0.0),
                numIntersectionPoints(0),
                primitiveIndex(0) {}

            bool operator < (const Intersection& rhs) const
            {
                if (distance < rhs.distance) return true;
                if (rhs.distance < distance) return false;
                if (primitiveIndex < rhs.primitiveIndex) return true;
                if (rhs.primitiveIndex < primitiveIndex) return false;
                if (nodePath < rhs.nodePath) return true;
                if (rhs.nodePath < nodePath ) return false;
                return (drawable < rhs.drawable);
            }

            enum { MaxNumIntesectionPoints=6 };

            double                          distance;     ///< 与参考平面的距离
            double                          maxDistance;  ///< 相交点与参考平面的最大距离
            osg::NodePath                   nodePath;     ///< 到相交节点的路径
            osg::ref_ptr<osg::Drawable>     drawable;     ///< 相交的可绘制对象
            osg::ref_ptr<osg::RefMatrix>    matrix;       ///< 变换矩阵
            Vec3_type                       localIntersectionPoint;  ///< 所有相交点的中心
            unsigned int                    numIntersectionPoints;   ///< 相交点数量
            Vec3_type                       intersectionPoints[MaxNumIntesectionPoints];  ///< 相交点数组
            unsigned int                    primitiveIndex; ///< 图元索引
        };

        typedef std::set<Intersection> Intersections;

        /** 插入相交结果 */
        inline void insertIntersection(const Intersection& intersection) { getIntersections().insert(intersection); }

        /** 获取相交结果集合 */
        inline Intersections& getIntersections() { return _parent ? _parent->_intersections : _intersections; }

        /** 获取第一个相交结果 */
        inline Intersection getFirstIntersection() { Intersections& intersections = getIntersections(); return intersections.empty() ? Intersection() : *(intersections.begin()); }


        /// 用于指定要检查的图元类型的维度枚举
        enum {
            POINT_PRIMITIVES = (1<<0),      /// 检查点
            LINE_PRIMITIVES = (1<<1),       /// 检查线段
            TRIANGLE_PRIMITIVES = (1<<2),   /// 检查三角形和其他可分解为三角形的图元（如四边形、多边形）
            ALL_PRIMITIVES = ( POINT_PRIMITIVES | LINE_PRIMITIVES | TRIANGLE_PRIMITIVES )
        };

        /** 设置应该进行相交测试的图元类型。*/
        void setPrimitiveMask(unsigned int mask) { _primitiveMask = mask; }

        /** 获取应该进行相交测试的图元类型。*/
        unsigned int getPrimitiveMask() const { return _primitiveMask; }

        /** 设置用于排序相交结果的平面。
         * 相交结果按localIntersectionPoint与参考平面的距离进行排序。
         * 参考平面的默认值是多面体的最后一个平面。
         */
        inline void setReferencePlane(const osg::Plane& plane) { _referencePlane = plane; }

        /** 获取参考平面 */
        inline const osg::Plane& getReferencePlane() const { return _referencePlane; }

#ifdef OSG_USE_DEPRECATED_API
        enum {
            DimZero = POINT_PRIMITIVES,    /// 已弃用，使用POINT_PRIMITIVES
            DimOne = LINE_PRIMITIVES,      /// 已弃用，使用LINE_PRIMITIVES
            DimTwo = TRIANGLE_PRIMITIVES,  /// 已弃用，使用TRIANGLE_PRIMITIVES
            AllDims =  ALL_PRIMITIVES      /// 已弃用，使用ALL_PRIMITIVES
        };

        /** 已弃用，使用setPrimitiveMask() */
        inline void setDimensionMask(unsigned int mask) { setPrimitiveMask(mask); }

        /** 已弃用，使用getPrimitiveMask() */
        inline unsigned int getDimensionMask() const { return getPrimitiveMask(); }
#endif

public:

        /** 克隆相交器实例 */
        virtual Intersector* clone(osgUtil::IntersectionVisitor& iv);

        /** 进入节点时的判断，决定是否继续遍历该节点 */
        virtual bool enter(const osg::Node& node);

        /** 离开节点时的清理操作 */
        virtual void leave();

        /** 与可绘制对象进行相交检测 */
        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable);

        /** 重置相交器状态 */
        virtual void reset();

        /** 检查是否包含相交结果 */
        virtual bool containsIntersections() { return !getIntersections().empty(); }

    protected:

        PolytopeIntersector* _parent;       // 父相交器指针

        osg::Polytope _polytope;            // 多面体

        unsigned int _primitiveMask;        ///< 应该检查的维度掩码
        osg::Plane _referencePlane;         ///< 用于排序相交的平面

        Intersections _intersections;       // 相交结果集合

};

}

#endif 