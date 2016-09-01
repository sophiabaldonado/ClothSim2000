//
//  ClothSimVR.cpp
//  ClothSim
//
//  Created by Sophia Baldonado on 8/31/16.
//
//
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"
#include "cinder/CameraUi.h"

#include "cinder/vr/vr.h"

#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/BufferTexture.h"
#include "cinder/gl/GlslProg.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const uint32_t POINTS_X = 50;
const uint32_t POINTS_Y = 50;
const uint32_t GRID_SIZE = 4;

const uint32_t POINTS_TOTAL = (POINTS_X * POINTS_Y);
const uint32_t CONNECTIONS_TOTAL = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;

const uint32_t POSITION_INDEX		= 0;
const uint32_t PREV_POSITION_INDEX	= 1;
const uint32_t CONNECTION_INDEX		= 2;

class ClothSimVR : public App {
public:
    ClothSimVR();
	void update(const ci::vr::Controller *controller);
	void draw(CameraPersp &cam);

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

ClothSimVR::ClothSimVR()
: mIterationsPerFrame( 36 ), mIterationIndex( 0 ),
mDrawPoints( true ), mDrawLines( false ),
mCurrentCamRotation( -1.0f ), mUpdate( true ),
mDrawMesh( true ), mCam( getWindowWidth(),
                        getWindowHeight(), 20.0f, 0.01f, 1000.0f )
{
    mCamUi = CameraUi( &mCam );
    vec3 eye = vec3( sin( mCurrentCamRotation ) * 140.0f, 40,
                    cos( mCurrentCamRotation ) * 140.0f );
    vec3 target = vec3( 0.0f );
    mCam.lookAt( eye, target );
    
    setupGlsl();
    setupBuffers();
    setupParams();
}

void ClothSimVR::setupBuffers()
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
            positions[n] = vec4((fi - 0.5f) * (float)GRID_SIZE,
                                (fj - 0.5f) * (float)GRID_SIZE,
                                2.0f,//0.6f * sinf(fi) * cosf(fj),
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
    
    // create indices to draw tris between cloth points
    int num_cells = (POINTS_X - 1) * (POINTS_Y - 1);
    int num_tris = num_cells * 2;
    int num_indices = num_tris * 3;
    
    mMeshIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(int), nullptr, GL_STATIC_DRAW );
    
    auto pIndices = (int *) mMeshIndices->mapReplace();
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
    mMeshIndices->unmap();
    
    // create the indices to draw links between the cloth points
    int lines = (POINTS_X - 1) * POINTS_Y + (POINTS_Y - 1) * POINTS_X;
    mLineIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), nullptr, GL_STATIC_DRAW );
    
    auto e = (int *) mLineIndices->mapReplace();
    for (j = 0; j < POINTS_Y; j++) {
        for (i = 0; i < POINTS_X - 1; i++) {
            *e++ = i + j * POINTS_X;
            *e++ = 1 + i + j * POINTS_X;
        }
    }
    
    for (i = 0; i < POINTS_X; i++) {
        for (j = 0; j < POINTS_Y - 1; j++) {
            *e++ = i + j * POINTS_X;
            *e++ = POINTS_X + i + j * POINTS_X;
        }
    }
    mLineIndices->unmap();
}

void ClothSimVR::setupGlsl()
{
    // These are the names of our out going vertices. GlslProg needs to
    // know which attributes should be captured by Transform FeedBack.
    std::vector<std::string> feedbackVaryings({
        "tf_position_mass",
        "tf_prev_position_mass"
    });
    
    gl::GlslProg::Format updateFormat;
    updateFormat.vertex( loadResource( "update.vert" ) )
				// Because we have separate buffers with which
				// to capture attributes, we're using GL_SEPERATE_ATTRIBS
				.feedbackFormat( GL_SEPARATE_ATTRIBS )
				// We also send the names of the attributes to capture
				.feedbackVaryings( feedbackVaryings );
    
    mUpdateGlsl = gl::GlslProg::create( updateFormat );
    // Set this, otherwise it will be set to vec3( 0, 0, 0 ),
    // which is in the center of the cloth
    mUpdateGlsl->uniform("rayPosition", vec3(10, 10, 0)); //mCam.getEyePoint());
    
    gl::GlslProg::Format renderFormat;
    renderFormat.vertex( loadResource( "render.vert" ) )
				.fragment( loadResource( "render.frag" ) );
    
    mRenderGlsl = gl::GlslProg::create( renderFormat );
}

void ClothSimVR::update(const ci::vr::Controller *controller)
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

		if (controller)
		{
			vec3 ray_pos;
			bool trigger = (controller->getTrigger()->getValue() > 0.2f);

			ray_pos = controller->getInputRay().getOrigin();
			mUpdateGlsl->uniform("trigger", trigger);
			mUpdateGlsl->uniform("rayPosition", ray_pos);
		}

		// Begin Transform feedback with the correct primitive,
		// In this case, we want GL_POINTS, because each vertex
		// exists by itself
		gl::beginTransformFeedback(GL_POINTS);
		// Now we issue our draw command which puts all of the
		// setup in motion and processes all the vertices
		gl::drawArrays(GL_POINTS, 0, POINTS_TOTAL);
		// After that we issue an endTransformFeedback command
		// to tell OpenGL that we're finished capturing vertices
		gl::endTransformFeedback();
	}
}

void ClothSimVR::draw(CameraPersp &cam)
{
    // gl::clear( Color( 0, 0, 0 ) );

    // Notice that this vao holds the buffers we've just
    // written to with Transform Feedback. It will show
    // the most recent positions
    gl::ScopedVao scopeVao( mVaos[mIterationIndex & 1] );
    gl::ScopedGlslProg scopeGlsl( mRenderGlsl );
	//gl::setMatrices(cam);
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
        gl::ScopedBuffer scopeBuffer( mMeshIndices );
        gl::drawElements( GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0 );
    }
    if ( mDrawLines ) {
        gl::ScopedBuffer scopeBuffer( mLineIndices );
        gl::drawElements( GL_LINES, CONNECTIONS_TOTAL * 2, GL_UNSIGNED_INT, nullptr );
    }

 //   mParams->draw();
}

void ClothSimVR::mouseDown( MouseEvent event )
{
#if 0
    if( event.isRightDown() ) {
        // I've disabled camera movement to use rightclick for "pull" interaction
        // mCamUi.mouseDrag( event.getPos(), true, false, false );
        mUpdateGlsl->uniform( "trigger", true );
        updateRayPosition( event.getPos(), true );
    } else { // left click for "push" interaction
        mUpdateGlsl->uniform( "trigger", false );
        updateRayPosition( event.getPos(), true );
    }
#endif
}

void ClothSimVR::mouseDrag( MouseEvent event )
{
#if 0
	if (event.isRightDown())
        updateRayPosition( event.getPos(), true );
	else
		updateRayPosition(event.getPos(), true);
#endif
}

void ClothSimVR::mouseUp( MouseEvent event )
{
#if 0
	updateRayPosition(event.getPos(), false);
#endif
}

void ClothSimVR::updateRayPosition( const ci::ivec2 &mousePos, bool useDistance )
{
#if 0
	auto ray = mCam.generateRay(mousePos, getWindowSize());
	auto dist = ci::distance(mCam.getEyePoint(), vec3());
	auto rayPosition = ray.calcPosition(useDistance ? dist : 0);
	mUpdateGlsl->uniform("rayPosition", rayPosition);
#endif
}

void ClothSimVR::setupParams()
{
	static vec3 gravity = vec3(0.0, -0.08, 0.0);
	static float restLength = 0.1;
	static float dampingConst = 2.8;
	static float springConstant = 7.1;

	mUpdateGlsl->uniform("gravity", gravity);
	mUpdateGlsl->uniform("rest_length", restLength);
	mUpdateGlsl->uniform("damping", dampingConst);
	mUpdateGlsl->uniform("spring", springConstant);

	dampingConst = 5.0;
	mUpdateGlsl->uniform("damping", dampingConst);

	gravity = vec3(0.0, -0.01, 0.0);
	mUpdateGlsl->uniform("gravity", gravity);

	restLength = 0.05;
	mUpdateGlsl->uniform("rest_length", restLength);

	dampingConst = 4.8;
	mUpdateGlsl->uniform("damping", dampingConst);

	springConstant = 7.1;
	mUpdateGlsl->uniform("spring", springConstant);
}

class ControllerBasicApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

private:
	ci::vr::Context*			mVrContext = nullptr;
	ci::vr::Hmd*				mHmd = nullptr;
	CameraPersp					mCamera;

	bool						mRecalcOrigin = false;

	gl::BatchRef				mGridBox;
	gl::BatchRef				mGridLines;

	const ci::vr::Controller	*mOculusXbox = nullptr;
	const ci::vr::Controller	*mOculusRemote = nullptr;

	const ci::vr::Controller	*mViveLeft = nullptr;
	const ci::vr::Controller	*mViveRight = nullptr;

	ClothSimVR				*mpClothSim = nullptr;

	void	onControllerConnect( const ci::vr::Controller *controller );
	void	onControllerDisconnect( const ci::vr::Controller *controller );
	void	onButtonDown( const ci::vr::Controller::Button *button );
	void	onButtonUp( const ci::vr::Controller::Button *button );
	void	onTrigger( const ci::vr::Controller::Trigger *trigger );
	void	onAxis( const ci::vr::Controller::Axis *axis );

	void	initGrid( const gl::GlslProgRef &shader );

	void	drawOculusXbox( const ci::vr::Controller *controller );
	void	drawOcculusRemote( const ci::vr::Controller *controller );
	void	drawViveController( const ci::vr::Controller *controller );
	void	drawScene();
};

void ControllerBasicApp::initGrid( const gl::GlslProgRef &shader )
{
	vec3 size = vec3( 100, 20, 100 );

	// Box
	{
		mGridBox = gl::Batch::create( geom::Cube().size( 1.01f*size ) >> geom::Translate( 0.0f, 0.0f, 0.0f ), shader );
	}

	// Lines
	{
		Rectf bounds = Rectf( -size.x/2.0f, -size.z/2.0f, size.x/2.0f, size.z/2.0f );
		int nx = static_cast<int>( size.x );
		int nz = static_cast<int>( size.z );
		float dx = bounds.getWidth() / static_cast<float>( nx );
		float dz = bounds.getHeight() / static_cast<float>( nz );
		float y = 0.01f;

		std::vector<vec3> vertices;

		for( int i = 0; i <= nx; ++i ) {
			float x = bounds.x1 + i * dx;
			vec3 p0 = vec3( x, y, bounds.y1 );
			vec3 p1 = vec3( x, y, bounds.y2 );
			vertices.push_back( p0 );
			vertices.push_back( p1 );
		}

		for( int i = 0; i <= nz; ++i ) {
			float z = bounds.y1 + i * dz;
			vec3 p0 = vec3( bounds.x1, y, z );
			vec3 p1 = vec3( bounds.x2, y, z );
			vertices.push_back( p0 );
			vertices.push_back( p1 );
		}

		gl::VboRef vbo = gl::Vbo::create( GL_ARRAY_BUFFER, vertices );
		geom::BufferLayout layout = geom::BufferLayout( { geom::AttribInfo( geom::Attrib::POSITION, 3, sizeof( vec3 ), 0 ) } );
		gl::VboMeshRef vboMesh = gl::VboMesh::create( static_cast<uint32_t>( vertices.size() ), GL_LINES, { std::make_pair( layout, vbo ) } );
		mGridLines = gl::Batch::create( vboMesh, shader );
	}
}

void ControllerBasicApp::setup()
{
	mCamera.lookAt( ci::vec3( 0, 0, 3 ), ci::vec3( 0, 0, 0 ) );

	gl::disableAlphaBlending();
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::color( Color::white() );

	try {
		ci::vr::initialize();
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "VR failed: " << e.what() );
	}

	try {
		mVrContext = ci::vr::beginSession(
			ci::vr::SessionOptions()
				.setTrackingOrigin( ci::vr::TRACKING_ORIGIN_SEATED )
				.setOriginOffset( vec3( 0, -1, -3 ) )
				.setControllersScanInterval( 0.25f )
				.setControllerConnected( std::bind( &ControllerBasicApp::onControllerConnect, this, std::placeholders::_1 ) )
				.setControllerDisconnected( std::bind( &ControllerBasicApp::onControllerDisconnect, this, std::placeholders::_1 ) )
		);
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Session failed: " << e.what() );
	}

	try {
		mHmd = mVrContext->getHmd();
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Hmd failed: " << e.what() );
	}

	mVrContext->getSignalControllerButtonDown().connect( std::bind( &ControllerBasicApp::onButtonDown, this, std::placeholders::_1 ) );
	mVrContext->getSignalControllerButtonUp().connect( std::bind( &ControllerBasicApp::onButtonUp, this, std::placeholders::_1 ) );
	mVrContext->getSignalControllerTrigger().connect( std::bind( &ControllerBasicApp::onTrigger, this, std::placeholders::_1 ) );
	mVrContext->getSignalControllerAxis().connect( std::bind( &ControllerBasicApp::onAxis, this, std::placeholders::_1 ) );

	auto shader = gl::getStockShader( gl::ShaderDef().color() );
	initGrid( shader );

	mpClothSim = new ClothSimVR();
}

void ControllerBasicApp::onControllerConnect( const ci::vr::Controller* controller )
{
	if( ! controller ) {
		return;
	}

	bool connected = false;
	if( ci::vr::API_OCULUS == controller->getApi() ) {
		switch( controller->getType() ) {
			case ci::vr::Controller::TYPE_REMOTE: {
				mOculusRemote = controller;
				connected = true;
			}
			break;

			case ci::vr::Controller::TYPE_XBOX: {
				mOculusXbox = controller;
				connected = true;
			}
			break;
		}
	}
	else if( ci::vr::API_OPENVR == controller->getApi() ) {
		switch( controller->getType() ) {
			case ci::vr::Controller::TYPE_LEFT: {
				mViveLeft = controller;
				connected = true;
			}
			break;

			case ci::vr::Controller::TYPE_RIGHT: {
				mViveRight = controller;
				connected = true;
			}
			break;
		}
	}

	if( connected ) {
		CI_LOG_I( "Controller connected: " << controller->getName() );
	}
}

void ControllerBasicApp::onControllerDisconnect( const ci::vr::Controller* controller )
{
	if( ! controller ) {
		return;
	}

	bool disconnected = false;
	if( ci::vr::API_OCULUS == controller->getApi() ) {
		switch( controller->getType() ) {
			case ci::vr::Controller::TYPE_REMOTE: {
				mOculusRemote = nullptr;
				disconnected = true;
			}
			break;

			case ci::vr::Controller::TYPE_XBOX: {
				mOculusXbox = nullptr;
				disconnected = true;
			}
			break;
		}
	}
	else if( ci::vr::API_OPENVR == controller->getApi() ) {
		switch( controller->getType() ) {
			case ci::vr::Controller::TYPE_LEFT: {
				mViveLeft = nullptr;
				disconnected = true;
			}
			break;

			case ci::vr::Controller::TYPE_RIGHT: {
				mViveRight = nullptr;
				disconnected = true;
			}
			break;
		}
	}

	if( disconnected ) {
		CI_LOG_I( "Controller disconnected: " << controller->getName() );
	}
}

void ControllerBasicApp::onButtonDown( const ci::vr::Controller::Button *button )
{
	CI_LOG_I( button->getInfo() );
}

void ControllerBasicApp::onButtonUp( const ci::vr::Controller::Button *button )
{
	CI_LOG_I( button->getInfo() );
}

void ControllerBasicApp::onTrigger( const ci::vr::Controller::Trigger *trigger )
{
	CI_LOG_I( trigger->getInfo() );
}

void ControllerBasicApp::onAxis( const ci::vr::Controller::Axis *axis )
{
	CI_LOG_I( axis->getInfo() );
}

void ControllerBasicApp::update()
{
	// Vive sometimes returns the wrong pose data initially so reinitialize the origin matrix after the first 60 frames.
	if( ( ci::vr::API_OPENVR == mVrContext->getApi() ) && ( ! mRecalcOrigin ) && ( mHmd->getElapsedFrames() > 60 ) ) {
		mHmd->calculateOriginMatrix();
		mRecalcOrigin = true;
	}

	mpClothSim->update(mViveLeft);
}

void drawButton( const vec2& pos, float radius, const ci::vr::Controller::Button *button, const Color& activeColor )
{
	if( button->isDown() ) {
		gl::color( activeColor );
		gl::drawSolidCircle( pos, radius, 16 );
	}
	else {
		gl::color( 0.3f, 0.3f, 0.3f );
		gl::drawStrokedCircle( pos, radius, 16 );
	}
}

void drawTrigger( const vec2& pos, float radius, const ci::vr::Controller::Trigger *trigger, const Color& activeColor )
{
	float s = mix( 0.25f, 1.0f, trigger->getValue() );

	gl::color( s*activeColor );
	gl::drawSolidCircle( pos, s*radius, 16 );
}

void drawAxis( const vec2& pos, float radius, const ci::vr::Controller::Axis *axis, const Color& activeColor )
{
	ci::vec2 dv = axis->getValue();
	float minorRadius = 0.2f*radius;

	gl::color( activeColor );
	gl::drawSolidCircle( pos + dv*radius, minorRadius, 16 );
}

void ControllerBasicApp::drawOculusXbox( const ci::vr::Controller *controller )
{
	if( ! controller ) {
		return;
	}

	// XYAB
	drawButton( vec2( 0.6f, 0.7f ) + vec2( -0.17f, 0 ), 0.06f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_X ), Color( 0.0f, 0.0f, 0.9f ) );
	drawButton( vec2( 0.6f, 0.7f ) + vec2(  0.17f, 0 ), 0.06f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_B ), Color( 0.9f, 0.0f, 0.0f ) );
	drawButton( vec2( 0.6f, 0.7f ) + vec2( 0,  0.17f ), 0.06f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_Y ), Color( 0.9f, 0.9f, 0.0f ) );
	drawButton( vec2( 0.6f, 0.7f ) + vec2( 0, -0.17f ), 0.06f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_A ), Color( 0.0f, 0.9f, 0.0f ) );

	// DPad
	drawButton( vec2( -0.4f, 0.2f ) + vec2( -0.16f, 0 ), 0.04f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_LEFT ),  Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( -0.4f, 0.2f ) + vec2(  0.16f, 0 ), 0.04f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_RIGHT ), Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( -0.4f, 0.2f ) + vec2( 0,  0.16f ), 0.04f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_UP ),    Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( -0.4f, 0.2f ) + vec2( 0, -0.16f ), 0.04f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_DOWN ),  Color( 0.9f, 0.9f, 0 ) );

	// Left/right thumbstick buttons
	drawButton( vec2( -0.90f, 0.7f ), 0.21f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_LTHUMBSTICK ), Color( 0.3f, 0.3f, 0.4f ) );
	drawButton( vec2(  0.40f, 0.2f ), 0.21f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_RTHUMBSTICK ), Color( 0.3f, 0.3f, 0.4f ) );

	// Left/right shoulder buttons
	drawButton( vec2( -0.90f, 1.15f ), 0.1f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_LSHOULDER ), Color( 0.9f, 0.4f, 0.0f ) );
	drawButton( vec2(  0.60f, 1.15f ), 0.1f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_RSHOULDER ), Color( 0.9f, 0.4f, 0.0f ) );

	// Enter/Back
	drawButton( vec2(  0.15f, 0.7f ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_ENTER ), Color( 0, 0.8f, 0.8f ) );
	drawButton( vec2( -0.19f, 0.7f ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_XBOX_BACK ), Color( 0, 0.8f, 0.8f ) );

	// Left/right triggers
	drawTrigger( vec2( -0.90f, 1.45f ), 0.12f, controller->getTrigger( ci::vr::Controller::HAND_LEFT ), Color( 0.9f, 0.1f, 0.9f ) );
	drawTrigger( vec2(  0.60f, 1.45f ), 0.12f, controller->getTrigger( ci::vr::Controller::HAND_RIGHT ), Color( 0.9f, 0.1f, 0.9f ) );

	// Left/right thumbsticks
	{
		gl::ScopedModelMatrix scopedModel;
		gl::translate( 0, 0, 0.01f );
		drawAxis( vec2( -0.90f, 0.7f ), 0.21f, controller->getAxis( ci::vr::Controller::HAND_LEFT ), Color( 0.9f, 0.1f, 0.0f ) );
		drawAxis( vec2(  0.40f, 0.2f ), 0.21f, controller->getAxis( ci::vr::Controller::HAND_RIGHT ), Color( 0.9f, 0.1f, 0.0f ) );
	}
}

void ControllerBasicApp::drawOcculusRemote( const ci::vr::Controller *controller )
{
	if( ! controller ) {
		return;
	}

	drawButton( vec2( 0, 0.8f ), 0.10f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_REMOTE_ENTER ), Color( 0, 0.9f, 0 ) );
	drawButton( vec2( 0, 0.3f ), 0.08f, controller->getButton( ci::vr::Controller::BUTTON_OCULUS_REMOTE_BACK ), Color( 0.9f, 0.3f, 0 ) );
	drawButton( vec2( 0, 0.8f ) + vec2( -0.18f, 0 ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_LEFT ),  Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( 0, 0.8f ) + vec2(  0.18f, 0 ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_RIGHT ), Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( 0, 0.8f ) + vec2( 0,  0.18f ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_UP ),    Color( 0.9f, 0.9f, 0 ) );
	drawButton( vec2( 0, 0.8f ) + vec2( 0, -0.18f ), 0.03f, controller->getButton( ci::vr::Controller::BUTTON_DPAD_DOWN ),  Color( 0.9f, 0.9f, 0 ) );
}

void ControllerBasicApp::drawViveController( const ci::vr::Controller *controller )
{
	if( ! controller ) {
		return;
	}

	// Application menu button
	drawButton( vec2( 0, 1.5f ), 0.10f, controller->getButton( ci::vr::Controller::BUTTON_VIVE_APPLICATION_MENU ), Color( 0, 0.9f, 0 ) );

	// Trigger
	drawTrigger( vec2( 0, 1.25f ), 0.12f, controller->getTrigger(), Color( 0.9f, 0.1f, 0.9f ) );

	// Trigger button
	drawButton( vec2( 0, 1.0f ), 0.10f, controller->getButton( ci::vr::Controller::BUTTON_VIVE_TRIGGER), Color( 0.9f, 0.3f, 0  ) );

	// Touchpad button
	drawButton( vec2( 0, 0.65f ), 0.21f, controller->getButton( ci::vr::Controller::BUTTON_VIVE_TOUCHPAD ), Color( 0.3f, 0.3f, 0.4f ) );

	// Grip button
	drawButton( vec2( 0, 0.25f ), 0.10f, controller->getButton( ci::vr::Controller::BUTTON_VIVE_GRIP ),  Color( 0.9f, 0.9f, 0 ) );

	// Touchpad
	{
		gl::ScopedModelMatrix scopedModel;
		gl::translate( 0, 0, 0.01f );
		drawAxis( vec2( 0, 0.65f ), 0.21f, controller->getAxis(), Color( 0.9f, 0.1f, 0.0f ) );
	}
}

void ControllerBasicApp::drawScene()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();

	mpClothSim->draw(mCamera);

	// Draw ground grid
	{
		gl::ScopedLineWidth scopedLineWidth( 1.0f );

		gl::color( 0.0f, 0.0f, 0.0f );
		mGridBox->draw();

		gl::color( 0.2f, 0.2f, 0.2f );
		mGridLines->draw();
	}

	gl::lineWidth( 3.0f );

	if( ci::vr::API_OCULUS == mVrContext->getApi() ) {
		{
			gl::ScopedModelMatrix scopedModel;
			gl::translate( -1.25f, 0, 0 );
			drawOculusXbox( mOculusXbox );
		}

		{
			gl::ScopedModelMatrix scopedModel;
			gl::translate( 0.5f, 0, 0 );
			drawOcculusRemote( mOculusRemote );
		}
	}
	else if( ci::vr::API_OPENVR == mVrContext->getApi() ) {
		{
			gl::ScopedModelMatrix scopedModel;
			gl::translate( -1.0f, 0, 0 );
			drawViveController( mViveLeft );
		}

		{
			gl::ScopedModelMatrix scopedModel;
			gl::translate( 1.0f, 0, 0 );
			drawViveController( mViveRight );
		}

		// Draw controller input rays
		{
			float s = 1000.0f;

			if( mViveLeft ) {
				const auto& ir = mViveLeft->getInputRay();
				vec3 dir = ir.getDirection();
				vec3 orig = ir.getOrigin() + ( 0.055f * dir );
				gl::color( 0.45f, 0.0f, 0.5f );
				gl::drawLine( orig, orig + ( s * dir ) );
			}

			if( mViveRight ) {
				const auto& ir = mViveRight->getInputRay();
				vec3 dir = ir.getDirection();
				vec3 orig = ir.getOrigin() + ( 0.055f * dir );
				gl::color(0.45f, 0.0f, 0.5f);
				gl::drawLine( orig, orig + ( s * dir ) );
			}
		}
	}

	// Draw coordinate frame
	{
		gl::drawCoordinateFrame( 1.0f );
	}

	// Draw HMD input ray (cyan sphere)
	{
		const auto& ir = mHmd->getInputRay();
		vec3 dir = ir.getDirection();
		vec3 P = ir.getOrigin() + ( 5.0f * dir );
		gl::color( 0.01f, 0.7f, 0.9f );
		gl::drawSphere( P, 0.04f );
	}
}

void ControllerBasicApp::draw()
{
	gl::clear( Color( 0.02f, 0.02f, 0.1f ) );
	if( mHmd )
	{
		mHmd->bind();

		for( auto eye : mHmd->getEyes() )
		{
			mHmd->enableEye( eye );
			drawScene();
			mHmd->drawControllers( eye );
		}
		mHmd->unbind();

		// Draw mirrored
		gl::viewport( getWindowSize() );
		gl::setMatricesWindow( getWindowSize() );
		mHmd->drawMirrored( getWindowBounds() );
	}
	else
	{
		gl::viewport( getWindowSize() );
		gl::setMatrices( mCamera );
		drawScene();
	}
}

void prepareSettings( App::Settings *settings )
{
	settings->setTitle( "Cinder VR ControllerBasic" );
	settings->setWindowSize( 1920/2, 1080/2 );
}

CINDER_APP( ControllerBasicApp, RendererGl( RendererGl::Options().msaa(0) ), prepareSettings )
