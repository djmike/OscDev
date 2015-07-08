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

	ci::params::InterfaceGlRef	mParams;
	float						mFps;
	int32_t						mTestIndex;
};


#include "cinder/app/RendererGl.h"
#include "cinder/ip/Fill.h"
#include "cinder/Log.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

template<typename T>
string getTestOutput( const string &testType, const T &v, const OscTree &oscAttr, const BufferRef &buffer )
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
		gl::draw( mTexture, ivec2( 10, 160 ) );
	}

	if ( mSurface ) {
		gl::draw( gl::Texture::create( *mSurface ) );
	}

	if ( mSurfaceOsc ) {
		gl::draw( gl::Texture::create( *mSurfaceOsc ), vec2( 330.0f, 0.0f ) );
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
		"double"
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
	OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
	OscTree valueInt = OscTree( 4096 );
	OscTree valueStr = OscTree( "Hello, OSC" );

	message.pushBack( valueInt );
	message.pushBack( valueStr );

	BufferRef messageBuffer = message.toBuffer();
	size_t allocSize		= messageBuffer->getAllocatedSize();
	size_t bufferSize		= messageBuffer->getSize();

	CI_LOG_V(  "Test message: " 
		<< "\n\tbuffer alloc size: " << allocSize 
		<< "\n\tbuffer size: " << bufferSize );
}

void OscDevApp::testInt32()
{
	int32_t v = 2048;
	OscTree testInt32 = OscTree( v );
	BufferRef bufferInt32 = testInt32.toBuffer();
	void* data = bufferInt32->getData();
	int32_t value = *reinterpret_cast<int32_t *>( data );
	
	//CI_LOG_V(  "Test int32_t" 
	//	<< "\n\tdata: " << testInt32.getValue()->getData() 
	//	<< "\n\tvalue: " << testInt32.getValue<int32_t>() 
	//	<< "\n\ttype tag: " << testInt32.getTypeTag() 
	//	<< "\n\tv: " << v 
	//	<< "\n\tbuffer value: " << value );

	CI_LOG_V( getTestOutput<int32_t>( "int32_t", v, testInt32, bufferInt32 ) );
}

void OscDevApp::testString()
{
	string v = "Hello, World!";
	OscTree attrString( v );
	BufferRef bufferString = attrString.toBuffer();

	CI_LOG_V( getTestOutput<string>( "string", v, attrString, bufferString ) );
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
	for ( size_t i = 0; i < 5; ++i ) {
		output << "data[ " << i << " ]: " << int32_t( data[ i ] ) << "\n";
		if ( data[ i ] != v[ i ] ) {
			CI_LOG_E( "<<< ERROR >>> Array values do not match up\n\tindex: " << i 
				<< "\n\tdata: " << int32_t( data[ i ] ) 
				<< "\n\tv: " << int32_t( v[ i ] ) );
		}
	}
	CI_LOG_V( "data output: \n" << output.str() );


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
	for ( size_t i = 0; i < 10; ++i ) {
		output << "data[ " << i << " ]: " << dataDbl[ i ] << "\n";
		if ( dataDbl[ i ] != arrDoubles[ i ] ) {
			CI_LOG_E( "<<< ERROR >>> Array values do not match up\n\tindex: " << i 
				<< "\n\tdataDbl: " << dataDbl[ i ] 
				<< "\n\tarrDoubles: " << arrDoubles[ i ] );
		}
	}
	CI_LOG_V( "data output: \n" << output.str() );
}

void OscDevApp::testInt64()
{
	int64_t v = std::numeric_limits<int64_t>::max();
	OscTree attrInt64( v );
	BufferRef bufferInt64 = attrInt64.toBuffer();

	CI_LOG_V( getTestOutput<int64_t>( "int64_t", v, attrInt64, bufferInt64 ) );
}

void OscDevApp::testFloat()
{
	float v = static_cast<float>( M_PI );
	OscTree attrFloat( v );
	BufferRef bufferFloat = attrFloat.toBuffer();

	CI_LOG_V( getTestOutput<float>( "float", v, attrFloat, bufferFloat ) );
}

void OscDevApp::testDouble()
{
	double v = M_PI * 2.0;
	OscTree attrDouble( v );
	BufferRef bufferDouble = attrDouble.toBuffer();

	CI_LOG_V( getTestOutput<double>( "double", v, attrDouble, bufferDouble ) );
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

void OscDevApp::write()
{
	if ( mUdpSession && mUdpSession->getSocket()->is_open() ) {
		//OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
		//message.pushBack( OscTree( 3.1415f ) );
		//message.pushBack( OscTree( mTestStr ) );
		//message.pushBack( OscTree( 4096 ) );

		mUdpSession->write( UdpSession::stringToBuffer( mRequest ) );
	} else {
		mText.push_back( "Connecting to: " + mHost + ":" + to_string( mPort ) );
		mUdpClient->connect( mHost, static_cast<uint16_t>( mPort ) );
	}
}

CINDER_APP( OscDevApp, RendererGl, OscDevApp::prepareSettings )
