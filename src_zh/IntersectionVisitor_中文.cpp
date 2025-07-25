/* -*-c++-*- OpenSceneGraph - 版权所有 (C) 1998-2006 Robert Osfield
 *
 * 本库是开源的，可以在OpenSceneGraph公共许可证(OSGPL) 0.0版本或
 * (您选择的)任何更新版本的条款下重新分发和/或修改。完整的许可证在
 * 本发行版附带的LICENSE文件中，以及openscenegraph.org网站上。
 *
 * 本库的分发希望对您有用，但不提供任何保证；甚至不暗示
 * 适销性或特定用途适用性的保证。详见OpenSceneGraph公共许可证。
*/


#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <osg/PagedLOD>
#include <osg/Transform>
#include <osg/Projection>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/Billboard>
#include <osg/Geometry>
#include <osg/Notify>
#include <osg/io_utils>

using namespace osgUtil;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IntersectorGroup - 相交器组
//

IntersectorGroup::IntersectorGroup()
{
}

void IntersectorGroup::addIntersector(Intersector* intersector)
{
    _intersectors.push_back(intersector);
}

void IntersectorGroup::clear()
{
    _intersectors.clear();
}

Intersector* IntersectorGroup::clone(osgUtil::IntersectionVisitor& iv)
{
    IntersectorGroup* ig = new IntersectorGroup;

    // 现在复制所有未被禁用的相交器
    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        if (!(*itr)->disabled())
        {
            ig->addIntersector( (*itr)->clone(iv) );
        }
    }

    return ig;
}

bool IntersectorGroup::enter(const osg::Node& node)
{
    if (disabled()) return false;

    bool foundIntersections = false;

    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        if ((*itr)->disabled()) (*itr)->incrementDisabledCount();
        else if ((*itr)->enter(node)) foundIntersections = true;
        else (*itr)->incrementDisabledCount();
    }

    if (!foundIntersections)
    {
        // 需要调用leave来清理DisabledCount
        leave();
        return false;
    }

    // 我们找到了至少一个合适的相交器，所以返回true
    return true;
}

void IntersectorGroup::leave()
{
    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        if ((*itr)->disabled()) (*itr)->decrementDisabledCount();
    }
}

void IntersectorGroup::intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable)
{
    if (disabled()) return;

    unsigned int numTested = 0;
    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        if (!(*itr)->disabled())
        {
            (*itr)->intersect(iv, drawable);

            ++numTested;
        }
    }

    // OSG_NOTICE<<"Number testing "<<numTested<<std::endl;

}

void IntersectorGroup::reset()
{
    Intersector::reset();

    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        (*itr)->reset();
    }
}

bool IntersectorGroup::containsIntersections()
{
    for(Intersectors::iterator itr = _intersectors.begin();
        itr != _intersectors.end();
        ++itr)
    {
        if ((*itr)->containsIntersections()) return true;
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IntersectionVisitor - 相交访问者
//

IntersectionVisitor::IntersectionVisitor(Intersector* intersector, ReadCallback* readCallback):
    osg::NodeVisitor(osg::NodeVisitor::INTERSECTION_VISITOR, osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
{
    _useKdTreesWhenAvailable = true;
    _dummyTraversal = false;

    _lodSelectionMode = USE_HIGHEST_LEVEL_OF_DETAIL;
    _eyePointDirty = true;

    LineSegmentIntersector* ls = dynamic_cast<LineSegmentIntersector*>(intersector);
    if (ls)
    {
        setReferenceEyePoint(ls->getStart());
        setReferenceEyePointCoordinateFrame(ls->getCoordinateFrame());
    }
    else
    {
        setReferenceEyePoint(osg::Vec3(0.0f,0.0f,0.0f));
        setReferenceEyePointCoordinateFrame(Intersector::VIEW);
    }

    setIntersector(intersector);

    setReadCallback(readCallback);
}

void IntersectionVisitor::setIntersector(Intersector* intersector)
{
    // 保持引用以防相交器已经在_intersectorStack中，否则clear可能会删除它
    osg::ref_ptr<Intersector> temp = intersector;

    _intersectorStack.clear();

    if (intersector) _intersectorStack.push_back(intersector);
}

void IntersectionVisitor::reset()
{
    if (!_intersectorStack.empty())
    {
        osg::ref_ptr<Intersector> intersector = _intersectorStack.front();
        intersector->reset();

        _intersectorStack.clear();
        _intersectorStack.push_back(intersector);
    }
}

void IntersectionVisitor::apply(osg::Node& node)
{
    // OSG_NOTICE<<"apply(Node&)"<<std::endl;

    if (!enter(node)) return;

    // OSG_NOTICE<<"inside apply(Node&)"<<std::endl;

    traverse(node);

    leave();
}

void IntersectionVisitor::apply(osg::Group& group)
{
    if (!enter(group)) return;

    traverse(group);

    leave();
}

void IntersectionVisitor::apply(osg::Drawable& drawable)
{
    intersect( &drawable );
}

void IntersectionVisitor::apply(osg::Geode& geode)
{
    // OSG_NOTICE<<"apply(Geode&)"<<std::endl;

    if (!enter(geode)) return;

    // OSG_NOTICE<<"inside apply(Geode&)"<<std::endl;

    for(unsigned int i=0; i<geode.getNumChildren(); ++i)
    {
        geode.getChild(i)->accept(*this);
    }

    leave();
}

void IntersectionVisitor::apply(osg::Billboard& billboard)
{
    if (!enter(billboard)) return;

#if 1
    // IntersectVisitor没有getEyeLocal()，我们可以使用NodeVisitor::getEyePoint()吗？
    osg::Vec3 eye_local = getEyePoint();

    for(unsigned int i = 0; i < billboard.getNumChildren(); i++ )
    {
        const osg::Vec3& pos = billboard.getPosition(i);
        osg::ref_ptr<osg::RefMatrix> billboard_matrix = new osg::RefMatrix;
        if (getViewMatrix())
        {
            if (getModelMatrix()) billboard_matrix->mult( *getModelMatrix(), *getViewMatrix() );
            else billboard_matrix->set( *getViewMatrix() );
        }
        else if (getModelMatrix()) billboard_matrix->set( *getModelMatrix() );

        billboard.computeMatrix(*billboard_matrix,eye_local,pos);

        if (getViewMatrix()) billboard_matrix->postMult( osg::Matrix::inverse(*getViewMatrix()) );
        pushModelMatrix(billboard_matrix.get());

        // 现在推送一个新的相交器克隆变换到新的局部坐标
        push_clone();

        billboard.getChild(i)->accept(*this);

        // 现在推送一个新的相交器克隆变换到新的局部坐标
        pop_clone();

        popModelMatrix();

    }
#else

    for(unsigned int i=0; i<billboard.getNumDrawables(); ++i)
    {
        intersect( billboard.getDrawable(i) );
    }
#endif

    leave();
}

void IntersectionVisitor::apply(osg::LOD& lod)
{
    if (!enter(lod)) return;

    traverse(lod);

    leave();
}


void IntersectionVisitor::apply(osg::PagedLOD& plod)
{
    if (!enter(plod)) return;

    if (plod.getNumFileNames()>0)
    {
#if 1
        // 确定最高分辨率子节点的范围值
        float targetRangeValue;
        if( plod.getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT )
            targetRangeValue = 1e6; // 初始化高值以查找最小值
        else
            targetRangeValue = 0; // 初始化低值以查找最大值

        const osg::LOD::RangeList rl = plod.getRangeList();
        osg::LOD::RangeList::const_iterator rit;
        for( rit = rl.begin();
             rit != rl.end();
             rit++ )
        {
            if( plod.getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT )
            {
                if( rit->first < targetRangeValue )
                    targetRangeValue = rit->first;
            }
            else
            {
                if( rit->first > targetRangeValue )
                    targetRangeValue = rit->first;
            }
        }

        // 仅对以最大分辨率显示的子节点执行相交测试
        unsigned int childIndex;
        for( rit = rl.begin(), childIndex = 0;
             rit != rl.end();
             rit++, childIndex++ )
        {
            if( rit->first != targetRangeValue )
                // 这不是最高分辨率子节点之一
                continue;

            osg::ref_ptr<osg::Node> child( NULL );
            if( plod.getNumChildren() > childIndex )
                child = plod.getChild( childIndex );

            if( (!child.valid()) && (_readCallback.valid()) )
            {
                // 子节点为NULL；如果我们有readCallback，则尝试加载它...
                unsigned int validIndex( childIndex );
                if (plod.getNumFileNames() <= childIndex)
                    validIndex = plod.getNumFileNames()-1;

                child = _readCallback->readNodeFile( plod.getDatabasePath() + plod.getFileName( validIndex ) );
            }

            if ( !child.valid() && plod.getNumChildren()>0)
            {
                // 子节点仍然为NULL，所以只使用列表末尾的那个
                child = plod.getChild( plod.getNumChildren()-1 );
            }

            if (child.valid())
            {
                child->accept(*this);
            }
        }
#else
        // 比上面的块更旧的代码，假设PagedLOD正确排序
        // 即低分辨率子节点优先，没有重复范围

        osg::ref_ptr<osg::Node> highestResChild;

        if (plod.getNumFileNames() != plod.getNumChildren() && _readCallback.valid())
        {
            highestResChild = _readCallback->readNodeFile( plod.getDatabasePath() + plod.getFileName(plod.getNumFileNames()-1) );
        }

        if ( !highestResChild.valid() && plod.getNumChildren()>0)
        {
            highestResChild = plod.getChild( plod.getNumChildren()-1 );
        }

        if (highestResChild.valid())
        {
            highestResChild->accept(*this);
        }
#endif
    }

    leave();
}


void IntersectionVisitor::apply(osg::Transform& transform)
{
    if (!enter(transform)) return;

    osg::ref_ptr<osg::RefMatrix> matrix = _modelStack.empty() ? new osg::RefMatrix() : new osg::RefMatrix(*_modelStack.back());
    transform.computeLocalToWorldMatrix(*matrix,this);

    // 如果变换是绝对参考，我们想要忽略视图矩阵
    if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
    {
        pushViewMatrix(new osg::RefMatrix());
    }

    pushModelMatrix(matrix.get());

    // 现在推送一个新的相交器克隆变换到新的局部坐标
    push_clone();

    traverse(transform);

    // 弹出克隆
    pop_clone();

    popModelMatrix();

    if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
    {
        popViewMatrix();
    }

    // 清理当前相交器中任何缓存的剔除变量
    leave();
}


void IntersectionVisitor::apply(osg::Projection& projection)
{
    if (!enter(projection)) return;

    pushProjectionMatrix(new osg::RefMatrix(projection.getMatrix()) );

    // 现在推送一个新的相交器克隆变换到新的局部坐标
    push_clone();

    traverse(projection);

    // 弹出克隆
    pop_clone();

    popProjectionMatrix();

    leave();
}


void IntersectionVisitor::apply(osg::Camera& camera)
{
    // OSG_NOTICE<<"apply(Camera&)"<<std::endl;

    // 注意，现在注释掉是因为默认Camera设置是激活剔除的。这应该改变吗？
    // if (!enter(camera)) return;

    // OSG_NOTICE<<"inside apply(Camera&)"<<std::endl;

    osg::RefMatrix* projection = NULL;
    osg::RefMatrix* view = NULL;
    osg::RefMatrix* model = NULL;

    if (camera.getReferenceFrame()==osg::Transform::RELATIVE_RF && getProjectionMatrix() && getViewMatrix())
    {
        if (camera.getTransformOrder()==osg::Camera::POST_MULTIPLY)
        {
            projection = new osg::RefMatrix(*getProjectionMatrix()*camera.getProjectionMatrix());
            view = new osg::RefMatrix(*getViewMatrix()*camera.getViewMatrix());
            model = new osg::RefMatrix(*getModelMatrix());
        }
        else // 前乘
        {
            projection = new osg::RefMatrix(camera.getProjectionMatrix()*(*getProjectionMatrix()));
            view = new osg::RefMatrix(*getViewMatrix());
            model = new osg::RefMatrix(camera.getViewMatrix()*(*getModelMatrix()));
        }
    }
    else
    {
        // 绝对参考框架
        projection = new osg::RefMatrix(camera.getProjectionMatrix());
        view = new osg::RefMatrix(camera.getViewMatrix());
        model =  new osg::RefMatrix();
    }

    if (camera.getViewport()) pushWindowMatrix( camera.getViewport() );
    pushProjectionMatrix(projection);
    pushViewMatrix(view);
    pushModelMatrix(model);

    // 现在推送一个新的相交器克隆变换到新的局部坐标
    push_clone();

    traverse(camera);

    // 弹出克隆
    pop_clone();

    popModelMatrix();
    popViewMatrix();
    popProjectionMatrix();
    if (camera.getViewport()) popWindowMatrix();

    // leave();
}

osg::Vec3 IntersectionVisitor::getEyePoint() const
{
    if (!_eyePointDirty) return _eyePoint;

    osg::Matrix matrix;
    switch (_referenceEyePointCoordinateFrame)
    {
        case(Intersector::WINDOW):
            if (getWindowMatrix()) matrix.preMult( *getWindowMatrix() );
            if (getProjectionMatrix()) matrix.preMult( *getProjectionMatrix() );
            if (getViewMatrix()) matrix.preMult( *getViewMatrix() );
            if (getModelMatrix()) matrix.preMult( *getModelMatrix() );
            break;
        case(Intersector::PROJECTION):
            if (getProjectionMatrix()) matrix.preMult( *getProjectionMatrix() );
            if (getViewMatrix()) matrix.preMult( *getViewMatrix() );
            if (getModelMatrix()) matrix.preMult( *getModelMatrix() );
            break;
        case(Intersector::VIEW):
            if (getViewMatrix()) matrix.preMult( *getViewMatrix() );
            if (getModelMatrix()) matrix.preMult( *getModelMatrix() );
            break;
        case(Intersector::MODEL):
            if (getModelMatrix()) matrix = *getModelMatrix();
            break;
    }

    osg::Matrix inverse;
    inverse.invert(matrix);

    _eyePoint = _referenceEyePoint * inverse;
    _eyePointDirty = false;

    return _eyePoint;
}

float IntersectionVisitor::getDistanceToEyePoint(const osg::Vec3& pos, bool /*withLODScale*/) const
{
    if (_lodSelectionMode==USE_EYE_POINT_FOR_LOD_LEVEL_SELECTION)
    {
        return (pos-getEyePoint()).length();
    }
    else
    {
        return 0.0f;
    }
} 