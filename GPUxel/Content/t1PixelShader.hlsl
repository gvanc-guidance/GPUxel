//          
//      
//          ghv 20190409 Pixel Shader downstream from CS Compute Shader; 
//          
//          


Texture2D           output_as_SRV       : register(t0);

SamplerState        LinearSampler       : register(s0);




struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};




float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 rgba = output_as_SRV.Sample(LinearSampler, input.tex);

    return rgba;
}











