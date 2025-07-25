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

#ifndef OSG_POLYTOPE
#define OSG_POLYTOPE 1

#include <osg/Plane>
#include <osg/fast_back_stack>

namespace osg {


/** 用于表示由平面集合组成的凸裁剪体积的多面体类。
  * 添加平面时，它们的法线应该指向内部（指向体积内部） */
class OSG_EXPORT Polytope
{

    public:

        typedef unsigned int                    ClippingMask;    // 裁剪掩码类型
        typedef std::vector<Plane>              PlaneList;       // 平面列表类型
        typedef std::vector<Vec3>               VertexList;      // 顶点列表类型
        typedef fast_back_stack<ClippingMask>   MaskStack;       // 掩码栈类型

        inline Polytope() {setupMask();}

        inline Polytope(const Polytope& cv) :
            _maskStack(cv._maskStack),
            _resultMask(cv._resultMask),
            _planeList(cv._planeList),
            _referenceVertexList(cv._referenceVertexList) {}

        inline Polytope(const PlaneList& pl) : _planeList(pl) {setupMask();}

        inline ~Polytope() {}

        inline void clear() { _planeList.clear(); setupMask(); }

        inline Polytope& operator = (const Polytope& cv)
        {
            if (&cv==this) return *this;
            _maskStack = cv._maskStack;
            _resultMask = cv._resultMask;
            _planeList = cv._planeList;
            _referenceVertexList = cv._referenceVertexList;
            return *this;
        }

        /** 创建一个多面体，它是一个以0,0,0为中心、边长为2个单位的立方体。*/
        void setToUnitFrustum(bool withNear=true, bool withFar=true)
        {
            _planeList.clear();
            _planeList.push_back(Plane(1.0,0.0,0.0,1.0)); // 左平面
            _planeList.push_back(Plane(-1.0,0.0,0.0,1.0)); // 右平面
            _planeList.push_back(Plane(0.0,1.0,0.0,1.0)); // 底平面
            _planeList.push_back(Plane(0.0,-1.0,0.0,1.0)); // 顶平面
            if (withNear) _planeList.push_back(Plane(0.0,0.0,1.0,1.0)); // 近平面
            if (withFar) _planeList.push_back(Plane(0.0,0.0,-1.0,1.0)); // 远平面
            setupMask();
        }

        /** 创建一个等效于BoundingBox的多面体。*/
        void setToBoundingBox(const BoundingBox& bb)
        {
            _planeList.clear();
            _planeList.push_back(Plane(1.0,0.0,0.0,-bb.xMin())); // 左平面
            _planeList.push_back(Plane(-1.0,0.0,0.0,bb.xMax())); // 右平面
            _planeList.push_back(Plane(0.0,1.0,0.0,-bb.yMin())); // 底平面
            _planeList.push_back(Plane(0.0,-1.0,0.0,bb.yMax())); // 顶平面
            _planeList.push_back(Plane(0.0,0.0,1.0,-bb.zMin())); // 近平面
            _planeList.push_back(Plane(0.0,0.0,-1.0,bb.zMax())); // 远平面
            setupMask();
        }

        inline void setAndTransformProvidingInverse(const Polytope& pt, const osg::Matrix& matrix)
        {
            _referenceVertexList = pt._referenceVertexList;

            unsigned int resultMask = pt._maskStack.back();
            if (resultMask==0)
            {
                _maskStack.back() = 0;
                _resultMask = 0;
                _planeList.clear();
                return;
            }

            ClippingMask selector_mask = 0x1;

            unsigned int numActivePlanes = 0;

            // 计算活动平面的数量
            PlaneList::const_iterator itr;
            for(itr=pt._planeList.begin();
                itr!=pt._planeList.end();
                ++itr)
            {
                if (resultMask&selector_mask) ++numActivePlanes;
                selector_mask <<= 1;
            }

            _planeList.resize(numActivePlanes);
            _resultMask = 0;
            selector_mask = 0x1;
            unsigned int index = 0;
            for(itr=pt._planeList.begin();
                itr!=pt._planeList.end();
                ++itr)
            {
                if (resultMask&selector_mask)
                {
                     _planeList[index] = *itr;
                     _planeList[index++].transformProvidingInverse(matrix);
                    _resultMask = (_resultMask<<1) | 1;
                }
                selector_mask <<= 1;
            }

            _maskStack.back() = _resultMask;
        }

        inline void set(const PlaneList& pl) { _planeList = pl; setupMask(); }


        inline void add(const osg::Plane& pl) { _planeList.push_back(pl); setupMask(); }

        /** 翻转/反转所有平面的方向。*/
        inline void flip()
        {
            for(PlaneList::iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                itr->flip();
            }
        }

        inline bool empty() const { return _planeList.empty(); }

        inline PlaneList& getPlaneList() { return _planeList; }

        inline const PlaneList& getPlaneList() const { return _planeList; }


        inline void setReferenceVertexList(VertexList& vertices) { _referenceVertexList=vertices; }

        inline VertexList& getReferenceVertexList() { return _referenceVertexList; }

        inline const VertexList& getReferenceVertexList() const { return _referenceVertexList; }


        inline void setupMask()
        {
            _resultMask = 0;
            for(unsigned int i=0;i<_planeList.size();++i)
            {
                _resultMask = (_resultMask<<1) | 1;
            }
            _maskStack.push_back(_resultMask);
        }

        inline ClippingMask& getCurrentMask() { return _maskStack.back(); }

        inline ClippingMask getCurrentMask() const { return _maskStack.back(); }

        inline void setResultMask(ClippingMask mask) { _resultMask=mask; }

        inline ClippingMask getResultMask() const { return _resultMask; }

        MaskStack& getMaskStack() { return _maskStack; }

        const MaskStack& getMaskStack() const { return _maskStack; }


        inline void pushCurrentMask()
        {
            _maskStack.push_back(_resultMask);
        }

        inline void popCurrentMask()
        {
            _maskStack.pop_back();
        }

        /** 检查顶点是否包含在裁剪集内。*/
        inline bool contains(const osg::Vec3& v) const
        {
            if (!_maskStack.back()) return true;

            unsigned int selector_mask = 0x1;
            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if ((_maskStack.back()&selector_mask) && (itr->distance(v)<0.0f)) return false;
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查顶点列表的任何部分是否包含在裁剪集内。*/
        inline bool contains(const std::vector<Vec3>& vertices)
        {
            if (!_maskStack.back()) return true;

            _resultMask = _maskStack.back();

            for(std::vector<Vec3>::const_iterator vitr = vertices.begin();
                vitr != vertices.end();
                ++vitr)
            {
                const osg::Vec3& v = *vitr;
                bool outside = false;
                ClippingMask selector_mask = 0x1;
                for(PlaneList::const_iterator itr=_planeList.begin();
                    itr!=_planeList.end() && !outside;
                    ++itr)
                {
                    if ((_maskStack.back()&selector_mask) && (itr->distance(v)<0.0f)) outside = true;
                    selector_mask <<= 1;
                }

                if (!outside) return true;
            }
            return false;
        }

        /** 检查包围球的任何部分是否包含在裁剪集内。
            使用掩码来确定应该使用哪些平面进行检查，并
            修改掩码以关闭不会对任何内部对象的裁剪有贡献的平面。
            此功能在osgUtil::CullVisitor中使用
            来防止冗余的平面检查。*/
        inline bool contains(const osg::BoundingSphere& bs)
        {
            if (!_maskStack.back()) return true;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;

            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    int res=itr->intersect(bs);
                    if (res<0) return false; // 在裁剪集外部
                    else if (res>0) _resultMask ^= selector_mask; // 后续对此平面的检查不需要
                }
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查包围盒的任何部分是否包含在裁剪集内。
            使用掩码来确定应该使用哪些平面进行检查，并
            修改掩码以关闭不会对任何内部对象的裁剪有贡献的平面。
            此功能在osgUtil::CullVisitor中使用
            来防止冗余的平面检查。*/
        inline bool contains(const osg::BoundingBox& bb)
        {
            if (!_maskStack.back()) return true;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;

            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    int res=itr->intersect(bb);
                    if (res<0) return false; // 在裁剪集外部
                    else if (res>0) _resultMask ^= selector_mask; // 后续对此平面的检查不需要
                }
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查顶点列表的所有部分是否都包含在裁剪集内。*/
        inline bool containsAllOf(const std::vector<Vec3>& vertices)
        {
            if (!_maskStack.back()) return false;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;

            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    int res=itr->intersect(vertices);
                    if (res<1) return false;  // 相交，或在平面下方
                    _resultMask ^= selector_mask; // 后续对此平面的检查不需要
                }
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查整个包围球是否包含在裁剪集内。*/
        inline bool containsAllOf(const osg::BoundingSphere& bs)
        {
            if (!_maskStack.back()) return false;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;

            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    int res=itr->intersect(bs);
                    if (res<1) return false;  // 相交，或在平面下方
                    _resultMask ^= selector_mask; // 后续对此平面的检查不需要
                }
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查整个包围盒是否包含在裁剪集内。*/
        inline bool containsAllOf(const osg::BoundingBox& bb)
        {
            if (!_maskStack.back()) return false;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;

            for(PlaneList::const_iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    int res=itr->intersect(bb);
                    if (res<1) return false;  // 相交，或在平面下方
                    _resultMask ^= selector_mask; // 后续对此平面的检查不需要
                }
                selector_mask <<= 1;
            }
            return true;
        }

        /** 检查三角形的任何部分是否包含在多面体内。*/
        bool contains(const osg::Vec3f& v0, const osg::Vec3f& v1, const osg::Vec3f& v2) const;


        /** 通过矩阵变换裁剪集。注意，此操作执行
          * 矩阵逆的计算，因为平面必须
          * 乘以逆转置来变换它。这
          * 使得此操作昂贵。如果逆已经在
          * 其他地方计算过，则使用transformProvidingInverse()。
          * 参见 http://www.worldserver.com/turk/computergraphics/NormalTransformations.pdf*/
        inline void transform(const osg::Matrix& matrix)
        {
            osg::Matrix inverse;
            inverse.invert(matrix);
            transformProvidingInverse(inverse);
        }

        /** 通过提供预逆矩阵来变换裁剪集。
          * 详细信息参见transform。 */
        inline void transformProvidingInverse(const osg::Matrix& matrix)
        {
            if (!_maskStack.back()) return;

            _resultMask = _maskStack.back();
            ClippingMask selector_mask = 0x1;
            for(PlaneList::iterator itr=_planeList.begin();
                itr!=_planeList.end();
                ++itr)
            {
                if (_resultMask&selector_mask)
                {
                    itr->transformProvidingInverse(matrix);
                }
                selector_mask <<= 1;
            }
        }

    protected:


        MaskStack                           _maskStack;             // 掩码栈
        ClippingMask                        _resultMask;            // 结果掩码
        PlaneList                           _planeList;             // 平面列表
        VertexList                          _referenceVertexList;   // 参考顶点列表

};

}    // end of namespace

#endif 