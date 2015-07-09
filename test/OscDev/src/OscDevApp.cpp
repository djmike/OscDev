#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "UdpClient.h"
#include "OscTree.h"

class OscDevApp : public ci::app::App
{
public:
	static void prepareSettings( ci::app::App::Settings * settings );

	OscDevApp();
	~OscDevApp();

	void	draw() override;
	void	setup() override;
	void	update() override;

private:
	void	onConnect( UdpSessionRef session );
	void	onError( std::string error, size_t bytesTransferred );
	void	onWrite( size_t bytesTransferred );
	void	write();

	void	testInt32();
	void	testString();
	void	testBlobImage();
	void	testBlobArray();
	void	testInt64();
	void	testFloat();
	void	testDouble();
	void	testMessage();
	void	testFromBuffer();
	
private:
	UdpClientRef				mUdpClient;
	UdpSessionRef				mUdpSession;
	std::string					mHost;
	int32_t						mPort;
	std::string					mRequest;

	ci::Font					mFont;
	std::vector<std::string>	mText;
	ci::gl::TextureRef			mTexture;

	ci::Surface8uRef			mSurface;
	ci::Surface8uRef			mSurfaceOsc;
	ci::Surface8uRef			mSurfaceDiff;
	ci::Surface8uRef			mSurfaceBufferTestA;
	ci::Surface8uRef			mSurfaceBufferTestB;

	ci::params::InterfaceGlRef	mParams;
	float						mFps;
	int32_t						mTestIndex;
};


#include "cinder/app/RendererGl.h"
#include "cinder/ip/Fill.h"
#include "cinder/Log.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include <limits>

using namespace ci;
using namespace ci::app;
using namespace std;

template<typename T>
inline string getTestOutput( const string &testType, const T &v, const OscTree &oscAttr, const BufferRef &buffer )
{
	stringstream ss;

	ss << "Test " << testType  
		<< "\n\tdata: " << oscAttr.getValue()->getData() 
		<< "\n\tvalue: " << oscAttr.getValue<T>() 
		<< "\n\ttype tag: " << oscAttr.getTypeTag() 
		<< "\n\tv: " << v 
		<< "\n\tbuffer value: " << ( *reinterpret_cast<T*>( buffer->getData() ) ) 
		<< "\n\tbuffer size: " << buffer->getSize() 
		<< "\n\tbuffer alloc size: " << buffer->getAllocatedSize();

	return ss.str();
}

template<>
inline string getTestOutput<string>( const string &testType, const string &v, const OscTree &oscAttr, const BufferRef &buffer )
{
	stringstream ss;

	ss << "Test " << testType  
		<< "\n\tdata: " << oscAttr.getValue()->getData() 
		<< "\n\tvalue: " << oscAttr.getValue<string>() 
		<< "\n\ttype tag: " << oscAttr.getTypeTag() 
		<< "\n\tv: " << v 
		<< "\n\tbuffer value: " << string( reinterpret_cast<const char*>( buffer->getData() ) ) 
		<< "\n\tbuffer size: " << buffer->getSize() 
		<< "\n\tbuffer alloc size: " << buffer->getAllocatedSize();

	return ss.str();
}

void OscDevApp::prepareSettings( App::Settings * settings )
{
	settings->setFrameRate( 60.0f );
	settings->prepareWindow( Window::Format().size( 1280, 720 ).title( "Osc Dev" ) );
}

OscDevApp::OscDevApp() : 
	mHost( "127.0.0.1" ), 
	mPort( 2000 ), 
	mRequest( "Hello, server" ), 
	mFont( "Georgia", 24 ), 
	mFps( 0.0f ), 
	mTestIndex( 0 )
{
}

OscDevApp::~OscDevApp()
{
}

void OscDevApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

	if ( mTexture ) {
		gl::draw( mTexture, vec2( 10.0f, 240.0f ) );
	}

	vec2 windowSize = vec2( getWindowSize() );
	vec2 surfacePos;
	if ( mSurface ) {
		surfacePos.x = windowSize.x - 320.0f * 3.0f - 20.0f;
		gl::draw( gl::Texture::create( *mSurface ), surfacePos );
	}

	if ( mSurfaceOsc ) {
		surfacePos.x = windowSize.x - 320.0f * 2.0f - 10.0f;
		gl::draw( gl::Texture::create( *mSurfaceOsc ), surfacePos );
	}

	if ( mSurfaceDiff ) {
		surfacePos.x = windowSize.x - 320.0f;
		gl::draw( gl::Texture::create( *mSurfaceDiff ), surfacePos );
		gl::ScopedColor scopedColor( Colorf( 1.0f, 0.0f, 1.0f ) );
		gl::drawStrokedRect( Rectf( surfacePos, surfacePos + vec2( 320.0f, 240.0f ) ), 3.0f );
	}

	mParams->draw();
}

void OscDevApp::onConnect( UdpSessionRef session )
{
	mText.push_back( "Connected" );

	mUdpSession = session;
	mUdpSession->connectErrorEventHandler( &OscDevApp::onError, this );
	mUdpSession->connectWriteEventHandler( &OscDevApp::onWrite, this );

	write();
}

void OscDevApp::onError( std::string error, size_t bytesTransferred )
{
	string text = "Error";
	if ( !error.empty() ) {
		text += ": " + error;
	}
	mText.push_back( text );
}

void OscDevApp::onWrite( size_t bytesTransferred )
{
	mText.push_back( to_string( bytesTransferred ) + " bytes written" );
}

void OscDevApp::setup()
{
	mUdpClient = UdpClient::create( io_service() );
	mUdpClient->connectConnectEventHandler( &OscDevApp::onConnect, this );
	mUdpClient->connectErrorEventHandler( &OscDevApp::onError, this );
	mUdpClient->connectResolveEventHandler( [ & ]()
	{
		mText.push_back( "Endpoint resolved" );
	} );

	auto setHost = [ & ]( string host ) -> void
	{
		mHost = host;
	};

	auto getHost = [ & ]() -> string
	{
		return mHost;
	};

	auto setPort = [ & ]( int32_t port ) -> void
	{
		mPort = port;
	};

	auto getPort = [ & ]() -> int32_t
	{
		return mPort;
	};

	const vector<string> mTestNames = {
		"message", 
		"int32", 
		"string", 
		"blob image", 
		"blob array", 
		"int64", 
		"float", 
		"double", 
		"buffer"
	};

	auto runTest = [ & ]() -> void
	{
		switch ( mTestIndex ) {
			case 0:
				testMessage();
				break;
			case 1:
				testInt32();
				break;
			case 2:
				testString();
				break;
			case 3:
				testBlobImage();
				break;
			case 4:
				testBlobArray();
				break;
			case 5:
				testInt64();
				break;
			case 6:
				testFloat();
				break;
			case 7:
				testDouble();
				break;
			case 8:
				testFromBuffer();
				break;
		};
	};

	auto runAllTests = [ & ]() -> void
	{
		testMessage();
		testInt32();
		testString();
		testInt64();
		testFloat();
		testDouble();
		testBlobArray();
		testBlobImage();
	};

	mParams = params::InterfaceGl::create( "Params", ivec2( 240, 120 ) );
	mParams->addParam<float>(	"FPS",		&mFps,		true	);
	mParams->addParam<string>(	"Host",		setHost,	getHost );
	mParams->addParam<int32_t>( "Port",		setPort,	getPort ).min( 0 ).max( 65535 ).step( 1 ).keyDecr( "p" ).keyIncr( "P" );
	mParams->addParam<string>(	"Request",	&mRequest			);
	mParams->addParam(			"Test",		mTestNames, &mTestIndex );
	mParams->addButton( "Run test",		runTest,		"key=r" );
	mParams->addButton( "Run all test", runAllTests,	"key=R" );
	mParams->addButton( "Write", bind( &OscDevApp::write, this ), "key=w" );

	gl::enableAlphaBlending();
}

void OscDevApp::testMessage()
{
	string addr = "/foo/bar/baz";
	int32_t attrInt = 4096;
	string attrStr = "Hello, OSC";
	OscTree message = OscTree::makeMessage( addr );
	OscTree valueInt = OscTree( attrInt );
	OscTree valueStr = OscTree( attrStr );

	message.pushBack( valueInt );
	message.pushBack( valueStr );

	BufferRef messageBuffer = message.toBuffer();
	size_t allocSize		= messageBuffer->getAllocatedSize();
	size_t bufferSize		= messageBuffer->getSize();

	CI_LOG_V(  "Test message: " 
		<< "\n\tmessage address: " << message.getAddress() 
		<< "\n\tmessage child int32_t: " << message.getChildren()[ 0 ].getValue<int32_t>() 
		<< "\n\tmessage child string: " << message.getChildren()[ 1 ].getValue<string>() 
		<< "\n\tbuffer alloc size: " << allocSize 
		<< "\n\tbuffer size: " << bufferSize );

	bool passed = ( message.getAddress() == addr && 
		message.getChildren()[ 0 ].getValue<int32_t>() == attrInt && 
		message.getChildren()[ 1 ].getValue<string>() == attrStr );

	string result = "Test message ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testInt32()
{
	int32_t v = 2048;
	OscTree attrInt32 = OscTree( v );
	BufferRef bufferInt32 = attrInt32.toBuffer();
	void* data = bufferInt32->getData();
	int32_t value = *reinterpret_cast<int32_t *>( data );
	
	//CI_LOG_V(  "Test int32_t" 
	//	<< "\n\tdata: " << attrInt32.getValue()->getData() 
	//	<< "\n\tvalue: " << attrInt32.getValue<int32_t>() 
	//	<< "\n\ttype tag: " << attrInt32.getTypeTag() 
	//	<< "\n\tv: " << v 
	//	<< "\n\tbuffer value: " << value );

	CI_LOG_V( getTestOutput<int32_t>( "int32_t", v, attrInt32, bufferInt32 ) );

	bool passed = ( ( attrInt32.getValue<int32_t>() == v ) && 
		( *reinterpret_cast<int32_t *>( attrInt32.getValue()->getData() ) == v ) && 
		( *reinterpret_cast<int32_t *>( bufferInt32->getData() ) == v ) );

	string result = "Test int32_t ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testString()
{
	string v = "Hello, World!";
	OscTree attrString( v );
	BufferRef bufferString = attrString.toBuffer();

	CI_LOG_V( getTestOutput<string>( "string", v, attrString, bufferString ) );


	string oscAttrValue			= attrString.getValue<string>();
	string oscAttrBufferValue	= reinterpret_cast<const char *>( attrString.getValue()->getData() );
	string oscBufferValue		= reinterpret_cast<const char *>( bufferString->getData() );
	bool passed					= ( oscAttrValue == v && oscBufferValue == v && oscAttrBufferValue == v );

	string result = "Test string ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testBlobImage()
{
	mSurface = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );
	ip::fill( mSurface.get(), Colorf( 1.0f, 0.0f, 0.0f ) );

	Perlin p = Perlin( 6 );
	vec2 pos;
	const vec2 noiseScale( 0.01f, 0.01f );
	auto iter = mSurface->getIter();
	while ( iter.line() ) {
		while ( iter.pixel() ) {
			pos = vec2( iter.getPos() );
			iter.g() = static_cast<uint8_t>( 255.0f * p.fBm( pos * noiseScale ) );
		}
	}

	OscTree testBlob = OscTree( mSurface->getData(), mSurface->getRowBytes() * mSurface->getHeight() );
	BufferRef bufferBlob = testBlob.toBuffer();

	CI_LOG_V( "Test blob image" 
		<< "\n\tdata: " << testBlob.getValue()->getData() 
		<< "\n\ttype tag: " << testBlob.getTypeTag() 
		<< "\n\tattr data size: " << testBlob.getValue()->getSize() 
		<< "\n\tattr alloc size: " << testBlob.getValue()->getAllocatedSize() 
		<< "\n\tbuffer data: " << bufferBlob->getData() 
		<< "\n\tbuffer data size: " << bufferBlob->getSize() 
		<< "\n\tbuffer alloc size: " << bufferBlob->getAllocatedSize() );

	//CI_LOG_V(  "Test blob image\n" );
	//CI_LOG_V(  testBlob.getTypeTag() );
	//CI_LOG_V(  testBlob.getValue()->getData() );
	//CI_LOG_V(  testBlob.getValue()->getSize() );
	//CI_LOG_V(  testBlob.getValue()->getAllocatedSize() );
	//CI_LOG_V(  mSurface->getRowBytes() * mSurface->getHeight() );

	mSurfaceOsc = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );
	memcpy( mSurfaceOsc->getData(), testBlob.getValue()->getData(), testBlob.getValue()->getSize() );

	mSurfaceDiff = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );

	Surface8u::ConstIter iterSrc	= mSurface->getIter();
	Surface8u::ConstIter iterOsc	= mSurfaceOsc->getIter();
	Surface8u::Iter iterDiff		= mSurfaceDiff->getIter();

	bool passed = true;
	int8_t dr, dg, db;
	while ( iterSrc.line() && iterOsc.line() && iterDiff.line() ) {
		while ( iterSrc.pixel() && iterOsc.pixel() && iterDiff.pixel() ) {
			dr = iterSrc.r() - iterOsc.r();
			dg = iterSrc.g() - iterOsc.g();
			db = iterSrc.b() - iterOsc.b();

			passed = ( dr == 0 && dg == 0 && db == 0 );

			iterDiff.r() = 255 - ( ( dr < 0 ) ? -dr : dr );
			iterDiff.g() = 255 - ( ( dg < 0 ) ? -dg : dg );
			iterDiff.b() = 255 - ( ( db < 0 ) ? -db : dg );
		}
	}

	string result = "Test blob image ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testBlobArray()
{
	// test random array of data
	const array<uint8_t, 5> v = {
		10, 11, 12, 13, 14
	};

	OscTree testBlobArray( v.data(), 5 * sizeof( uint8_t ) );
	BufferRef bufferBlobArray = testBlobArray.toBuffer();

	CI_LOG_V( "Test blob array of uint8_t" 
		<< "\n\tdata: " << testBlobArray.getValue()->getData() 
		<< "\n\ttype tag: " << testBlobArray.getTypeTag() 
		<< "\n\tattr data size: " << testBlobArray.getValue()->getSize() 
		<< "\n\tattr alloc size: " << testBlobArray.getValue()->getAllocatedSize() 
		<< "\n\tbuffer data: " << bufferBlobArray->getData() 
		<< "\n\tbuffer data size: " << bufferBlobArray->getSize() 
		<< "\n\tbuffer alloc size: " << bufferBlobArray->getAllocatedSize() );

	// Read back the blob from the OscTree and make sure it matches the array above
	uint8_t * data = reinterpret_cast<uint8_t *>( testBlobArray.getValue()->getData() );
	stringstream output;
	bool passed = true;
	for ( size_t i = 0; i < 5; ++i ) {
		output << "data[ " << i << " ]: " << int32_t( data[ i ] ) << "\n";
		passed = ( data[ i ] == v[ i ] );
		if ( !passed ) {
			CI_LOG_E( "<<< ERROR >>> Array values do not match up\n\tindex: " << i 
				<< "\n\tdata: " << int32_t( data[ i ] ) 
				<< "\n\tv: " << int32_t( v[ i ] ) );
		}
	}
	CI_LOG_V( "data output: \n" << output.str() );

	string result = "Test blob array uint8_t ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );

	array<double, 10> arrDoubles;
	for ( auto& d : arrDoubles ) {
		d = randFloat();
	}

	OscTree testBlobArrayDbls( arrDoubles.data(), 10 * sizeof( double ) );
	BufferRef bufferBlobArrayDbls = testBlobArrayDbls.toBuffer();

	CI_LOG_V( "Test blob array of double" 
		<< "\n\tdata: " << testBlobArrayDbls.getValue()->getData() 
		<< "\n\ttype tag: " << testBlobArrayDbls.getTypeTag() 
		<< "\n\tattr data size: " << testBlobArrayDbls.getValue()->getSize() 
		<< "\n\tattr alloc size: " << testBlobArrayDbls.getValue()->getAllocatedSize() 
		<< "\n\tbuffer data: " << bufferBlobArrayDbls->getData() 
		<< "\n\tbuffer data size: " << bufferBlobArrayDbls->getSize() 
		<< "\n\tbuffer alloc size: " << bufferBlobArrayDbls->getAllocatedSize() );

	double * dataDbl = reinterpret_cast<double *>( testBlobArrayDbls.getValue()->getData() );
	output.str( "" );
	output.clear();
	passed = true;
	for ( size_t i = 0; i < 10; ++i ) {
		output << "data[ " << i << " ]: " << dataDbl[ i ] << "\n";
		passed = ( dataDbl[ i ] == arrDoubles[ i ] );
		if ( !passed ) {
			CI_LOG_E( "<<< ERROR >>> Array values do not match up\n\tindex: " << i 
				<< "\n\tdataDbl: " << dataDbl[ i ] 
				<< "\n\tarrDoubles: " << arrDoubles[ i ] );
		}
	}
	CI_LOG_V( "data output: \n" << output.str() );

	result = "Test blob array double ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );

	// TODO: 
	// - test other types of blob data
}

void OscDevApp::testInt64()
{
	int64_t v = std::numeric_limits<int64_t>::max();
	OscTree attrInt64( v );
	BufferRef bufferInt64 = attrInt64.toBuffer();

	CI_LOG_V( getTestOutput<int64_t>( "int64_t", v, attrInt64, bufferInt64 ) );

	bool passed = ( ( attrInt64.getValue<int64_t>() == v ) && 
		( *reinterpret_cast<int64_t *>( attrInt64.getValue()->getData() ) == v ) && 
		( *reinterpret_cast<int64_t *>( bufferInt64->getData() ) == v ) );

	string result = "Test int64_t ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testFloat()
{
	float v = static_cast<float>( M_PI );
	OscTree attrFloat( v );
	BufferRef bufferFloat = attrFloat.toBuffer();

	CI_LOG_V( getTestOutput<float>( "float", v, attrFloat, bufferFloat ) );

	bool passed = ( ( attrFloat.getValue<float>() == v ) && 
		( *reinterpret_cast<float *>( attrFloat.getValue()->getData() ) == v ) && 
		( *reinterpret_cast<float *>( bufferFloat->getData() ) == v ) );

	string result = "Test float ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::testDouble()
{
	double v = M_PI * 2.0;
	OscTree attrDouble( v );
	BufferRef bufferDouble = attrDouble.toBuffer();

	CI_LOG_V( getTestOutput<double>( "double", v, attrDouble, bufferDouble ) );

	bool passed = ( ( attrDouble.getValue<double>() == v ) && 
		( *reinterpret_cast<double *>( attrDouble.getValue()->getData() ) == v ) && 
		( *reinterpret_cast<double *>( bufferDouble->getData() ) == v ) );

	string result = "Test double ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
}

void OscDevApp::update()
{
	mFps = getAverageFps();

	// Render text.
	if ( !mText.empty() ) {
		TextBox tbox = TextBox().alignment( TextBox::LEFT ).font( mFont ).size( ivec2( getWindowWidth() - 250, TextBox::GROW ) ).text( "" );
		for ( vector<string>::const_reverse_iterator iter = mText.rbegin(); iter != mText.rend(); ++iter ) {
			tbox.appendText( "> " + *iter + "\n" );
		}
		tbox.setColor( ColorAf( 1.0f, 0.8f, 0.75f, 1.0f ) );
		tbox.setBackgroundColor( ColorAf::black() );
		tbox.setPremultiplied( false );
		mTexture = gl::Texture::create( tbox.render() );
		while ( mText.size() > 75 ) {
			mText.erase( mText.begin() );
		}
	}
}

void OscDevApp::testFromBuffer()
{
	float f		= 3.14159f;
	string s	= "Hello";
	int32_t i	= 512;
	int64_t h	= numeric_limits<int64_t>::max();
	double d	= 1.61803398875;

	// test blob
	mSurfaceBufferTestA = Surface8u::create( 4, 4, true, SurfaceChannelOrder::RGBA );
	ip::fill( mSurfaceBufferTestA.get(), ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ), Area( 0, 0, mSurfaceBufferTestA->getWidth() / 2, mSurfaceBufferTestA->getHeight() ) );
	ip::fill( mSurfaceBufferTestA.get(), ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ), Area( mSurfaceBufferTestA->getWidth() / 2, 0, mSurfaceBufferTestA->getWidth(), mSurfaceBufferTestA->getHeight() ) );

	OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
	message.pushBack( OscTree( i ) );
	message.pushBack( OscTree( f ) );
	message.pushBack( OscTree( s ) );
	message.pushBack( OscTree( mSurfaceBufferTestA->getData(), mSurfaceBufferTestA->getRowBytes() * mSurfaceBufferTestA->getHeight() ) );
	message.pushBack( OscTree( h ) );
	message.pushBack( OscTree( d ) );

	BufferRef buffer = message.toBuffer();
	OscTree fromBuffer( buffer );

	vector<OscTree>::const_iterator childBegIter = fromBuffer.getChildren().cbegin();
	vector<OscTree>::const_iterator childEndIter = fromBuffer.getChildren().cend();

	for ( ; childBegIter != childEndIter; ++childBegIter ) {
		if ( childBegIter->getTypeTag() == 'i' ) {
			int32_t value = childBegIter->getValue<int32_t>();
			CI_LOG_I( "Test int 32:\n\toriginal value: " << i << "\n\tfromBuffer value: " << value 
				<< "\n\tequal? " << ( ( value == i ) ? "yes" : "no" ) );
		} else if ( childBegIter->getTypeTag() == 'f' ) {
			float value = childBegIter->getValue<float>();
			CI_LOG_I( "Test float:\n\toriginal value: " << f << "\n\tfromBuffer value: " << value 
				<< "\n\tequal? " << ( ( value == f ) ? "yes" : "no" ) );
		} else if ( childBegIter->getTypeTag() == 's' ) {
			string value = childBegIter->getValue<string>();
			CI_LOG_I( "Test string:\n\toriginal value: " << s << "\n\tfromBuffer value: " << value 
				<< "\n\tequal? " << ( ( value == s ) ? "yes" : "no" ) );
		} else if ( childBegIter->getTypeTag() == 'h' ) {
			int64_t value = childBegIter->getValue<int64_t>();
			CI_LOG_I( "Test int 64\n\toriginal value: " << h << "\n\tfromBuffer value: " << value 
				<< "\n\tequal? " << ( ( value == h ) ? "yes" : "no" ) );
		} else if ( childBegIter->getTypeTag() == 'd' ) {
			double value = childBegIter->getValue<double>();
			CI_LOG_I( "Test double:\n\toriginal value: " << d << "\n\tfromBuffer value: " << value 
				<< "\n\tequal? " << ( ( value == d ) ? "yes" : "no" ) );
		} else if ( childBegIter->getTypeTag() == 'b' ) {
			CI_ASSERT( childBegIter->getValue()->getSize() == ( mSurfaceBufferTestA->getRowBytes() * mSurfaceBufferTestA->getHeight() ) );

			// compare the images
			uint8_t * pixelData = reinterpret_cast<uint8_t *>( childBegIter->getValue()->getData() );
			mSurfaceBufferTestB = Surface8u::create( 4, 4, true, SurfaceChannelOrder::RGBA );
			memcpy( mSurfaceBufferTestB->getData(), pixelData, childBegIter->getValue()->getSize() );

			Surface8u::ConstIter iterA = mSurfaceBufferTestA->getIter();
			Surface8u::ConstIter iterB = mSurfaceBufferTestB->getIter();
			bool passed = true;
			uint8_t dr, dg, db, da;
			while ( iterA.line() && iterB.line() ) {
				while ( iterA.pixel() && iterB.pixel() ) {
					dr = iterA.r() - iterB.r();
					dg = iterA.g() - iterB.g();
					db = iterA.b() - iterB.b();
					da = iterA.a() - iterB.a();

					passed = ( dr == 0 && dg == 0 && db == 0 && da == 0 );
					
					if ( !passed ) {
						CI_LOG_F( "<< ERROR >> images do not match\n\tx" << iterA.x() << "\n\ty: " << iterA.y()
							<< "\n\tA.r: " << iterA.r() << "\n\tA.g: " << iterA.g() << "\n\tA.b: " << iterA.b() << "\n\tA.a: " << iterA.a() 
							<< "\n\tB.r: " << iterB.r() << "\n\tB.r: " << iterB.r() << "\n\tB.r: " << iterB.r() << "\n\tB.a: " << iterB.a() );
					}
				}
			}

			CI_LOG_I( "Test blob image: " << ( ( passed ) ? "PASSED" : "FAILED" ) );
		}
	}
}

void OscDevApp::write()
{
	if ( mUdpSession && mUdpSession->getSocket()->is_open() ) {
		//OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
		//message.pushBack( OscTree( 3.1415f ) );
		//message.pushBack( OscTree( mTestStr ) );
		//message.pushBack( OscTree( 4096 ) );

		int32_t valueInt32 = 42;
		string valueString = "Testing 1, 2, 3. Testing.";
		float valueFloat = static_cast<float>( M_PI );
		array<int32_t, 4> valueBlobArray = { 1001, 1002, 1003, 1004 };

		OscTree attrInt32( valueInt32 );
		OscTree attrString( valueString );
		OscTree attrFloat( valueFloat );
		OscTree attrBlobArray( valueBlobArray.data(), valueBlobArray.size() * sizeof( int32_t ) );

		mSurface = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );
		ip::fill( mSurface.get(), Colorf( 1.0f, 0.0f, 0.0f ) );

		Perlin p = Perlin( 6 );
		vec2 pos;
		const vec2 noiseScale( 0.01f, 0.01f );
		auto iter = mSurface->getIter();
		while ( iter.line() ) {
			while ( iter.pixel() ) {
				pos = vec2( iter.getPos() );
				iter.g() = static_cast<uint8_t>( 255.0f * p.fBm( pos * noiseScale ) );
			}
		}

		OscTree attrBlobImage = OscTree( mSurface->getData(), mSurface->getRowBytes() * mSurface->getHeight() );

		OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
		message.pushBack( attrInt32 );
		message.pushBack( attrString );
		message.pushBack( attrFloat );
		message.pushBack( attrBlobArray );
		message.pushBack( attrBlobImage );

		//mUdpSession->write( UdpSession::stringToBuffer( mRequest ) );
		BufferRef messageBuffer = message.toBuffer();
		size_t origDataSize = messageBuffer->getSize();
		size_t origAllocSize = messageBuffer->getAllocatedSize();

		Buffer compressedBuffer = compressBuffer( *messageBuffer );
		messageBuffer = Buffer::create( compressedBuffer.getData(), compressedBuffer.getSize() );

		CI_LOG_I( "Compressed buffer: " 
			<< "\n\toriginal data size: " << origDataSize 
			<< "\n\toriginal alloc size: " << origAllocSize 
			<< "\n\tcompressed data size: " << messageBuffer->getSize() 
			<< "\n\tcompressed alloc size: " << messageBuffer->getAllocatedSize() );

		mUdpSession->write( messageBuffer );

		//const size_t kBufferSize = 501;
		//unique_ptr<uint8_t[]> data( new uint8_t[ kBufferSize ]() );
		//auto data2 = make_unique<uint8_t[]>( kBufferSize );
		//OscTree attrBlob2 = OscTree( data2.get(), kBufferSize * sizeof( uint8_t ) );

		//// addr: 4
		//// type tag: 4
		//// blob header: 4
		//// blob: 504

		//char addr[] = "/a";
		//OscTree message2 = OscTree::makeMessage( addr );
		//message2.pushBack( attrBlob2 );

		//auto buffer2 = message2.toBuffer();

		//CI_LOG_I( "addr size: " << sizeof( addr ) );
		//CI_LOG_I( "blob size: " << attrBlob2.getValue()->getSize() );
		//CI_LOG_I( "buffer2 size: " << buffer2->getSize() );

		//mUdpSession->write( buffer2 );
	} else {
		mText.push_back( "Connecting to: " + mHost + ":" + to_string( mPort ) );
		mUdpClient->connect( mHost, static_cast<uint16_t>( mPort ) );
	}
}

CINDER_APP( OscDevApp, RendererGl, OscDevApp::prepareSettings )
