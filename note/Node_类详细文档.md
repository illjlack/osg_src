# OpenSceneGraph Node类详细文档

## 概述

**Node类**是OpenSceneGraph场景图中所有内部节点的基类。它采用组合模式设计，为场景图的层次结构提供统一的接口和基础功能。

## 继承关系

```
Object
  └── Node
      ├── Group
      │   ├── Transform
      │   ├── Switch
      │   ├── LOD
      │   └── ...
      ├── Drawable
      │   ├── Geometry
      │   └── ...
      ├── Camera
      ├── OccluderNode
      └── ...
```

## 核心设计理念

### 组合模式（Composite Pattern）
Node类实现了组合模式，允许：
- 统一处理单个节点和组合节点
- 递归遍历整个场景图结构
- 提供一致的操作接口

### 访问者模式（Visitor Pattern）
通过`accept()`方法支持访问者模式，实现：
- 类型安全的节点访问
- 可扩展的遍历操作
- 分离算法与数据结构

## 主要特性

### 1. 层次结构管理

#### 父节点管理
```cpp
typedef std::vector<Group*> ParentList;

// 获取父节点列表
const ParentList& getParents() const;
ParentList getParents();

// 获取特定父节点
Group* getParent(unsigned int i);
const Group* getParent(unsigned int i) const;

// 获取父节点数量
unsigned int getNumParents() const;
```

#### 节点路径计算
```cpp
// 获取从根节点到此节点的所有路径
NodePathList getParentalNodePaths(osg::Node* haltTraversalAtNode=0) const;

// 获取世界变换矩阵列表
MatrixList getWorldMatrices(const osg::Node* haltTraversalAtNode=0) const;
```

### 2. 回调系统

Node类提供了三种类型的回调：

#### 更新回调（Update Callback）
在更新遍历期间调用，用于动画、物理更新等：
```cpp
// 设置更新回调
void setUpdateCallback(Callback* nc);
template<class T> void setUpdateCallback(const ref_ptr<T>& nc);

// 获取更新回调
Callback* getUpdateCallback();
const Callback* getUpdateCallback() const;

// 添加嵌套回调
void addUpdateCallback(Callback* nc);

// 移除回调
void removeUpdateCallback(Callback* nc);
```

#### 事件回调（Event Callback）
在事件遍历期间调用，处理用户输入等：
```cpp
// 设置事件回调
void setEventCallback(Callback* nc);

// 获取事件回调
Callback* getEventCallback();
const Callback* getEventCallback() const;

// 添加/移除事件回调
void addEventCallback(Callback* nc);
void removeEventCallback(Callback* nc);
```

#### 裁剪回调（Cull Callback）
在裁剪遍历期间调用，用于自定义裁剪逻辑：
```cpp
// 设置裁剪回调
void setCullCallback(Callback* nc);

// 获取裁剪回调
Callback* getCullCallback();
const Callback* getCullCallback() const;

// 添加/移除裁剪回调
void addCullCallback(Callback* nc);
void removeCullCallback(Callback* nc);
```

### 3. 包围体计算

#### 基本包围体操作
```cpp
// 获取包围球
const BoundingSphere& getBound() const;

// 计算包围球（虚函数，子类可重写）
virtual BoundingSphere computeBound() const;

// 标记包围体为脏，强制重新计算
void dirtyBound();

// 设置初始包围体
void setInitialBound(const osg::BoundingSphere& bsphere);
const BoundingSphere& getInitialBound() const;
```

#### 自定义包围体计算
```cpp
struct ComputeBoundingSphereCallback : public osg::Object {
    virtual BoundingSphere computeBound(const osg::Node&) const = 0;
};

// 设置自定义包围体计算回调
void setComputeBoundingSphereCallback(ComputeBoundingSphereCallback* callback);
```

### 4. 裁剪控制

```cpp
// 设置裁剪活动状态
void setCullingActive(bool active);
bool getCullingActive() const;

// 检查是否可以被裁剪
bool isCullingActive() const;

// 获取禁用裁剪的子节点数量
unsigned int getNumChildrenWithCullingDisabled() const;
```

### 5. 节点掩码（NodeMask）

节点掩码用于控制遍历行为：
```cpp
typedef unsigned int NodeMask;

// 设置/获取节点掩码（默认为0xffffffff）
void setNodeMask(NodeMask nm);
NodeMask getNodeMask() const;
```

**使用示例：**
```cpp
// 设置节点只在渲染遍历中可见
node->setNodeMask(0x1);

// 设置节点在所有遍历中都不可见
node->setNodeMask(0x0);

// 设置节点只在阴影渲染中可见
node->setNodeMask(0x2);
```

### 6. 状态管理

```cpp
// 设置状态集
void setStateSet(osg::StateSet* stateset);

// 获取或创建状态集
osg::StateSet* getOrCreateStateSet();

// 获取状态集
osg::StateSet* getStateSet();
const osg::StateSet* getStateSet() const;
```

### 7. 描述信息

```cpp
typedef std::vector<std::string> DescriptionList;

// 设置描述列表
void setDescriptions(const DescriptionList& descriptions);

// 获取描述
DescriptionList& getDescriptions();
const DescriptionList& getDescriptions() const;

// 添加单个描述
void addDescription(const std::string& desc);

// 获取描述数量和单个描述
unsigned int getNumDescriptions() const;
std::string& getDescription(unsigned int i);
const std::string& getDescription(unsigned int i) const;
```

## 类型转换方法

Node类提供了一系列as*()方法进行安全的类型转换：

```cpp
// 基本转换
virtual Node* asNode() { return this; }
virtual const Node* asNode() const { return this; }

// 转换为其他节点类型
virtual Drawable* asDrawable() { return 0; }
virtual Geometry* asGeometry() { return 0; }
virtual Group* asGroup() { return 0; }
virtual Transform* asTransform() { return 0; }
virtual Switch* asSwitch() { return 0; }
virtual Geode* asGeode() { return 0; }
virtual OccluderNode* asOccluderNode() { return 0; }
virtual osgTerrain::Terrain* asTerrain() { return 0; }
```

## 访问者模式接口

```cpp
// 接受访问者
virtual void accept(NodeVisitor& nv);

// 向上遍历父节点
virtual void ascend(NodeVisitor& nv);

// 向下遍历子节点（基类为空实现）
virtual void traverse(NodeVisitor& /*nv*/) {}
```

## 使用示例

### 基本节点创建和设置
```cpp
// 创建节点
osg::ref_ptr<osg::Node> node = new osg::Node();

// 设置名称
node->setName("MyNode");

// 设置描述
node->addDescription("这是一个测试节点");

// 设置节点掩码
node->setNodeMask(0x1);

// 创建并设置状态集
osg::ref_ptr<osg::StateSet> stateset = node->getOrCreateStateSet();
```

### 更新回调示例
```cpp
class RotationCallback : public osg::NodeCallback {
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
        // 旋转逻辑
        osg::Transform* transform = node->asTransform();
        if (transform) {
            // 应用旋转...
        }
        
        // 继续遍历
        traverse(node, nv);
    }
};

// 应用回调
node->setUpdateCallback(new RotationCallback());
```

### 自定义包围体计算
```cpp
class CustomBoundCallback : public osg::Node::ComputeBoundingSphereCallback {
public:
    virtual osg::BoundingSphere computeBound(const osg::Node& node) const {
        // 自定义包围体计算逻辑
        return osg::BoundingSphere(osg::Vec3(0,0,0), 100.0f);
    }
};

node->setComputeBoundingSphereCallback(new CustomBoundCallback());
```

### 节点路径和世界矩阵
```cpp
// 获取节点到根的所有路径
osg::NodePathList paths = node->getParentalNodePaths();

// 获取世界变换矩阵
osg::MatrixList matrices = node->getWorldMatrices();

for (const osg::Matrix& matrix : matrices) {
    osg::Vec3 worldPos = osg::Vec3(0,0,0) * matrix;
    std::cout << "世界坐标: " << worldPos.x() << ", " 
              << worldPos.y() << ", " << worldPos.z() << std::endl;
}
```

## 性能考虑

### 包围体计算优化
- 只有在标记为脏时才重新计算包围体
- 使用合理的初始包围体
- 避免不必要的`dirtyBound()`调用

### 回调使用
- 避免在回调中进行重型计算
- 合理使用嵌套回调
- 及时移除不需要的回调

### 节点掩码
- 使用节点掩码进行高效的遍历控制
- 避免频繁修改节点掩码

## 线程安全

```cpp
// 设置线程安全的引用计数
virtual void setThreadSafeRefUnref(bool threadSafe);
```

Node类的引用计数可以设置为线程安全模式，但大部分操作仍需要外部同步。

## 内存管理

```cpp
// 调整OpenGL对象缓冲区大小
virtual void resizeGLObjectBuffers(unsigned int maxSize);

// 释放OpenGL资源
virtual void releaseGLObjects(osg::State* state = 0) const;
```

## 最佳实践

1. **正确使用智能指针**：始终使用`osg::ref_ptr`管理Node对象
2. **合理设置节点掩码**：使用位操作进行高效的遍历控制
3. **谨慎使用回调**：避免在关键路径上使用重型回调
4. **包围体管理**：只在必要时调用`dirtyBound()`
5. **内存管理**：及时释放不需要的OpenGL资源

Node类是OSG场景图的基础，理解其设计和功能对于高效使用OpenSceneGraph至关重要。 