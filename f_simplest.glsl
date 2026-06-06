#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct PointLight {
    vec3 position;
    vec3 color;
};

uniform PointLight light1;
uniform PointLight light2;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular; 
uniform vec3 viewPos;

vec3 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 specularColor) {
    vec3 lightDir = normalize(light.position - fragPos);

    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * light.color;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color;

    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    vec3 specular = specularStrength * spec * light.color * specularColor;
    
    return (ambient + diffuse + specular);
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 diffColor = texture(texture_diffuse, TexCoords).rgb;
    vec3 specColor = texture(texture_specular, TexCoords).rgb;

    vec3 lightingResult = CalculatePointLight(light1, norm, FragPos, viewDir, specColor);
    lightingResult += CalculatePointLight(light2, norm, FragPos, viewDir, specColor);
    
    vec3 finalColor = lightingResult * diffColor;
    
    FragColor = vec4(finalColor, 1.0);
}