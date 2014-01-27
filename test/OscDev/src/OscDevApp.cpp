#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/Fill.h"
#include "cinder/params/Params.h"
#include "cinder/Surface.h"

#include "OscTree.h"
#include "UdpClient.h"
#include "UdpServer.h"

class OscDevApp : public ci::app::AppNative
{
public:
	void prepareSettings( ci::app::AppNative::Settings* settings );
	void setup();
	void update();
	void draw();
	void keyDown( ci::app::KeyEvent event );

private:
	void onUdpError( std::string error, size_t bytesTransferred );

	void testInt32();
	void testString();
	void testBlobImage();
	void testBlobArray();
	void testInt64();
	void testFloat();
	void testDouble();
	void testMessage();
	void testFromBuffer();

	void setupParams();
	void setupUdp();
	void write();

	UdpServerRef	mServer;
	UdpSessionRef	mServerSession;
	UdpClientRef    mClient;
	UdpSessionRef   mClientSession;
	std::string     mHost;
	std::int32_t    mPort;

	ci::Surface8u	mSurface;
	ci::Surface8u	mSurfaceOsc;

	ci::params::InterfaceGlRef      mParams;
	std::string mTestStr;
};

#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include <limits>

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

void OscDevApp::keyDown( KeyEvent event )
{
	if ( event.getChar() == 't' ) {
		testFromBuffer();
	}
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

	setupParams();
	//setupUdp();

	testFromBuffer();
	// testMessage();
	// testInt32();
	// testString();
	// testBlobImage();
	// testBlobArray();
	// testFloat();
	// testInt64();
	// testDouble();
}

void OscDevApp::setupParams()
{
	mTestStr	= "Hello, OSC Server";
	mHost       = "127.0.0.1";
	mPort       = 2000;

	mParams     = params::InterfaceGl::create( "Params", Vec2i( 250, 300 ) );

	mParams->addParam( "Host", &mHost );
	mParams->addParam( "Port", &mPort );
	mParams->addSeparator();
	mParams->addButton( "Write", bind( &OscDevApp::write, this ), "key=w" );
}

void OscDevApp::setupUdp()
{
	//mServer = UdpServer::create( io_service() );
	//mServer->connectAcceptEventHandler( [ & ]( UdpSessionRef session )
	//{
	//	mServerSession = session;

	//	mServerSession->connectErrorEventHandler( &OscDevApp::onUdpError, this );
	//	mServerSession->connectReadEventHandler( [ & ]( Buffer buffer )
	//	{
	//		console() << "Server Session read " << toString( buffer.getDataSize() ) << " bytes\n"
	//			<< UdpSession::bufferToString( buffer ) << endl;

	//		mServerSession->read();
	//	} );

	//	mServerSession->connectReadCompleteEventHandler( [ & ]()
	//	{
	//		console() << "Server session read complete" << endl;
	//	} );

	//	mServerSession->read();
	//} );

	//mServer->accept( mPort );

	mClient = UdpClient::create( io_service() );
	mClient->connectErrorEventHandler( &OscDevApp::onUdpError, this );
	mClient->connectConnectEventHandler( [&]( UdpSessionRef session )
	{
		console() << "UdpClient connected " << mHost << ":" << mPort << endl;
		mClientSession = session;

		mClientSession->connectErrorEventHandler( &OscDevApp::onUdpError, this );

		mClientSession->connectWriteEventHandler( [&]( size_t bytesTransferred )
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

void OscDevApp::testFromBuffer()
{
	float f		= 3.14159f;
	string s	= "Hello";
	int32_t i	= 512;
	int64_t h	= numeric_limits<int64_t>::max();
	double d	= 1.61803398875;

	// test blob
	Surface8u b = Surface8u( 4, 4, true, SurfaceChannelOrder::RGBA );
	ip::fill( &b, ColorAf( 0.0f, 1.0f, 0.0f, 1.0f ), Area( 0, 0, b.getWidth() / 2, b.getHeight() ) );
	ip::fill( &b, ColorAf( 0.0f, 0.0f, 1.0f, 1.0f ), Area( b.getWidth() / 2, 0, b.getWidth(), b.getHeight() ) );

	mSurface = b.clone( true );

	OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
	message.pushBack( OscTree( i ) );
	message.pushBack( OscTree( f ) );
	message.pushBack( OscTree( s ) );
	message.pushBack( OscTree( b.getData(), b.getRowBytes() * b.getHeight() ) );
	message.pushBack( OscTree( h ) );
	message.pushBack( OscTree( d ) );

	Buffer buffer = message.toBuffer();
	OscTree fromBuffer( buffer );

	vector<OscTree>::const_iterator childBegIter = fromBuffer.getChildren().cbegin();
	vector<OscTree>::const_iterator childEndIter = fromBuffer.getChildren().cend();

	for ( ; childBegIter != childEndIter; ++childBegIter ) {
		if ( childBegIter->getTypeTag() == 'i' ) {
			int32_t value = childBegIter->getValue<int32_t>();
			console() << "original value: " << i << "\nfromBuffer value: " << value 
				<< "\nequal? " << ( ( value == i ) ? "yes" : "no" ) << endl;
		} else if ( childBegIter->getTypeTag() == 'f' ) {
			float value = childBegIter->getValue<float>();
			console() << "original value: " << f << "\nfromBuffer value: " << value 
				<< "\nequal? " << ( ( value == f ) ? "yes" : "no" ) << endl;
		} else if ( childBegIter->getTypeTag() == 's' ) {
			string value = childBegIter->getValue<string>();
			console() << "original value: " << s << "\nfromBuffer value: " << value 
				<< "\nequal? " << ( ( value == s ) ? "yes" : "no" ) << endl;
		} else if ( childBegIter->getTypeTag() == 'h' ) {
			int64_t value = childBegIter->getValue<int64_t>();
			console() << "original value: " << h << "\nfromBuffer value: " << value 
				<< "\nequal? " << ( ( value == h ) ? "yes" : "no" ) << endl;
		} else if ( childBegIter->getTypeTag() == 'd' ) {
			double value = childBegIter->getValue<double>();
			console() << "original value: " << d << "\nfromBuffer value: " << value 
				<< "\nequal? " << ( ( value == d ) ? "yes" : "no" ) << endl;
		}
	}
}

void OscDevApp::write()
{
	if ( mClientSession && mClientSession->getSocket()->is_open() ) {
		OscTree message = OscTree::makeMessage( "/foo/bar/baz" );
		message.pushBack( OscTree( 3.14159f ) );
		message.pushBack( OscTree( mTestStr ) );
		message.pushBack( OscTree( 4096 ) );

		Buffer buffer = message.toBuffer();
		mClientSession->write( buffer );

		//Buffer buffer = UdpSession::stringToBuffer( mTestStr );
		//mClientSession->write( buffer );
	}
}

CINDER_APP_NATIVE( OscDevApp, RendererGl )