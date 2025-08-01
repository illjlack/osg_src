#include <osgFX/AnisotropicLighting>
#include <osgFX/Registry>

#include <osg/VertexProgram>
#include <osg/Texture2D>
#include <osg/TexEnv>

#include <osgDB/ReadFile>

#include <sstream>

using namespace osgFX;

namespace
{

    // 一个状态属性类，用于获取初始逆视图矩阵并将其发送到顶点程序
    // 各向异性光照效果需要世界空间的光照计算，因此需要逆视图矩阵来变换坐标
    // 注意：由于VertexProgram缺乏对每上下文参数的支持，
    // 此类只在渲染第一个上下文时将矩阵发送到顶点程序。
    // 所有后续上下文将使用第一个上下文的矩阵。
    class ViewMatrixExtractor: public osg::StateAttribute {
    public:
        // 默认构造函数：初始化成员变量
        ViewMatrixExtractor()
        :    osg::StateAttribute(),
            _vp(0),             // 顶点程序指针
            _param(0),          // 参数索引
            _first_context(-1)  // 第一个上下文ID，-1表示未设置
        {
        }

        // 拷贝构造函数：用于深拷贝和浅拷贝操作
        ViewMatrixExtractor(const ViewMatrixExtractor& copy, const osg::CopyOp& copyop)
        :    osg::StateAttribute(copy, copyop),
            _vp(static_cast<osg::VertexProgram *>(copyop(copy._vp.get()))),
            _param(copy._param),
            _first_context(-1)  // 重置上下文ID
        {
        }

        // 参数化构造函数：指定顶点程序和参数索引
        ViewMatrixExtractor(osg::VertexProgram *vp, int param)
        :    osg::StateAttribute(),
            _vp(vp),            // 关联的顶点程序
            _param(param),      // 矩阵参数在顶点程序中的起始索引
            _first_context(-1)
        {
        }

        // OSG宏：定义状态属性的类型信息
        META_StateAttribute(osgFX, ViewMatrixExtractor, VIEWMATRIXEXTRACTOR);

        // 比较函数：用于状态排序和优化
        int compare(const osg::StateAttribute &sa) const
        {
            COMPARE_StateAttribute_Types(ViewMatrixExtractor, sa);
            if (_vp.get() != rhs._vp.get()) return -1;
            if (_param < rhs._param) return -1;
            if (_param > rhs._param) return 1;
            return 0;
        }

        // 应用状态：在渲染时调用，设置顶点程序参数
        void apply(osg::State& state) const
        {
            // 记录第一个渲染上下文的ID
            if (_first_context == -1) {
                _first_context = state.getContextID();
            }
            
            // 只在第一个上下文中设置矩阵参数
            // 这是因为VertexProgram不支持多上下文参数
            if (state.getContextID() == (unsigned int)_first_context) {
                if (_vp.valid()) {
                    // 获取初始逆视图矩阵（从世界坐标到视图坐标的逆变换）
                    osg::Matrix M = state.getInitialInverseViewMatrix();
                    
                    // 将4x4矩阵按列分解为4个Vec4，发送给顶点程序
                    // 这样顶点程序就可以将顶点从视图空间变换到世界空间
                    for (int i=0; i<4; ++i) {
                        _vp->setProgramLocalParameter(_param+i, osg::Vec4(M(0, i), M(1, i), M(2, i), M(3, i)));
                    }
                }
            }
        }

    private:
        mutable osg::ref_ptr<osg::VertexProgram> _vp;  // 关联的顶点程序
        int _param;                                     // 参数起始索引
        mutable int _first_context;                     // 第一个渲染上下文ID
    };

}

namespace
{

    // 创建默认的各向异性纹理
    // 这个纹理模拟了各向异性材质的光照特性（如拉丝金属、丝绸等）
    osg::Image* create_default_image()
    {
        const int _texturesize = 16;  // 纹理尺寸：16x16像素
        osg::ref_ptr<osg::Image> image = new osg::Image;
        
        // 创建RGB格式的图像数据
        image->setImage(_texturesize, _texturesize, 1, 3, GL_RGB, GL_UNSIGNED_BYTE, 
                       new unsigned char[3*_texturesize*_texturesize], osg::Image::USE_NEW_DELETE);
        
        // 生成各向异性纹理图案
        for (int i=0; i<_texturesize; ++i) {
            for (int j=0; j<_texturesize; ++j) {
                float s = static_cast<float>(j) / (_texturesize-1);  // 水平坐标[0,1]
                float t = static_cast<float>(i) / (_texturesize-1);  // 垂直坐标[0,1]
                
                // 基础亮度：随t变化，模拟光照渐变
                float lum = t * 0.75f;
                
                // 红色分量：添加余弦波纹，模拟各向异性反射
                float red = lum + 0.2f * powf(cosf(s*10), 3.0f);
                float green = lum;  // 绿色保持基础亮度
                
                // 蓝色分量：添加正弦波纹，与红色形成互补
                float blue = lum + 0.2f * powf(sinf(s*10), 3.0f);
                
                // 限制颜色值在[0,1]范围内
                if (red > 1) red = 1;
                if (red < 0) red = 0;
                if (blue > 1) blue = 1;
                if (blue < 0) blue = 0;
                
                // 设置像素值（转换为8位整数）
                *(image->data(j, i)+0) = static_cast<unsigned char>(red * 255);
                *(image->data(j, i)+1) = static_cast<unsigned char>(green * 255);
                *(image->data(j, i)+2) = static_cast<unsigned char>(blue * 255);
            }
        }
        return image.release();
    }

}

namespace
{

    // 注册代理：将AnisotropicLighting效果注册到osgFX系统
    Registry::Proxy proxy(new AnisotropicLighting);

    // 默认技术实现：各向异性光照的具体渲染技术
    class DefaultTechnique: public Technique {
    public:

        // 构造函数：指定光源编号和纹理
        DefaultTechnique(int lightnum, osg::Texture2D *texture)
        :    Technique(),
            _lightnum(lightnum),    // 使用的光源编号
            _texture(texture)       // 各向异性纹理
        {
        }

        // 获取所需的OpenGL扩展
        void getRequiredExtensions(std::vector<std::string>& extensions) const
        {
            extensions.push_back("GL_ARB_vertex_program");  // 需要顶点程序支持
        }

    protected:

        // 定义渲染通道：设置各向异性光照的着色器和状态
        void define_passes()
        {
            std::ostringstream vp_oss;
            
            // ARB顶点程序代码：实现各向异性光照计算
            vp_oss <<
                "!!ARBvp1.0\n"                              // ARB顶点程序版本声明
                "PARAM c5 = { 0, 0, 0, 1 };"                // 常量：(0,0,0,1)
                "PARAM c4 = { 0, 0, 0, 0 };"                // 常量：(0,0,0,0)
                "TEMP R0, R1, R2, R3, R4, R5, R6, R7, R8, R9;" // 临时寄存器
                "ATTRIB v18 = vertex.normal;"               // 输入：顶点法线
                "ATTRIB v16 = vertex.position;"             // 输入：顶点位置
                "PARAM s259[4] = { state.matrix.mvp };"     // MVP矩阵（模型-视图-投影）
                "PARAM s18 = state.light[" << _lightnum << "].position;"  // 光源位置
                "PARAM s223[4] = { state.matrix.modelview };"             // 模型视图矩阵
                "PARAM c0[4] = { program.local[0..3] };"    // 逆视图矩阵（由ViewMatrixExtractor设置）
                
                // === 变换顶点到裁剪空间 ===
                "    DP4 result.position.x, s259[0], v16;" // 计算裁剪空间X坐标
                "    DP4 result.position.y, s259[1], v16;" // 计算裁剪空间Y坐标
                "    DP4 result.position.z, s259[2], v16;" // 计算裁剪空间Z坐标
                "    DP4 result.position.w, s259[3], v16;" // 计算裁剪空间W坐标
                
                // === 构建世界空间变换矩阵 ===
                // 将逆视图矩阵的各行重组，用于将视图空间坐标变换到世界空间
                "    MOV R9, c0[0];"                        // 第0行
                "    MUL R0, R9.y, s223[1];"
                "    MAD R0, R9.x, s223[0], R0;"
                "    MAD R0, R9.z, s223[2], R0;"
                "    MAD R8, R9.w, s223[3], R0;"            // 计算变换矩阵第0行
                "    DP4 R0.x, R8, v16;"                    // 变换后的X坐标
                
                "    MOV R7, c0[1];"                        // 第1行
                "    MUL R1, R7.y, s223[1];"
                "    MAD R1, R7.x, s223[0], R1;"
                "    MAD R1, R7.z, s223[2], R1;"
                "    MAD R6, R7.w, s223[3], R1;"            // 计算变换矩阵第1行
                "    DP4 R0.y, R6, v16;"                    // 变换后的Y坐标
                
                "    MOV R5, c0[2];"                        // 第2行
                "    MUL R1, R5.y, s223[1];"
                "    MAD R1, R5.x, s223[0], R1;"
                "    MAD R1, R5.z, s223[2], R1;"
                "    MAD R4, R5.w, s223[3], R1;"            // 计算变换矩阵第2行
                "    DP4 R0.z, R4, v16;"                    // 变换后的Z坐标
                
                "    MOV R3, c0[3];"                        // 第3行
                "    MUL R1, R3.y, s223[1];"
                "    MAD R1, R3.x, s223[0], R1;"
                "    MAD R1, R3.z, s223[2], R1;"
                "    MAD R1, R3.w, s223[3], R1;"            // 计算变换矩阵第3行
                "    DP4 R0.w, R1, v16;"                    // 变换后的W坐标
                
                // === 计算到观察点的向量 ===
                "    MOV R1.x, R9.w;"                       // 提取观察点位置
                "    MOV R1.y, R7.w;"
                "    MOV R1.z, R5.w;"
                "    MOV R1.w, R3.w;"
                "    ADD R2, R1, -R0;"                      // 计算从顶点到观察点的向量
                "    DP4 R0.x, R2, R2;"                     // 计算向量长度的平方
                "    RSQ R1.x, R0.x;"                       // 计算长度的倒数平方根
                
                // === 计算到光源的向量 ===
                "    DP4 R0.x, R9, s18;"                    // 变换光源位置到世界空间
                "    DP4 R0.y, R7, s18;"
                "    DP4 R0.z, R5, s18;"
                "    DP4 R0.w, R3, s18;"
                "    DP4 R1.y, R0, R0;"                     // 计算光源向量长度的平方
                "    RSQ R1.y, R1.y;"                       // 计算长度的倒数平方根
                "    MUL R3, R1.y, R0;"                     // 归一化光源向量
                
                // === 计算半角向量 ===
                "    MAD R2, R1.x, R2, R3;"                // 视线向量 + 光源向量
                "    DP4 R1.x, R2, R2;"                     // 计算半角向量长度的平方
                "    RSQ R1.x, R1.x;"                       // 计算长度的倒数平方根
                "    MUL R1, R1.x, R2;"                     // 归一化半角向量
                
                // === 变换法线到世界空间 ===
                "    DP3 R2.x, R8.xyzx, v18.xyzx;"         // 变换法线X分量
                "    DP3 R2.y, R6.xyzx, v18.xyzx;"         // 变换法线Y分量
                "    DP3 R2.z, R4.xyzx, v18.xyzx;"         // 变换法线Z分量
                "    MOV R2.w, c4.x;"                       // 设置W分量为0
                
                // === 计算各向异性光照的纹理坐标 ===
                "    DP4 R1.x, R1, R2;"                     // 半角向量与法线的点积（高光强度）
                "    MAX result.texcoord[0].x, R1.x, c4.x;" // 限制为非负值，作为U纹理坐标
                "    DP4 R0.x, R0, R2;"                     // 光源向量与法线的点积（漫反射强度）
                "    MAX result.texcoord[0].y, R0.x, c4.x;" // 限制为非负值，作为V纹理坐标
                "END\n";

            // 创建状态集
            osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;

            // === 设置顶点程序 ===
            osg::ref_ptr<osg::VertexProgram> vp = new osg::VertexProgram;
            vp->setVertexProgram(vp_oss.str());  // 设置顶点程序代码
            ss->setAttributeAndModes(vp.get(), osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

            // === 设置矩阵提取器 ===
            // 将逆视图矩阵传递给顶点程序的参数0-3
            ss->setAttributeAndModes(new ViewMatrixExtractor(vp.get(), 0), 
                                   osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

            // === 设置各向异性纹理 ===
            ss->setTextureAttributeAndModes(0, _texture.get(), 
                                          osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

            // === 设置纹理环境 ===
            osg::ref_ptr<osg::TexEnv> texenv = new osg::TexEnv;
            texenv->setMode(osg::TexEnv::DECAL);  // 使用DECAL模式，直接使用纹理颜色
            ss->setTextureAttributeAndModes(0, texenv.get(), 
                                          osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);

            // === 禁用Alpha测试（兼容性处理） ===
            #if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GL3_AVAILABLE)
                ss->setMode( GL_ALPHA_TEST, osg::StateAttribute::OFF );
            #else
                OSG_NOTICE<<"Warning: osgFX::AnisotropicLighting unable to disable GL_ALPHA_TEST."<<std::endl;
            #endif

            // 添加渲染通道
            addPass(ss.get());
        }

    private:
        int _lightnum;                           // 使用的光源编号
        osg::ref_ptr<osg::Texture2D> _texture;  // 各向异性纹理
    };

}


// ===============================================
// AnisotropicLighting类实现
// ===============================================

// 默认构造函数：初始化各向异性光照效果
AnisotropicLighting::AnisotropicLighting()
:    Effect(),                  // 继承自Effect基类
    _lightnum(0),              // 默认使用0号光源
    _texture(new osg::Texture2D) // 创建2D纹理对象
{
    // 设置默认的各向异性纹理
    _texture->setImage(create_default_image());
    
    // 设置纹理包装模式为边缘夹紧，防止边缘重复
    _texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
    _texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
}

// 拷贝构造函数：用于对象复制
AnisotropicLighting::AnisotropicLighting(const AnisotropicLighting& copy, const osg::CopyOp& copyop)
:    Effect(copy, copyop),      // 调用基类拷贝构造函数
    _lightnum(copy._lightnum),  // 复制光源编号
    _texture(static_cast<osg::Texture2D *>(copyop(copy._texture.get()))) // 复制纹理对象
{
}

// 定义渲染技术：创建具体的渲染实现
bool AnisotropicLighting::define_techniques()
{
    // 添加默认的各向异性光照技术
    addTechnique(new DefaultTechnique(_lightnum, _texture.get()));
    return true;
}
