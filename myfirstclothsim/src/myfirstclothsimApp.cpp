#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class myfirstclothsimApp : public App {
  public:
	void setup() override;
    void prepareSettings( Settings *settings );
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void myfirstclothsimApp::setup()
{
}

void myfirstclothsimApp::prepareSettings( Settings *settings )
{
    settings->setWindowSize( 1000, 100 );
    settings->setFrameRate( 60.0f );
}

void myfirstclothsimApp::mouseDown( MouseEvent event )
{
}

void myfirstclothsimApp::update()
{
}

void myfirstclothsimApp::draw()
{
	gl::clear( Color( 0.4, 0, 0.52 ) );
//    float gray = sin( getElapsedSeconds() ) * 0.5f + 0.5f;
//    gl::clear( Color( gray, gray, gray ), true );
//    gl::drawSolidCircle( Vec2f( 15.0f, 25.0f ), 50.0f )
}

CINDER_APP( myfirstclothsimApp, RendererGl )
