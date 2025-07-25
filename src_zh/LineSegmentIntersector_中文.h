/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/

#ifndef OSGUTIL_LINESEGMENTINTERSECTOR
#define OSGUTIL_LINESEGMENTINTERSECTOR 1

#include <osgUtil/IntersectionVisitor>

namespace osgUtil
{

/** 实现与场景图进行线段相交的具体类。
  * 需要与IntersectionVisitor配合使用。 */
class OSGUTIL_EXPORT LineSegmentIntersector : public Intersector
{
    public:

        /** 构造一个在MODEL坐标系中运行在指定起点和终点之间的LineSegmentIntersector。 */
        LineSegmentIntersector(const osg::Vec3d& start, const osg::Vec3d& end);

        /** 构造一个在指定坐标框架中运行在指定起点和终点之间的LineSegmentIntersector。 */
        LineSegmentIntersector(CoordinateFrame cf, const osg::Vec3d& start, const osg::Vec3d& end, LineSegmentIntersector* parent = NULL,
                               osgUtil::Intersector::IntersectionLimit intersectionLimit = osgUtil::Intersector::NO_LIMIT);

        /** 支持WINDOW或PROJECTION坐标拾取的便捷构造函数
          * 在WINDOW坐标系中，创建起点值为(x,y,0)，终点值为(x,y,1)。
          * 在PROJECTION坐标系（裁剪空间立方体）中，创建起点值为(x,y,-1)，终点值为(x,y,1)。
          * 在VIEW和MODEL坐标系中，创建起点值为(x,y,0)，终点值为(x,y,1)。*/
        LineSegmentIntersector(CoordinateFrame cf, double x, double y);

        struct OSGUTIL_EXPORT Intersection
        {
            Intersection():
                ratio(-1.0),
                primitiveIndex(0) {}

            bool operator < (const Intersection& rhs) const { return ratio < rhs.ratio; }

            typedef std::vector<unsigned int>   IndexList;   // 索引列表类型
            typedef std::vector<double>         RatioList;   // 比例列表类型

            double                          ratio;                      // 相交点在线段上的比例位置(0.0-1.0)
            osg::NodePath                   nodePath;                  // 到相交节点的路径
            osg::ref_ptr<osg::Drawable>     drawable;                  // 相交的可绘制对象
            osg::ref_ptr<osg::RefMatrix>    matrix;                    // 变换矩阵
            osg::Vec3d                      localIntersectionPoint;    // 局部坐标系中的相交点
            osg::Vec3                       localIntersectionNormal;   // 局部坐标系中的相交法线
            IndexList                       indexList;                 // 顶点索引列表
            RatioList                       ratioList;                 // 重心坐标比例列表
            unsigned int                    primitiveIndex;            // 图元索引

            /** 获取局部坐标系中的相交点 */
            const osg::Vec3d& getLocalIntersectPoint() const { return localIntersectionPoint; }
            /** 获取世界坐标系中的相交点 */
            osg::Vec3d getWorldIntersectPoint() const { return matrix.valid() ? localIntersectionPoint * (*matrix) : localIntersectionPoint; }

            /** 获取局部坐标系中的相交法线 */
            const osg::Vec3& getLocalIntersectNormal() const { return localIntersectionNormal; }
            /** 获取世界坐标系中的相交法线 */
            osg::Vec3 getWorldIntersectNormal() const { return matrix.valid() ? osg::Matrix::transform3x3(osg::Matrix::inverse(*matrix),localIntersectionNormal) : localIntersectionNormal; }

            /** 将相交点映射到所相交对象上的纹理的便捷函数。
             *  当对象上有可用纹理时，返回Texture指针和被击中对象的纹理坐标，否则返回NULL。*/
            osg::Texture* getTextureLookUp(osg::Vec3& tc) const;

        };

        typedef std::multiset<Intersection> Intersections;  // 相交结果集合类型

        /** 插入相交结果 */
        inline void insertIntersection(const Intersection& intersection) { getIntersections().insert(intersection); }

        /** 获取相交结果集合 */
        inline Intersections& getIntersections() { return _parent ? _parent->_intersections : _intersections; }

        /** 获取第一个相交结果 */
        inline Intersection getFirstIntersection() { Intersections& intersections = getIntersections(); return intersections.empty() ? Intersection() : *(intersections.begin()); }

        /** 设置线段起点 */
        inline void setStart(const osg::Vec3d& start) { _start = start; }
        /** 获取线段起点 */
        inline const osg::Vec3d& getStart() const { return _start; }

        /** 设置线段终点 */
        inline void setEnd(const osg::Vec3d& end) { _end = end; }
        /** 获取线段终点 */
        inline const osg::Vec3d& getEnd() const { return _end; }

    public:

        /** 克隆相交器实例 */
        virtual Intersector* clone(osgUtil::IntersectionVisitor& iv);

        /** 进入节点时的判断，决定是否继续遍历该节点 */
        virtual bool enter(const osg::Node& node);

        /** 离开节点时的清理操作 */
        virtual void leave();

        /** 与可绘制对象进行相交检测 */
        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable);

        /** 与可绘制对象进行相交检测（指定线段起点和终点） */
        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable,
                               const osg::Vec3d& s, const osg::Vec3d& e);

        /** 重置相交器状态 */
        virtual void reset();

        /** 检查是否包含相交结果 */
        virtual bool containsIntersections() { return !getIntersections().empty(); }

        /** 计算将父相交器（通常是当前相交器）的局部坐标系转换为
            子相交器的子坐标系的矩阵。
            cf参数指示父相交器的坐标框架。 */
        static osg::Matrix getTransformation(osgUtil::IntersectionVisitor& iv, CoordinateFrame cf);

protected:

        /** 检查线段是否与包围球相交 */
        bool intersects(const osg::BoundingSphere& bs);
        /** 相交并裁剪到包围盒 */
        bool intersectAndClip(osg::Vec3d& s, osg::Vec3d& e,const osg::BoundingBox& bb);

        LineSegmentIntersector* _parent;    // 父相交器指针

        osg::Vec3d  _start;                 // 线段起点
        osg::Vec3d  _end;                   // 线段终点

        Intersections _intersections;       // 相交结果集合

};

}

#endif 