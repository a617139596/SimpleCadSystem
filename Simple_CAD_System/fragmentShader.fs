#version 330 core

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;
  
uniform vec3 color;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool isFrameMode;

void main()
{
    if(isFrameMode){
        FragColor = vec4(color, 1.0);
        return;
	}
        
    float ambientStrength = 0.3f;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(lightDir, norm), 0.0f);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.5f;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    vec3 result = (ambient + diffuse + spec) * color;
    FragColor = vec4(result, 1.0);
}