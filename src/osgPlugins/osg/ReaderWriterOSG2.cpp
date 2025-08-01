/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
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
// Written by Wang Rui, (C) 2010

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <stdlib.h>
#include "AsciiStreamOperator.h"
#include "BinaryStreamOperator.h"
#include "XmlStreamOperator.h"

using namespace osgDB;

#define CATCH_EXCEPTION(s) \
    if (s.getException()) return (s.getException()->getError() + " At " + s.getException()->getField());

#define OSG_REVERSE(value) ( ((value & 0x000000ff)<<24) | ((value & 0x0000ff00)<<8) | ((value & 0x00ff0000)>>8) | ((value & 0xff000000)>>24) )

InputIterator* readInputIterator( std::istream& fin, const Options* options )
{
    bool extensionIsAscii = false, extensionIsXML = false;
    if ( options )
    {
        const std::string& optionString = options->getPluginStringData("fileType");
        if ( optionString=="Ascii") extensionIsAscii = true;
        else if ( optionString=="XML" ) extensionIsXML = true;
    }

    if ( !extensionIsAscii && !extensionIsXML )
    {
        unsigned int headerLow = 0, headerHigh = 0;
        fin.read( (char*)&headerLow, INT_SIZE );
        fin.read( (char*)&headerHigh, INT_SIZE );
        if ( headerLow==OSG_HEADER_LOW && headerHigh==OSG_HEADER_HIGH )
        {
            OSG_INFO<<"Reading OpenSceneGraph binary file with the same endian as this computer."<<std::endl;
            return new BinaryInputIterator(&fin, 0); // endian the same so no byte swap required
        }
        else if ( headerLow==OSG_REVERSE(OSG_HEADER_LOW) && headerHigh==OSG_REVERSE(OSG_HEADER_HIGH) )
        {
            OSG_INFO<<"Reading OpenSceneGraph binary file with the different endian to this computer, doing byte swap."<<std::endl;
            return new BinaryInputIterator(&fin, 1); // endian different so byte swap required
        }

        fin.seekg( 0, std::ios::beg );
    }

    if ( !extensionIsXML )
    {
        std::string header; fin >> header;
        if ( header=="#Ascii" )
        {
            return new AsciiInputIterator(&fin);
        }
        fin.seekg( 0, std::ios::beg );
    }

    if ( 1 )
    {
        std::string header; std::getline( fin, header );
        if ( !header.compare(0, 5, "<?xml") )
        {
            return new XmlInputIterator(&fin);
        }
        fin.seekg( 0, std::ios::beg );
    }
    return NULL;
}

OutputIterator* writeOutputIterator( std::ostream& fout, const Options* options )
{
    // Read precision parameter, for text & XML formats
    int precision(-1);
    if ( options ) {
        std::istringstream iss(options->getOptionString());
        std::string opt;
        while (iss >> opt)
        {
            if(opt=="PRECISION" || opt=="precision")
            {
                iss >> precision;
            }
        }
    }

    const std::string optionString = (options!=0) ? options->getPluginStringData("fileType") : std::string();
    if (optionString == "Ascii")
    {
        fout << std::string("#Ascii") << ' ';
        return new AsciiOutputIterator(&fout, precision);
    }
    else if ( optionString == "XML")
    {
        fout << std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>") << std::endl;
        return new XmlOutputIterator(&fout, precision);
    }
    else
    {
        unsigned int low = OSG_HEADER_LOW, high = OSG_HEADER_HIGH;
        fout.write( (char*)&low, INT_SIZE );
        fout.write( (char*)&high, INT_SIZE );
        return new BinaryOutputIterator(&fout);
    }
}

// ReaderWriterOSG2类：OSG新版本序列化格式的读写器
// 支持多种格式：osg2(通用), osgt(ASCII文本), osgb(二进制), osgx(XML)
class ReaderWriterOSG2 : public osgDB::ReaderWriter
{
public:
    ReaderWriterOSG2()
    {
        // 注册支持的文件扩展名和格式描述
        supportsExtension( "osg2", "OpenSceneGraph extendable format" );
        supportsExtension( "osgt", "OpenSceneGraph extendable ascii format" );    // ASCII文本格式
        supportsExtension( "osgb", "OpenSceneGraph extendable binary format" );   // 二进制格式（最高效）
        supportsExtension( "osgx", "OpenSceneGraph extendable XML format" );      // XML格式（可读性好）

        // 支持的导入/导出选项
        supportsOption( "Ascii", "Import/Export option: Force reading/writing ascii file" );
        supportsOption( "XML", "Import/Export option: Force reading/writing XML file" );
        supportsOption( "ForceReadingImage", "Import option: Load an empty image instead if required file missed" );
        supportsOption( "SchemaData", "Export option: Record inbuilt schema data into a binary file" );
        supportsOption( "SchemaFile=<file>", "Import/Export option: Use/Record an ascii schema file" );
        supportsOption( "Compressor=<n>", "Export option: Use an inbuilt or user-defined compressor" );
        supportsOption( "WriteImageHint=<hint>", "Export option: Hint of writing image to stream: "
                        "<IncludeData> writes Image::data() directly; "
                        "<IncludeFile> writes the image file itself to stream; "
                        "<UseExternal> writes only the filename; " );
    }

    virtual const char* className() const { return "OpenSceneGraph Native Format Reader/Writer"; }


    // 准备读取操作：根据文件扩展名设置相应的读取模式和选项
    Options* prepareReading( ReadResult& result, std::string& fileName, std::ios::openmode& mode, const Options* options ) const
    {
        // 获取文件扩展名（转为小写）
        std::string ext = osgDB::getLowerCaseFileExtension( fileName );
        
        // 检查是否支持该扩展名
        if ( !acceptsExtension(ext) )
        {
            result = ReadResult::FILE_NOT_HANDLED;
            return 0;
        }
        
        // 查找文件的完整路径
        fileName = osgDB::findDataFile( fileName, options );
        if ( fileName.empty() )
        {
            result = ReadResult::FILE_NOT_FOUND;
            return 0;
        }

        // 创建本地选项副本，设置数据库路径
        osg::ref_ptr<Options> local_opt = options ? static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        local_opt->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        
        // 根据文件扩展名设置文件类型和打开模式
        if ( ext=="osgt" ) 
        {
            // ASCII文本格式：人类可读，调试友好，但文件较大
            local_opt->setPluginStringData( "fileType", "Ascii" );
        }
        else if ( ext=="osgx" ) 
        {
            // XML格式：结构化文本，可读性好，支持验证
            local_opt->setPluginStringData( "fileType", "XML" );
        }
        else  if ( ext=="osgb" )
        {
            // OSGB二进制格式：最紧凑，加载最快，但不可直接编辑
            local_opt->setPluginStringData( "fileType", "Binary" );
            mode |= std::ios::binary;  // 重要：必须以二进制模式打开文件
        }
        else
        {
            // 默认情况：使用二进制模式
            local_opt->setPluginStringData( "fileType", std::string() );
            mode |= std::ios::binary;
        }

        return local_opt.release();
    }

    // 从文件读取对象：统一的对象读取接口
    virtual ReadResult readObject( const std::string& file, const Options* options ) const
    {
        ReadResult result = ReadResult::FILE_LOADED;
        std::string fileName = file;
        std::ios::openmode mode = std::ios::in;
        
        // 准备读取设置
        Options* local_opt = prepareReading( result, fileName, mode, options );
        if ( !result.success() ) return result;

        // 打开文件流并读取
        osgDB::ifstream istream( fileName.c_str(), mode );
        return readObject( istream, local_opt );
    }

    // 从输入流读取对象：核心的对象反序列化逻辑
    virtual ReadResult readObject( std::istream& fin, const Options* options ) const
    {
        // 创建输入迭代器：根据文件类型选择相应的解析器
        osg::ref_ptr<InputIterator> ii = readInputIterator(fin, options);
        if ( !ii ) return ReadResult::FILE_NOT_HANDLED;

        // 创建输入流对象：负责实际的反序列化工作
        InputStream is( options );

        // 启动反序列化过程：识别文件内容类型
        osgDB::InputStream::ReadType readType = is.start(ii.get());
        if ( readType==InputStream::READ_UNKNOWN )
        {
            CATCH_EXCEPTION(is);  // 捕获并处理异常
            return ReadResult::FILE_NOT_HANDLED;
        }
        
        // 解压缩数据（如果文件被压缩）
        is.decompress(); CATCH_EXCEPTION(is);

        // 读取并返回对象
        osg::ref_ptr<osg::Object> obj = is.readObject(); CATCH_EXCEPTION(is);
        return obj;
    }

    // 从文件读取图像：图像特化的读取接口
    virtual ReadResult readImage( const std::string& file, const Options* options ) const
    {
        ReadResult result = ReadResult::FILE_LOADED;
        std::string fileName = file;
        std::ios::openmode mode = std::ios::in;
        
        // 准备读取设置
        Options* local_opt = prepareReading( result, fileName, mode, options );
        if ( !result.success() ) return result;

        // 打开文件流并读取图像
        osgDB::ifstream istream( fileName.c_str(), mode );
        return readImage( istream, local_opt );
    }

    // 从输入流读取图像：核心的图像反序列化逻辑
    virtual ReadResult readImage( std::istream& fin, const Options* options ) const
    {
        // 创建输入迭代器
        osg::ref_ptr<InputIterator> ii = readInputIterator(fin, options);
        if ( !ii ) return ReadResult::FILE_NOT_HANDLED;

        // 创建输入流对象
        InputStream is( options );
        
        // 验证文件内容类型必须是图像
        if ( is.start(ii.get())!=InputStream::READ_IMAGE )
        {
            CATCH_EXCEPTION(is);
            return ReadResult::FILE_NOT_HANDLED;
        }

        // 解压并读取图像数据
        is.decompress(); CATCH_EXCEPTION(is);
        osg::ref_ptr<osg::Image> image = is.readImage(); CATCH_EXCEPTION(is);

        return image;
    }

    // 从文件读取节点：节点特化的读取接口，PagedLOD经常使用此接口
    virtual ReadResult readNode( const std::string& file, const Options* options ) const
    {
        ReadResult result = ReadResult::FILE_LOADED;
        std::string fileName = file;
        std::ios::openmode mode = std::ios::in;
        
        // 准备读取设置
        Options* local_opt = prepareReading( result, fileName, mode, options );
        if ( !result.success() ) return result;

        // 打开文件流并读取节点
        osgDB::ifstream istream( fileName.c_str(), mode );
        return readNode( istream, local_opt );
    }

    // 从输入流读取节点：核心的节点反序列化逻辑
    virtual ReadResult readNode( std::istream& fin, const Options* options ) const
    {
        // 创建输入迭代器
        osg::ref_ptr<InputIterator> ii = readInputIterator(fin, options);
        if ( !ii ) return ReadResult::FILE_NOT_HANDLED;

        // 创建输入流对象
        InputStream is( options );
        
        // 启动反序列化：验证内容类型为场景或对象
        osgDB::InputStream::ReadType readType = is.start(ii.get());
        if ( readType!=InputStream::READ_SCENE && readType!=InputStream::READ_OBJECT )
        {
            CATCH_EXCEPTION(is);
            return ReadResult::FILE_NOT_HANDLED;
        }

        // 解压并读取节点数据
        is.decompress(); CATCH_EXCEPTION(is);
        osg::ref_ptr<osg::Node> node = is.readObjectOfType<osg::Node>(); CATCH_EXCEPTION(is);
        if ( !node ) return ReadResult::FILE_NOT_HANDLED;
        return node;
    }

    // 准备写入操作：根据文件扩展名设置相应的写入模式
    Options* prepareWriting( WriteResult& result, const std::string& fileName, std::ios::openmode& mode, const Options* options ) const
    {
        // 获取文件扩展名
        std::string ext = osgDB::getLowerCaseFileExtension( fileName );
        if ( !acceptsExtension(ext) ) result = WriteResult::FILE_NOT_HANDLED;

        // 创建本地选项副本
        osg::ref_ptr<Options> local_opt = options ? static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        local_opt->getDatabasePathList().push_front(osgDB::getFilePath(fileName));
        
        // 根据扩展名设置写入格式
        if ( ext=="osgt" ) 
        {
            local_opt->setPluginStringData( "fileType", "Ascii" );
        }
        else if ( ext=="osgx" ) 
        {
            local_opt->setPluginStringData( "fileType", "XML" );
        }
        else  if ( ext=="osgb" )
        {
            // OSGB二进制写入：高效的数据序列化
            local_opt->setPluginStringData( "fileType", "Binary" );
            mode |= std::ios::binary;  // 必须以二进制模式写入
        }
        else
        {
            // 默认二进制模式
            local_opt->setPluginStringData( "fileType", std::string() );
            mode |= std::ios::binary;
        }

        return local_opt.release();
    }

    virtual WriteResult writeObject( const osg::Object& object, const std::string& fileName, const Options* options ) const
    {
        WriteResult result = WriteResult::FILE_SAVED;
        std::ios::openmode mode = std::ios::out;
        osg::ref_ptr<Options> local_opt = prepareWriting( result, fileName, mode, options );
        if ( !result.success() ) return result;

        osgDB::ofstream fout( fileName.c_str(), mode );
        if ( !fout ) return WriteResult::ERROR_IN_WRITING_FILE;

        result = writeObject( object, fout, local_opt.get() );
        fout.close();
        return result;
    }

    virtual WriteResult writeObject( const osg::Object& object, std::ostream& fout, const Options* options ) const
    {
        osg::ref_ptr<OutputIterator> oi = writeOutputIterator(fout, options);

        OutputStream os( options );
        os.start( oi.get(), OutputStream::WRITE_OBJECT ); CATCH_EXCEPTION(os);
        os.writeObject( &object ); CATCH_EXCEPTION(os);
        os.compress( &fout ); CATCH_EXCEPTION(os);

        oi->flush();
        if ( !os.getSchemaName().empty() )
        {
            osgDB::ofstream schemaStream( os.getSchemaName().c_str(), std::ios::out );
            if ( !schemaStream.fail() ) os.writeSchema( schemaStream );
            schemaStream.close();
        }

        if ( fout.fail() ) return WriteResult::ERROR_IN_WRITING_FILE;
        return WriteResult::FILE_SAVED;
    }

    virtual WriteResult writeImage( const osg::Image& image, const std::string& fileName, const Options* options ) const
    {
        WriteResult result = WriteResult::FILE_SAVED;
        std::ios::openmode mode = std::ios::out;
        osg::ref_ptr<Options> local_opt = prepareWriting( result, fileName, mode, options );
        if ( !result.success() ) return result;

        osgDB::ofstream fout( fileName.c_str(), mode );
        if ( !fout ) return WriteResult::ERROR_IN_WRITING_FILE;

        result = writeImage( image, fout, local_opt.get() );
        fout.close();
        return result;
    }

    virtual WriteResult writeImage( const osg::Image& image, std::ostream& fout, const Options* options ) const
    {
        osg::ref_ptr<OutputIterator> oi = writeOutputIterator(fout, options);

        OutputStream os( options );
        os.start( oi.get(), OutputStream::WRITE_IMAGE ); CATCH_EXCEPTION(os);
        os.writeImage( &image ); CATCH_EXCEPTION(os);
        os.compress( &fout ); CATCH_EXCEPTION(os);

        oi->flush();
        if ( !os.getSchemaName().empty() )
        {
            osgDB::ofstream schemaStream( os.getSchemaName().c_str(), std::ios::out );
            if ( !schemaStream.fail() ) os.writeSchema( schemaStream );
            schemaStream.close();
        }

        if ( fout.fail() ) return WriteResult::ERROR_IN_WRITING_FILE;
        return WriteResult::FILE_SAVED;
    }

    virtual WriteResult writeNode( const osg::Node& node, const std::string& fileName, const Options* options ) const
    {
        WriteResult result = WriteResult::FILE_SAVED;
        std::ios::openmode mode = std::ios::out;
        osg::ref_ptr<Options> local_opt = prepareWriting( result, fileName, mode, options );
        if ( !result.success() ) return result;

        osgDB::ofstream fout( fileName.c_str(), mode );
        if ( !fout ) return WriteResult::ERROR_IN_WRITING_FILE;

        result = writeNode( node, fout, local_opt.get() );
        fout.close();
        return result;
    }

    virtual WriteResult writeNode( const osg::Node& node, std::ostream& fout, const Options* options ) const
    {
        osg::ref_ptr<OutputIterator> oi = writeOutputIterator(fout, options);

        OutputStream os( options );
        os.start( oi.get(), OutputStream::WRITE_SCENE ); CATCH_EXCEPTION(os);
        os.writeObject( &node ); CATCH_EXCEPTION(os);
        os.compress( &fout ); CATCH_EXCEPTION(os);

        oi->flush();
        if ( !os.getSchemaName().empty() )
        {
            osgDB::ofstream schemaStream( os.getSchemaName().c_str(), std::ios::out );
            if ( !schemaStream.fail() ) os.writeSchema( schemaStream );
            schemaStream.close();
        }

        if ( fout.fail() ) return WriteResult::ERROR_IN_WRITING_FILE;
        return WriteResult::FILE_SAVED;
    }
};

REGISTER_OSGPLUGIN( osg2, ReaderWriterOSG2 )
