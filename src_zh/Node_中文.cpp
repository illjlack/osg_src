/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/

#include <osg/Node>
#include <osg/Group>
#include <osg/NodeVisitor>
#include <osg/Notify>
#include <osg/OccluderNode>
#include <osg/Transform>
#include <osg/UserDataContainer>

#include <algorithm>

using namespace osg;

namespace osg
{
    /// 用于生成 NodePathList 的辅助类。
    class CollectParentPaths : public NodeVisitor
    {
    public:
        CollectParentPaths(const osg::Node* haltTraversalAtNode=0) :
            osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_PARENTS),
            _haltTraversalAtNode(haltTraversalAtNode)
        {
            setNodeMaskOverride(0xffffffff);
        }

        virtual void apply(osg::Node& node)
        {
            if (node.getNumParents()==0 || &node==_haltTraversalAtNode)
            {
                _nodePaths.push_back(getNodePath());
            }
            else
            {
                traverse(node);
            }
       }

        const Node*     _haltTraversalAtNode;  // 停止遍历的节点
        NodePath        _nodePath;             // 节点路径
        NodePathList    _nodePaths;            // 节点路径列表
    };
}



// 默认构造函数
Node::Node()
    :Object(true)
{
    _boundingSphereComputed = false;                    // 包围球未计算
    _nodeMask = 0xffffffff;                            // 节点掩码（全部位设置）

    _numChildrenRequiringUpdateTraversal = 0;          // 需要更新遍历的子节点数量

    _numChildrenRequiringEventTraversal = 0;           // 需要事件遍历的子节点数量

    _cullingActive = true;                             // 裁剪激活
    _numChildrenWithCullingDisabled = 0;               // 禁用裁剪的子节点数量

    _numChildrenWithOccluderNodes = 0;                 // 拥有遮挡器节点的子节点数量
}

// 拷贝构造函数
Node::Node(const Node& node,const CopyOp& copyop):
        Object(node,copyop),                                        // 调用基类拷贝构造函数
        _initialBound(node._initialBound),                          // 复制初始包围体
        _boundingSphere(node._boundingSphere),                      // 复制包围球
        _boundingSphereComputed(node._boundingSphereComputed),      // 复制包围球计算状态
        _parents(),                                                 // 父节点列表留空，由 Group 管理
        _updateCallback(copyop(node._updateCallback.get())),       // 复制更新回调
        _numChildrenRequiringUpdateTraversal(0),                   // 假设还没有子节点
        _numChildrenRequiringEventTraversal(0),                    // 假设还没有子节点
        _cullCallback(copyop(node._cullCallback.get())),           // 复制裁剪回调
        _cullingActive(node._cullingActive),                       // 复制裁剪激活状态
        _numChildrenWithCullingDisabled(0),                        // 假设还没有子节点
        _numChildrenWithOccluderNodes(0),                          // 复制遮挡器节点数量
        _nodeMask(node._nodeMask)                                  // 复制节点掩码
{
    setStateSet(copyop(node._stateset.get()));
}

// 析构函数
Node::~Node()
{
    // 干净地分离任何关联的 stateset（包括移除父链接）
    setStateSet(0);
}

// 添加父节点
void Node::addParent(osg::Group* parent)
{
    OpenThreads::ScopedPointerLock<OpenThreads::Mutex> lock(getRefMutex());

    _parents.push_back(parent);
}

// 移除父节点
void Node::removeParent(osg::Group* parent)
{
    OpenThreads::ScopedPointerLock<OpenThreads::Mutex> lock(getRefMutex());

    ParentList::iterator pitr = std::find(_parents.begin(), _parents.end(), parent);
    if (pitr!=_parents.end()) _parents.erase(pitr);
}

// 接受访问者
void Node::accept(NodeVisitor& nv)
{
    if (nv.validNodeMask(*this))
    {
        nv.pushOntoNodePath(this);      // 将此节点推入路径
        nv.apply(*this);                // 应用访问者
        nv.popFromNodePath();           // 从路径中弹出此节点
    }
}

// 向上遍历
void Node::ascend(NodeVisitor& nv)
{
    std::for_each(_parents.begin(),_parents.end(),NodeAcceptOp(nv));
}

// 设置状态集
void Node::setStateSet(osg::StateSet* stateset)
{
    // 如果没有变化则什么也不做
    if (_stateset==stateset) return;

    // 跟踪是否需要考虑更新或事件遍历的需要
    int delta_update = 0;
    int delta_event = 0;

    // 从当前状态集的父列表中移除此节点
    if (_stateset.valid())
    {
        _stateset->removeParent(this);
        if (_stateset->requiresUpdateTraversal()) --delta_update;
        if (_stateset->requiresEventTraversal()) --delta_event;
    }

    // 设置状态集
    _stateset = stateset;

    // 将此节点添加到新状态集的父列表中
    if (_stateset.valid())
    {
        _stateset->addParent(this);
        if (_stateset->requiresUpdateTraversal()) ++delta_update;
        if (_stateset->requiresEventTraversal()) ++delta_event;
    }

    if (delta_update!=0)
    {
        setNumChildrenRequiringUpdateTraversal(getNumChildrenRequiringUpdateTraversal()+delta_update);
    }

    if (delta_event!=0)
    {
        setNumChildrenRequiringEventTraversal(getNumChildrenRequiringEventTraversal()+delta_event);
    }
}

// 获取或创建状态集
osg::StateSet* Node::getOrCreateStateSet()
{
    if (!_stateset) setStateSet(new StateSet);
    return _stateset.get();
}

// 获取父节点路径
NodePathList Node::getParentalNodePaths(osg::Node* haltTraversalAtNode) const
{
    CollectParentPaths cpp(haltTraversalAtNode);
    const_cast<Node*>(this)->accept(cpp);
    return cpp._nodePaths;
}

// 获取世界变换矩阵
MatrixList Node::getWorldMatrices(const osg::Node* haltTraversalAtNode) const
{
    CollectParentPaths cpp(haltTraversalAtNode);
    const_cast<Node*>(this)->accept(cpp);

    MatrixList matrices;

    for(NodePathList::iterator itr = cpp._nodePaths.begin();
        itr != cpp._nodePaths.end();
        ++itr)
    {
        NodePath& nodePath = *itr;
        if (nodePath.empty())
        {
            matrices.push_back(osg::Matrix::identity());
        }
        else
        {
            matrices.push_back(osg::computeLocalToWorld(nodePath));
        }
    }

    return matrices;
}

// 设置更新回调
void Node::setUpdateCallback(Callback* nc)
{
    // 如果没有变化就直接返回
    if (_updateCallback==nc) return;

    // 更新回调已被更改，需要更新
    // _updateCallback 和可能的 numChildrenRequiringAppTraversal
    // 如果回调数量发生变化

    // 更新父节点的 numChildrenRequiringAppTraversal
    // 注意，如果 _numChildrenRequiringUpdateTraversal!=0，那么
    // 父节点不会受到任何应用回调变化的影响，
    // 所以不需要通知它们
    if (_numChildrenRequiringUpdateTraversal==0 && !_parents.empty())
    {
        int delta = 0;
        if (_updateCallback.valid()) --delta;
        if (nc) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenRequiringUpdateTraversal(
                        (*itr)->getNumChildrenRequiringUpdateTraversal()+delta );
            }

        }
    }

    // 设置应用回调本身
    _updateCallback = nc;

}

// 设置需要更新遍历的子节点数量
void Node::setNumChildrenRequiringUpdateTraversal(unsigned int num)
{
    // 如果没有变化就直接返回
    if (_numChildrenRequiringUpdateTraversal==num) return;

    // 注意，如果设置了 _updateCallback，那么
    // 父节点不会受到 _numChildrenRequiringUpdateTraversal 
    // 的任何变化影响，所以不需要通知它们
    if (!_updateCallback && !_parents.empty())
    {

        // 需要将变化传递给父节点
        int delta = 0;
        if (_numChildrenRequiringUpdateTraversal>0) --delta;
        if (num>0) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenRequiringUpdateTraversal(
                    (*itr)->getNumChildrenRequiringUpdateTraversal()+delta
                    );
            }

        }
    }

    // 最后更新此对象的值
    _numChildrenRequiringUpdateTraversal=num;

}

// 设置事件回调
void Node::setEventCallback(Callback* nc)
{
    // 如果没有变化就直接返回
    if (_eventCallback==nc) return;

    // 事件回调已被更改，需要更新
    // _EventCallback 和可能的 numChildrenRequiringAppTraversal
    // 如果回调数量发生变化

    // 更新父节点的 numChildrenRequiringAppTraversal
    // 注意，如果 _numChildrenRequiringEventTraversal!=0，那么
    // 父节点不会受到任何应用回调变化的影响，
    // 所以不需要通知它们
    if (_numChildrenRequiringEventTraversal==0 && !_parents.empty())
    {
        int delta = 0;
        if (_eventCallback.valid()) --delta;
        if (nc) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenRequiringEventTraversal(
                        (*itr)->getNumChildrenRequiringEventTraversal()+delta );
            }

        }
    }

    // 设置应用回调本身
    _eventCallback = nc;

}

// 设置需要事件遍历的子节点数量
void Node::setNumChildrenRequiringEventTraversal(unsigned int num)
{
    // 如果没有变化就直接返回
    if (_numChildrenRequiringEventTraversal==num) return;

    // 注意，如果设置了 _EventCallback，那么
    // 父节点不会受到 _numChildrenRequiringEventTraversal 
    // 的任何变化影响，所以不需要通知它们
    if (!_eventCallback && !_parents.empty())
    {

        // 需要将变化传递给父节点
        int delta = 0;
        if (_numChildrenRequiringEventTraversal>0) --delta;
        if (num>0) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenRequiringEventTraversal(
                    (*itr)->getNumChildrenRequiringEventTraversal()+delta
                    );
            }

        }
    }

    // 最后更新此对象的值
    _numChildrenRequiringEventTraversal=num;

}

// 设置裁剪是否激活
void Node::setCullingActive(bool active)
{
    // 如果没有变化就直接返回
    if (_cullingActive == active) return;

    // 裁剪激活状态已被更改，需要更新
    // _cullActive 和可能的父节点 numChildrenWithCullingDisabled
    // 如果裁剪禁用状态发生变化

    // 更新父节点的 _numChildrenWithCullingDisabled
    // 注意，如果 _numChildrenWithCullingDisabled!=0，那么
    // 父节点不会受到任何应用回调变化的影响，
    // 所以不需要通知它们
    if (_numChildrenWithCullingDisabled==0 && !_parents.empty())
    {
        int delta = 0;
        if (!_cullingActive) --delta;
        if (!active) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenWithCullingDisabled(
                        (*itr)->getNumChildrenWithCullingDisabled()+delta );
            }

        }
    }

    // 设置裁剪激活状态本身
    _cullingActive = active;
}

// 设置禁用裁剪的子节点数量
void Node::setNumChildrenWithCullingDisabled(unsigned int num)
{
    // 如果没有变化就直接返回
    if (_numChildrenWithCullingDisabled==num) return;

    // 注意，如果 _cullingActive 为 false，那么
    // 父节点不会受到 _numChildrenWithCullingDisabled 
    // 的任何变化影响，所以不需要通知它们
    if (_cullingActive && !_parents.empty())
    {

        // 需要将变化传递给父节点
        int delta = 0;
        if (_numChildrenWithCullingDisabled>0) --delta;
        if (num>0) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenWithCullingDisabled(
                    (*itr)->getNumChildrenWithCullingDisabled()+delta
                    );
            }

        }
    }

    // 最后更新此对象的值
    _numChildrenWithCullingDisabled=num;
}

// 设置拥有遮挡器节点的子节点数量
void Node::setNumChildrenWithOccluderNodes(unsigned int num)
{
    // 如果没有变化就直接返回
    if (_numChildrenWithOccluderNodes==num) return;

    // 注意，如果此节点是 OccluderNode，那么
    // 父节点不会受到 _numChildrenWithOccluderNodes 
    // 的任何变化影响，所以不需要通知它们
    if (!dynamic_cast<OccluderNode*>(this) && !_parents.empty())
    {

        // 需要将变化传递给父节点
        int delta = 0;
        if (_numChildrenWithOccluderNodes>0) --delta;
        if (num>0) ++delta;
        if (delta!=0)
        {
            // 回调数量已更改，需要传递给
            // 父节点，以便它们知道此子图是否需要应用遍历
            for(ParentList::iterator itr =_parents.begin();
                itr != _parents.end();
                ++itr)
            {
                (*itr)->setNumChildrenWithOccluderNodes(
                    (*itr)->getNumChildrenWithOccluderNodes()+delta
                    );
            }

        }
    }

    // 最后更新此对象的值
    _numChildrenWithOccluderNodes=num;

}

// 是否包含遮挡器节点
bool Node::containsOccluderNodes() const
{
    return _numChildrenWithOccluderNodes>0 || dynamic_cast<const OccluderNode*>(this);
}

// 设置描述列表
void Node::setDescriptions(const DescriptionList& descriptions)
{
    // 只有在需要时才分配描述列表（和关联的 UseDataContainer）
    if (!descriptions.empty() || getUserDataContainer())
    {
        getOrCreateUserDataContainer()->setDescriptions(descriptions);
    }
}

// 获取描述列表
Node::DescriptionList& Node::getDescriptions()
{
    return getOrCreateUserDataContainer()->getDescriptions();
}

static OpenThreads::Mutex s_mutex_StaticDescriptionList;
static const Node::DescriptionList& getStaticDescriptionList()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_StaticDescriptionList);
    static Node::DescriptionList s_descriptionList;
    return s_descriptionList;
}

// 获取const描述列表
const Node::DescriptionList& Node::getDescriptions() const
{
    if (_userDataContainer) return _userDataContainer->getDescriptions();
    else return getStaticDescriptionList();
}

// 获取单个描述
std::string& Node::getDescription(unsigned int i)
{
    return getOrCreateUserDataContainer()->getDescriptions()[i];
}

// 获取单个const描述
const std::string& Node::getDescription(unsigned int i) const
{
    if (_userDataContainer) return _userDataContainer->getDescriptions()[i];
    else return getStaticDescriptionList()[i];
}

// 获取描述数量
unsigned int Node::getNumDescriptions() const
{
    return _userDataContainer ? _userDataContainer->getDescriptions().size() : 0;
}

// 添加描述
void Node::addDescription(const std::string& desc)
{
    getOrCreateUserDataContainer()->getDescriptions().push_back(desc);
}

// 计算包围体
BoundingSphere Node::computeBound() const
{
    return BoundingSphere();
}

// 标记包围体为脏
void Node::dirtyBound()
{
    if (_boundingSphereComputed)
    {
        _boundingSphereComputed = false;

        // 标记父包围球为脏以确保所有都是有效的
        for(ParentList::iterator itr=_parents.begin();
            itr!=_parents.end();
            ++itr)
        {
            (*itr)->dirtyBound();
        }

    }
}

// 设置线程安全引用计数
void Node::setThreadSafeRefUnref(bool threadSafe)
{
    Object::setThreadSafeRefUnref(threadSafe);

    if (_stateset.valid()) _stateset->setThreadSafeRefUnref(threadSafe);
    if (_updateCallback.valid()) _updateCallback->setThreadSafeRefUnref(threadSafe);
    if (_eventCallback.valid()) _eventCallback->setThreadSafeRefUnref(threadSafe);
    if (_cullCallback.valid()) _cullCallback->setThreadSafeRefUnref(threadSafe);
}

// 调整GL对象缓冲区大小
void Node::resizeGLObjectBuffers(unsigned int maxSize)
{
    if (_stateset.valid()) _stateset->resizeGLObjectBuffers(maxSize);
    if (_updateCallback.valid()) _updateCallback->resizeGLObjectBuffers(maxSize);
    if (_eventCallback.valid()) _eventCallback->resizeGLObjectBuffers(maxSize);
    if (_cullCallback.valid()) _cullCallback->resizeGLObjectBuffers(maxSize);
}

// 释放GL对象
void Node::releaseGLObjects(osg::State* state) const
{
    if (_stateset.valid()) _stateset->releaseGLObjects(state);
    if (_updateCallback.valid()) _updateCallback->releaseGLObjects(state);
    if (_eventCallback.valid()) _eventCallback->releaseGLObjects(state);
    if (_cullCallback.valid()) _cullCallback->releaseGLObjects(state);
} 