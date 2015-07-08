#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "UdpServer.h"

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

private:
	int32_t						mPort;
	UdpServerRef				mUdpServer;
	UdpSessionRef				mUdpSession;

	ci::Font					mFont;
	std::vector<std::string>	mText;
	ci::gl::TextureRef			mTexture;

	ci::params::InterfaceGlRef	mParams;
	float						mFps;
};

#include "cinder/app/RendererGl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void OscDevServerApp::prepareSettings( App::Settings * settings )
{
#if NDEBUG
	settings->setConsoleWindowEnabled();
#endif
	settings->setFrameRate( 60.0f );
	settings->prepareWindow( Window::Format().size( 640, 640 ).title( "Osc Dev Server" ) );
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

	if ( mTexture ) {
		gl::draw( mTexture, ivec2( 10, 100 ) );
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

	mUdpSession->read();
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

	string response = UdpSession::bufferToString( buffer );
	string text = "Response";
	if ( !response.empty() ) {
		text += ": " + response;
	}
	mText.push_back( text );

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
