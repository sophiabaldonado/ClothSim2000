#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"


using namespace ci;
using namespace ci::app;
using namespace std;
//using namespace gl;

const uint32_t POINTS_X				= 50;
const uint32_t POINTS_Y				= 50;
const uint32_t POINTS_TOTAL			= (POINTS_X * POINTS_Y);
const uint32_t CONNECTIONS_TOTAL	= (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;

const uint32_t POSITION_INDEX		= 0;
const uint32_t VELOCITY_INDEX		= 1;
const uint32_t CONNECTION_INDEX		= 2;

class myfirstclothsimApp : public App {
  public:
//	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
    
    void setupBuffers();
    void setupGlsl();
    void setupParams();
    
    gl::BatchRef        mRect;
    gl::GlslProgRef		mGlsl;
    
    CameraPersp         mCam;
    float               mCurrentCamRotation;
    uint32_t			mIterationsPerFrame, mIterationIndex;
    bool				mDrawPoints, mDrawLines, mUpdate;
    
    ci::params::InterfaceGlRef			mParams;
    
};

myfirstclothsimApp::myfirstclothsimApp()
: mIterationsPerFrame( 16 ), mIterationIndex( 0 ),
    mDrawPoints( true ), mDrawLines( true ),
    mCurrentCamRotation( 0.0f ), mUpdate( true ),
    mCam( getWindowWidth(), getWindowHeight(), 20.0f, 0.0f, 1000.0f )
{
    vec3 eye = vec3( sin( mCurrentCamRotation ) * 140.0f, 0,
                    cos( mCurrentCamRotation ) * 140.0f );
    vec3 target = vec3( 0.0f );
    mCam.lookAt(eye, target );
    
    setup Glsl();
    setupBuffers();
    setupPrams();
}

void myfirstclothsimApp::setupBuffers()
{
    mCam.lookAt( vec3( 3, 2, 4 ), vec3( 0 ) );
    
    mGlsl = gl::GlslProg::create( gl::GlslProg::Format()
        .vertex(	CI_GLSL( 150,
            uniform mat4	ciModelViewProjection;
            in vec4			ciPosition;
            in vec4         ciColor;
            out vec4        Color;
                                                    
                                                    
            void main( void ) {
                for( int i = 0; i < 1; ++i ) {
                    gl_Position	= ciModelViewProjection * ciPosition + i;
                    Color = ciColor;
                }
            }
        ) )
        .fragment(	CI_GLSL( 150,
            in vec4             Color;
            out vec4			oColor;
                                                    
            void main( void ) {
                oColor = Color;
            }
        ) )
    );
    
    auto sheet = geom::Rect().colors( true );
                                    
//    cube.colors( ColorAf( 0.45f, 0.0f , 0.5f ),
//                 ColorAf( 0.45f, 0.0f , 0.5f ),
//                 ColorAf( 0.45f, 0.0f , 0.5f ),
//                 ColorAf( 0.45f, 0.0f , 0.5f ) );
    mRect = gl::Batch::create( sheet, mGlsl );
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
}

void myfirstclothsimApp::mouseDown( MouseEvent event )
{
}

void myfirstclothsimApp::update()
{
}

void myfirstclothsimApp::draw()
{
    mat4 pz = gl::getModelViewProjection();
//    cout << pz << endl;
    gl::clear( Color( 0.2f, 0.2f, 0.2f ) );
    gl::setMatrices( mCam );
    mRect->draw();
}


CINDER_APP( myfirstclothsimApp, RendererGl )
