Texture2D colorTexture : register(t0);

SamplerState samplerState : register(s0);

cbuffer exposureBuffer : register(b0)
{
    float4 exposure;
}

static const float A = 0.1f;    // Shoulder Strength
static const float B = 0.50f;   // Linear Strength
static const float C = 0.10f;   // Linear Angle
static const float D = 0.20f;   // Toe Strength
static const float E = 0.02f;   // Toe Numerator
static const float F = 0.30f;   // Toe Denomenator
static const float W = 11.2f;   // Linear White Point Value


struct VSIn
{
    uint vertexId : SV_VERTEXID;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};


float3 Uncharted2Tonemap(float3 x)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}


VSOut VS(VSIn input)
{
    VSOut vertices[4] =
    {
        { -1.0f, 1.0f, 0.0f, 1.0f },    { 0.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },     { 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f, 1.0f },   { 0.0f, 1.0f },
        { 1.0f, -1.0f, 0.0f, 1.0f },    { 1.0f, 1.0f }
    };

    return vertices[input.vertexId];
}


float3 PS(VSOut input) : SV_TARGET
{
    float3 color = colorTexture.Sample(samplerState, input.texCoord).rgb;

    float3 curr = Uncharted2Tonemap(exposure.r * color);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(W);

    return curr * whiteScale;
}
    