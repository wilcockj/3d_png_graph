#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;      // Not required

in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;
uniform mat4 matModel;

// Output vertex attributes (to fragment shader)
out vec4 fragColor;

// NOTE: Add here your custom variables

void main()
{
    // Compute MVP for current instance
    mat4 mvpi = mvp*instanceTransform;



    // Calculate final vertex position
    gl_Position = mvpi*vec4(vertexPosition, 1.0);

    mat4 matModeli = matModel * instanceTransform;
    vec4 fragWorldPosition = instanceTransform * vec4(vertexPosition,1.0);
    vec3 colorFactor = clamp(abs(fragWorldPosition.xyz) / vec3(5.0, 5.0, 5.0), 0.0, 1.0);
    fragColor = vec4(colorFactor,1.0);
}
