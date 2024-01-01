#version 100

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;      // Not required

attribute mat4 instanceTransform;
// GLSL 100 does not support matrix attributes like 'in mat4'
// You'll need to handle instance transformations differently, perhaps using a uniform array

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;
uniform mat4 matModel;

// Output vertex attributes (to fragment shader)
// GLSL 100 uses 'varying' instead of 'out'
varying vec4 fragColor;

void main()
{
    // Since GLSL 100 doesn't support matrix attributes, instanceTransform is not used
    // Compute MVP for current instance
    mat4 mvpi = mvp * instanceTransform; // Adjust this line based on how you handle instances

    // Calculate final vertex position
    gl_Position = mvpi*vec4(vertexPosition, 1.0);

    // Similarly, adjust matModeli and fragWorldPosition calculations
    mat4 matModeli = matModel;
    vec4 fragWorldPosition = instanceTransform * vec4(vertexPosition,1.0);
    vec3 colorFactor = clamp(abs(fragWorldPosition.xyz) / vec3(5.0, 5.0, 5.0), 0.0, 1.0);
    fragColor = vec4(colorFactor,1.0);
}

