/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/

#ifndef OSG_NODEVISITOR
#define OSG_NODEVISITOR 1

#include <osg/Node>
#include <osg/Matrix>
#include <osg/FrameStamp>

#include <osg/ValueMap>
#include <osg/ValueStack>


namespace osgUtil { class UpdateVisitor; class CullVisitor; class IntersectionVisitor; }
namespace osgGA { class EventVisitor; }

namespace osg {

class Billboard;
class ClearNode;
class ClipNode;
class CoordinateSystemNode;
class Geode;
class Group;
class LightSource;
class LOD;
class MatrixTransform;
class OccluderNode;
class OcclusionQueryNode;
class PagedLOD;
class PositionAttitudeTransform;
class AutoTransform;
class MultiViewAutoTransform;
class Projection;
class ProxyNode;
class Sequence;
class Switch;
class TexGenNode;
class Transform;
class Camera;
class CameraView;
class Drawable;
class Geometry;
class CullStack;





const unsigned int UNINITIALIZED_FRAME_NUMBER=0xffffffff;

#define META_NodeVisitor(library, name) \
        virtual const char* libraryName() const { return #library; }\
        virtual const char* className() const { return #name; }

/** 用于对osg::Node进行类型安全操作的访问者。
    基于GOF的访问者模式。NodeVisitor对于在场景图中的节点上开发类型安全操作
    (按照访问者模式)非常有用，并且还支持可选的场景图遍历，允许一次对整个场景
    应用操作。访问者模式使用双重分发技术作为调用NodeVisitor适当apply(..)
    方法的机制。要使用此功能，必须使用在每个Node子类中扩展的Node::accept(NodeVisitor)，
    而不是直接使用NodeVisitor apply。所以使用root->accept(myVisitor);
    而不是myVisitor.apply(*root)。后一种方法将绕过双重分发，
    不会调用适当的NodeVisitor::apply(..)方法。 */
class OSG_EXPORT NodeVisitor : public virtual Object
{
    public:

        enum TraversalMode
        {
            TRAVERSE_NONE,              // 不遍历
            TRAVERSE_PARENTS,           // 遍历父节点
            TRAVERSE_ALL_CHILDREN,      // 遍历所有子节点
            TRAVERSE_ACTIVE_CHILDREN    // 遍历活动子节点
        };

        enum VisitorType
        {
            NODE_VISITOR = 0,           // 节点访问者
            UPDATE_VISITOR,             // 更新访问者
            EVENT_VISITOR,              // 事件访问者
            COLLECT_OCCLUDER_VISITOR,   // 收集遮挡器访问者
            CULL_VISITOR,               // 裁剪访问者
            INTERSECTION_VISITOR        // 相交测试访问者
        };

        NodeVisitor(TraversalMode tm=TRAVERSE_NONE);

        NodeVisitor(VisitorType type,TraversalMode tm=TRAVERSE_NONE);

        NodeVisitor(const NodeVisitor& nv, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        virtual ~NodeVisitor();

        META_Object(osg, NodeVisitor)

        /** 如果Object是NodeVisitor，则将'this'转换为NodeVisitor指针，否则返回0。
          * 等价于dynamic_cast<NodeVisitor*>(this)。*/
        virtual NodeVisitor* asNodeVisitor() { return this; }

        /** 如果Object是NodeVisitor，则将'const this'转换为const NodeVisitor指针，否则返回0。
          * 等价于dynamic_cast<const NodeVisitor*>(this)。*/
        virtual const NodeVisitor* asNodeVisitor() const { return this; }

        /** 如果Object是osgUtil::UpdateVisitor，则将'this'转换为osgUtil::UpdateVisitor指针，否则返回0。
          * 等价于dynamic_cast<osgUtil::UpdateVisitor*>(this)。*/
        virtual osgUtil::UpdateVisitor* asUpdateVisitor() { return 0; }

        /** 如果Object是osgUtil::UpdateVisitor，则将'const this'转换为const osgUtil::UpdateVisitor指针，否则返回0。
          * 等价于dynamic_cast<const osgUtil::UpdateVisitor*>(this)。*/
        virtual const osgUtil::UpdateVisitor* asUpdateVisitor() const { return 0; }

        /** 如果Object是osgUtil::CullVisitor，则将'this'转换为osgUtil::CullVisitor指针，否则返回0。
          * 等价于dynamic_cast<osgUtil::CullVisitor*>(this)。*/
        virtual osgUtil::CullVisitor* asCullVisitor() { return 0; }

        /** 如果Object是osgUtil::CullVisitor，则将'const this'转换为const osgUtil::CullVisitor指针，否则返回0。
          * 等价于dynamic_cast<const osgUtil::CullVisitor*>(this)。*/
        virtual const osgUtil::CullVisitor* asCullVisitor() const { return 0; }

        /** 如果Object是osgGA::EventVisitor，则将'this'转换为osgGA::EventVisitor指针，否则返回0。
          * 等价于dynamic_cast<osgGA::EventVisitor*>(this)。*/
        virtual osgGA::EventVisitor* asEventVisitor() { return 0; }

        /** 如果Object是osgGA::EventVisitor，则将'const this'转换为const osgGA::EventVisitor指针，否则返回0。
          * 等价于dynamic_cast<const osgGA::EventVisitor*>(this)。*/
        virtual const osgGA::EventVisitor* asEventVisitor() const { return 0; }

        /** 如果Object是osgUtil::IntersectionVisitor，则将'this'转换为osgUtil::IntersectionVisitor指针，否则返回0。
          * 等价于dynamic_cast<osgUtil::IntersectionVisitor*>(this)。*/
        virtual osgUtil::IntersectionVisitor* asIntersectionVisitor() { return 0; }

        /** 如果Object是osgUtil::IntersectionVisitor，则将'const this'转换为const osgUtil::IntersectionVisitor指针，否则返回0。
          * 等价于dynamic_cast<const osgUtil::IntersectionVisitor*>(this)。*/
        virtual const osgUtil::IntersectionVisitor* asIntersectionVisitor() const { return 0; }

        /** 如果Object是osg::CullStack，则将'this'转换为osg::CullStack指针，否则返回0。
          * 等价于dynamic_cast<osg::CullStack*>(this)。*/
        virtual osg::CullStack* asCullStack() { return 0; }

        /** 如果Object是osg::CullStack，则将'const this'转换为const osg::CullStack指针，否则返回0。
          * 等价于dynamic_cast<const osg::CullStack*>(this)。*/
        virtual const osg::CullStack* asCullStack() const { return 0; }



        /** 重置访问者的方法。如果您的访问者在遍历过程中累积状态，
            并且您计划重用访问者，这会很有用。
            要为下一次遍历刷新该状态：在每次遍历之前调用reset()。*/
        virtual void reset() {}


        /** 设置访问者类型，用于在场景遍历过程中区分不同的访问者，
          * 通常在Node::traverse()方法中使用，为不同类型的遍历/访问者选择相应的行为。*/
        inline void setVisitorType(VisitorType type) { _visitorType = type; }

        /** 获取访问者类型。*/
        inline VisitorType getVisitorType() const { return _visitorType; }

        /** 设置遍历编号。通常用于表示帧计数。*/
        inline void setTraversalNumber(unsigned int fn) { _traversalNumber = fn; }

        /** 获取遍历编号。通常用于表示帧计数。*/
        inline unsigned int getTraversalNumber() const { return _traversalNumber; }

        /** 设置与此遍历关联的FrameStamp。*/
        inline void setFrameStamp(FrameStamp* fs) { _frameStamp = fs; }

        /** 获取与此遍历关联的FrameStamp。*/
        inline const FrameStamp* getFrameStamp() const { return _frameStamp.get(); }


        /** 设置此NodeVisitor的TraversalMask。
          * TraversalMask由NodeVisitor::validNodeMask()方法使用，
          * 以确定是否对节点及其子图进行操作。
          * validNodeMask()在Node::accept()方法中任何调用NodeVisitor::apply()之前自动调用，
          * 只有在validNodeMask返回true时才调用apply()。注意，如果NodeVisitor::_traversalMask为0，
          * 则所有节点的所有操作都将被关闭。而将_traversalMask和_nodeMaskOverride都设置为0xffffffff
          * 将允许访问者对所有节点工作，无论它们自己的Node::_nodeMask状态如何。*/
        inline void setTraversalMask(Node::NodeMask mask) { _traversalMask = mask; }

        /** 获取TraversalMask。*/
        inline Node::NodeMask getTraversalMask() const { return _traversalMask; }

        /** 设置NodeMaskOverride掩码。
          * 在validNodeMask()中使用，通过将NodeVisitor::_nodeMaskOverride与Node自己的Node::_nodeMask进行OR运算
          * 来确定是否对节点或其子图进行操作。
          * 通常用于强制开启可能被其自己的Node::_nodeMask关闭的节点。*/
        inline void setNodeMaskOverride(Node::NodeMask mask) { _nodeMaskOverride = mask; }

        /** 获取NodeMaskOverride掩码。*/
        inline Node::NodeMask getNodeMaskOverride() const { return _nodeMaskOverride; }

        /** 由Node及其子类的Node::accept()方法调用的方法，如果结果为true，
          * 则用于剔除节点及其子图的操作。
          * 返回NodeVisitor::_traversalMask与NodeVisitor::_nodeMaskOverride和Node::_nodeMask之间按位OR的
          * 按位AND的结果是否为true。
          * _traversalMask的默认值为0xffffffff，_nodeMaskOverride为0x0，
          * osg::Node::_nodeMask为0xffffffff。 */
        inline bool validNodeMask(const osg::Node& node) const
        {
            return (getTraversalMask() & (getNodeMaskOverride() | node.getNodeMask()))!=0;
        }

        /** 设置Node::traverse()在决定遍历节点的哪些子节点时使用的遍历模式。
            如果通过setTraverseVisitor()附加了NodeVisitor，
            并且新模式不是TRAVERSE_VISITOR，则分离附加的访问者。
            默认模式为TRAVERSE_NONE。*/
        inline void setTraversalMode(TraversalMode mode) { _traversalMode = mode; }

        /** 获取遍历模式。*/
        inline TraversalMode getTraversalMode() const { return _traversalMode; }


        /** 设置用于存储可在一系列遍历中重用的值的ValueMap。 */
        inline void setValueMap(ValueMap* ps) { _valueMap = ps; }

        /** 获取ValueMap。 */
        inline ValueMap* getValueMap() { return _valueMap.get(); }

        /** 获取ValueMap。 */
        inline const ValueMap* getValueMap() const { return _valueMap.get(); }

        /** 获取ValueMap。 */
        inline ValueMap* getOrCreateValueMap()
        {
            if (!_valueMap) _valueMap = new ValueMap;
            return _valueMap.get();
        }

        /** 设置在遍历过程中用于堆叠值的ValueStack。 */
        inline void setValueStack(ValueStack* ps) { _valueStack = ps; }

        /** 获取ValueStack。 */
        inline ValueStack* getValueStack() { return _valueStack.get(); }

        /** 获取const ValueStack。 */
        inline const ValueStack* getValueStack() const { return _valueStack.get(); }

        /** 获取ValueStack。 */
        inline ValueStack* getOrCreateValueStack()
        {
            if (!_valueStack) _valueStack = new ValueStack;
            return _valueStack.get();
        }


        /** 处理节点遍历的方法。
            如果您打算使用访问者主动遍历场景图，
            请确保accept()方法调用此方法，除非它们直接处理遍历。*/
        inline void traverse(Node& node)
        {
            if (_traversalMode==TRAVERSE_PARENTS) node.ascend(*this);
            else if (_traversalMode!=TRAVERSE_NONE) node.traverse(*this);
        }

        /** 由osg::Node::accept()方法在调用NodeVisitor::apply(..)之前调用的方法。
          * 因此，列表的末尾将是apply(..)内当前正在访问的节点，
          * 列表的其余部分将是从最顶层节点向下到当前节点的节点的父级序列。
          * 注意，用户通常不会调用pushNodeOnPath()，因为它会被Node::accept()方法自动调用。*/
        inline void pushOntoNodePath(Node* node) { if (_traversalMode!=TRAVERSE_PARENTS) _nodePath.push_back(node); else _nodePath.insert(_nodePath.begin(),node); }

        /** 由osg::Node::accept()方法在调用NodeVisitor::apply(..)之后调用的方法。
          * 注意，用户通常不会调用popFromNodePath()，因为它会被Node::accept()方法自动调用。*/
        inline void popFromNodePath()            { if (_traversalMode!=TRAVERSE_PARENTS) _nodePath.pop_back(); else _nodePath.erase(_nodePath.begin()); }

        /** 从最顶层应用的节点到当前正在访问的节点，获取非const NodePath。*/
        NodePath& getNodePath() { return _nodePath; }

        /** 从最顶层应用的节点到当前正在访问的节点，获取const NodePath。*/
        const NodePath& getNodePath() const { return _nodePath; }

        /** 获取局部坐标中的眼点。
          * 注意，不是所有NodeVisitor都实现此方法，主要是裁剪访问者会实现。*/
        virtual osg::Vec3 getEyePoint() const { return Vec3(0.0f,0.0f,0.0f); }

        /** 获取局部坐标中的视点。
          * 注意，不是所有NodeVisitor都实现此方法，主要是裁剪访问者会实现。*/
        virtual osg::Vec3 getViewPoint() const { return getEyePoint(); }

        /** 获取从点到眼点的距离，距离值在局部坐标系中。
          * 注意，不是所有NodeVisitor都实现此方法，主要是裁剪访问者会实现。
          * 如果未实现getDistanceFromEyePoint(pos)，则返回默认值0.0。*/
        virtual float getDistanceToEyePoint(const Vec3& /*pos*/, bool /*useLODScale*/) const { return 0.0f; }

        /** 获取点到眼点的距离，距离值在眼坐标系中。
          * 注意，不是所有NodeVisitor都实现此方法，主要是裁剪访问者会实现。
          * 如果未实现getDistanceFromEyePoint(pos)，则返回默认值0.0。*/
        virtual float getDistanceFromEyePoint(const Vec3& /*pos*/, bool /*useLODScale*/) const { return 0.0f; }

        /** 获取从点到视点的距离，距离值在局部坐标系中。
          * 注意，不是所有NodeVisitor都实现此方法，主要是裁剪访问者会实现。
          * 如果未实现getDistanceToViewPoint(pos)，则返回默认值0.0。*/
        virtual float getDistanceToViewPoint(const Vec3& /*pos*/, bool /*useLODScale*/) const { return 0.0f; }

        virtual void apply(Drawable& drawable);
        virtual void apply(Geometry& geometry);

        virtual void apply(Node& node);

        virtual void apply(Geode& node);
        virtual void apply(Billboard& node);

        virtual void apply(Group& node);

        virtual void apply(ProxyNode& node);

        virtual void apply(Projection& node);

        virtual void apply(CoordinateSystemNode& node);

        virtual void apply(ClipNode& node);
        virtual void apply(TexGenNode& node);
        virtual void apply(LightSource& node);

        virtual void apply(Transform& node);
        virtual void apply(Camera& node);
        virtual void apply(CameraView& node);
        virtual void apply(MatrixTransform& node);
        virtual void apply(PositionAttitudeTransform& node);
        virtual void apply(AutoTransform& node);

        virtual void apply(Switch& node);
        virtual void apply(Sequence& node);
        virtual void apply(LOD& node);
        virtual void apply(PagedLOD& node);
        virtual void apply(ClearNode& node);
        virtual void apply(OccluderNode& node);
        virtual void apply(OcclusionQueryNode& node);


        /** 用于管理数据库分页的回调，例如由PagedLOD节点生成的。*/
        class DatabaseRequestHandler : public osg::Referenced
        {
        public:

            DatabaseRequestHandler():
                Referenced(true) {}

             virtual void requestNodeFile(const std::string& fileName, osg::NodePath& nodePath, float priority, const FrameStamp* framestamp, osg::ref_ptr<osg::Referenced>& databaseRequest, const osg::Referenced* options=0) = 0;

        protected:
            virtual ~DatabaseRequestHandler() {}
        };

        /** 设置数据库请求的处理器。*/
        void setDatabaseRequestHandler(DatabaseRequestHandler* handler) { _databaseRequestHandler = handler; }

        /** 获取数据库请求的处理器。*/
        DatabaseRequestHandler* getDatabaseRequestHandler() { return _databaseRequestHandler.get(); }

        /** 获取数据库请求的const处理器。*/
        const DatabaseRequestHandler* getDatabaseRequestHandler() const { return _databaseRequestHandler.get(); }


        /** 用于管理图像分页的回调，例如由PagedLOD节点生成的。*/
        class ImageRequestHandler : public osg::Referenced
        {
        public:

            ImageRequestHandler():
                Referenced(true) {}

            virtual double getPreLoadTime() const = 0;

            virtual osg::ref_ptr<osg::Image> readRefImageFile(const std::string& fileName, const osg::Referenced* options=0) = 0;

            virtual void requestImageFile(const std::string& fileName,osg::Object* attachmentPoint, int attachmentIndex, double timeToMergeBy, const FrameStamp* framestamp, osg::ref_ptr<osg::Referenced>& imageRequest, const osg::Referenced* options=0) = 0;

        protected:
            virtual ~ImageRequestHandler() {}
        };

        /** 设置图像请求的处理器。*/
        void setImageRequestHandler(ImageRequestHandler* handler) { _imageRequestHandler = handler; }

        /** 获取图像请求的处理器。*/
        ImageRequestHandler* getImageRequestHandler() { return _imageRequestHandler.get(); }

        /** 获取图像请求的const处理器。*/
        const ImageRequestHandler* getImageRequestHandler() const { return _imageRequestHandler.get(); }



    protected:

        VisitorType                     _visitorType;           // 访问者类型
        unsigned int                    _traversalNumber;       // 遍历编号

        ref_ptr<FrameStamp>             _frameStamp;            // 帧戳

        TraversalMode                   _traversalMode;         // 遍历模式
        Node::NodeMask                  _traversalMask;         // 遍历掩码
        Node::NodeMask                  _nodeMaskOverride;      // 节点掩码覆盖

        NodePath                        _nodePath;              // 节点路径

        ref_ptr<DatabaseRequestHandler> _databaseRequestHandler; // 数据库请求处理器
        ref_ptr<ImageRequestHandler>    _imageRequestHandler;    // 图像请求处理器

        osg::ref_ptr<ValueMap>          _valueMap;              // 值映射
        osg::ref_ptr<ValueStack>        _valueStack;            // 值堆栈
};


/** 用于辅助访问osg::Node数组的便利函数对象。*/
class NodeAcceptOp
{
    public:

        NodeAcceptOp(NodeVisitor& nv):_nv(nv) {}
        NodeAcceptOp(const NodeAcceptOp& naop):_nv(naop._nv) {}

        void operator () (Node* node) { node->accept(_nv); }
        void operator () (ref_ptr<Node> node) { node->accept(_nv); }

    protected:

        NodeAcceptOp& operator = (const NodeAcceptOp&) { return *this; }

        NodeVisitor& _nv;
};


class PushPopObject
{
public:


    PushPopObject(NodeVisitor* nv, const Referenced* key, Object* value):
        _valueStack(0),
        _key(0)
    {
        if (key)
        {
            _valueStack = nv->getOrCreateValueStack();
            _key = key;
            _valueStack->push(_key, value);
        }
    }

    PushPopObject(ValueStack* valueStack, const Referenced* key, Object* value):
        _valueStack(valueStack),
        _key(0)
    {
        if (_valueStack && key)
        {
            _key = key;
            _valueStack->push(_key, value);
        }
    }

    inline ~PushPopObject()
    {
        if (_valueStack) _valueStack->pop(_key);
    }

protected:
    ValueStack* _valueStack;
    const Referenced* _key;
};

class PushPopValue
{
public:


    template<typename T>
    PushPopValue(NodeVisitor* nv, const Referenced* key, const T& value):
        _valueStack(0),
        _key(0)
    {
        if (key)
        {
            _valueStack = nv->getOrCreateValueStack();
            _key = key;
            _valueStack->push(_key, value);
        }
    }

    template<typename T>
    PushPopValue(ValueStack* valueStack, const Referenced* key, const T& value):
        _valueStack(valueStack),
        _key(0)
    {
        if (_valueStack && key)
        {
            _key = key;
            _valueStack->push(_key, value);
        }
    }

    inline ~PushPopValue()
    {
        if (_valueStack) _valueStack->pop(_key);
    }

protected:
    ValueStack* _valueStack;
    const Referenced* _key;
};

template<>
inline ValueStack* getOrCreateUserObjectOfType<NodeVisitor,ValueStack>(NodeVisitor* nv)
{
    return nv->getOrCreateValueStack();
}

template<>
inline ValueMap* getOrCreateUserObjectOfType<NodeVisitor,ValueMap>(NodeVisitor* nv)
{
    return nv->getOrCreateValueMap();
}


}

#endif 