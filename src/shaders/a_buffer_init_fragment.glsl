#version 420 core

// A-Buffer fragments storage
uniform layout(binding = 0, rgba32f) image2DArray aBufferTex;
// A-Buffer fragment counter
uniform layout(binding = 1, r32ui) uimage2D aBufferCounterTex;

vec4 ShadeFragment();

void main()
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    // Atomic increment of the counter
    uint abidx = imageAtomicAdd(aBufferCounterTex, coords, 1);

    // Create fragment to be stored
    vec4 color = ShadeFragment();

    // Choose what we store per fragment
    vec4 abuffval;
    abuffval.rgb = color.rgb; // Alpha will be used in final fragment
    abuffval.w = gl_FragCoord.z; // Will be used for sorting

    // Put fragment into A-Buffer
    imageStore(aBufferTex, ivec3(coords, abidx), abuffval);
}
