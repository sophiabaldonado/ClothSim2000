// Update Vertex Shader
// OpenGL SuperBible Chapter 7
// Graham Sellers
#version 330 core

// This input vector contains the vertex position in xyz, and the
// mass of the vertex in w
layout (location = 0) in vec4 position_mass;        // POSITION_INDEX
// This is the previous of the vertex
layout (location = 1) in vec4 prev_position_mass;	// PREV_POSITION_INDEX
// This is our connection vector
layout (location = 2) in ivec4 connection;          // CONNECTION_INDEX

// This is a TBO that will be bound to the same buffer as the
// position_mass input attribute
uniform samplerBuffer tex_position;

uniform vec3 rayPosition;
uniform float ciElapsedSeconds;
uniform bool trigger;

// The outputs of the vertex shader are the same as the inputs
out vec4 tf_position_mass;
out vec4 tf_prev_position_mass;

// A uniform to hold the timestep. The application can update this.
uniform float timestep = 0.05;

// The global spring constant
uniform float spring = 50;

// Gravity
uniform vec3 gravity = vec3(0.0, -0.08, 0.0);

// Global damping constant
uniform float damping = 0.1;

// Spring resting length
uniform float rest_length = 1.0;

vec3 calcRayIntersection( vec3 pos )
{   // this is for pinching/pulling on cloth with trigger
    vec3 retPos = pos;
    if (trigger) {
        if (rayPosition.x > pos.x - 1 &&
            rayPosition.x < pos.x + 1 &&
            rayPosition.y > pos.y - 1 &&
            rayPosition.y < pos.y + 1 &&
            rayPosition.z > pos.z - 1.5 &&
            rayPosition.z < pos.z + 1.5 &&
            connection[0] != -1 && connection[1] != -1 &&
            connection[2] != -1 && connection[3] != -1) {

            retPos = vec3(rayPosition.x, rayPosition.y, rayPosition.z);
        }
    } else {
        
    vec3 center = rayPosition;
    vec3 moveDirection = (pos - center);
    float l = length(moveDirection);
    float radius = 3.0;
        
    if (l < radius) {  // see if the pos is in the sphere
            retPos = (pos + normalize(moveDirection) * (radius - l) );
        }
  }
    return retPos;
}

void main(void)
{
    vec3 pos = position_mass.xyz;               // pos can be our position
    pos = calcRayIntersection( pos );
    float mass = position_mass.w;               // the mass of our vertex, right now is always 1

    vec3 old_position = prev_position_mass.xyz; // save the previous position
    vec3 vel = (pos - old_position) * damping;  // calculate velocity using current & prev position

    vec3 F = gravity * mass - damping * vel;    // F is the force on the mass
    bool fixed_node = true;                     // Becomes false when force is applied

    
    for( int i = 0; i < 4; i++ ) {
        if( connection[i] != -1 ) {
            // q is the position of the other vertex
            vec3 q = texelFetch(tex_position, connection[i]).xyz;
            vec3 delta = q - pos;
            float point_distance = length(delta);
            F += -spring * (rest_length - point_distance) * normalize(delta);
            fixed_node = false;
        }
    }

    // If this is a fixed node, reset force to zero
    if( fixed_node ) {
        F = vec3(0.0);
    }

    // Acceleration due to force
    vec3 acc = F / mass;
    
    vec3 displacement = vel + acc * timestep * timestep;

    // Write the outputs
    tf_prev_position_mass = position_mass;
    tf_position_mass = vec4(pos + displacement, mass);
}
