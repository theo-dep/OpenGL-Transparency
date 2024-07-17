#version 420 core

uniform layout(binding = 0, rgba32f) image2DArray aBufferTex;
uniform layout(binding = 1, r32ui) uimage2D aBufferCounterTex;

#define ABUFFER_SIZE 16

uniform vec3 BackgroundColor;
uniform float Alpha;

out vec4 FragColor;

void main()
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    // Load the number of fragments in the current pixel.
    uint abNumFrag = min(imageLoad(aBufferCounterTex, coords).r, ABUFFER_SIZE);

    // Load fragments into a local memory array for sorting
    vec4 fragmentList[ABUFFER_SIZE];
	for (uint i = 0; i < abNumFrag; i++) {
        fragmentList[i] = imageLoad(aBufferTex, ivec3(coords, i));
    }

    // Sort fragments in local memory array
    // bubble sort
    for (int i = int(abNumFrag - 2); i >= 0; --i) {
        for (int j = 0; j <= i; ++j) {
            if (fragmentList[j].w > fragmentList[j+1].w) {
    	  	    vec4 temp = fragmentList[j+1];
    	  	    fragmentList[j+1] = fragmentList[j];
    	  	    fragmentList[j] = temp;
            }
        }
    }

    // Blend fragments front-to-back
    vec4 finalColor = vec4(0.0);
    for (uint i = 0; i < abNumFrag; i++) {
        vec4 fragment = fragmentList[i];

        vec4 color;
        color.rgb = fragment.rgb;
        color.a = Alpha; // uses constant alpha

		color.rgb = color.rgb * color.a;

		finalColor = finalColor + color * (1.0 - finalColor.a);
	}

	FragColor = finalColor + vec4(BackgroundColor, 1.0) * (1.0 - finalColor.a);
}
