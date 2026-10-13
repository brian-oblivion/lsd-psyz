#version 450

// The displayed picture darkened by its ambient occlusion, blurred over 4x4
// pixels (the rotation pattern) among neighbours at about the same depth.

layout(set = 2, binding = 0) uniform sampler2D texColor;
layout(set = 2, binding = 1) uniform sampler2D texDepth;
layout(set = 2, binding = 2) uniform sampler2D texAo;

layout(set = 3, binding = 0) uniform UBO {
    vec4 region; // xy: the picture's origin in texColor and texDepth, zw: its
                 // size (texels)
    vec4 params; // x: strength
};

layout(location = 0) out vec4 FragColor;

float depthAt(ivec2 p) {
    if (any(lessThan(p, ivec2(0))) || any(greaterThanEqual(p, ivec2(region.zw)))) {
        return 0.0;
    }
    vec4 d = texelFetch(texDepth, ivec2(region.xy) + p, 0);
    return d.r + d.g;
}

// ao.frag's: the depth with one-pixel cracks closed
float surfaceDepthAt(ivec2 p) {
    float z = depthAt(p);
    if (z <= 0.0) {
        return z;
    }
    float l = depthAt(p - ivec2(1, 0)), r = depthAt(p + ivec2(1, 0));
    if (l > 0.0 && r > 0.0 && z > max(l, r) * 1.005) {
        return 0.5 * (l + r);
    }
    float u = depthAt(p - ivec2(0, 1)), d = depthAt(p + ivec2(0, 1));
    if (u > 0.0 && d > 0.0 && z > max(u, d) * 1.005) {
        return 0.5 * (u + d);
    }
    return z;
}

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec4 c = texelFetch(texColor, ivec2(region.xy) + p, 0);
    float z = surfaceDepthAt(p);
    if (z <= 0.0) {
        FragColor = c;
        return;
    }
    float sum = 0.0, wsum = 0.0;
    for (int dy = -2; dy <= 1; dy++) {
        for (int dx = -2; dx <= 1; dx++) {
            ivec2 q = p + ivec2(dx, dy);
            float zq = depthAt(q);
            if (zq <= 0.0) {
                continue;
            }
            float d = (zq - z) / (0.02 * z + 1.0);
            float w = exp(-d * d);
            sum += texelFetch(texAo, q, 0).r * w;
            wsum += w;
        }
    }
    float ao = wsum > 0.0 ? sum / wsum : 1.0;
    ao = clamp(1.0 - (1.0 - ao) * params.x, 0.0, 1.0);
    FragColor = vec4(c.rgb * ao, c.a);
}
