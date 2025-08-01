/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include <osg/PagedLOD>
#include <osg/CullStack>
#include <osg/Notify>

#include <algorithm>

using namespace osg;

// PerRangeData结构：存储每个距离范围的分页数据
// 每个子节点都有对应的PerRangeData，包含文件名、优先级、过期时间等信息
PagedLOD::PerRangeData::PerRangeData():
    _priorityOffset(0.0f),      // 优先级偏移量：调整加载优先级
    _priorityScale(1.0f),       // 优先级缩放因子：控制优先级计算的权重
    _minExpiryTime(0.0),        // 最小过期时间：子节点保持在内存中的最短时间（秒）
    _minExpiryFrames(0),        // 最小过期帧数：子节点保持在内存中的最少帧数
    _timeStamp(0.0f),           // 时间戳：记录最后一次访问的时间
    _frameNumber(0),            // 帧号：记录最后一次访问的帧号
    _frameNumberOfLastReleaseGLObjects(0) {} // 最后一次释放GL对象的帧号

// 拷贝构造函数：复制PerRangeData的所有成员
PagedLOD::PerRangeData::PerRangeData(const PerRangeData& prd):
    _filename(prd._filename),                   // 外部文件名
    _priorityOffset(prd._priorityOffset),       // 优先级偏移
    _priorityScale(prd._priorityScale),         // 优先级缩放
    _minExpiryTime(prd._minExpiryTime),         // 最小过期时间
    _minExpiryFrames(prd._minExpiryFrames),     // 最小过期帧数
    _timeStamp(prd._timeStamp),                 // 时间戳
    _frameNumber(prd._frameNumber),             // 帧号
    _frameNumberOfLastReleaseGLObjects(prd._frameNumberOfLastReleaseGLObjects), // GL对象释放帧号
    _databaseRequest(prd._databaseRequest) {}   // 数据库请求对象

// 赋值运算符：安全复制PerRangeData对象
PagedLOD::PerRangeData& PagedLOD::PerRangeData::operator = (const PerRangeData& prd)
{
    if (this==&prd) return *this;  // 防止自赋值
    _filename = prd._filename;
    _priorityOffset = prd._priorityOffset;
    _priorityScale = prd._priorityScale;
    _timeStamp = prd._timeStamp;
    _frameNumber = prd._frameNumber;
    _frameNumberOfLastReleaseGLObjects = prd._frameNumberOfLastReleaseGLObjects;
    _databaseRequest = prd._databaseRequest;
    _minExpiryTime = prd._minExpiryTime;
    _minExpiryFrames = prd._minExpiryFrames;
    return *this;
}

// PagedLOD默认构造函数：初始化分页LOD节点
PagedLOD::PagedLOD()
{
    _frameNumberOfLastTraversal = 0;           // 最后一次遍历的帧号
    _centerMode = USER_DEFINED_CENTER;         // 中心模式：使用用户定义的中心点（与LOD默认值不同）
    _radius = -1;                              // 半径：-1表示使用自动计算的包围球
    _numChildrenThatCannotBeExpired = 0;       // 不可过期的子节点数量：确保始终保留的最小子节点数
    _disableExternalChildrenPaging = false;    // 是否禁用外部子节点分页：false表示启用分页功能
}

// PagedLOD拷贝构造函数：用于深拷贝和浅拷贝
PagedLOD::PagedLOD(const PagedLOD& plod,const CopyOp& copyop):
    LOD(plod,copyop),                          // 调用基类LOD的拷贝构造函数
    _databaseOptions(plod._databaseOptions),   // 数据库选项
    _databasePath(plod._databasePath),         // 数据库路径
    _frameNumberOfLastTraversal(plod._frameNumberOfLastTraversal), // 最后遍历帧号
    _numChildrenThatCannotBeExpired(plod._numChildrenThatCannotBeExpired), // 不可过期子节点数
    _disableExternalChildrenPaging(plod._disableExternalChildrenPaging),   // 分页禁用标志
    _perRangeDataList(plod._perRangeDataList)  // 每个范围的数据列表
{
}

// PagedLOD析构函数
PagedLOD::~PagedLOD()
{
}

// 设置数据库路径：用于前缀子节点的文件名
// 这个路径会被添加到子节点文件名前面，形成完整的文件路径
void PagedLOD::setDatabasePath(const std::string& path)
{
    _databasePath = path;
    if (!_databasePath.empty())
    {
        // 确保路径以正确的斜杠结尾
        char& lastCharacter = _databasePath[_databasePath.size()-1];
        const char unixSlash = '/';     // Unix风格的路径分隔符
        const char winSlash = '\\';     // Windows风格的路径分隔符

        // 将Windows风格的反斜杠转换为Unix风格的正斜杠
        if (lastCharacter==winSlash)
        {
            lastCharacter = unixSlash;
        }
        // 如果路径末尾没有斜杠，则添加一个
        else if (lastCharacter!=unixSlash)
        {
            _databasePath += unixSlash;
        }

/*
        // 注释掉的代码：原本用于确保路径分隔符的正确性
        // make sure the last character is the appropriate slash
        #ifdef WIN32
            const char slash = '\\';
        #else
            const char slash = '/';
        #endif
        if (lastCharacter != slash)
        {
            _databasePath += slash;
        }
#else
        if (lastCharacter==winSlash)
        {
            lastCharacter = unixSlash;
        }
        else if (lastCharacter!=unixSlash)
        {
            _databasePath += unixSlash;
        }
#endif
*/
    }
}


// traverse方法：PagedLOD的核心功能，负责遍历和动态加载子节点
// 这是PagedLOD最重要的方法，实现了基于距离的动态分页加载
void PagedLOD::traverse(NodeVisitor& nv)
{
    // 设置遍历的帧号，让外部节点知道这个节点的活跃程度
    // 只在剔除访问器（CULL_VISITOR）遍历时更新，因为只有在渲染时才需要考虑LOD
    if (nv.getFrameStamp() &&
        nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR)
    {
        setFrameNumberOfLastTraversal(nv.getFrameStamp()->getFrameNumber());
    }

    // 获取当前时间戳和帧号，用于记录访问信息
    double timeStamp = nv.getFrameStamp()?nv.getFrameStamp()->getReferenceTime():0.0;
    unsigned int frameNumber = nv.getFrameStamp()?nv.getFrameStamp()->getFrameNumber():0;
    bool updateTimeStamp = nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR;

    // 根据遍历模式决定如何处理子节点
    switch(nv.getTraversalMode())
    {
        case(NodeVisitor::TRAVERSE_ALL_CHILDREN):
            // 遍历所有子节点模式：直接访问所有已加载的子节点
            std::for_each(_children.begin(),_children.end(),NodeAcceptOp(nv));
            break;
            
        case(NodeVisitor::TRAVERSE_ACTIVE_CHILDREN):
        {
            // 遍历活跃子节点模式：根据距离或像素大小选择合适的子节点
            float required_range = 0;
            
            // 根据范围模式计算所需的范围值
            if (_rangeMode==DISTANCE_FROM_EYE_POINT)
            {
                // 距离模式：计算观察点到节点中心的距离
                required_range = nv.getDistanceToViewPoint(getCenter(),true);
            }
            else
            {
                // 像素大小模式：根据屏幕上的像素大小计算范围
                osg::CullStack* cullStack = nv.asCullStack();
                if (cullStack && cullStack->getLODScale()>0.0f)
                {
                    // 使用剔除栈计算限制的像素大小，除以LOD缩放因子
                    required_range = cullStack->clampedPixelSize(getBound()) / cullStack->getLODScale();
                }
                else
                {
                    // 后备方案：选择最高分辨率的瓦片，通过找出最大范围
                    for(unsigned int i=0;i<_rangeList.size();++i)
                    {
                        required_range = osg::maximum(required_range,_rangeList[i].first);
                    }
                }
            }

            int lastChildTraversed = -1;      // 最后一个遍历的子节点索引
            bool needToLoadChild = false;     // 是否需要加载新的子节点
            
            // 遍历所有范围，找到符合当前距离的子节点
            for(unsigned int i=0;i<_rangeList.size();++i)
            {
                // 检查当前距离是否在这个范围内：min <= distance < max
                if (_rangeList[i].first<=required_range && required_range<_rangeList[i].second)
                {
                    // 如果对应的子节点已经加载
                    if (i<_children.size())
                    {
                        // 更新访问时间戳，用于过期策略
                        if (updateTimeStamp)
                        {
                            _perRangeDataList[i]._timeStamp=timeStamp;
                            _perRangeDataList[i]._frameNumber=frameNumber;
                        }

                        // 遍历这个子节点
                        _children[i]->accept(nv);
                        lastChildTraversed = (int)i;
                    }
                    else
                    {
                        // 子节点尚未加载，标记需要加载
                        needToLoadChild = true;
                    }
                }
            }

            // 如果需要加载新的子节点
            if (needToLoadChild)
            {
                unsigned int numChildren = _children.size();

                // 选择最后一个有效的子节点作为临时显示
                // 这样可以在加载新子节点时保持场景的连续性
                if (numChildren>0 && ((int)numChildren-1)!=lastChildTraversed)
                {
                    if (updateTimeStamp)
                    {
                        _perRangeDataList[numChildren-1]._timeStamp=timeStamp;
                        _perRangeDataList[numChildren-1]._frameNumber=frameNumber;
                    }
                    _children[numChildren-1]->accept(nv);
                }

                // 请求加载下一个未加载的子节点
                if (!_disableExternalChildrenPaging &&           // 分页功能未禁用
                    nv.getDatabaseRequestHandler() &&            // 有数据库请求处理器
                    numChildren<_perRangeDataList.size())         // 还有未加载的子节点
                {
                    // 根据距离在所需范围中的位置计算优先级
                    // 距离越接近范围的最小值，优先级越高
                    float priority = (_rangeList[numChildren].second-required_range)/(_rangeList[numChildren].second-_rangeList[numChildren].first);

                    // 对于PIXEL_SIZE_ON_SCREEN模式，反转优先级
                    // 因为在像素模式下，较大的值表示更重要
                    if(_rangeMode==PIXEL_SIZE_ON_SCREEN)
                    {
                        priority = -priority;
                    }

                    // 根据子节点的优先级偏移和缩放调整优先级
                    priority = _perRangeDataList[numChildren]._priorityOffset + priority * _perRangeDataList[numChildren]._priorityScale;

                    // 发起文件加载请求
                    if (_databasePath.empty())
                    {
                        // 没有数据库路径，直接使用文件名
                        nv.getDatabaseRequestHandler()->requestNodeFile(_perRangeDataList[numChildren]._filename,nv.getNodePath(),priority,nv.getFrameStamp(), _perRangeDataList[numChildren]._databaseRequest, _databaseOptions.get());
                    }
                    else
                    {
                        // 有数据库路径，将路径前缀添加到子节点的文件名前
                        nv.getDatabaseRequestHandler()->requestNodeFile(_databasePath+_perRangeDataList[numChildren]._filename,nv.getNodePath(),priority,nv.getFrameStamp(), _perRangeDataList[numChildren]._databaseRequest, _databaseOptions.get());
                    }
                }

            }


           break;
        }
        default:
            break;
    }
}


// 扩展PerRangeData列表到指定位置
// 确保_perRangeDataList有足够的元素来存储指定位置的数据
void PagedLOD::expandPerRangeDataTo(unsigned int pos)
{
    if (pos>=_perRangeDataList.size()) _perRangeDataList.resize(pos+1);
}

bool PagedLOD::addChild( Node *child )
{
    if (LOD::addChild(child))
    {
        expandPerRangeDataTo(_children.size()-1);
        return true;
    }
    return false;
}

bool PagedLOD::addChild(Node *child, float min, float max)
{
    if (LOD::addChild(child,min,max))
    {
        expandPerRangeDataTo(_children.size()-1);
        return true;
    }
    return false;
}


bool PagedLOD::addChild(Node *child, float min, float max,const std::string& filename, float priorityOffset, float priorityScale)
{
    if (LOD::addChild(child,min,max))
    {
        setFileName(_children.size()-1,filename);
        setPriorityOffset(_children.size()-1,priorityOffset);
        setPriorityScale(_children.size()-1,priorityScale);
        return true;
    }
    return false;
}

bool PagedLOD::removeChildren( unsigned int pos,unsigned int numChildrenToRemove)
{
    if (pos<_rangeList.size()) _rangeList.erase(_rangeList.begin()+pos, osg::minimum(_rangeList.begin()+(pos+numChildrenToRemove), _rangeList.end()) );
    if (pos<_perRangeDataList.size()) _perRangeDataList.erase(_perRangeDataList.begin()+pos, osg::minimum(_perRangeDataList.begin()+ (pos+numChildrenToRemove), _perRangeDataList.end()) );

    return Group::removeChildren(pos,numChildrenToRemove);
}

bool PagedLOD::removeExpiredChildren(double expiryTime, unsigned int expiryFrame, NodeList& removedChildren)
{
    if (_children.size()>_numChildrenThatCannotBeExpired)
    {
        unsigned cindex = _children.size() - 1;
        if (!_perRangeDataList[cindex]._filename.empty() &&
            _perRangeDataList[cindex]._timeStamp + _perRangeDataList[cindex]._minExpiryTime < expiryTime &&
            _perRangeDataList[cindex]._frameNumber + _perRangeDataList[cindex]._minExpiryFrames < expiryFrame)
        {
            osg::Node* nodeToRemove = _children[cindex].get();
            removedChildren.push_back(nodeToRemove);
            return Group::removeChildren(cindex,1);
        }
    }
    return false;
}
