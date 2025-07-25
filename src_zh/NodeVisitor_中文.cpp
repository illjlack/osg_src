/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/
#include <osg/NodeVisitor>

#include <osg/Billboard>
#include <osg/ClearNode>
#include <osg/ClipNode>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Group>
#include <osg/LightSource>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/OccluderNode>
#include <osg/OcclusionQueryNode>
#include <osg/PagedLOD>
#include <osg/PositionAttitudeTransform>
#include <osg/AutoTransform>
#include <osg/Projection>
#include <osg/ProxyNode>
#include <osg/Sequence>
#include <osg/Switch>
#include <osg/TexGenNode>
#include <osg/Transform>
#include <osg/Camera>
#include <osg/CameraView>
#include <osg/Geometry>

#include <stdlib.h>

using namespace osg;

// 构造函数：使用遍历模式初始化
NodeVisitor::NodeVisitor(TraversalMode tm):
    Object(true)
{
    _visitorType = NODE_VISITOR;                          // 设置访问者类型为节点访问者
    _traversalNumber = osg::UNINITIALIZED_FRAME_NUMBER;   // 初始化遍历编号为未初始化状态

    _traversalMode = tm;                                  // 设置遍历模式
    _traversalMask = 0xffffffff;                         // 设置遍历掩码（全部开启）
    _nodeMaskOverride = 0x0;                             // 设置节点掩码覆盖（全部关闭）
}

// 构造函数：使用访问者类型和遍历模式初始化
NodeVisitor::NodeVisitor(VisitorType type,TraversalMode tm):
    Object(true)
{
    _visitorType = type;                                 // 设置指定的访问者类型
    _traversalNumber = osg::UNINITIALIZED_FRAME_NUMBER;  // 初始化遍历编号为未初始化状态

    _traversalMode = tm;                                 // 设置遍历模式
    _traversalMask = 0xffffffff;                        // 设置遍历掩码（全部开启）
    _nodeMaskOverride = 0x0;                            // 设置节点掩码覆盖（全部关闭）
}

// 拷贝构造函数
NodeVisitor::NodeVisitor(const NodeVisitor& nv, const osg::CopyOp& copyop):
    Object(nv, copyop),
    _visitorType(nv._visitorType),                      // 复制访问者类型
    _traversalNumber(nv._traversalNumber),              // 复制遍历编号
    _traversalMode(nv._traversalMode),                  // 复制遍历模式
    _traversalMask(nv._traversalMask),                  // 复制遍历掩码
    _nodeMaskOverride(nv._nodeMaskOverride)             // 复制节点掩码覆盖
{
}

// 析构函数
NodeVisitor::~NodeVisitor()
{
    // 如果存在遍历访问者，则从中分离
}

// 应用于基础节点 - 这是所有节点类型的根方法
void NodeVisitor::apply(Node& node)
{
    traverse(node);  // 调用遍历方法继续访问子节点
}

// 应用于可绘制对象 - 转换为Node类型处理
void NodeVisitor::apply(Drawable& drawable)
{
    apply(static_cast<Node&>(drawable));
}

// 应用于几何体 - 转换为Drawable类型处理
void NodeVisitor::apply(Geometry& drawable)
{
    apply(static_cast<Drawable&>(drawable));
}

// 应用于几何节点 - 转换为Group类型处理
void NodeVisitor::apply(Geode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于广告牌节点 - 转换为Geode类型处理
void NodeVisitor::apply(Billboard& node)
{
    apply(static_cast<Geode&>(node));
}

// 应用于组节点 - 转换为Node类型处理
void NodeVisitor::apply(Group& node)
{
    apply(static_cast<Node&>(node));
}

// 应用于代理节点 - 转换为Group类型处理
void NodeVisitor::apply(ProxyNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于投影节点 - 转换为Group类型处理
void NodeVisitor::apply(Projection& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于坐标系统节点 - 转换为Group类型处理
void NodeVisitor::apply(CoordinateSystemNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于裁剪节点 - 转换为Group类型处理
void NodeVisitor::apply(ClipNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于纹理生成节点 - 转换为Group类型处理
void NodeVisitor::apply(TexGenNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于光源节点 - 转换为Group类型处理
void NodeVisitor::apply(LightSource& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于变换节点 - 转换为Group类型处理
void NodeVisitor::apply(Transform& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于相机节点 - 转换为Transform类型处理
void NodeVisitor::apply(Camera& node)
{
    apply(static_cast<Transform&>(node));
}

// 应用于相机视图节点 - 转换为Transform类型处理
void NodeVisitor::apply(CameraView& node)
{
    apply(static_cast<Transform&>(node));
}

// 应用于矩阵变换节点 - 转换为Transform类型处理
void NodeVisitor::apply(MatrixTransform& node)
{
    apply(static_cast<Transform&>(node));
}

// 应用于位置姿态变换节点 - 转换为Transform类型处理
void NodeVisitor::apply(PositionAttitudeTransform& node)
{
    apply(static_cast<Transform&>(node));
}

// 应用于自动变换节点 - 转换为Transform类型处理
void NodeVisitor::apply(AutoTransform& node)
{
    apply(static_cast<Transform&>(node));
}

// 应用于开关节点 - 转换为Group类型处理
void NodeVisitor::apply(Switch& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于序列节点 - 转换为Group类型处理
void NodeVisitor::apply(Sequence& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于细节层次节点 - 转换为Group类型处理
void NodeVisitor::apply(LOD& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于分页细节层次节点 - 转换为LOD类型处理
void NodeVisitor::apply(PagedLOD& node)
{
    apply(static_cast<LOD&>(node));
}

// 应用于清除节点 - 转换为Group类型处理
void NodeVisitor::apply(ClearNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于遮挡器节点 - 转换为Group类型处理
void NodeVisitor::apply(OccluderNode& node)
{
    apply(static_cast<Group&>(node));
}

// 应用于遮挡查询节点 - 转换为Group类型处理
void NodeVisitor::apply(OcclusionQueryNode& node)
{
    apply(static_cast<Group&>(node));
} 