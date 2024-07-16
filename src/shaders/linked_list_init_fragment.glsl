#version 430 core

layout (early_fragment_tests) in;

struct NodeType {
    vec4 color;
    float depth;
    uint next;
};

layout (binding = 0, r32ui) uniform uimage2D headPointers;
layout (binding = 0, offset = 0) uniform atomic_uint nextNodeCounter;
layout (binding = 0, std430) buffer linkedLists {
    NodeType nodes[];
};

uniform uint MaxNodes;

vec4 ShadeFragment();

void main()
{
    // Get the index of the next empty slot in the buffer
    uint nodeIdx = atomicCounterIncrement(nextNodeCounter);

    // Is our buffer full?  If so, we don't add the fragment
    // to the list.
    if (nodeIdx < MaxNodes) {
        // Our fragment will be the new head of the linked list, so
        // replace the value at gl_FragCoord.xy with our new node's
        // index.  We use imageAtomicExchange to make sure that this
        // is an atomic operation.  The return value is the old head
        // of the list (the previous value), which will become the
        // next element in the list once our node is inserted.
        uint prevHead = imageAtomicExchange(headPointers, ivec2(gl_FragCoord.xy), nodeIdx);

        // Here we set the color and depth of this new node to the color
        // and depth of the fragment.  The next pointer, points to the
        // previous head of the list.
        nodes[nodeIdx].color = ShadeFragment();
        nodes[nodeIdx].depth = gl_FragCoord.z;
        nodes[nodeIdx].next = prevHead;
    }
}
