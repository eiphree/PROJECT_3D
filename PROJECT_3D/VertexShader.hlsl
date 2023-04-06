cbuffer vs_const_buffer_t {
    float4x4 matWorldViewProj;
    float4 padding[12];
};
struct vs_output_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};
vs_output_t main(
    float3 pos : POSITION,
    float4 col : COLOR,
    float2 tex : TEXCOORD
   // row_major float4x4 mat_w : WORLD,
   // uint instance_id : SV_InstanceID
) {
    vs_output_t result;
    result.position = mul(
        float4(pos, 1.0f), matWorldViewProj
    );
    result.color = col;
    result.tex = tex;
    return result;
}
