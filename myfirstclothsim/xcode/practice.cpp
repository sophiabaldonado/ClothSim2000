//
//  practice.cpp
//  myfirstclothsim
//
//  Created by Sophia Baldonado on 8/12/16.
//
//

#include "practice.hpp"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Easing.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace gl;

class practice : public App {
public:
    void setup() override;
    void prepareSettings( Settings *settings );
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;
    
    static const int NUM_NOTES = 12;
    
    CameraPersp mCam;
    BatchRef mBox = mBox;
    BatchRef mNotes[NUM_NOTES];
    
    Texture2dRef mTexture;
    BatchRef mSphere;
    GlslProgRef mGlsl;
};

void practice::setup()
{
    
    mCam.lookAt( vec3( 3, 2, 4 ), vec3( 0 ) );
    
    auto clouds = loadImage( loadAsset( "clouds.png" ) );
    mTexture = Texture2d::create( clouds );
    mTexture->bind();
    
    auto shader = ShaderDef().texture().lambert();
    mGlsl = getStockShader( shader );
    auto sphere = geom::Sphere().subdivisions( 50 );
    mSphere = Batch::create( sphere, mGlsl );
    
    enableDepthWrite();
    enableDepthRead();
    
    
//    practicing 3D
    
//    auto lambert = ShaderDef().lambert().color();
//    GlslProgRef shader = getStockShader( lambert );
//    mBox = Batch::create( geom::Cube(), shader );
//    
//    for( int i = 0; i < NUM_NOTES; ++i ) {
//        float rel = i / (float)NUM_NOTES;
//        float noteHeight = 1.0f / NUM_NOTES;
//        auto note = geom::Cube().size( 1, noteHeight, 1 );
//        auto trans = geom::Translate( 0, rel + 2, 0 );
//        auto color = geom::Constant( geom::COLOR, Color( CM_HSV, rel , 1, 1 ) );
//        mNotes[i] = Batch::create( note >> trans >> color, shader );
//    }
//    
//    
//    mCam.lookAt( vec3( 3, 4.5, 5 ), vec3( 0, 1, 0 ) );
}

void practice::mouseDown( MouseEvent event )
{
}

void practice::update()
{
}

void practice::draw() {
    clear( Color( 0.2f, 0.2f, 0.2f ) );
    setMatrices ( mCam );
    
    mSphere->draw();
    
//    gl::draw( mTex );
    
}

//Practice Rendering 3D Shapes and Animating with Cinder

//void practice::draw()
//{
//    clear();
//    enableDepthRead();
//    enableDepthWrite();
//    
//    setMatrices( mCam );
//    
////    CameraPersp cam;
////    cam.lookAt( vec3( 3, 5, 5 ), vec3( 0, 1, 0 ) );
////    setMatrices( cam );
//
//    
////    console() << gl::getProjectionMatrix() << endl;
//    
//    auto lambert = ShaderDef().lambert().color();
//    auto shader = getStockShader( lambert );
//    shader->bind();
//    
//    int numSpheres = 600;
//    float maxAngle = M_PI * 10;
//    float spiralRadius = 1;
//    float height = 2;
//    float boxSize = 0.05f;
//    float anim = getElapsedFrames() / 20.0f;
//    
//    for( int s = 0; s < numSpheres; ++s ) {
//        float rel = s / (float)numSpheres;
//        float angle = rel * maxAngle;
//        float y = fabs( cos( rel * M_PI + anim ) ) * (height + (s/(-300)));
//        float r = rel * spiralRadius;
//        vec3 offset( r * cos( angle ), y / 2 , r * sin( angle ) );
//        
//        pushModelMatrix();
//        translate( offset );
//        scale ( vec3( boxSize, y, boxSize ) );
//        color( Color( CM_HSV, rel, 1, 1 ) );
////        drawSphere( vec3(), 0.1f, 30 );
////        drawCube( vec3(), vec3( 1 ) );
//        mBox->draw();
//        popModelMatrix();
//    }
//    
//    const float delay = 0.25f;
//    // time in seconds for one slice to rotate
//    const float rotationTime = 1.5f;
//    // time in seconds to delay each slice's rotation
//    const float rotationOffset = 0.1f; // seconds
//    // total time for entire animation
//    const float totalTime = delay + rotationTime +
//    NUM_NOTES * rotationOffset;
//    
//    // loop every 'totalTime' seconds
//    float time = fmod( getElapsedFrames() / 30.0f, totalTime );
//    
//    for( int i = 0; i < NUM_NOTES; ++i ) {
//        // animates from 0->1
//        float rotation = 0;
//        // when does the slice begin rotating
//        float startTime = i * rotationOffset;
//        // when does it complete
//        float endTime = startTime + rotationTime;
//        // are we in the middle of our time section?
//        if( time > startTime && time < endTime )
//            rotation = ( time - startTime ) / rotationTime;
//        // ease fn on rotation, then convert to radians
//        float angle = easeInOutQuint( rotation ) * M_PI / 1.0f;
//        
//        gl::ScopedModelMatrix scpModelMtx;
//        gl::rotate( angleAxis( angle, vec3( 0, 1, 0 ) ) );
//        mNotes[i]->draw();
//    }
//    
//}

//Practice rendering 2D Shapes with Cinder

//void practice::draw()
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


CINDER_APP( practice, RendererGl );
