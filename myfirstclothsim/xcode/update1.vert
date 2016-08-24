// Update Vertex Shader
// OpenGL SuperBible Chapter 7
// Graham Sellers
#version 330 core

// This input vector contains the vertex position in xyz, and the
// mass of the vertex in w
layout (location = 0) in vec4 position_mass;	// POSITION_INDEX
// This is the current velocity of the vertex
layout (location = 1) in vec4 velocity;			// VELOCITY_INDEX
// This is our connection vector
layout (location = 2) in ivec4 connection;		// CONNECTION_INDEX

// This is a TBO that will be bound to the same buffer as the
// position_mass input attribute
uniform samplerBuffer tex_position;

uniform vec3 rayPosition;
uniform float ciElapsedSeconds;

// The outputs of the vertex shader are the same as the inputs
out vec4 tf_position_mass;
out vec4 tf_velocity;

// A uniform to hold the timestep. The application can update this.
uniform float timestep = 0.07;

// The global spring constant
uniform float spring = 7.1;

// Gravity
uniform vec3 gravity = vec3(0.0, -0.08, 0.0);

// Global damping constant
uniform float damping = 2.8;

// Spring resting length
uniform float rest_length = 0.88;

vec3 calcRayIntersection( vec3 pos )
{
    vec3 retPos = pos;
    if (rayPosition.x > pos.x - 1 &&
        rayPosition.x < pos.x + 1 &&
        rayPosition.y > pos.y - 1 &&
        rayPosition.y < pos.y + 1 &&
        rayPosition.z > pos.z - 1 &&
        rayPosition.z < pos.z + 1 &&
        connection[0] != -1 && connection[1] != -1 &&
        connection[2] != -1 && connection[3] != -1) {
        retPos = vec3(rayPosition.x, rayPosition.y, rayPosition.z);
    }
    return retPos;
}

void main(void)
{
    vec3 pos = position_mass.xyz;    // pos can be our position
    pos = calcRayIntersection( pos );
    float mass = position_mass.w;     // the mass of our vertex
    
    // new
//    vec3 old_position = velocity;             // im switching the buffer name
//    vec3 vel = (pos - old_position) * damping; // calculate velocity using current/prev position
    
    // old
    vec3 vel = vec3(velocity);

    vec3 F = gravity * mass - damping * vel;  // F is the force on the mass
    bool fixed_node = true;        // Becomes false when force is applied
    
    for( int i = 0; i < 4; i++) {
        if( connection[i] != -1 ) {
            // q is the position of the other vertex
            vec3 q = texelFetch(tex_position, connection[i]).xyz;
            vec3 d = q - pos;
            float x = length(d);
            F += -spring * (rest_length - x) * normalize(d);
            fixed_node = false;
        }
    }
    
    // If this is a fixed node, reset force to zero
    if( fixed_node ) {
        F = vec3(0.0);
    }
    
    //new
//    vec3 vel = (position - old_position) * damping;
//    old_position = pos;
//    acc = vec3( 0.0 );
//    pos += vel + acc * timestep;
    
    
    
    //old
    // Accelleration due to force
    vec3 acc = F / mass;
    // Displacement
    vec3 displacement = vel * timestep + 0.5 * acc * timestep * timestep;
//     Final velocity
    vec3 new_vel = vel + acc * timestep;
//     Constrain the absolute value of the displacement per step
    displacement = clamp(displacement, vec3(-25.0), vec3(25.0));
    
    
    
    // Write the outputs
    // new
//    tf_position_mass = vec4(pos + vel + acc * timestep, mass);
//    tf_velocity = old_position;
    
    // old
    tf_position_mass = vec4(pos + displacement, mass);
    tf_velocity = vec4(new_vel.x, new_vel.y, new_vel.z, 0);
}
