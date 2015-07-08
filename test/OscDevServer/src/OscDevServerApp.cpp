#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "UdpServer.h"
#include "OscTree.h"

class OscDevServerApp : public ci::app::App
{
public:
	static void prepareSettings( ci::app::App::Settings * settings );

	OscDevServerApp();
	~OscDevServerApp();

	void draw() override;
	void setup() override;
	void update() override;
	
private:
	void	accept();
	void	onAccept( UdpSessionRef session );
	void	onError( std::string error, size_t bytesTransferred );
	void	onRead( ci::BufferRef buffer );
	void	onReadComplete();
	void	onWrite( size_t bytesTransferred );
	void	readOsc( const ci::BufferRef& buffer );

private:
	int32_t						mPort;
	UdpServerRef				mUdpServer;
	UdpSessionRef				mUdpSession;

	ci::Font					mFont;
	std::vector<std::string>	mText;
	ci::gl::TextureRef			mTexture;

	ci::Surface8uRef			mSurface;
	ci::Surface8uRef			mSurfaceOsc;
	ci::Surface8uRef			mSurfaceDiff;

	ci::params::InterfaceGlRef	mParams;
	float						mFps;
};

#include "cinder/app/RendererGl.h"
#include "cinder/ip/Fill.h"
#include "cinder/Log.h"
#include "cinder/Perlin.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void OscDevServerApp::prepareSettings( App::Settings * settings )
{
#if NDEBUG
	settings->setConsoleWindowEnabled();
#endif
	settings->setFrameRate( 60.0f );
	settings->prepareWindow( Window::Format().size( 1280, 720 ).title( "Osc Dev Server" ) );
}

OscDevServerApp::OscDevServerApp() :
	mPort( 2000 ), 
	mFont( "Georgia", 24 ), 
	mFps( 0.0f )
{
}

OscDevServerApp::~OscDevServerApp()
{
}

void OscDevServerApp::accept()
{
	if ( mUdpSession ) {
		mUdpSession.reset();
	}

	if ( mUdpServer ) {
		mUdpServer->accept( static_cast<uint16_t>( mPort ) );

		mText.push_back( "Listening on port: " + to_string( mPort ) );
	}
}

void OscDevServerApp::draw()
{
	gl::clear( Colorf::black() );

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

	if ( mTexture ) {
		gl::draw( mTexture, vec2( 10, 240 ) );
	}

	mParams->draw();
}

void OscDevServerApp::onAccept( UdpSessionRef session )
{
	mUdpSession = session;
	mUdpSession->connectErrorEventHandler( &OscDevServerApp::onError, this );
	mUdpSession->connectReadEventHandler( &OscDevServerApp::onRead, this );
	mUdpSession->connectReadCompleteEventHandler( &OscDevServerApp::onReadComplete, this );
	mUdpSession->connectWriteEventHandler( &OscDevServerApp::onWrite, this );

	mUdpSession->read( numeric_limits<uint16_t>::max() );
}

void OscDevServerApp::onError( std::string error, size_t bytesTransferred )
{
	string text = "Error";
	if ( !error.empty() ) {
		text += ": " + error;
	}
	mText.push_back( text );
}

void OscDevServerApp::onRead( ci::BufferRef buffer )
{
	mText.push_back( to_string( buffer->getSize() ) + " bytes read" );

	//string response = UdpSession::bufferToString( buffer );
	//string text = "Response";
	//if ( !response.empty() ) {
	//	text += ": " + response;
	//}
	//mText.push_back( text );

	readOsc( buffer );

	mUdpSession->read();
}

void OscDevServerApp::onReadComplete()
{
	mText.push_back( "Read complete" );

	mUdpSession->read();
}

void OscDevServerApp::onWrite( size_t bytesTransferred )
{
	mText.push_back( to_string( bytesTransferred ) + " bytes written" );

	mUdpSession->read();
}

void OscDevServerApp::readOsc( const BufferRef &buffer )
{
	//return;

	size_t origDataSize = buffer->getSize();
	size_t origAllocSize = buffer->getAllocatedSize();

	Buffer uncompressedBuffer = decompressBuffer( *buffer );
	BufferRef oscBuffer = Buffer::create( uncompressedBuffer.getData(), uncompressedBuffer.getSize() );
	OscTree oscPacket( oscBuffer );

	CI_LOG_I( "Compressed buffer: " 
		<< "\n\toriginal data size: " << origDataSize 
		<< "\n\toriginal alloc size: " << origAllocSize 
		<< "\n\tdecompressed data size: " << oscBuffer->getSize() 
		<< "\n\tdecompressed alloc size: " << oscBuffer->getAllocatedSize() );

	// Expected values
	int32_t valueInt32 = 42;
	string valueString = "Testing 1, 2, 3. Testing.";
	float valueFloat = static_cast<float>( M_PI );
	array<int32_t, 4> valueBlobArray = { 1001, 1002, 1003, 1004 };
	
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

	bool passed = ( valueInt32 == oscPacket.getChildren()[ 0 ].getValue<int32_t>() );
	string result = "Test int32_t: ";
	result += ( passed ) ? "PASSED" : "FAILED";
	result += "\n\tvalue: " + to_string( oscPacket.getChildren()[ 0 ].getValue<int32_t>() );
	CI_LOG_I( result );
	mText.push_back( result );

	passed = ( valueString == oscPacket.getChildren()[ 1 ].getValue<string>() );
	result = "Test string: ";
	result += ( passed ) ? "PASSED" : "FAILED";
	result += "\n\tvalue: " + oscPacket.getChildren()[ 1 ].getValue<string>();
	CI_LOG_I( result );
	mText.push_back( result );

	passed = ( valueFloat == oscPacket.getChildren()[ 2 ].getValue<float>() );
	result = "Test float: ";
	result += ( passed ) ? "PASSED" : "FAILED";
	result += "\n\tvalue: " + to_string( oscPacket.getChildren()[ 2 ].getValue<float>() );
	CI_LOG_I( result );
	mText.push_back( result );

	passed = true;
	int32_t * pInt32Data = reinterpret_cast<int32_t *>( oscPacket.getChildren()[ 3 ].getValue()->getData() );
	array<int32_t, 4> oscBlobArray;
	for ( size_t i = 0; i < 4; ++i ) {
		passed = ( pInt32Data[ i ] == valueBlobArray[ i ] );
		if ( !passed ) {
			CI_LOG_F( "<< ERROR Blob Array mismatch >> index: " << i 
				<< "\n\tpInt32Data: " << pInt32Data[ i ] 
				<< "\n\tvalueBlobArray: " << valueBlobArray[ i ] );
		}
		oscBlobArray[ i ] = pInt32Data[ i ];
	}
	result = "Test blob array: ";
	result += ( passed ) ? "PASSED" : "FAILED";
	string arrayValues;
	for_each( oscBlobArray.cbegin(), oscBlobArray.cend(), [ & ]( int32_t v ) -> void
	{
		arrayValues += to_string( v ) + ", ";
	} );
	result += "\n\tvalue: " + arrayValues;
	CI_LOG_I( result );
	mText.push_back( result );

	mSurfaceOsc = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );
	memcpy( mSurfaceOsc->getData(), oscPacket.getChildren()[ 4 ].getValue()->getData(), mSurfaceOsc->getRowBytes() * mSurfaceOsc->getHeight() );
	size_t szA = oscPacket.getChildren()[ 4 ].getValue()->getSize();
	size_t szB = mSurfaceOsc->getRowBytes() * mSurfaceOsc->getHeight();
	CI_ASSERT( szA == szB );

	mSurfaceDiff = Surface8u::create( 320, 240, false, SurfaceChannelOrder::RGB );

	Surface8u::ConstIter iterSrc	= mSurface->getIter();
	Surface8u::ConstIter iterOsc	= mSurfaceOsc->getIter();
	Surface8u::Iter iterDiff		= mSurfaceDiff->getIter();

	passed = true;
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

	result = "Test blob image ";
	if ( passed ) {
		result += "PASSED";
	} else {
		result += "FAILED";
		CI_LOG_F( "<<< FATAL Test Failure >>> " + result );
	}
	mText.push_back( result );
	CI_LOG_I( result );
}

void OscDevServerApp::setup()
{
	mUdpServer = UdpServer::create( io_service() );
	mUdpServer->connectAcceptEventHandler( &OscDevServerApp::onAccept, this );
	mUdpServer->connectErrorEventHandler( &OscDevServerApp::onError, this );

	mParams = params::InterfaceGl::create( "Params", ivec2( 200, 75 ) );
	mParams->addParam<float>(	"FPS",	&mFps, true );

	auto setPort = [ & ]( int32_t port ) -> void
	{
		mPort = port;
	};

	auto getPort = [ & ]() -> int32_t
	{
		return mPort;
	};

	mParams->addParam<int32_t>( "Port", setPort, getPort ).min( 0 ).max( 65535 ).keyDecr( "p" ).keyIncr( "P" ).step( 1 );

	// Draw text more legibly
	gl::enableAlphaBlending();

	// Start listening
	accept();
}

void OscDevServerApp::update()
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

CINDER_APP( OscDevServerApp, RendererGl, OscDevServerApp::prepareSettings )
