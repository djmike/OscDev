#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/Fill.h"
#include "cinder/params/Params.h"
#include "cinder/Surface.h"

#include "OscTree.h"
#include "UdpClient.h"
#include "TcpServer.h"

class OscDevApp : public ci::app::AppNative
{
public:
	void prepareSettings( ci::app::AppNative::Settings* settings );
	void setup();
	void update();
	void draw();

	void onUdpError( std::string error, size_t bytesTransferred );

private:
	void testInt32();
	void testString();
	void testBlobImage();
	void testBlobArray();
	void testInt64();
	void testFloat();
	void testDouble();
	void testMessage();

	void setupUdp();
	void write();

	UdpClientRef    mClient;
	UdpSessionRef   mSession;
	std::string     mHost;
	std::int32_t    mPort;

	ci::Surface8u mSurface;
	ci::Surface8u mSurfaceOsc;

	ci::params::InterfaceGlRef      mParams;
	std::string mTestStr;
};

#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void OscDevApp::draw()
{
	gl::clear();

	if ( mSurface ) {
		gl::draw( gl::Texture( mSurface ), Vec2f( 0.0f, 0.0f ) );
	}
	if ( mSurfaceOsc ) {
		gl::draw( gl::Texture( mSurfaceOsc ), Vec2f( 330.0f, 0.0f ) );
	}

	mParams->draw();
}

void OscDevApp::update()
{
}

void OscDevApp::onUdpError( std::string error, size_t bytesTransferred )
{
	console() << "<< UDP Error >> " << error << "\n\tbytesTransferred: " << bytesTransferred << endl;
}

void OscDevApp::prepareSettings( Settings* settings )
{
	Window::Format windowFormat;
	windowFormat.size( 640, 480 ).title( "OSC Dev" );

	settings->prepareWindow( windowFormat );
	settings->setFrameRate( 60.0f );
}

void OscDevApp::setup()
{
	console() << sizeof( int32_t ) << endl;
	console() << sizeof( uint8_t ) << endl;

	setupUdp();

	testMessage();
	// testInt32();
	// testString();
	// testBlobImage();
	// testBlobArray();
	// testFloat();
	// testInt64();
	// testDouble();
}

void OscDevApp::setupUdp()
{
	mTestStr = "Hello, OSC Server";
	mHost       = "127.0.0.1";
	mPort       = 2000;

	mParams     = params::InterfaceGl::create( "Params", Vec2i( 250, 300 ) );

	mParams->addParam( "Host", &mHost );
	mParams->addParam( "Port", &mPort );
	mParams->addSeparator();
	mParams->addButton( "Write", bind( &OscDevApp::write, this ), "key=w" );

	mClient     = UdpClient::create( io_service() );

	mClient->connectErrorEventHandler( &OscDevApp::onUdpError, this );
	mClient->connectConnectEventHandler( [&]( UdpSessionRef session )
	{
		console() << "UdpClient connected " << mHost << ":" << mPort << endl;
		mSession = session;

		mSession->connectErrorEventHandler( &OscDevApp::onUdpError, this );

		mSession->connectWriteEventHandler( [&]( size_t bytesTransferred )
		{
			console() << "bytesTransfered: " << bytesTransferred << endl;
		} );
	} );

	mClient->connectResolveEventHandler( [&]()
	{
		console() << "End point resolved" << endl;
	} );

	mClient->connect( mHost, static_cast< int16_t >( mPort ) );
}

void OscDevApp::testMessage()
{
	OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
	OscTree valueInt = OscTree( 4096 );
	OscTree valueStr = OscTree( "Hello, OSC" );

	message.pushBack( valueInt );
	message.pushBack( valueStr );

	Buffer messageBuffer = message.toBuffer();
	size_t allocSize = messageBuffer.getAllocatedSize();

	console() << "alloc size: " << allocSize << endl;
}

void OscDevApp::testInt32()
{
	int32_t v = 2048;
	OscTree testInt32 = OscTree( v );
	console() << "Test int32_t\n";
	console() << testInt32.getValue().getData() << endl;
	console() << testInt32.getValue<int32_t>() << endl;
	console() << testInt32.getTypeTag() << endl;

	Buffer bufferInt32 = testInt32.toBuffer();
}

void OscDevApp::testString()
{
	string s = "hello, world!";
	OscTree testString = OscTree( s );
	console() << "Test string\n";
	console() << testString.getValue().getData() << endl;
	console() << testString.getValue<string>() << endl;
	console() << testString.getTypeTag() << endl;

	console() << "data size: " << testString.getValue().getDataSize() << endl;
	console() << "alloc size: " << testString.getValue().getAllocatedSize() << endl;

	Buffer testStringBuff = testString.getValue();
	console() << "buff data size: " << testStringBuff.getDataSize() << endl;
	console() << "buff alloc size: " << testStringBuff.getAllocatedSize() << endl;

	Buffer testStringBuff2 = testString.toBuffer();

	console() << "buff2 data size: " << testStringBuff2.getDataSize() << endl;
	console() << "buff2 alloc size: " << testStringBuff2.getAllocatedSize() << endl;

	console() << "data size: " << testString.getValue().getDataSize() << endl;
	console() << "alloc size: " << testString.getValue().getAllocatedSize() << endl;

	console() << "buff data size: " << testStringBuff.getDataSize() << endl;
	console() << "buff alloc size: " << testStringBuff.getAllocatedSize() << endl;
}

void OscDevApp::testBlobImage()
{
	mSurface = Surface8u( 320, 240, false, SurfaceChannelOrder::RGB );
	ip::fill( &mSurface, Colorf( 1.0f, 0.0f, 0.0f ) );

	OscTree testBlob = OscTree( mSurface.getData(), mSurface.getRowBytes() * mSurface.getHeight() );
	console() << "Test blob\n";
	console() << testBlob.getTypeTag() << endl;
	console() << testBlob.getValue().getData() << endl;
	console() << testBlob.getValue().getDataSize() << endl;
	console() << testBlob.getValue().getAllocatedSize() << endl;
	console() << mSurface.getRowBytes() * mSurface.getHeight() << endl;

	testBlob.toBuffer();

	mSurfaceOsc = Surface8u( 320, 240, false, SurfaceChannelOrder::RGB );
	memcpy( mSurfaceOsc.getData(), testBlob.getValue().getData(), testBlob.getValue().getDataSize() );
}

void OscDevApp::testBlobArray()
{
	// test random array of data
	uint8_t* testArray = new uint8_t[ 5 ];

	for ( size_t i = 0; i < 5; ++i ) {
		testArray[ i ] = randInt();
	}

	for ( size_t i = 0; i < 5; ++i ) {
		console() << i << ": " << testArray[ i ] << endl;
	}

	OscTree testBlob2 = OscTree( testArray, 5 * sizeof( uint8_t ) );
	console() << "Test blob 2\n";
	console() << testBlob2.getTypeTag() << endl;
	console() << testBlob2.getValue().getData() << endl;
	console() << testBlob2.getValue().getDataSize() << endl;
	console() << testBlob2.getValue().getAllocatedSize() << endl;

	testBlob2.toBuffer();
}

void OscDevApp::testInt64()
{

}

void OscDevApp::testFloat()
{

}

void OscDevApp::testDouble()
{

}

void OscDevApp::write()
{
	if ( mSession && mSession->getSocket()->is_open() ) {
		//OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
		//message.pushBack( OscTree( 3.1415f ) );
		//message.pushBack( OscTree( mTestStr ) );
		//message.pushBack( OscTree( 4096 ) );

		//Buffer buffer = message.toBuffer();
		Buffer buffer = UdpSession::stringToBuffer( mTestStr );

		//mSession->write( buffer );
		mSession->write( buffer );
	}
}

CINDER_APP_NATIVE( OscDevApp, RendererGl )