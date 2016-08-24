#version 330 core

uniform float uMouseForce;
uniform vec3  uMousePos;

in vec3   iPosition;
in vec3   iOldPostion;
in vec3   iAcceleration;
in vec3   iAccumulatedNormal;
//in float  iDamping;
in vec4   iColor;

out vec3  position;
out vec3  old_position;
out vec3  acceleration;
out vec3  accumulated_normal;
//out float damping;
out vec4  color;

const float dt2 = 0.20 / (60.0 * 60.0); // timestep? idk what this is
const float damping = 0.01;

//uniform vec3 rayPosition;
//
//
//vec3 calcRayIntersection( vec3 pos )
//{
//    vec3 retPos = pos;
//    if (rayPosition.x > pos.x - 1 &&
//        rayPosition.x < pos.x + 1 &&
//        rayPosition.y > pos.y - 1 &&
//        rayPosition.y < pos.y + 1 &&
//        rayPosition.z > pos.z - 1 &&
//        rayPosition.z < pos.z + 1 &&
//        connection[0] != -1 && connection[1] != -1 &&
//        connection[2] != -1 && connection[3] != -1) {
//        retPos = vec3(rayPosition.x, rayPosition.y, rayPosition.z);
//    }
//    return retPos;
//}

void main()
{
//    vec3 p = pos;    // p can be our position
//    position = calcRayIntersection( p );
    
    position     =       iPosition;
    old_position =       iOldPostion;
    acceleration =       iAcceleration;
    color        =       iColor;
    
    // mouse interaction
    if( uMouseForce > 0.0 ) {
        vec3 dir = position - uMousePos;
        float d2 = length( dir );
        d2 *= d2;
        position += uMouseForce * dir / d2;
    }
    
    vec3 vel = (position - old_position) * damping;
    old_position = position;
    vec3 acc = vec3( 0.0 );
    position += vel + acc * dt2;
}