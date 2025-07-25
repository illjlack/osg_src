# PagedLOD类详细文档

## 概述

**PagedLOD**（Paged Level of Detail）是OpenSceneGraph中一个重要的节点类型，它继承自**LOD**类，专门用于实现大规模场景的动态分页加载和细节层次管理。

## 类继承关系

```
Object
  └── Node
      └── Group  
          └── LOD
              └── PagedLOD
```

## 核心功能

### 1. 动态分页加载
PagedLOD能够根据视点距离动态地从外部文件加载和卸载子节点，这对于处理大规模地形、城市模型等场景至关重要。

### 2. 内存管理
- **自动过期机制**：基于时间和帧数的子节点过期策略
- **最小保留数量**：确保始终保留指定数量的子节点在内存中
- **数据库分页器集成**：与osgDB::DatabasePager无缝协作

### 3. 优先级控制
- **优先级偏移**：控制加载请求的优先级
- **优先级缩放**：调整优先级计算的缩放因子

## 主要特性

### 构造函数特点
```cpp
PagedLOD();
```
- 中心模式设置为 `USER_DEFINED_CENTER`（与LOD的默认设置不同）
- 启用外部子节点分页功能

### 核心数据结构

#### PerRangeData结构
每个距离范围对应一个PerRangeData对象，包含：
- `_filename`：外部文件名
- `_priorityOffset`：优先级偏移
- `_priorityScale`：优先级缩放
- `_minExpiryTime`：最小过期时间
- `_minExpiryFrames`：最小过期帧数
- `_timeStamp`：时间戳
- `_frameNumber`：帧号
- `_databaseRequest`：数据库请求对象

## 主要方法

### 子节点管理
```cpp
// 添加子节点（带距离范围和文件名）
bool addChild(Node *child, float rmin, float rmax, 
              const std::string& filename, 
              float priorityOffset=0.0f, 
              float priorityScale=1.0f);

// 移除过期的子节点
bool removeExpiredChildren(double expiryTime, 
                          unsigned int expiryFrame, 
                          NodeList& removedChildren);
```

### 文件名管理
```cpp
// 设置/获取指定子节点的文件名
void setFileName(unsigned int childNo, const std::string& filename);
const std::string& getFileName(unsigned int childNo) const;

// 设置/获取数据库路径
void setDatabasePath(const std::string& path);
const std::string& getDatabasePath() const;
```

### 优先级控制
```cpp
// 设置优先级偏移和缩放
void setPriorityOffset(unsigned int childNo, float priorityOffset);
void setPriorityScale(unsigned int childNo, float priorityScale);
```

### 过期策略
```cpp
// 设置最小过期时间（秒）
void setMinimumExpiryTime(unsigned int childNo, double minTime);

// 设置最小过期帧数
void setMinimumExpiryFrames(unsigned int childNo, unsigned int minFrames);

// 设置不可过期的子节点数量
void setNumChildrenThatCannotBeExpired(unsigned int num);
```

## 使用示例

### 基本PagedLOD设置
```cpp
// 创建PagedLOD节点
osg::ref_ptr<osg::PagedLOD> pagedLOD = new osg::PagedLOD;

// 设置中心点和半径
pagedLOD->setCenter(osg::Vec3(0, 0, 0));
pagedLOD->setRadius(1000.0f);

// 设置数据库路径
pagedLOD->setDatabasePath("/path/to/models/");

// 添加低细节层次（近距离，内存中的几何体）
osg::ref_ptr<osg::Node> lowDetail = createLowDetailGeometry();
pagedLOD->addChild(lowDetail.get(), 0.0f, 500.0f);

// 添加高细节层次（远距离，外部文件）
pagedLOD->addChild(nullptr, 500.0f, FLT_MAX, "high_detail.osg");

// 设置过期策略
pagedLOD->setMinimumExpiryTime(0, 10.0);  // 10秒后可以过期
pagedLOD->setMinimumExpiryFrames(0, 60);  // 60帧后可以过期
pagedLOD->setNumChildrenThatCannotBeExpired(1);  // 至少保留1个子节点
```

### 地形分页示例
```cpp
// 创建地形瓦片的PagedLOD
osg::ref_ptr<osg::PagedLOD> createTerrainTile(int x, int y, int lod) {
    osg::ref_ptr<osg::PagedLOD> pagedLOD = new osg::PagedLOD;
    
    // 生成文件名
    std::string filename = "terrain_" + std::to_string(lod) + 
                          "_" + std::to_string(x) + 
                          "_" + std::to_string(y) + ".osg";
    
    // 设置文件名和距离范围
    pagedLOD->setFileName(0, filename);
    pagedLOD->setRange(0, 0.0f, 1000.0f);
    
    // 设置优先级
    pagedLOD->setPriorityOffset(0, lod);  // LOD层级越高，优先级越低
    pagedLOD->setPriorityScale(0, 1.0f);
    
    // 设置中心点（基于瓦片坐标计算）
    osg::Vec3 center(x * 1000.0f, y * 1000.0f, 0.0f);
    pagedLOD->setCenter(center);
    pagedLOD->setRadius(707.0f);  // sqrt(1000^2 + 1000^2) / 2
    
    return pagedLOD;
}
```

## 与DatabasePager的协作

PagedLOD与osgDB::DatabasePager紧密配合工作：

1. **请求队列**：PagedLOD将加载请求提交给DatabasePager
2. **后台加载**：DatabasePager在后台线程中加载外部文件
3. **动态插入**：加载完成后，子节点被动态插入到PagedLOD中
4. **内存管理**：DatabasePager管理内存使用，自动卸载不需要的节点

## 性能考虑

### 优化建议
1. **合理设置距离范围**：避免频繁的加载/卸载
2. **控制文件大小**：保持外部文件在合理大小范围内
3. **使用优先级系统**：确保重要内容优先加载
4. **设置合理的过期策略**：平衡内存使用和性能

### 常见应用场景
- **大规模地形渲染**：地形LOD瓦片
- **城市建模**：建筑物细节层次
- **飞行模拟**：机场和地标的动态加载
- **虚拟现实**：大场景的实时流式传输

## 注意事项

1. **中心模式差异**：PagedLOD默认使用`USER_DEFINED_CENTER`，与LOD的默认设置不同
2. **线程安全**：确保在多线程环境中正确使用
3. **文件路径**：确保外部文件路径正确且可访问
4. **内存管理**：监控内存使用，避免内存泄漏

## 相关类

- **osg::LOD**：基类，提供基本的LOD功能
- **osgDB::DatabasePager**：数据库分页器，处理文件加载
- **osg::ProxyNode**：另一种延迟加载节点
- **osgTerrain::TerrainTile**：地形瓦片的特殊实现 