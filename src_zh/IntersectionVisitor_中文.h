/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/

#ifndef OSGUTIL_INTERSECTIONVISITOR
#define OSGUTIL_INTERSECTIONVISITOR 1

#include <osg/NodeVisitor>
#include <osg/Drawable>
#include <osgUtil/Export>

#include <list>

namespace osgUtil
{

// 前向声明，允许Intersector引用它
class IntersectionVisitor;

/** 实现自定义相交技术的纯虚基类。
  * 要实现特定的相交技术，必须重写所有纯虚方法，
  * 如何执行此操作的具体示例可以在LineSegmentIntersector中看到。 */
class Intersector : public osg::Referenced
{
    public:

        enum CoordinateFrame
        {
            WINDOW,     // 窗口坐标系
            PROJECTION, // 投影坐标系
            VIEW,       // 视图坐标系
            MODEL       // 模型坐标系
        };

        enum IntersectionLimit
        {
            NO_LIMIT,               // 无限制
            LIMIT_ONE_PER_DRAWABLE, // 每个可绘制对象限制一个
            LIMIT_ONE,              // 限制一个
            LIMIT_NEAREST           // 限制最近的
        };

        Intersector(CoordinateFrame cf=MODEL, IntersectionLimit il=NO_LIMIT):
            _coordinateFrame(cf),
            _intersectionLimit(il),
            _disabledCount(0),
            _precisionHint(USE_DOUBLE_CALCULATIONS) {}

        /** 设置坐标框架 */
        void setCoordinateFrame(CoordinateFrame cf) { _coordinateFrame = cf; }

        /** 获取坐标框架 */
        CoordinateFrame getCoordinateFrame() const { return _coordinateFrame; }

        /** 设置相交限制 */
        void setIntersectionLimit(IntersectionLimit limit) { _intersectionLimit = limit; }

        /** 获取相交限制 */
        IntersectionLimit getIntersectionLimit() const { return _intersectionLimit; }

        /** 克隆相交器实例 */
        virtual Intersector* clone(osgUtil::IntersectionVisitor& iv) = 0;

        /**派生的相交器也就是要定义这部分，
            enter快熟判断剔除节点，且能做一些可以递归变换的操作，
            在leave里恢复，
            instersect来做详细的相交判断*/
        /** 进入节点时的判断 */
        virtual bool enter(const osg::Node& node) = 0;

        /** 离开节点时的操作 */
        virtual void leave() = 0;

        /** 与可绘制对象进行相交检测 */
        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable) = 0;

        /** 重置相交器状态 */
        virtual void reset() { _disabledCount = 0; }

        /** 检查是否包含相交结果 */
        virtual bool containsIntersections() = 0;

        /** 是否被禁用 */
        inline bool disabled() const { return _disabledCount!=0; }

        /** 增加禁用计数 */
        inline void incrementDisabledCount() { ++_disabledCount; }

        /** 减少禁用计数 */
        inline void decrementDisabledCount() { if (_disabledCount>0) --_disabledCount; }

        /** 是否达到限制 */
        inline bool reachedLimit() { return _intersectionLimit == LIMIT_ONE && containsIntersections(); }

        /** 内部相交计算中使用的精度提示 */
        enum PrecisionHint
        {
            USE_DOUBLE_CALCULATIONS,  // 使用双精度计算
            USE_FLOAT_CALCULATIONS    // 使用单精度计算
        };

        /** 设置相交计算中使用的精度提示 */
        void setPrecisionHint(PrecisionHint hint) { _precisionHint = hint; }

        /** 获取相交计算中应使用的精度提示 */
        PrecisionHint getPrecisionHint() const { return _precisionHint; }

protected:

        CoordinateFrame   _coordinateFrame;   // 坐标框架
        IntersectionLimit _intersectionLimit; // 相交限制
        unsigned int      _disabledCount;     // 禁用计数
        PrecisionHint     _precisionHint;     // 精度提示
};


/** 用于传递多个相交器通过场景图的具体类。
  * 需要与IntersectionVisitor配合使用。 */
class OSGUTIL_EXPORT IntersectorGroup : public Intersector
{
    public:

        IntersectorGroup();

        /** 添加一个相交器 */
        void addIntersector(Intersector* intersector);

        typedef std::vector< osg::ref_ptr<Intersector> > Intersectors;

        /** 获取相交器列表 */
        Intersectors& getIntersectors() { return _intersectors; }

        /** 清除相交器列表 */
        void clear();

    public:

        /** 克隆相交器实例 */
        virtual Intersector* clone(osgUtil::IntersectionVisitor& iv);

        /** 进入节点时的判断 */
        virtual bool enter(const osg::Node& node);

        /** 离开节点时的操作 */
        virtual void leave();

        /** 与可绘制对象进行相交检测 */
        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable);

        /** 重置相交器状态 */
        virtual void reset();

        /** 检查是否包含相交结果 */
        virtual bool containsIntersections();

    protected:

        Intersectors _intersectors;  // 相交器列表

};

/** IntersectionVisitor用于测试与场景的相交，使用通用的osgUtil::Intersector遍历场景并对场景进行测试。
  * 要实现不同类型的相交技术，需要实现osgUtil::Intersector的自定义版本，
  * 然后将构造的相交器传递给IntersectionVisitor。*/
class OSGUTIL_EXPORT IntersectionVisitor : public osg::NodeVisitor
{
    public:

        /** 用于实现外部文件读取的回调，允许分页数据库的支持集成到IntersectionVisitor中。
          * 具体实现可以在osgDB中找到。
          * 注意，这种松散耦合方法是必需的，因为osgUtil独立于实现文件读取的osgDB，
          * 而osgDB本身依赖于osgUtil，因此更紧密的集成会导致循环依赖。*/
        struct ReadCallback : public osg::Referenced
        {
            virtual osg::ref_ptr<osg::Node> readNodeFile(const std::string& filename) = 0;
        };


        IntersectionVisitor(Intersector* intersector=0, ReadCallback* readCallback=0);

        META_NodeVisitor(osgUtil, IntersectionVisitor)

        /** 如果Object是IntersectionVisitor，则将'this'转换为osgUtil::IntersectionVisitor指针，否则返回0。
          * 等价于dynamic_cast<osgUtil::IntersectionVisitor*>(this)。*/
        virtual osgUtil::IntersectionVisitor* asIntersectionVisitor() { return this; }

        /** 如果Object是IntersectionVisitor，则将'const this'转换为const osgUtil::IntersectionVisitor指针，否则返回0。
          * 等价于dynamic_cast<const osgUtil::IntersectionVisitor*>(this)。*/
        virtual const osgUtil::IntersectionVisitor* asIntersectionVisitor() const { return this; }

        /** 重置访问者状态 */
        virtual void reset();

        /** 设置将用于与场景相交并存储任何碰撞的相交器 */
        void setIntersector(Intersector* intersector);

        /** 获取将用于与场景相交并存储任何碰撞的相交器 */
        Intersector* getIntersector() { return _intersectorStack.empty() ? 0 : _intersectorStack.front().get(); }

        /** 获取将用于与场景相交并存储任何碰撞的常量相交器 */
        const Intersector* getIntersector() const { return _intersectorStack.empty() ? 0 : _intersectorStack.front().get(); }


        /** 设置相交器在场景图中找到KdTree时是否应该使用KdTree */
        void setUseKdTreeWhenAvailable(bool useKdTrees) { _useKdTreesWhenAvailable = useKdTrees; }

        /** 获取相交器是否应该使用KdTree */
        bool getUseKdTreeWhenAvailable() const { return _useKdTreesWhenAvailable; }

        /** 设置是否进行虚拟遍历 */
        void setDoDummyTraversal(bool dummy) { _dummyTraversal = dummy; }
        /** 获取是否进行虚拟遍历 */
        bool getDoDummyTraversal() const { return _dummyTraversal; }


        /** 设置读取回调 */
        void setReadCallback(ReadCallback* rc) { _readCallback = rc; }

        /** 获取读取回调 */
        ReadCallback* getReadCallback() { return _readCallback.get(); }

        /** 获取常量读取回调 */
        const ReadCallback* getReadCallback() const { return _readCallback.get(); }


        /** 窗口矩阵堆栈操作 */
        void pushWindowMatrix(osg::RefMatrix* matrix) { _windowStack.push_back(matrix); _eyePointDirty = true; }
        void pushWindowMatrix(osg::Viewport* viewport) { _windowStack.push_back(new osg::RefMatrix( viewport->computeWindowMatrix()) ); _eyePointDirty = true; }
        void popWindowMatrix() { _windowStack.pop_back(); _eyePointDirty = true; }
        osg::RefMatrix* getWindowMatrix() { return _windowStack.empty() ? 0 :  _windowStack.back().get(); }
        const osg::RefMatrix* getWindowMatrix() const { return _windowStack.empty() ? 0 :  _windowStack.back().get(); }

        /** 投影矩阵堆栈操作 */
        void pushProjectionMatrix(osg::RefMatrix* matrix) { _projectionStack.push_back(matrix); _eyePointDirty = true; }
        void popProjectionMatrix() { _projectionStack.pop_back(); _eyePointDirty = true; }
        osg::RefMatrix* getProjectionMatrix() { return _projectionStack.empty() ? 0 :  _projectionStack.back().get(); }
        const osg::RefMatrix* getProjectionMatrix() const { return _projectionStack.empty() ? 0 :  _projectionStack.back().get(); }

        /** 视图矩阵堆栈操作 */
        void pushViewMatrix(osg::RefMatrix* matrix) { _viewStack.push_back(matrix); _eyePointDirty = true; }
        void popViewMatrix() { _viewStack.pop_back(); _eyePointDirty = true; }
        osg::RefMatrix* getViewMatrix() { return _viewStack.empty() ? 0 :  _viewStack.back().get(); }
        const osg::RefMatrix* getViewMatrix() const { return _viewStack.empty() ? 0 :  _viewStack.back().get(); }

        /** 模型矩阵堆栈操作 */
        void pushModelMatrix(osg::RefMatrix* matrix) { _modelStack.push_back(matrix); _eyePointDirty = true; }
        void popModelMatrix() { _modelStack.pop_back(); _eyePointDirty = true; }
        osg::RefMatrix* getModelMatrix() { return _modelStack.empty() ? 0 :  _modelStack.back().get(); }
        const osg::RefMatrix* getModelMatrix() const { return _modelStack.empty() ? 0 :  _modelStack.back().get(); }


        /** 设置用于需要眼点来定位自己的节点（如告示牌）的参考眼点 */
        void setReferenceEyePoint(const osg::Vec3& ep) { _referenceEyePoint = ep; _eyePointDirty = true; }

        /** 获取参考眼点 */
        const osg::Vec3& getReferenceEyePoint() const { return _referenceEyePoint; }

        /** 设置参考眼点的坐标框架 */
        void setReferenceEyePointCoordinateFrame(Intersector::CoordinateFrame cf) { _referenceEyePointCoordinateFrame = cf; }

        /** 获取参考眼点的坐标框架 */
        Intersector::CoordinateFrame getReferenceEyePointCoordinateFrame() const { return _referenceEyePointCoordinateFrame; }


        /** 获取给定遍历点的局部坐标框架中的眼点 */
        virtual osg::Vec3 getEyePoint() const;

        enum LODSelectionMode
        {
            USE_HIGHEST_LEVEL_OF_DETAIL,         // 使用最高详细级别
            USE_EYE_POINT_FOR_LOD_LEVEL_SELECTION // 使用眼点进行LOD级别选择
        };

        /** 设置LOD选择方案 */
        void setLODSelectionMode(LODSelectionMode mode) { _lodSelectionMode = mode; }

        /** 获取LOD选择方案 */
        LODSelectionMode getLODSelectionMode() const { return _lodSelectionMode; }

         /** 获取从点到眼点的距离，距离值在局部坐标系中。
          * 在进行LOD计算时使用伪眼点（上述）计算。 */
        virtual float getDistanceToEyePoint(const osg::Vec3& pos, bool withLODScale) const;

    public:

        /** 各种节点类型的apply方法 */
        virtual void apply(osg::Node& node);
        virtual void apply(osg::Drawable& drawable);
        virtual void apply(osg::Geode& geode);
        virtual void apply(osg::Billboard& geode);
        virtual void apply(osg::Group& group);
        virtual void apply(osg::LOD& lod);
        virtual void apply(osg::PagedLOD& lod);
        virtual void apply(osg::Transform& transform);
        virtual void apply(osg::Projection& projection);
        virtual void apply(osg::Camera& camera);

    protected:

        /** 用栈来代替函数栈递归，因为矩阵不好恢复 */
        /** 内联辅助方法 */
        inline bool enter(const osg::Node& node) { return _intersectorStack.empty() ? false : _intersectorStack.back()->enter(node); }
        inline void leave() { _intersectorStack.back()->leave(); }
        inline void intersect(osg::Drawable* drawable) { _intersectorStack.back()->intersect(*this, drawable); }
        inline void push_clone() { _intersectorStack.push_back ( _intersectorStack.front()->clone(*this) ); }
        inline void pop_clone() { if (_intersectorStack.size()>=2) _intersectorStack.pop_back(); }

        typedef std::list< osg::ref_ptr<Intersector> > IntersectorStack;
        IntersectorStack _intersectorStack;              // 相交器堆栈

        bool _useKdTreesWhenAvailable;                   // 可用时是否使用KdTree
        bool _dummyTraversal;                            // 是否进行虚拟遍历

        osg::ref_ptr<ReadCallback> _readCallback;       // 读取回调

        typedef std::list< osg::ref_ptr<osg::RefMatrix> > MatrixStack;
        MatrixStack _windowStack;                        // 窗口矩阵堆栈
        MatrixStack _projectionStack;                    // 投影矩阵堆栈
        MatrixStack _viewStack;                          // 视图矩阵堆栈
        MatrixStack _modelStack;                         // 模型矩阵堆栈

        osg::Vec3                       _referenceEyePoint;               // 参考眼点
        Intersector::CoordinateFrame    _referenceEyePointCoordinateFrame; // 参考眼点坐标框架
        LODSelectionMode                _lodSelectionMode;                // LOD选择模式

        mutable bool                    _eyePointDirty;                   // 眼点是否脏标志
        mutable osg::Vec3               _eyePoint;                        // 眼点
};

}

#endif 