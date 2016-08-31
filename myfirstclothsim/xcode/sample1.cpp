#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"
#include "cinder/CameraUi.h"

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/BufferTexture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/TriMesh.h"


using namespace ci;
using namespace ci::app;
using namespace std;

const uint32_t POINTS_X				= 40;
const uint32_t POINTS_Y				= 40;
const uint32_t POINTS_TOTAL			= (POINTS_X * POINTS_Y);
const uint32_t CONNECTIONS_TOTAL	= (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;

const uint32_t POSITION_INDEX		= 0;
const uint32_t PREV_POSITION_INDEX	= 1;
const uint32_t CONNECTION_INDEX		= 2;

class ClothSimulationApp : public App {
public:
    ClothSimulationApp();
    void update() override;
    void draw() override;
    
    void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void updateRayPosition( const ci::ivec2 &mousePos, bool useDistance );
    
    void setupBuffers();
    void setupGlsl();
    void setupParams();
    
    std::array<gl::VaoRef, 2>			mVaos;
    std::array<gl::VboRef, 2>			mPositions, mPrevPositions, mConnections;
    std::array<gl::BufferTextureRef, 2>	mPositionBufTexs;
    gl::VboRef							mLineIndices;
    gl::VboRef							mMeshIndices;
    gl::GlslProgRef						mUpdateGlsl, mRenderGlsl;
    
    CameraPersp							mCam;
    CameraUi							mCamUi;
    float								mCurrentCamRotation;
    uint32_t							mIterationsPerFrame, mIterationIndex;
    bool								mDrawPoints, mDrawLines, mDrawMesh, mUpdate;
    
    ci::params::InterfaceGlRef			mParams;
};

ClothSimulationApp::ClothSimulationApp()
: mIterationsPerFrame( 36 ), mIterationIndex( 0 ),
mDrawPoints( true ), mDrawLines( true ),
mCurrentCamRotation( 0.0f ), mUpdate( true ),
mDrawMesh( true ), mCam( getWindowWidth(),
getWindowHeight(), 20.0f, 0.01f, 1000.0f )
{
    mCamUi = CameraUi( &mCam );
    vec3 eye = vec3( sin( mCurrentCamRotation ) * 140.0f, 0,
                    cos( mCurrentCamRotation ) * 140.0f );
    vec3 target = vec3( 0.0f );
    mCam.lookAt( eye, target );
    
    setupGlsl();
    setupBuffers();
    setupParams();
}

void ClothSimulationApp::setupBuffers()
{
    
    int i, j;
    
    std::array<vec4, POINTS_TOTAL> positions;
    std::array<vec4, POINTS_TOTAL> prevPositions;
    std::array<ivec4, POINTS_TOTAL> connections;
    
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
            // zero out prevPositions
            prevPositions[n] = vec4( 0.0f );
            // to create connections we'll use an integer buffer.
            // The value -1 refers to the fact that there's no
            // connection. This helps for the edge cases of the plane
            // and also the top of the cloth which we want to be static.
            connections[n] = ivec4( -1 );
            
            // check the edge cases and initialize the connections to be
            // basically, above, below, left, and right of this point
            if ( j != ( POINTS_Y - 1 ) ) {
                if ( i != 0 ) connections[n][0] = n - 1;					// left
                if ( j != 0 ) connections[n][1] = n - POINTS_X;				// above
                if (i != (POINTS_X - 1)) connections[n][2] = n + 1;			// right
                if (j != (POINTS_Y - 1)) connections[n][3] = n + POINTS_X;	// below
            }
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
                gl::ScopedBuffer sccopeBuffer( mPositions[i] );
                gl::vertexAttribPointer( POSITION_INDEX, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) 0 );
                gl::enableVertexAttribArray( POSITION_INDEX );
            }
            // buffer the prev positions
            mPrevPositions[i] = gl::Vbo::create( GL_ARRAY_BUFFER, prevPositions.size() * sizeof(vec4), prevPositions.data(), GL_STATIC_DRAW );
            {
                // bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
                gl::ScopedBuffer scopeBuffer( mPrevPositions[i] );
                gl::vertexAttribPointer( PREV_POSITION_INDEX, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) 0 );
                gl::enableVertexAttribArray( PREV_POSITION_INDEX );
            }
            // buffer the connections
            mConnections[i] = gl::Vbo::create( GL_ARRAY_BUFFER, connections.size() * sizeof(ivec4), connections.data(), GL_STATIC_DRAW );
            {
                // bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
                gl::ScopedBuffer scopeBuffer( mConnections[i] );
                gl::vertexAttribIPointer( CONNECTION_INDEX, 4, GL_INT, 0, (const GLvoid*) 0 );
                gl::enableVertexAttribArray( CONNECTION_INDEX );
            }
        }
    }
    // create your two BufferTextures that correspond to your position buffers.
    mPositionBufTexs[0] = gl::BufferTexture::create( mPositions[0], GL_RGBA32F );
    mPositionBufTexs[1] = gl::BufferTexture::create( mPositions[1], GL_RGBA32F );
    
    int lines = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;
    // create the indices to draw links between the cloth points
    mLineIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), nullptr, GL_STATIC_DRAW );

    
    int num_cells = (POINTS_X - 1) * (POINTS_Y - 1);
    int num_tris = num_cells * 2;
    int num_indices = num_tris * 3;

    mLineIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(int), nullptr, GL_STATIC_DRAW );
    
    auto pIndices = (int *) mLineIndices->mapReplace();
    for (j = 0; j < POINTS_Y; j++) {
        for (i = 0; i < POINTS_X - 1; i++) {
#define GET_VERTEX(x, y) ((x) + ((y) * POINTS_X))
            *pIndices++ = GET_VERTEX(i, j);
            *pIndices++ = GET_VERTEX(i + 1, j);
            *pIndices++ = GET_VERTEX(i + 1, j + 1);
            
            *pIndices++ = GET_VERTEX(i, j);
            *pIndices++ = GET_VERTEX(i + 1, j + 1);
            *pIndices++ = GET_VERTEX(i, j + 1);
#undef GET_VERTEX
        }
    }
    mLineIndices->unmap();

    
    
    
//    auto e = (int *) mLineIndices->mapReplace();
//    for (j = 0; j < POINTS_Y; j++) {
//        for (i = 0; i < POINTS_X - 1; i++) {
//            *e++ = i + j * POINTS_X;
//            *e++ = 1 + i + j * POINTS_X;
//        }
//    }
//    
//    for (i = 0; i < POINTS_X; i++) {
//        for (j = 0; j < POINTS_Y - 1; j++) {
//            *e++ = i + j * POINTS_X;
//            *e++ = POINTS_X + i + j * POINTS_X;
//        }
//    }
//    mLineIndices->unmap();
    
//    //   mMesh = gl::VboMesh::create( POINTS_TOTAL, GL_TRIANGLES, { gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::POSITION, 3 ) }, mIndices.size(), GL_UNSIGNED_SHORT );
//    
//    
//    int m = 0;
//    int colSteps = POINTS_Y * 2;
//    int rowSteps = POINTS_X - 1;
//    vector<int> indices;
//    for( int r = 0; r < rowSteps; r++ ) {
//        for( int c = 0; c < colSteps; c++ ) {
//            int t = c + r * colSteps;
//            
//            if( c == colSteps -1 ) {
//                indices.push_back( m );
//            } else {
//                indices.push_back( m );
//                
//                if( t % 2 == 0 ) {
//                    m += POINTS_Y;
//                } else {
//                    ( r % 2 == 0 ) ? m -= ( POINTS_Y - 1 ): n -= ( POINTS_Y + 1 );
//                }
//                
//            }
//        }
//    }
}

void ClothSimulationApp::setupGlsl()
{
    // These are the names of our out going vertices. GlslProg needs to
    // know which attributes should be captured by Transform FeedBack.
    std::vector<std::string> feedbackVaryings({
        "tf_position_mass",
        "tf_prev_position_mass"
    });
    
    gl::GlslProg::Format updateFormat;
    updateFormat.vertex( loadResource( "update1.vert" ) )
				// Because we have separate buffers with which
				// to capture attributes, we're using GL_SEPERATE_ATTRIBS
				.feedbackFormat( GL_SEPARATE_ATTRIBS )
				// We also send the names of the attributes to capture
				.feedbackVaryings( feedbackVaryings );
    
    mUpdateGlsl = gl::GlslProg::create( updateFormat );
    // Set this, otherwise it will be set to vec3( 0, 0, 0 ),
    // which is in the center of the cloth
    mUpdateGlsl->uniform( "rayPosition", mCam.getEyePoint() );
    
    gl::GlslProg::Format renderFormat;
    renderFormat.vertex( loadResource( "render.vert" ) )
				.fragment( loadResource( "render.frag" ) );
    
    mRenderGlsl = gl::GlslProg::create( renderFormat );
}

void ClothSimulationApp::update()
{
    if( ! mUpdate ) return;
    
    gl::ScopedGlslProg	scopeGlsl( mUpdateGlsl );
    gl::ScopedState		scopeState( GL_RASTERIZER_DISCARD, true );
    
    for( auto i = mIterationsPerFrame; i != 0; --i ) {
        // Bind the vao that has the original vbo attached,
        // these buffers will be used to read from.
        gl::ScopedVao scopedVao( mVaos[mIterationIndex & 1] );
        // Bind the BufferTexture, which contains the positions
        // of the first vbo. We'll cycle through the neighbors
        // using the connection buffer so that we can derive our
        // next position and velocity to write to Transform Feedback
        gl::ScopedTextureBind scopeTex( mPositionBufTexs[mIterationIndex & 1]->getTarget(), mPositionBufTexs[mIterationIndex & 1]->getId() );
        
        // We iterate our index so that we'll be using the
        // opposing buffers to capture the data
        mIterationIndex++;
        
        // Now bind our opposing buffers to the correct index
        // so that we can capture the values coming from the shader
        gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, POSITION_INDEX, mPositions[mIterationIndex & 1] );
        gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, PREV_POSITION_INDEX, mPrevPositions[mIterationIndex & 1] );
        
        // Begin Transform feedback with the correct primitive,
        // In this case, we want GL_POINTS, because each vertex
        // exists by itself
        gl::beginTransformFeedback( GL_POINTS );
        // Now we issue our draw command which puts all of the
        // setup in motion and processes all the vertices
        gl::drawArrays( GL_POINTS, 0, POINTS_TOTAL );
        // After that we issue an endTransformFeedback command
        // to tell OpenGL that we're finished capturing vertices
        gl::endTransformFeedback();
    }
    
    

//    ci::TriMesh *mesh = mMesh.get();
//    vec3 mProf = vec3(2, 2, 2);
//    mat4 mFrames = mat4(0.0);
//    mat4 mFrames1 = mat4(1.0);
//    
//    mesh->clear();
////    std::array<vec3, POINTS_TOTAL> mPs;
//    
//    for( int i = 0; i < mPositions.size() - 1; ++i ) {
//        mat4 mat0 = mFrames;
//        mat4 mat1 = mFrames1;
//        
//        float r0 = sin( (float)(i + 0)/(float)(mPositions.size() - 1)*3.141592f );
//        float r1 = sin( (float)(i + 1)/(float)(mPositions.size() - 1)*3.141592f );
//        float rs0 = (1 - 1)*r0 + 1;
//        float rs1 = (1 - 1)*r1 + 1;
//        
//        for( int ci = 0; ci < mProf.size(); ++ci ) {
//            int idx0 = ci;
//            int idx1 = (ci == (mProf.size() - 1)) ? 0 : ci + 1;
//            vec3 P0 = vec3( mat0 * vec4(mProf[idx0]*rs0, 1) );
//            vec3 P1 = vec3( mat0 * vec4(mProf[idx1]*rs0, 1) );
//            vec3 P2 = vec3( mat1 * vec4(mProf[idx1]*rs1, 1) );
//            vec3 P3 = vec3( mat1 * vec4(mProf[idx0]*rs1, 1) );
//            addQuadToMesh( *mesh, P0, P3, P2, P1 );
//        }
//    }
}

void ClothSimulationApp::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    
    // Notice that this vao holds the buffers we've just
    // written to with Transform Feedback. It will show
    // the most recent positions
    gl::ScopedVao scopeVao( mVaos[mIterationIndex & 1] );
    gl::ScopedGlslProg scopeGlsl( mRenderGlsl );
    gl::setMatrices( mCam );
    gl::setDefaultShaderVars();
    
    int num_cells = (POINTS_X - 1) * (POINTS_Y - 1);
    int num_tris = num_cells * 2;
    int num_indices = num_tris * 3;
    
    if( mDrawPoints ) {
        gl::pointSize( 4.0f);
        gl::drawArrays( GL_POINTS, 0, POINTS_TOTAL );
        gl::drawElements( GL_TRIANGLE_STRIP, POINTS_X * (POINTS_Y-1) * 2 + POINTS_Y-1, GL_UNSIGNED_INT, 0 );
    }
    if( mDrawMesh ) {
        gl::ScopedBuffer scopeBuffer( mLineIndices );
        gl::drawElements( GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0 );
    } else if ( mDrawLines ) {
        gl::ScopedBuffer scopeBuffer( mLineIndices );
        gl::drawElements( GL_LINES, CONNECTIONS_TOTAL * 2, GL_UNSIGNED_INT, nullptr );
    }
    
    mParams->draw();
}

void ClothSimulationApp::mouseDown( MouseEvent event )
{
    if( event.isRightDown() )
//        mCamUi.mouseDrag( event.getPos(), true, false, false );
        mUpdateGlsl->uniform( "trigger", true );
//        updateRayPosition( event.getPos(), true );
    else
        updateRayPosition( event.getPos(), true );
}

void ClothSimulationApp::mouseDrag( MouseEvent event )
{
    if( event.isRightDown() )
        mCamUi.mouseDrag( event.getPos(), true, false, false );
    else
        updateRayPosition( event.getPos(), true );
}

void ClothSimulationApp::mouseUp( MouseEvent event )
{
    updateRayPosition( event.getPos(), false );
}

void ClothSimulationApp::updateRayPosition( const ci::ivec2 &mousePos, bool useDistance )
{
    auto ray = mCam.generateRay( mousePos, getWindowSize() );
    auto dist = ci::distance( mCam.getEyePoint(), vec3() );
    auto rayPosition = ray.calcPosition( useDistance ? dist : 0 );
    mUpdateGlsl->uniform( "rayPosition", rayPosition );
}

void ClothSimulationApp::setupParams()
{
    static vec3 gravity = vec3( 0.0, -0.08, 0.0 );
    static float restLength = 1.0;
    static float dampingConst = 0.01;
    static float springConstant = 50;
    
    mParams = params::InterfaceGl::create( "Cloth Simulation", ivec2( 250, 250 ) );
    mParams->addText( "Update Params" );
    mParams->addParam( "Update", &mUpdate );
    mParams->addParam( "Updates/Frame", &mIterationsPerFrame ).min( 1 );
    mParams->addParam( "Gravity", &gravity ).updateFn(
                                                      [&](){
                                                          mUpdateGlsl->uniform( "gravity", gravity );
                                                      });
    mParams->addParam( "Rest Length", &restLength )
    .min( 0.0f ).max( 30.0f ).updateFn(
                                       [&](){
                                           mUpdateGlsl->uniform( "rest_length", restLength );
                                       });
    mParams->addParam( "Damping Constant", &dampingConst )
    .min( 1.5f ).max( 25.0f ).updateFn(
                                       [&](){
                                           mUpdateGlsl->uniform( "damping", dampingConst );
                                       });
    mParams->addParam( "Spring Constant", &springConstant )
    .min( 0.1f ).max( 17.0f ).updateFn(
                                       [&](){
                                           mUpdateGlsl->uniform( "spring", springConstant );
                                       });
    mParams->addButton( "Eject Button",
                       [&](){
                           restLength = 1.0;
                           mUpdateGlsl->uniform( "rest_length", restLength );
                           dampingConst = 0.01;
                           mUpdateGlsl->uniform( "damping", dampingConst );
                       });
    mParams->addButton( "Reset",
                       [&](){
                           gravity = vec3( 0.0, -0.08, 0.0 );
                           mUpdateGlsl->uniform( "gravity", gravity );
                           restLength = 0.88;
                           mUpdateGlsl->uniform( "rest_length", restLength );
                           dampingConst = 2.8;
                           mUpdateGlsl->uniform( "damping", dampingConst );
                           springConstant = 7.1;
                           mUpdateGlsl->uniform( "spring", springConstant );
                       });
    mParams->addSeparator();
    mParams->addText( "Render Params" );
    mParams->addParam( "Draw Lines", &mDrawLines );
    mParams->addParam( "Draw Points", &mDrawPoints );
    mParams->addParam( "Draw Mesh", &mDrawMesh );
    mParams->addText( "Right Mouse Button Rotates" );
}

CINDER_APP( ClothSimulationApp, RendererGl(),
           [&]( App::Settings *settings ) {
               settings->setWindowSize( 1280, 720 );
           })
