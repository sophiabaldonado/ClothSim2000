#include "practice.hpp"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Easing.h"


using namespace ci;
using namespace ci::app;
using namespace std;

const uint32_t POINTS_X				= 20;
const uint32_t POINTS_Y				= 20;
const uint32_t POINTS_TOTAL			= (POINTS_X * POINTS_Y);

struct Particle {
//    vec3    pos;
//    vec3    ppos;
//    vec3    home;
    ColorA  color;
//    float   damping;
    
    bool movable; // can the particle move or not ? used to pin parts of the cloth
    
    float mass; // the mass of the particle (is always 1 in this example)
    vec3 pos; // the current position of the particle in 3D space
    vec3 old_pos; // the position of the particle in the previous time step, used as part of the verlet numerical integration scheme
    vec3 acceleration; // a vector representing the current acceleration of the particle
    vec3 accumulated_normal; // an accumulated normal (i.e. non normalized), used for OpenGL soft shading
};


class myfirstclothsimApp : public App {
public:
    void	setup() override;
    void    update() override;
    void	draw() override;
    
    void    setupBuffers();
    void    setupGlsl();
private:
    
    CameraPersp         mCam;
    gl::BatchRef        mRect;
    gl::GlslProgRef		mRenderProg, mUpdateProg;
    
    // Descriptions of particle data layout
    gl::VaoRef		mAttributes[2];
    // Buffers holding raw particle data on GPU
    gl::VboRef		mParticleBuffer[2];
    
    std::array<gl::VaoRef, 2>			mVaos;
    std::array<gl::VboRef, 2>			mPositions;
    
    // Current source and destination buffers for transform feedback.
    // Source and destination are swapped each frame after update.
    std::uint32_t	mSourceIndex		= 0;
    std::uint32_t	mDestinationIndex	= 1;
    
    // Mouse state suitable for passing as uniforms to update program
    bool			mMouseDown = false;
    float			mMouseForce = 0.0f;
    vec3			mMousePos = vec3( 0, 0, 0 );
};

void myfirstclothsimApp::setup()
{
    setupGlsl();
    setupBuffers();
}

void myfirstclothsimApp::setupGlsl()
{
    // Create a default color shader.
    //    mRenderProg = gl::getStockShader( gl::ShaderDef().color() );
    
    for( int i = 0; i < 2; ++i )
    {	// Describe the particle layout for OpenGL.
        mAttributes[i] = gl::Vao::create();
        gl::ScopedVao vao( mAttributes[i] );
        
        // Define attributes as offsets into the bound particle buffer
        gl::ScopedBuffer buffer( mParticleBuffer[i] );
        gl::enableVertexAttribArray( 0 );
        gl::enableVertexAttribArray( 1 );
        gl::enableVertexAttribArray( 2 );
        gl::enableVertexAttribArray( 3 );
        //        gl::enableVertexAttribArray( 4 );
        //        gl::enableVertexAttribArray( 5 );
        gl::vertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, pos) );
        gl::vertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, color) );
        gl::vertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, old_pos) );
        gl::vertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, acceleration) );
        //        gl::vertexAttribPointer( 4, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, accumulated_normal) );
        //        gl::vertexAttribPointer( 5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, damping) );
    }
    
    mUpdateProg = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadResource( "newUpdate.vert" ) )
                                       .feedbackFormat( GL_INTERLEAVED_ATTRIBS ) // may have to use GL_SEPARATE_ATTRIBS and separate buffers
                                       .feedbackVaryings( { "position", "old_position", "acceleration", "color" } )
                                       .attribLocation( "iPosition", 0 )
                                       .attribLocation( "iColor", 1 )
                                       .attribLocation( "iOldPosition", 2 )
                                       .attribLocation( "iAcceleration", 3 )
                                       //                                       .attribLocation( "iAccumulatedNormal", 4 )
                                       //                                       .attribLocation( "iDamping", 5 )
                                       );
    
    gl::GlslProg::Format renderFormat;
    renderFormat.vertex( loadResource( "render.vert" ) )
    .fragment( loadResource( "render.frag" ) );
    
    mRenderProg = gl::GlslProg::create( renderFormat );
}

void myfirstclothsimApp::setupBuffers()
{
    mCam.lookAt( vec3( 3, 2, 3 ), vec3( 0 ) );
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
    
    vector<Particle> particles;
    particles.assign( POINTS_TOTAL, Particle() );
//    const float azimuth = 256.0f * M_PI / particles.size();
//    const float inclination = M_PI / particles.size();
//    const float radius = 80.0f;
//    acceleration = 
//    vec3 center = vec3( getWindowCenter() + vec2( 0.0f, 40.0f ), 0.0f );
    
//    for( int i = 0; i < particles.size(); ++i )
//    {	// assign starting values to particles.
//        float x = radius * sin( inclination * i ) * cos( azimuth * i );
//        float y = radius * cos( inclination * i );
//        float z = radius * sin( inclination * i ) * sin( azimuth * i );
//        
//        auto &p = particles.at( i );
//        p.pos = center + vec3( x, y, z );
//        p.home = p.pos;
//        p.ppos = p.home + Rand::randVec3() * 10.0f; // random initial velocity
//        p.damping = Rand::randFloat( 0.965f, 0.985f );
//        p.color = Color( CM_HSV, lmap<float>( i, 0.0f, particles.size(), 0.0f, 0.66f ), 1.0f, 1.0f );
//    }
    
    int i, j;
    
    std::array<vec4, POINTS_TOTAL> positions;
//    std::array<vec3, POINTS_TOTAL> velocities;
//    std::array<ivec4, POINTS_TOTAL> connections;
    
    int n = 0;
    for ( j = 0; j < POINTS_Y; j++ ) {
        float fj = (float)j / (float)POINTS_Y;
        for ( i = 0; i < POINTS_X; i++) {
            float fi = (float)i / (float)POINTS_X;
            
            // create our initial positions, Basically a plane
            positions[n] = vec4( ( fi - 0.5f ) * (float)POINTS_X,
                                ( fj - 0.5f ) * (float)POINTS_Y,
                                0.6f * sinf( fi ) * cosf( fj ),
                                1.0f );
//            // zero out velocities
//            velocities[n] = vec3( 0.0f );
//            // to create connections we'll use an integer buffer.
//            // The value -1 refers to the fact that there's no
//            // connection. This helps for the edge cases of the plane
//            // and also the top of the cloth which we want to be static.
//            connections[n] = ivec4( -1 );
//            
//            // check the edge cases and initialize the connections to be
//            // basically, above, below, left, and right of this point
//            if ( j != ( POINTS_Y - 1 ) ) {
//                if ( i != 0 ) connections[n][0] = n - 1;					// left
//                if ( j != 0 ) connections[n][1] = n - POINTS_X;				// above
//                if (i != (POINTS_X - 1)) connections[n][2] = n + 1;			// right
//                if (j != (POINTS_Y - 1)) connections[n][3] = n + POINTS_X;	// below
//            }
            n++;
        }
    }
    
    
    for ( i = 0; i < 2; i++ ) {
        mVaos[i] = gl::Vao::create();
        gl::ScopedVao scopeVao( mVaos[i] );
        {
            // buffer the positions
            mPositions[i] = gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec4), positions.data(), GL_STATIC_DRAW );
            {
                // bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
                gl::ScopedBuffer scopeBuffer( mPositions[i] );
                gl::vertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) 0 );
                gl::enableVertexAttribArray( 0 );
            }
//            cout << mPositions[i] << endl;
        }
    }
    
    // create your two BufferTextures that correspond to your position buffers.
    mPositionBufTexs[0] = gl::BufferTexture::create( mPositions[0], GL_RGBA32F );
    mPositionBufTexs[1] = gl::BufferTexture::create( mPositions[1], GL_RGBA32F );
    
    cout << positions[0] << endl;
    
    
    // Create particle buffers on GPU and copy data into the first buffer.
    // Mark as static since we only write from the CPU once.
    mParticleBuffer[mSourceIndex] = gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW );
    mParticleBuffer[mDestinationIndex] = gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), nullptr, GL_STATIC_DRAW );
    
    
    
    // Listen to mouse events so we can send data as uniforms.
    getWindow()->getSignalMouseDown().connect( [this]( MouseEvent event )
                                              {
                                                  mMouseDown = true;
                                                  mMouseForce = 500.0f;
                                                  mMousePos = vec3( event.getX(), event.getY(), 0.0f );
                                              } );
    getWindow()->getSignalMouseDrag().connect( [this]( MouseEvent event )
                                              {
                                                  mMousePos = vec3( event.getX(), event.getY(), 0.0f );
                                              } );
    getWindow()->getSignalMouseUp().connect( [this]( MouseEvent event )
                                            {
                                                mMouseForce = 0.0f;
                                                mMouseDown = false;
                                            } );

}

void myfirstclothsimApp::update()
{
    // Update particles on the GPU
    gl::ScopedGlslProg prog( mUpdateProg );
    gl::ScopedState rasterizer( GL_RASTERIZER_DISCARD, true );	// turn off fragment stage
    mUpdateProg->uniform( "uMouseForce", mMouseForce );
    mUpdateProg->uniform( "uMousePos", mMousePos );
    
    // Bind the source data (Attributes refer to specific buffers).
    gl::ScopedVao source( mAttributes[mSourceIndex] );
    // Bind destination as buffer base.
    gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex] );
    gl::beginTransformFeedback( GL_POINTS );
    
    // Draw source into destination, performing our vertex transformations.
    gl::drawArrays( GL_POINTS, 0, POINTS_TOTAL );
    
    gl::endTransformFeedback();
    
    // Swap source and destination for next loop
    std::swap( mSourceIndex, mDestinationIndex );
    
    // Update mouse force.
    if( mMouseDown ) {
        mMouseForce = 150.0f;
    }
}

void myfirstclothsimApp::draw()
{
    gl::clear( Color( 0.2f, 0.2f, 0.3f ) );
    gl::setMatrices( mCam );
//    gl::setMatricesWindowPersp( getWindowSize() );

    
    gl::ScopedGlslProg render( mRenderProg );
    gl::ScopedVao vao( mAttributes[mSourceIndex] );
    gl::context()->setDefaultShaderVars();
    gl::drawArrays( GL_POINTS, 0, POINTS_TOTAL );

    
}

CINDER_APP( myfirstclothsimApp, RendererGl(), [] ( App::Settings *settings ) {
    settings->setWindowSize( 1280, 720 );
    settings->setMultiTouchEnabled( false );
});