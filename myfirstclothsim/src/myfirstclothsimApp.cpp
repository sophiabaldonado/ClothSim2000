#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Shader.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace gl;

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
    settings->setWindowSize( 1000, 800 );
    settings->setFrameRate( 60.0f );
}

void myfirstclothsimApp::mouseDown( MouseEvent event )
{
}

void myfirstclothsimApp::update()
{
}

//Practice Rendering 3D Shapes and Animating with Cinder

void myfirstclothsimApp::draw()
{
    clear();
    enableDepthRead();
    enableDepthWrite();
    
    CameraPersp cam;
    cam.lookAt( vec3( 3, 5, 5 ), vec3( 0, 1, 0 ) );
    setMatrices( cam );

//    console() << gl::getProjectionMatrix() << endl;
    
    auto lambert = ShaderDef().lambert().color();
    auto shader = getStockShader( lambert );
    shader->bind();

    int numSpheres = 64;
    float maxAngle = M_PI * 7;
    float spiralRadius = 1;
    float height = 2;
    float anim = getElapsedFrames() / 30.0f;
    
    for( int s = 0; s < numSpheres; ++s ) {
        float rel = s / (float)numSpheres;
        float angle = rel * maxAngle;
        float y = fabs( cos( rel * M_PI + anim ) ) * height;
        float r = rel * spiralRadius;
        vec3 offset( r * cos( angle ), y / 2 , r * sin( angle ) );
        
        pushModelMatrix();
        translate( offset );
        scale ( vec3( 0.05f, y, 0.05f ) );
        color( Color( CM_HSV, rel, 1, 1 ) );
//        drawSphere( vec3(), 0.1f, 30 );
        drawCube( vec3(), vec3( 1 ) );
        popModelMatrix();
    }
    
}

//Practice rendering 2D Shapes with Cinder

//void myfirstclothsimApp::draw()
//{
//    gl::clear();
//
//    gl::pushModelMatrix();
//    gl::translate( getWindowCenter() );
//
//    int numCircles = 16;
//    float radius = getWindowHeight() / 2 - 30;
//
//
//    for( int c = 0; c < numCircles; ++c ) {
//        float rel = c / (float)numCircles;
//        float angle = rel * M_PI * 2;
//        vec2 offset( cos( angle ), sin( angle ) );
//
//
//        gl::pushModelMatrix();
//        gl::translate( offset * radius );
//        gl::color ( Color( 0.45, 0, 0.5 ) );
////        gl::scale( 3, 0.25f );
//        gl::drawSolidCircle( vec2(), 30 );
//        gl::popModelMatrix();
//    }
//
//    gl::color( Color( 0, 0.25f, 0.25f ) );
//    gl::drawStrokedCircle( vec2 (), 50 );
//
//    gl::popModelMatrix();
//}


CINDER_APP( myfirstclothsimApp, RendererGl )
