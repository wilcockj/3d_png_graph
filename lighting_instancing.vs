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
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec4 fragWorldPosition;

// NOTE: Add here your custom variables

void main()
{
    // Compute MVP for current instance
    mat4 mvpi = mvp*instanceTransform;

    // Send vertex attributes to fragment shader
    fragPosition = vec3(mvpi*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    //fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));


    // Calculate final vertex position
    gl_Position = mvpi*vec4(vertexPosition, 1.0);

    mat4 matModeli = matModel * instanceTransform;
    fragWorldPosition = instanceTransform * vec4(vertexPosition,1.0);
    vec3 colorFactor = clamp((fragWorldPosition.xyz) / vec3(5.0, 5.0, 5.0), 0.0, 1.0);
    fragColor = vec4(colorFactor,1.0);
}
