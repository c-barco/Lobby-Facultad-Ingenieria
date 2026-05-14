#version 330 core

#define NUMBER_OF_POINT_LIGHTS 11

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform int transparency;
uniform int useTexture;
uniform vec3 objectColor;

// Function prototypes
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor );
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor );
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor );

void main( )
{
    vec3 norm = normalize( Normal );
    vec3 viewDir = normalize( viewPos - FragPos );

    vec3 baseColor = (useTexture == 1) ? vec3( texture( material.diffuse, TexCoords ) ) : objectColor;
    float alpha    = (useTexture == 1) ? texture( material.diffuse, TexCoords ).a : 1.0;

    vec3 result = CalcDirLight( dirLight, norm, viewDir, baseColor );

    for ( int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++ )
    {
        result += CalcPointLight( pointLights[i], norm, FragPos, viewDir, baseColor );
    }

    result += CalcSpotLight( spotLight, norm, FragPos, viewDir, baseColor );

    color = vec4( result, alpha );

    if ( color.a < 0.1 && transparency == 1 )
        discard;
}

vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor )
{
    vec3 lightDir = normalize( -light.direction );

    float diff = max( dot( normal, lightDir ), 0.0 );

    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );

    vec3 specColor = (useTexture == 1) ? vec3( texture( material.specular, TexCoords ) ) : vec3( 0.5 );

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * baseColor;
    vec3 specular = light.specular * spec * specColor;

    return ( ambient + diffuse + specular );
}

vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor )
{
    vec3 lightDir = normalize( light.position - fragPos );

    float diff = max( dot( normal, lightDir ), 0.0 );

    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );

    float distance    = length( light.position - fragPos );
    float attenuation = 1.0 / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );

    vec3 specColor = (useTexture == 1) ? vec3( texture( material.specular, TexCoords ) ) : vec3( 0.5 );

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * baseColor;
    vec3 specular = light.specular * spec * specColor;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ( ambient + diffuse + specular );
}

vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor )
{
    vec3 lightDir = normalize( light.position - fragPos );

    float diff = max( dot( normal, lightDir ), 0.0 );

    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );

    float distance    = length( light.position - fragPos );
    float attenuation = 1.0 / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );

    float theta    = dot( lightDir, normalize( -light.direction ) );
    float epsilon  = light.cutOff - light.outerCutOff;
    float intensity = clamp( ( theta - light.outerCutOff ) / epsilon, 0.0, 1.0 );

    vec3 specColor = (useTexture == 1) ? vec3( texture( material.specular, TexCoords ) ) : vec3( 0.5 );

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * baseColor;
    vec3 specular = light.specular * spec * specColor;

    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ( ambient + diffuse + specular );
}
