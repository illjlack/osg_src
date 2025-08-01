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

#ifndef OSGUTIL_OPTIMIZER
#define OSGUTIL_OPTIMIZER

#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/Geometry>
#include <osg/Transform>
#include <osg/Texture2D>

#include <osgUtil/Export>

#include <set>

namespace osgUtil {

// 前向声明
class Optimizer;

/** 实现优化器技术的辅助基类。*/
class OSGUTIL_EXPORT BaseOptimizerVisitor : public osg::NodeVisitor
{
    public:

        BaseOptimizerVisitor(Optimizer* optimizer, unsigned int operation):
            osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
            _optimizer(optimizer),
            _operationType(operation)
        {
            setNodeMaskOverride(0xffffffff);
        }

        inline bool isOperationPermissibleForObject(const osg::StateSet* object) const;
        inline bool isOperationPermissibleForObject(const osg::StateAttribute* object) const;
        inline bool isOperationPermissibleForObject(const osg::Drawable* object) const;
        inline bool isOperationPermissibleForObject(const osg::Node* object) const;

    protected:

        Optimizer*      _optimizer;
        unsigned int _operationType;
};

/** 遍历场景图以提高效率。参见OptimizationOptions。
  * 使用示例请参考examples/osgimpostor或osgviewer。
  */

class OSGUTIL_EXPORT Optimizer
{

    public:

        Optimizer() {}
        virtual ~Optimizer() {}

        enum OptimizationOptions
        {
            FLATTEN_STATIC_TRANSFORMS = (1 << 0),               // 展平静态变换
            REMOVE_REDUNDANT_NODES =    (1 << 1),               // 移除冗余节点
            REMOVE_LOADED_PROXY_NODES = (1 << 2),               // 移除已加载的代理节点
            COMBINE_ADJACENT_LODS =     (1 << 3),               // 合并相邻的LOD
            SHARE_DUPLICATE_STATE =     (1 << 4),               // 共享重复状态
            MERGE_GEOMETRY =            (1 << 5),               // 合并几何体
            CHECK_GEOMETRY =            (1 << 6),               // 检查几何体（已弃用，目前为空操作）
            MAKE_FAST_GEOMETRY =        (1 << 7),               // 制作快速几何体
            SPATIALIZE_GROUPS =         (1 << 8),               // 空间化分组
            COPY_SHARED_NODES =         (1 << 9),               // 复制共享节点
            TRISTRIP_GEOMETRY =         (1 << 10),              // 三角带几何体
            TESSELLATE_GEOMETRY =       (1 << 11),              // 镶嵌几何体
            OPTIMIZE_TEXTURE_SETTINGS = (1 << 12),              // 优化纹理设置
            MERGE_GEODES =              (1 << 13),              // 合并几何节点
            FLATTEN_BILLBOARDS =        (1 << 14),              // 展平广告牌
            TEXTURE_ATLAS_BUILDER =     (1 << 15),              // 纹理图集构建器
            STATIC_OBJECT_DETECTION =   (1 << 16),              // 静态对象检测
            FLATTEN_STATIC_TRANSFORMS_DUPLICATING_SHARED_SUBGRAPHS = (1 << 17), // 展平静态变换并复制共享子图
            INDEX_MESH =                (1 << 18),              // 索引网格
            VERTEX_POSTTRANSFORM =      (1 << 19),              // 顶点后变换
            VERTEX_PRETRANSFORM =       (1 << 20),              // 顶点预变换
            BUFFER_OBJECT_SETTINGS =    (1 << 21),              // 缓冲对象设置
            DEFAULT_OPTIMIZATIONS = FLATTEN_STATIC_TRANSFORMS |
                                REMOVE_REDUNDANT_NODES |
                                REMOVE_LOADED_PROXY_NODES |
                                COMBINE_ADJACENT_LODS |
                                SHARE_DUPLICATE_STATE |
                                MERGE_GEOMETRY |
                                MAKE_FAST_GEOMETRY |
                                CHECK_GEOMETRY |
                                OPTIMIZE_TEXTURE_SETTINGS |
                                STATIC_OBJECT_DETECTION,        // 默认优化
            ALL_OPTIMIZATIONS = FLATTEN_STATIC_TRANSFORMS_DUPLICATING_SHARED_SUBGRAPHS |
                                REMOVE_REDUNDANT_NODES |
                                REMOVE_LOADED_PROXY_NODES |
                                COMBINE_ADJACENT_LODS |
                                SHARE_DUPLICATE_STATE |
                                MERGE_GEODES |
                                MERGE_GEOMETRY |
                                MAKE_FAST_GEOMETRY |
                                CHECK_GEOMETRY |
                                SPATIALIZE_GROUPS |
                                COPY_SHARED_NODES |
                                TRISTRIP_GEOMETRY |
                                OPTIMIZE_TEXTURE_SETTINGS |
                                TEXTURE_ATLAS_BUILDER |
                                STATIC_OBJECT_DETECTION |
                                BUFFER_OBJECT_SETTINGS          // 所有优化
        };

        /** 将内部数据重置为初始状态 - 清除getPermissibleOptionsMap。*/
        void reset();

        /** 使用一系列由OptimizationOptions指定的优化访问者遍历节点及其子图。*/
        void optimize(osg::Node* node);

        template<class T> void optimize(const osg::ref_ptr<T>& node) { optimize(node.get()); }

        /** 使用一系列由OptimizationOptions指定的优化访问者遍历节点及其子图。*/
        virtual void optimize(osg::Node* node, unsigned int options);

        template<class T> void optimize(const osg::ref_ptr<T>& node, unsigned int options) { optimize(node.get(), options); }


        /** 用于自定义场景图中对象允许哪些操作的回调。*/
        struct IsOperationPermissibleForObjectCallback : public osg::Referenced
        {
            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::StateSet* stateset,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(stateset,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::StateAttribute* attribute,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(attribute,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::Drawable* drawable,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(drawable,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::Node* node,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(node,option);
            }

        };

        /** 设置用于自定义场景图中对象允许哪些操作的回调。*/
        void setIsOperationPermissibleForObjectCallback(IsOperationPermissibleForObjectCallback* callback) { _isOperationPermissibleForObjectCallback=callback; }

        /** 获取用于自定义场景图中对象允许哪些操作的回调。*/
        IsOperationPermissibleForObjectCallback* getIsOperationPermissibleForObjectCallback() { return _isOperationPermissibleForObjectCallback.get(); }

        /** 获取用于自定义场景图中对象允许哪些操作的回调。*/
        const IsOperationPermissibleForObjectCallback* getIsOperationPermissibleForObjectCallback() const { return _isOperationPermissibleForObjectCallback.get(); }


        inline void setPermissibleOptimizationsForObject(const osg::Object* object, unsigned int options)
        {
            _permissibleOptimizationsMap[object] = options;
        }

        inline unsigned int getPermissibleOptimizationsForObject(const osg::Object* object) const
        {
            PermissibleOptimizationsMap::const_iterator itr = _permissibleOptimizationsMap.find(object);
            if (itr!=_permissibleOptimizationsMap.end()) return itr->second;
            else return 0xffffffff;
        }


        inline bool isOperationPermissibleForObject(const osg::StateSet* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::StateAttribute* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::Drawable* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::Node* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        bool isOperationPermissibleForObjectImplementation(const osg::StateSet* stateset, unsigned int option) const
        {
            return (option & getPermissibleOptimizationsForObject(stateset))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::StateAttribute* attribute, unsigned int option) const
        {
            return (option & getPermissibleOptimizationsForObject(attribute))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::Drawable* drawable, unsigned int option) const
        {
            if (option & (REMOVE_REDUNDANT_NODES|MERGE_GEOMETRY))
            {
                if (drawable->getUserData()) return false;
                if (drawable->getUpdateCallback()) return false;
                if (drawable->getEventCallback()) return false;
                if (drawable->getCullCallback()) return false;
            }
            return (option & getPermissibleOptimizationsForObject(drawable))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::Node* node, unsigned int option) const
        {
            if (option & (REMOVE_REDUNDANT_NODES|COMBINE_ADJACENT_LODS|FLATTEN_STATIC_TRANSFORMS))
            {
                if (node->getUserData()) return false;
                if (node->getUpdateCallback()) return false;
                if (node->getEventCallback()) return false;
                if (node->getCullCallback()) return false;
                if (node->getNumDescriptions()>0) return false;
                if (node->getStateSet()) return false;
                if (node->getNodeMask()!=0xffffffff) return false;
                // if (!node->getName().empty()) return false;
            }

            return (option & getPermissibleOptimizationsForObject(node))!=0;
        }

    protected:

        osg::ref_ptr<IsOperationPermissibleForObjectCallback> _isOperationPermissibleForObjectCallback;

        typedef std::map<const osg::Object*,unsigned int> PermissibleOptimizationsMap;
        PermissibleOptimizationsMap _permissibleOptimizationsMap;

    public:

        /** 通过将静态变换应用到场景图叶子上的几何体，然后移除现在冗余的变换，
          * 来展平静态变换节点。具有多个父路径的静态变换子图不会被展平，
          * 如果需要，则必须复制子图 - 为此请使用
          * FlattenStaticTransformsDuplicatingSharedSubgraphsVisitor。 */
        class OSGUTIL_EXPORT FlattenStaticTransformsVisitor : public BaseOptimizerVisitor
        {
            public:

                FlattenStaticTransformsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, FLATTEN_STATIC_TRANSFORMS) {}

                virtual void apply(osg::Node& geode);
                virtual void apply(osg::Drawable& drawable);
                virtual void apply(osg::Billboard& geode);
                virtual void apply(osg::ProxyNode& node);
                virtual void apply(osg::PagedLOD& node);
                virtual void apply(osg::Transform& transform);

                bool removeTransforms(osg::Node* nodeWeCannotRemove);

            protected:

                typedef std::vector<osg::Transform*>                TransformStack;
                typedef std::set<osg::Drawable*>                    DrawableSet;
                typedef std::set<osg::Billboard*>                   BillboardSet;
                typedef std::set<osg::Node* >                       NodeSet;
                typedef std::set<osg::Transform*>                   TransformSet;

                TransformStack  _transformStack;
                NodeSet         _excludedNodeSet;
                DrawableSet     _drawableSet;
                BillboardSet    _billboardSet;
                TransformSet    _transformSet;
        };

        /** FlattenStaticTransformsDuplicatingSharedSubgraphsVisitor类似于
          * FlattenStaticTransformsVisitor，旨在从场景图中移除静态变换，
          * 将变换下推到场景图的几何叶子，但不同之处在于，
          * 在不同变换之间共享的任何子图都会被复制并单独展平。
          * 这导致移除了更多的静态变换，但也意味着生成更多数据，
          * 因此可能并不总是最适合使用的展平访问者。*/
        class OSGUTIL_EXPORT FlattenStaticTransformsDuplicatingSharedSubgraphsVisitor : public BaseOptimizerVisitor
        {
            public:

                FlattenStaticTransformsDuplicatingSharedSubgraphsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, FLATTEN_STATIC_TRANSFORMS_DUPLICATING_SHARED_SUBGRAPHS) {}

                virtual void reset();

                virtual void apply(osg::Group& group);
                virtual void apply(osg::Transform& transform);
                virtual void apply(osg::LOD& lod);
                virtual void apply(osg::Geode& geode);
                virtual void apply(osg::Billboard& billboard);

            protected:

                void transformGeode(osg::Geode& geode);
                void transformDrawable(osg::Drawable& drawable);
                void transformBillboard(osg::Billboard& billboard);

                std::vector<osg::Matrix> _matrixStack;

        };

        /** 合并彼此相邻的静态变换节点。*/
        class OSGUTIL_EXPORT CombineStaticTransformsVisitor : public BaseOptimizerVisitor
        {
            public:

                CombineStaticTransformsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, FLATTEN_STATIC_TRANSFORMS) {}

                virtual void apply(osg::MatrixTransform& transform);

                bool removeTransforms(osg::Node* nodeWeCannotRemove);

            protected:

                typedef std::set<osg::MatrixTransform*> TransformSet;
                TransformSet  _transformSet;
        };

        /** 移除冗余节点，如只有一个子节点的组。*/
        class OSGUTIL_EXPORT RemoveEmptyNodesVisitor : public BaseOptimizerVisitor
        {
            public:


                typedef std::set<osg::Node*> NodeList;
                NodeList                     _redundantNodeList;

                RemoveEmptyNodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, REMOVE_REDUNDANT_NODES) {}

                virtual void apply(osg::Group& group);

                void removeEmptyNodes();

        };

        /** 移除冗余节点，如只有一个子节点的组。*/
        class OSGUTIL_EXPORT RemoveRedundantNodesVisitor : public BaseOptimizerVisitor
        {
            public:

                typedef std::set<osg::Node*> NodeList;
                NodeList                     _redundantNodeList;

                RemoveRedundantNodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, REMOVE_REDUNDANT_NODES) {}

                virtual void apply(osg::Group& group);
                virtual void apply(osg::Transform& transform);

                bool isOperationPermissible(osg::Node& node);

                void removeRedundantNodes();

        };

        /** 移除已加载的代理节点。*/
        class OSGUTIL_EXPORT RemoveLoadedProxyNodesVisitor : public BaseOptimizerVisitor
        {
            public:

                typedef std::set<osg::Node*> NodeList;
                NodeList                     _redundantNodeList;

                RemoveLoadedProxyNodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, REMOVE_LOADED_PROXY_NODES) {}

                virtual void apply(osg::ProxyNode& group);

                void removeRedundantNodes();

        };

        /** 镶嵌所有几何体，以移除多边形。*/
        class OSGUTIL_EXPORT TessellateVisitor : public BaseOptimizerVisitor
        {
            public:

                typedef std::set<osg::Group*>  GroupList;
                GroupList                      _groupList;

                TessellateVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, TESSELLATE_GEOMETRY) {}

                virtual void apply(osg::Geometry& geom);

        };

        /** 通过合并具有互补范围的相邻LOD来优化LOD组。*/
        class OSGUTIL_EXPORT CombineLODsVisitor : public BaseOptimizerVisitor
        {
            public:

                typedef std::set<osg::Group*>  GroupList;
                GroupList                      _groupList;

                CombineLODsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, COMBINE_ADJACENT_LODS) {}

                virtual void apply(osg::LOD& lod);

                void combineLODs();

        };

        /** 通过移除重复状态并用共享实例替换来优化场景图中的状态，
          * 包括StateAttribute和整个StateSet。*/
        class OSGUTIL_EXPORT StateVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                StateVisitor(bool combineDynamicState,
                             bool combineStaticState,
                             bool combineUnspecifiedState,
                             Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, SHARE_DUPLICATE_STATE)
                {
                    _optimize[osg::Object::DYNAMIC] = combineDynamicState;
                    _optimize[osg::Object::STATIC] = combineStaticState;
                    _optimize[osg::Object::UNSPECIFIED] = combineUnspecifiedState;
                }

                /** 空访问者，为下次遍历做好准备。*/
                virtual void reset();

                virtual void apply(osg::Node& node);

                void optimize();

            protected:

                void addStateSet(osg::StateSet* stateset, osg::Node* node);

                inline bool optimize(osg::Object::DataVariance variance)
                {
                    return _optimize[variance];
                }

                typedef std::set<osg::Node*>               NodeSet;
                typedef std::map<osg::StateSet*, NodeSet>  StateSetMap;

                // 注意，DYNAMIC、STATIC和UNSPECIFIED各有一个元素
                bool _optimize[3];

                StateSetMap _statesets;

        };

        /** 合并几何节点
          */
        class OSGUTIL_EXPORT MergeGeodesVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                MergeGeodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, MERGE_GEODES) {}

                virtual void apply(osg::Group& group);

                bool mergeGeodes(osg::Group& group);

            protected:

                bool mergeGeode(osg::Geode& lhs, osg::Geode& rhs);

        };

        class OSGUTIL_EXPORT MakeFastGeometryVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                MakeFastGeometryVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, MAKE_FAST_GEOMETRY) {}

                virtual void apply(osg::Geometry& geom);

        };

        class OSGUTIL_EXPORT MergeGeometryVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                MergeGeometryVisitor(Optimizer* optimizer=0) :
                    BaseOptimizerVisitor(optimizer, MERGE_GEOMETRY),
                    _targetMaximumNumberOfVertices(10000) {}

                void setTargetMaximumNumberOfVertices(unsigned int num)
                {
                    _targetMaximumNumberOfVertices = num;
                }

                unsigned int getTargetMaximumNumberOfVertices() const
                {
                    return _targetMaximumNumberOfVertices;
                }

                virtual void apply(osg::Group& group) { mergeGroup(group); traverse(group); }
                virtual void apply(osg::Billboard&) { /* 什么都不做*/ }

                bool mergeGroup(osg::Group& group);

                static bool geometryContainsSharedArrays(osg::Geometry& geom);

                static bool mergeGeometry(osg::Geometry& lhs,osg::Geometry& rhs);

                static bool mergePrimitive(osg::DrawArrays& lhs,osg::DrawArrays& rhs);
                static bool mergePrimitive(osg::DrawArrayLengths& lhs,osg::DrawArrayLengths& rhs);
                static bool mergePrimitive(osg::DrawElementsUByte& lhs,osg::DrawElementsUByte& rhs);
                static bool mergePrimitive(osg::DrawElementsUShort& lhs,osg::DrawElementsUShort& rhs);
                static bool mergePrimitive(osg::DrawElementsUInt& lhs,osg::DrawElementsUInt& rhs);

            protected:

                unsigned int _targetMaximumNumberOfVertices;

        };

        /** 将场景空间化为平衡的四叉/八叉树。*/
        class OSGUTIL_EXPORT SpatializeGroupsVisitor : public BaseOptimizerVisitor
        {
            public:

                SpatializeGroupsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, SPATIALIZE_GROUPS) {}

                virtual void apply(osg::Group& group);
                virtual void apply(osg::Geode& geode);

                bool divide(unsigned int maxNumTreesPerCell=8);

                bool divide(osg::Group* group, unsigned int maxNumTreesPerCell);
                bool divide(osg::Geode* geode, unsigned int maxNumTreesPerCell);

                typedef std::set<osg::Group*> GroupsToDivideList;
                GroupsToDivideList _groupsToDivideList;

                typedef std::set<osg::Geode*> GeodesToDivideList;
                GeodesToDivideList _geodesToDivideList;
        };

        /** 复制任何共享的子图，启用静态变换的展平。*/
        class OSGUTIL_EXPORT CopySharedSubgraphsVisitor : public BaseOptimizerVisitor
        {
            public:

                CopySharedSubgraphsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, COPY_SHARED_NODES) {}

                virtual void apply(osg::Node& node);

                void copySharedNodes();

                typedef std::set<osg::Node*> SharedNodeList;
                SharedNodeList _sharedNodeList;

        };


        /** 对所有纹理应用设置。*/
        class OSGUTIL_EXPORT TextureVisitor : public BaseOptimizerVisitor
        {
            public:

                TextureVisitor(bool changeAutoUnRef, bool valueAutoUnRef,
                               bool changeClientImageStorage, bool valueClientImageStorage,
                               bool changeAnisotropy, float valueAnisotropy,
                               Optimizer* optimizer=0):
                        BaseOptimizerVisitor(optimizer, OPTIMIZE_TEXTURE_SETTINGS),
                        _changeAutoUnRef(changeAutoUnRef), _valueAutoUnRef(valueAutoUnRef),
                        _changeClientImageStorage(changeClientImageStorage), _valueClientImageStorage(valueClientImageStorage),
                        _changeAnisotropy(changeAnisotropy), _valueAnisotropy(valueAnisotropy) {}

                virtual void apply(osg::Node& node);

                void apply(osg::StateSet& stateset);
                void apply(osg::Texture& texture);

                bool            _changeAutoUnRef, _valueAutoUnRef;
                bool            _changeClientImageStorage, _valueClientImageStorage;
                bool            _changeAnisotropy;
                float           _valueAnisotropy;

        };

        /** 展平MatrixTransform/Billboard对。*/
        class OSGUTIL_EXPORT FlattenBillboardVisitor : public BaseOptimizerVisitor
        {
            public:
                FlattenBillboardVisitor(Optimizer* optimizer=0):
                        BaseOptimizerVisitor(optimizer, FLATTEN_BILLBOARDS) {}

                typedef std::vector<osg::NodePath> NodePathList;
                typedef std::map<osg::Billboard*, NodePathList > BillboardNodePathMap;

                virtual void reset();

                virtual void apply(osg::Billboard& billboard);

                void process();

                BillboardNodePathMap _billboards;

        };

        /** 纹理图集构建器创建一组纹理/图像，每个都包含多个图像。
          * 纹理图集用于使更广泛的数据批处理成为可能。 */
        class OSGUTIL_EXPORT TextureAtlasBuilder
        {
        public:
            TextureAtlasBuilder();

            void reset();

            void setMaximumAtlasSize(int width, int height);

            int getMaximumAtlasWidth() const { return _maximumAtlasWidth; }
            int getMaximumAtlasHeight() const { return _maximumAtlasHeight; }

            void setMargin(int margin);
            int getMargin() const { return _margin; }

            void addSource(const osg::Image* image);
            void addSource(const osg::Texture2D* texture);

            unsigned int getNumSources() const { return _sourceList.size(); }
            const osg::Image* getSourceImage(unsigned int i) { return _sourceList[i]->_image.get(); }
            const osg::Texture2D* getSourceTexture(unsigned int i) { return _sourceList[i]->_texture.get(); }

            void buildAtlas();
            osg::Image* getImageAtlas(unsigned int i);
            osg::Texture2D* getTextureAtlas(unsigned int i);
            osg::Matrix getTextureMatrix(unsigned int i);

            osg::Image* getImageAtlas(const osg::Image* image);
            osg::Texture2D* getTextureAtlas(const osg::Image* image);
            osg::Matrix getTextureMatrix(const osg::Image* image);

            osg::Image* getImageAtlas(const osg::Texture2D* textue);
            osg::Texture2D* getTextureAtlas(const osg::Texture2D* texture);
            osg::Matrix getTextureMatrix(const osg::Texture2D* texture);

        protected:

            int _maximumAtlasWidth;
            int _maximumAtlasHeight;
            int _margin;


            // 前向声明
            class Atlas;

            class Source : public osg::Referenced
            {
            public:
                Source():
                    _x(0),_y(0),_atlas(0) {}

                Source(const osg::Image* image):
                    _x(0),_y(0),_atlas(0),_image(image) {}

                Source(const osg::Texture2D* texture):
                    _x(0),_y(0),_atlas(0),_texture(texture) { if (texture) _image = texture->getImage(); }

                int _x;
                int _y;
                Atlas* _atlas;

                osg::ref_ptr<const osg::Image> _image;
                osg::ref_ptr<const osg::Texture2D> _texture;

                bool suitableForAtlas(int maximumAtlasWidth, int maximumAtlasHeight, int margin);
                osg::Matrix computeTextureMatrix() const;


            protected:

                virtual ~Source() {}
            };

            typedef std::vector< osg::ref_ptr<Source> > SourceList;

            class Atlas : public osg::Referenced
            {
            public:
                Atlas(int width, int height, int margin):
                    _maximumAtlasWidth(width),
                    _maximumAtlasHeight(height),
                    _margin(margin),
                    _x(0),
                    _y(0),
                    _width(0),
                    _height(0),
                    _indexFirstOfRow(0){}

                int _maximumAtlasWidth;
                int _maximumAtlasHeight;
                int _margin;

                osg::ref_ptr<osg::Texture2D> _texture;
                osg::ref_ptr<osg::Image> _image;

                SourceList _sourceList;

                int _x;
                int _y;
                int _width;
                int _height;
                unsigned int _indexFirstOfRow; ///< 包含最后一行第一个元素的索引。
                enum FitsIn
                {
                    DOES_NOT_FIT_IN_ANY_ROW,        // 不适合任何行
                    FITS_IN_CURRENT_ROW,            // 适合当前行
                    IN_NEXT_ROW                     // 在下一行
                };
                FitsIn doesSourceFit(Source* source);
                bool addSource(Source* source);
                void clampToNearestPowerOfTwoSize();
                void copySources();

            protected:
                virtual ~Atlas() {}
            };

            typedef std::vector< osg::ref_ptr<Atlas> > AtlasList;

            Source* getSource(const osg::Image* image);
            Source* getSource(const osg::Texture2D* texture);

            SourceList _sourceList;
            AtlasList _atlasList;
            private:
                struct CompareSrc
                {
                    bool operator()(osg::ref_ptr<Source> src1, osg::ref_ptr<Source> src2) const
                    {
                        return src1->_image->t() > src2->_image->t();
                    }
                };
                void completeRow(unsigned int indexAtlas);
        };


        /** 通过将纹理合并到纹理图集中来优化场景图中的纹理使用。
          * 使用纹理图集可以减少场景中单独状态的数量，减少状态更改
          * 并提高使用更大几何体批次的机会。*/
        class OSGUTIL_EXPORT TextureAtlasVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                TextureAtlasVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, TEXTURE_ATLAS_BUILDER) {}


                TextureAtlasBuilder& getTextureAtlasBuilder() { return _builder; }

                /** 空访问者，为下次遍历做好准备。*/
                virtual void reset();

                virtual void apply(osg::Node& node);
                virtual void apply(osg::Drawable& node);

                void optimize();

            protected:

                bool pushStateSet(osg::StateSet* stateset);
                void popStateSet();

                typedef std::set<osg::Drawable*>  Drawables;
                typedef std::map<osg::StateSet*, Drawables>  StateSetMap;
                typedef std::set<osg::Texture2D*>  Textures;
                typedef std::vector<osg::StateSet*>  StateSetStack;

                TextureAtlasBuilder _builder;

                StateSetMap     _statesetMap;
                StateSetStack   _statesetStack;
                Textures        _textures;

        };

        /** 优化场景中StateSet和Geometry对象的设置，
          * 使它们在没有关联回调时具有STATIC DataVariance。 */
        class OSGUTIL_EXPORT StaticObjectDetectionVisitor : public BaseOptimizerVisitor
        {
            public:

                /// 默认遍历所有子节点。
                StaticObjectDetectionVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, STATIC_OBJECT_DETECTION) {}

                virtual void apply(osg::Node& node);
                virtual void apply(osg::Drawable& drawable);

            protected:

                void applyStateSet(osg::StateSet& stateset);

        };

        /** 对所有几何体应用设置。*/
        class OSGUTIL_EXPORT BufferObjectVisitor : public BaseOptimizerVisitor
        {
            public:

                BufferObjectVisitor(bool changeVBO, bool valueVBO,
                                    bool changeVertexArrayObject, bool valueVertexArrayObject,
                                    bool changeDisplayList, bool valueDisplayList,
                                    Optimizer* optimizer=0):
                        BaseOptimizerVisitor(optimizer, BUFFER_OBJECT_SETTINGS),
                        _changeVertexBufferObject(changeVBO), _valueVertexBufferObject(valueVBO),
                        _changeVertexArrayObject(changeVertexArrayObject), _valueVertexArrayObject(valueVertexArrayObject),
                        _changeDisplayList(changeDisplayList), _valueDisplayList(valueDisplayList) {}

                virtual void apply(osg::Geometry& geometry);

                bool _changeVertexBufferObject, _valueVertexBufferObject;
                bool _changeVertexArrayObject, _valueVertexArrayObject;
                bool _changeDisplayList, _valueDisplayList;

        };
};

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::StateSet* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::StateAttribute* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::Drawable* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::Node* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

}

#endif 