#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "OscTree.h"
#include "UdpClient.h"

class OscDevApp : public ci::app::AppNative
{
public:
    void prepareSettings( ci::app::AppNative::Settings* settings );
	void setup();
	void update();
	void draw();
    
    void onUdpError( std::string error, size_t bytesTransferred );
    
private:
    UdpClientRef    mClient;
    UdpSessionRef   mSession;
    std::string     mHost;
    std::int32_t    mPort;
    
    ci::params::InterfaceGlRef      mParams;
};

using namespace ci;
using namespace ci::app;
using namespace std;

void OscDevApp::draw()
{
    gl::clear();
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
    windowFormat.size( 1280, 720 ).title( "OSC Dev" );
    
    settings->prepareWindow( windowFormat );
    settings->setFrameRate( 60.0f );
}

void OscDevApp::setup()
{
	console() << sizeof( int32_t ) << endl;
	console() << sizeof( uint8_t ) << endl;
	
	int32_t v	= 2048;
	OscTree mTest = OscTree( v );
	console() << mTest.getValue().getData() << endl;
	console() << mTest.getValue<int32_t>() << endl;
	
    mHost       = "127.0.0.1";
    mPort       = 2000;
    
    mParams     = params::InterfaceGl::create( "Params", Vec2i( 250, 300 ) );
    
    mParams->addParam( "Host", &mHost );
    mParams->addParam( "Port", &mPort );
    
    mClient     = UdpClient::create( io_service() );
    
    mClient->connectErrorEventHandler( &OscDevApp::onUdpError, this );
    mClient->connectConnectEventHandler( [&]( UdpSessionRef session )
    {
        console() << "UdpClient connected " << mHost << ":" << mPort << endl;
        mSession = session;
        
        mSession->connectErrorEventHandler( &OscDevApp::onUdpError, this );
        mSession->connectCloseEventHandler( [&]()
        {
            console() << "Session Closed" << endl;
        } );
        
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

CINDER_APP_NATIVE( OscDevApp, RendererGl )
