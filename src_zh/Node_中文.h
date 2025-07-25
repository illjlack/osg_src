/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/


#ifndef OSG_NODE
#define OSG_NODE 1

#include <osg/Object>
#include <osg/StateSet>
#include <osg/BoundingSphere>
#include <osg/BoundingBox>
#include <osg/Callback>

#include <string>
#include <vector>


// 前向声明 osgTerrain::Terrain 以便于 asTerrain() 方法的声明
namespace osgTerrain {
class Terrain;
}

namespace osg {

// 强制声明类以便于 as*() 方法的声明
class NodeVisitor;
class Drawable;
class Geometry;
class Group;
class Transform;
class Node;
class Switch;
class Geode;
class Camera;
class OccluderNode;

/** 节点指针向量，用于描述从根节点到后代节点的路径。*/
typedef std::vector< Node* > NodePath;

/** 节点路径向量，通常用于描述从一个节点到其可能的根节点的所有路径。*/
typedef std::vector< NodePath > NodePathList;

/** 矩阵向量，通常用于描述从一个节点到其可能的根节点的所有路径对应的变换矩阵。*/
typedef std::vector< Matrix > MatrixList;


/** META_Node 宏定义了标准的 clone、isSameKindAs、className 和 accept 方法。
  * 当从 Node 继承子类时使用，使得定义所需的纯虚函数更加方便。*/
#define META_Node(library,name) \
        virtual osg::Object* cloneType() const { return new name (); } \
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new name (*this,copyop); } \
        virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const name *>(obj)!=NULL; } \
        virtual const char* className() const { return #name; } \
        virtual const char* libraryName() const { return #library; } \
        virtual void accept(osg::NodeVisitor& nv) { if (nv.validNodeMask(*this)) { nv.pushOntoNodePath(this); nv.apply(*this); nv.popFromNodePath(); } } \


/** 场景图中所有内部节点的基类。
    提供最常见节点操作的接口（组合模式）。
*/
class OSG_EXPORT Node : public Object
{
    public:

        /** 构造一个节点。
            将父节点列表初始化为空，节点名称为 ""，
            包围球脏标志为 true。*/
        Node();

        /** 使用 CopyOp 管理深复制与浅复制的拷贝构造函数。*/
        Node(const Node&,const CopyOp& copyop=CopyOp::SHALLOW_COPY);

        /** 克隆与节点相同类型的对象。*/
        virtual Object* cloneType() const { return new Node(); }

        /** 返回节点的克隆，返回类型为 Object*。*/
        virtual Object* clone(const CopyOp& copyop) const { return new Node(*this,copyop); }

        /** 如果 this 和 obj 是同一类型的对象，返回 true。*/
        virtual bool isSameKindAs(const Object* obj) const { return dynamic_cast<const Node*>(obj)!=NULL; }

        /** 返回节点库的名称。*/
        virtual const char* libraryName() const { return "osg"; }

        /** 返回节点类类型的名称。*/
        virtual const char* className() const { return "Node"; }

        /** 如果 Object 是 Node，则将 'this' 转换为 Node 指针，否则返回 0。
          * 等价于 dynamic_cast<Node*>(this)。*/
        virtual Node* asNode() { return this; }

        /** 如果 Object 是 Node，则将 'const this' 转换为 const Node 指针，否则返回 0。
          * 等价于 dynamic_cast<const Node*>(this)。*/
        virtual const Node* asNode() const { return this; }

        /** 如果 Node 是 Drawable，则将 'this' 转换为 Drawable 指针，否则返回 0。
          * 等价于 dynamic_cast<Group*>(this)。*/
        virtual Drawable* asDrawable() { return 0; }
        /** 如果 Node 是 Drawable，则将 'const this' 转换为 const Drawable 指针，否则返回 0。
          * 等价于 dynamic_cast<const Group*>(this)。*/
        virtual const Drawable* asDrawable() const { return 0; }

        /** 如果 Node 是 Geometry，则将 'this' 转换为 Geometry 指针，否则返回 0。
          * 等价于 dynamic_cast<Group*>(this)。*/
        virtual Geometry* asGeometry() { return 0; }
        /** 如果 Node 是 Geometry，则将 'const this' 转换为 const Geometry 指针，否则返回 0。
          * 等价于 dynamic_cast<const Group*>(this)。*/
        virtual const Geometry* asGeometry() const { return 0; }

        /** 如果 Node 是 Group，则将 'this' 转换为 Group 指针，否则返回 0。
          * 等价于 dynamic_cast<Group*>(this)。*/
        virtual Group* asGroup() { return 0; }
        /** 如果 Node 是 Group，则将 'const this' 转换为 const Group 指针，否则返回 0。
          * 等价于 dynamic_cast<const Group*>(this)。*/
        virtual const Group* asGroup() const { return 0; }

        /** 如果 Node 是 Transform，则将 'this' 转换为 Transform 指针，否则返回 0。
          * 等价于 dynamic_cast<Transform*>(this)。*/
        virtual Transform* asTransform() { return 0; }

        /** 如果 Node 是 Transform，则将 'const this' 转换为 const Transform 指针，否则返回 0。
          * 等价于 dynamic_cast<const Transform*>(this)。*/
        virtual const Transform* asTransform() const { return 0; }

        /** 如果 Node 是 OccluderNode，则将 'this' 转换为 OccluderNode 指针，否则返回 0。
          * 等价于 dynamic_cast<OccluderNode*>(this)。*/
        virtual OccluderNode* asOccluderNode() { return 0; }
        /** 如果 Node 是 OccluderNode，则将 'const this' 转换为 const OccluderNode 指针，否则返回 0。
          * 等价于 dynamic_cast<const OccluderNode*>(this)。*/
        virtual const OccluderNode* asOccluderNode() const { return 0; }


        /** 如果 Node 是 Switch，则将 'this' 转换为 Switch 指针，否则返回 0。
          * 等价于 dynamic_cast<Switch*>(this)。*/
        virtual Switch* asSwitch() { return 0; }

        /** 如果 Node 是 Switch，则将 'const this' 转换为 const Switch 指针，否则返回 0。
          * 等价于 dynamic_cast<const Switch*>(this)。*/
        virtual const Switch* asSwitch() const { return 0; }

        /** 如果 Node 是 Geode，则将 'this' 转换为 Geode 指针，否则返回 0。
          * 等价于 dynamic_cast<Geode*>(this)。*/
        virtual Geode* asGeode() { return 0; }

        /** 如果 Node 是 Geode，则将 'const this' 转换为 const Geode 指针，否则返回 0。
          * 等价于 dynamic_cast<const Geode*>(this)。*/
        virtual const Geode* asGeode() const { return 0; }

        /** 如果 Node 是 Terrain，则将 'this' 转换为 Transform 指针，否则返回 0。
          * 等价于 dynamic_cast<Terrrain*>(this)。*/
        virtual osgTerrain::Terrain* asTerrain() { return 0; }

        /** 如果 Node 是 Terrain，则将 'const this' 转换为 const Terrain 指针，否则返回 0。
          * 等价于 dynamic_cast<const Terrain*>(this)。*/
        virtual const osgTerrain::Terrain* asTerrain() const { return 0; }


        /** 访问者模式：使用此节点的类型调用 NodeVisitor 的 apply 方法。*/
        virtual void accept(NodeVisitor& nv);
        /** 向上遍历：使用 NodeVisitor 调用父节点的 accept 方法。*/
        virtual void ascend(NodeVisitor& nv);
        /** 向下遍历：使用 NodeVisitor 调用子节点的 accept 方法。*/
        virtual void traverse(NodeVisitor& /*nv*/) {}

        /** osg::Group 指针向量，用于存储节点的父节点。*/
        typedef std::vector<Group*> ParentList;

        /** 获取节点的父节点列表。 */
        inline const ParentList& getParents() const { return _parents; }

        /** 获取节点父节点列表的副本。返回副本是为了
          * 防止对父节点列表的修改。*/
        inline ParentList getParents() { return _parents; }

        inline Group* getParent(unsigned int i)  { return _parents[i]; }

        /**
         * 获取节点的单个 const 父节点。
         * @param i 要获取的父节点的索引。
         * @return 父节点 i。
         */
        inline const Group* getParent(unsigned int i) const  { return _parents[i]; }

        /**
         * 获取节点的父节点数量。
         * @return 此节点的父节点数量。
         */
        inline unsigned int getNumParents() const { return static_cast<unsigned int>(_parents.size()); }



        /** 获取父节点路径的节点路径列表。
          * 可选的 Node* haltTraversalAtNode 允许用户在指定节点处停止遍历。 */
        NodePathList getParentalNodePaths(osg::Node* haltTraversalAtNode=0) const;

        /** 获取将此节点从局部坐标变换到世界坐标的矩阵列表。
          * 可选的 Node* haltTraversalAtNode 允许用户在指定节点处停止遍历。 */
        MatrixList getWorldMatrices(const osg::Node* haltTraversalAtNode=0) const;


        /** 设置更新节点回调，在更新遍历期间调用。 */
        void setUpdateCallback(Callback* nc);

        template<class T> void setUpdateCallback(const ref_ptr<T>& nc) { setUpdateCallback(nc.get()); }

        /** 获取更新节点回调，在更新遍历期间调用。 */
        inline Callback* getUpdateCallback() { return _updateCallback.get(); }

        /** 获取 const 更新节点回调，在更新遍历期间调用。 */
        inline const Callback* getUpdateCallback() const { return _updateCallback.get(); }

        /** 便利方法，如果节点的更新回调不存在就设置它，否则将其嵌套到现有的回调中。 */
        inline void addUpdateCallback(Callback* nc) {
            if (nc != NULL) {
                if (_updateCallback.valid()) _updateCallback->addNestedCallback(nc);
                else setUpdateCallback(nc);
            }
        }

        template<class T> void addUpdateCallback(const ref_ptr<T>& nc) { addUpdateCallback(nc.get()); }

        /** 便利方法，从节点中移除给定的回调，即使该回调是嵌套的。如果找不到给定的回调，不会有错误返回。 */
        inline void removeUpdateCallback(Callback* nc) {
            if (nc != NULL && _updateCallback.valid()) {
                if (_updateCallback == nc)
                {
                    ref_ptr<osg::Callback> new_nested_callback = nc->getNestedCallback();
                    nc->setNestedCallback(0);
                    setUpdateCallback(new_nested_callback.get());
                }
                else _updateCallback->removeNestedCallback(nc);
            }
        }

        template<class T> void removeUpdateCallback(const ref_ptr<T>& nc) { removeUpdateCallback(nc.get()); }

        /** 获取此节点需要更新遍历的子节点数量，
          * 因为它们或其子节点附加了更新回调。*/
        inline unsigned int getNumChildrenRequiringUpdateTraversal() const { return _numChildrenRequiringUpdateTraversal; }


        /** 设置事件节点回调，在事件遍历期间调用。 */
        void setEventCallback(Callback* nc);

        template<class T> void setEventCallback(const ref_ptr<T>& nc) { setEventCallback(nc.get()); }

        /** 获取事件节点回调，在事件遍历期间调用。 */
        inline Callback* getEventCallback() { return _eventCallback.get(); }

        /** 获取 const 事件节点回调，在事件遍历期间调用。 */
        inline const Callback* getEventCallback() const { return _eventCallback.get(); }

        /** 便利方法，如果节点的事件回调不存在就设置它，否则将其嵌套到现有的回调中。 */
        inline void addEventCallback(Callback* nc) {
            if (nc != NULL) {
                if (_eventCallback.valid()) _eventCallback->addNestedCallback(nc);
                else setEventCallback(nc);
            }
        }

        template<class T> void addEventCallback(const ref_ptr<T>& nc) { addEventCallback(nc.get()); }

        /** 便利方法，从节点中移除给定的回调，即使该回调是嵌套的。如果找不到给定的回调，不会有错误返回。 */
        inline void removeEventCallback(Callback* nc) {
            if (nc != NULL && _eventCallback.valid()) {
                if (_eventCallback == nc)
                {
                    ref_ptr<osg::Callback> new_nested_callback = nc->getNestedCallback();
                    nc->setNestedCallback(0);
                    setEventCallback(new_nested_callback.get()); // 用嵌套的回调替换当前回调
                }
                else _eventCallback->removeNestedCallback(nc);
            }
        }

        template<class T> void removeEventCallback(const ref_ptr<T>& nc) { removeEventCallback(nc.get()); }

            /** 获取此节点需要事件遍历的子节点数量，
          * 因为它们或其子节点附加了事件回调。*/
        inline unsigned int getNumChildrenRequiringEventTraversal() const { return _numChildrenRequiringEventTraversal; }


        /** 设置裁剪节点回调，在裁剪遍历期间调用。 */
        void setCullCallback(Callback* nc) { _cullCallback = nc; }

        template<class T> void setCullCallback(const ref_ptr<T>& nc) { setCullCallback(nc.get()); }

        /** 获取裁剪节点回调，在裁剪遍历期间调用。 */
        inline Callback* getCullCallback() { return _cullCallback.get(); }

        /** 获取 const 裁剪节点回调，在裁剪遍历期间调用。 */
        inline const Callback* getCullCallback() const { return _cullCallback.get(); }

        /** 便利方法，如果节点的裁剪回调不存在就设置它，否则将其嵌套到现有的回调中。 */
        inline void addCullCallback(Callback* nc) {
            if (nc != NULL) {
                if (_cullCallback.valid()) _cullCallback->addNestedCallback(nc);
                else setCullCallback(nc);
            }
        }

        template<class T> void addCullCallback(const ref_ptr<T>& nc) { addCullCallback(nc.get()); }

        /** 便利方法，从节点中移除给定的回调，即使该回调是嵌套的。如果找不到给定的回调，不会有错误返回。 */
        inline void removeCullCallback(Callback* nc) {
            if (nc != NULL && _cullCallback.valid()) {
                if (_cullCallback == nc)
                {
                    ref_ptr<osg::Callback> new_nested_callback = nc->getNestedCallback();
                    nc->setNestedCallback(0);
                    setCullCallback(new_nested_callback.get()); // 用嵌套的回调替换当前回调
                }
                else _cullCallback->removeNestedCallback(nc);
            }
        }

        template<class T> void removeCullCallback(const ref_ptr<T>& nc) { removeCullCallback(nc.get()); }

        /** 设置此节点的视椎体/小特征裁剪为活动或非活动。
          * _cullingActive 的默认值为 true。用作裁剪遍历的指导。*/
        void setCullingActive(bool active);

        /** 获取此节点的视椎体/小特征 _cullingActive 标志。用作裁剪遍历的指导。*/
        inline bool getCullingActive() const { return _cullingActive; }

        /** 获取此节点禁用裁剪的子节点数量。*/
        inline unsigned int getNumChildrenWithCullingDisabled() const { return _numChildrenWithCullingDisabled; }

        /** 如果此节点在裁剪遍历期间可以被视椎体、遮挡或小特征裁剪剔除，返回 true。
          * 注意，只有当没有子节点禁用裁剪，且本地 _cullingActive 标志为 true，且包围体有效时才返回 true。*/
        inline bool isCullingActive() const { return _numChildrenWithCullingDisabled==0 && _cullingActive && getBound().valid(); }

        /** 获取此节点中是 OccluderNode 或拥有 OccluderNode 的子节点数量。*/
        inline unsigned int getNumChildrenWithOccluderNodes() const { return _numChildrenWithOccluderNodes; }


        /** 如果此节点是 OccluderNode 或此节点下的子图包含 OccluderNode，返回 true。*/
        bool containsOccluderNodes() const;


        /**
        * 这是一组表示节点的位（标志）。
        * 默认值是 0xffffffff（所有位都设置）。
        *
        * 这些标志最常见的用途是在场景图的遍历过程中。
        * 例如，当遍历场景图时，osg::NodeVisitor 对其 TraversalMask 与节点的 NodeMask 进行
        * 按位与运算，以确定是否应该处理/遍历该节点。
        *
        * 例如，如果一个节点的 NodeMask 值为 0x02（只设置第2位）
        * 而 osg::Camera 的 CullMask 为 0x4（第2位未设置），那么在裁剪遍历期间，
        * 裁剪遍历从 Camera 的 CullMask 获取其 TraversalMask，该节点和任何子节点
        * 都会被忽略，从而被视为"剔除"，因此不会被渲染。
        * 相反，如果 osg::Camera CullMask 为 0x3（第2位已设置），那么该节点
        * 会被处理，子节点会被检查。
        */
        typedef unsigned int NodeMask;
        /** 设置节点掩码。*/
        inline void setNodeMask(NodeMask nm) { _nodeMask = nm; }
        /** 获取节点掩码。*/
        inline NodeMask getNodeMask() const { return _nodeMask; }



        /** 设置节点的 StateSet。*/
        void setStateSet(osg::StateSet* stateset);

        template<class T> void setStateSet(const osg::ref_ptr<T>& stateset) { setStateSet(stateset.get()); }

        /** 返回节点的 StateSet，如果不存在则创建一个
          * 设置到节点并返回新创建的 StateSet。这确保
          * 总是返回有效的 StateSet 并可以直接使用。*/
        osg::StateSet* getOrCreateStateSet();

        /** 返回节点的 StateSet。如果没有附加 stateset 则返回 NULL。*/
        inline osg::StateSet* getStateSet() { return _stateset.get(); }

        /** 返回节点的 const StateSet。如果没有附加 stateset 则返回 NULL。*/
        inline const osg::StateSet* getStateSet() const { return _stateset.get(); }


        /** 用于描述对象的 std::string 向量。*/
        typedef std::vector<std::string> DescriptionList;

        /** 设置字符串描述列表。*/
        void setDescriptions(const DescriptionList& descriptions);

        /** 获取节点的描述列表。*/
        DescriptionList& getDescriptions();

        /** 获取 const 节点的 const 描述列表。*/
        const DescriptionList& getDescriptions() const;


        /** 获取 const 节点的单个 const 描述。*/
        const std::string& getDescription(unsigned int i) const;

        /** 获取节点的单个描述。*/
        std::string& getDescription(unsigned int i);

        /** 获取节点的描述数量。*/
        unsigned int getNumDescriptions() const;

        /** 向节点添加描述字符串。*/
        void addDescription(const std::string& desc);


        /** 设置计算整体包围体时使用的初始包围体。*/
        void setInitialBound(const osg::BoundingSphere& bsphere) { _initialBound = bsphere; dirtyBound(); }

        /** 设置计算整体包围体时使用的初始包围体。*/
        const BoundingSphere& getInitialBound() const { return _initialBound; }

        /** 标记此节点的包围球为脏。
            强制在下次调用 getBound() 时重新计算。*/
        void dirtyBound();


        inline const BoundingSphere& getBound() const
        {
            if(!_boundingSphereComputed)
            {
                _boundingSphere = _initialBound;
                if (_computeBoundCallback.valid())
                    _boundingSphere.expandBy(_computeBoundCallback->computeBound(*this));
                else
                    _boundingSphere.expandBy(computeBound());

                _boundingSphereComputed = true;
            }
            return _boundingSphere;
        }

        /** 计算围绕节点几何体或子节点的包围球。
            当包围球通过 dirtyBound() 被标记为脏时，此方法会被 getBound() 自动调用。*/
        virtual BoundingSphere computeBound() const;

        /** 允许用户覆盖包围体默认计算的回调。*/
        struct ComputeBoundingSphereCallback : public osg::Object
        {
            ComputeBoundingSphereCallback() {}

            ComputeBoundingSphereCallback(const ComputeBoundingSphereCallback& org,const CopyOp& copyop):
                Object(org, copyop) {}

            META_Object(osg,ComputeBoundingSphereCallback);

           virtual BoundingSphere computeBound(const osg::Node&) const { return BoundingSphere(); }
        };

        /** 设置计算包围回调以覆盖默认的 computeBound。*/
        void setComputeBoundingSphereCallback(ComputeBoundingSphereCallback* callback) { _computeBoundCallback = callback; }

        template<class T> void setComputeBoundingSphereCallback(const ref_ptr<T>& callback) { setComputeBoundingSphereCallback(callback.get()); }

        /** 获取计算包围回调。*/
        ComputeBoundingSphereCallback* getComputeBoundingSphereCallback() { return _computeBoundCallback.get(); }

        /** 获取 const 计算包围回调。*/
        const ComputeBoundingSphereCallback* getComputeBoundingSphereCallback() const { return _computeBoundCallback.get(); }

        /** 设置是否使用互斥锁确保 ref() 和 unref() 线程安全。*/
        virtual void setThreadSafeRefUnref(bool threadSafe);

        /** 将任何每上下文 GLObject 缓冲区调整为指定大小。 */
        virtual void resizeGLObjectBuffers(unsigned int /*maxSize*/);

        /** 如果 State 非零，此函数释放指定图形上下文的任何关联 OpenGL 对象。
           * 否则，释放所有图形上下文的 OpenGL 对象。 */
        virtual void releaseGLObjects(osg::State* = 0) const;


    protected:

        /** 节点析构函数。注意，是受保护的，所以节点不能
            通过被取消引用和引用计数为零之外的方式删除（参见 osg::Referenced），
            防止删除仍在使用的节点。这也意味着
            节点不能在栈上创建，即 Node node 不会编译，
            强制所有节点都在堆上创建，即 Node* node
            = new Node()。*/
        virtual ~Node();



        BoundingSphere                          _initialBound;         // 初始包围体
        ref_ptr<ComputeBoundingSphereCallback>  _computeBoundCallback; // 计算包围体回调
        mutable BoundingSphere                  _boundingSphere;       // 包围球
        mutable bool                            _boundingSphereComputed; // 包围球是否已计算

        void addParent(osg::Group* parent);
        void removeParent(osg::Group* parent);

        ParentList _parents;                              // 父节点列表
        friend class osg::Group;
        friend class osg::Drawable;
        friend class osg::StateSet;

        ref_ptr<Callback> _updateCallback;               // 更新回调
        unsigned int _numChildrenRequiringUpdateTraversal; // 需要更新遍历的子节点数量
        void setNumChildrenRequiringUpdateTraversal(unsigned int num);

        ref_ptr<Callback> _eventCallback;                // 事件回调
        unsigned int _numChildrenRequiringEventTraversal;  // 需要事件遍历的子节点数量
        void setNumChildrenRequiringEventTraversal(unsigned int num);

        ref_ptr<Callback> _cullCallback;                 // 裁剪回调

        bool _cullingActive;                             // 裁剪是否活动
        unsigned int _numChildrenWithCullingDisabled;   // 禁用裁剪的子节点数量
        void setNumChildrenWithCullingDisabled(unsigned int num);

        unsigned int _numChildrenWithOccluderNodes;     // 拥有遮挡器节点的子节点数量
        void setNumChildrenWithOccluderNodes(unsigned int num);

        NodeMask _nodeMask;                              // 节点掩码

        ref_ptr<StateSet> _stateset;                     // 状态集

};

}

#endif 