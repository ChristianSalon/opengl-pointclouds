#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec4 vColor;

layout(location = 0) out vec4 fColor;

uniform mat4 view;

void main() {
    const vec3 lightPosition = vec3(5.0, 10.0, 5.0);
    const vec3 lightColor = vec3(1.0, 1.0, 0.9);
    const float ambientStrength = 0.2;
    const float specStrength = 0.5;
    const float shininess = 32.0;

    vec3 viewPosition = vec3(inverse(view)[3]);

    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(lightPosition - vPosition);
    vec3 viewDirection = normalize(viewPosition - vPosition);
    vec3 reflectDirection = reflect(-lightDirection, normal);

    vec3 ambient = ambientStrength * lightColor;
    
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
    vec3 specular = specStrength * spec * lightColor;

    fColor = vec4((ambient + diffuse + specular) * vColor.rgb, 1.0);
}
