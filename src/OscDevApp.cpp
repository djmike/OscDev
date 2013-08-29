#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

class OscDevApp : public ci::app::AppNative
{
public:
    void prepareSettings( ci::app::AppNative::Settings* settings );
	void setup();
	void update();
	void draw();
    
private:
    
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

void OscDevApp::prepareSettings( Settings* settings )
{
    Window::Format windowFormat;
    windowFormat.size( 1280, 720 ).title( "OSC Dev" );
    
    settings->prepareWindow( windowFormat );
    settings->setFrameRate( 60.0f );
}

void OscDevApp::setup()
{
}

CINDER_APP_NATIVE( OscDevApp, RendererGl )
