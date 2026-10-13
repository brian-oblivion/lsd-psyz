#version 450

// Ambient occlusion of the displayed picture, from the depth target that
// psx.frag writes: how much of the hemisphere around each pixel's normal,
// out to a radius in view units, is behind the depth buffer. 1 where the
// pixel has no depth.

layout(set = 2, binding = 0) uniform sampler2D texDepth;

layout(set = 3, binding = 0) uniform UBO {
    vec4 region; // xy: the picture's origin in texDepth, zw: its size (texels)
    vec4 proj;   // fx, fy, cx, cy: SXY = c + f * view / depth (SXY units)
    vec4 params; // x: radius (view units), y: the most it may span (texels),
                 // z: texels per SXY unit
};

layout(location = 0) out vec4 AoOut;

const vec3 kernel[16] = vec3[](
    vec3(0.0003, 0.0763, 0.0700),
    vec3(-0.0721, -0.0368, 0.0803),
    vec3(0.0055, 0.1235, 0.0453),
    vec3(-0.1118, 0.0882, 0.0643),
    vec3(0.0826, -0.1597, 0.0544),
    vec3(-0.1463, -0.0992, 0.1417),
    vec3(-0.0486, -0.2620, 0.0561),
    vec3(0.0201, -0.0240, 0.3235),
    vec3(-0.3751, -0.0338, 0.0789),
    vec3(-0.3679, -0.1149, 0.2353),
    vec3(-0.2725, 0.3574, 0.2721),
    vec3(-0.0186, 0.5058, 0.3338),
    vec3(-0.4573, 0.4522, 0.2611),
    vec3(-0.5279, 0.0328, 0.5855),
    vec3(-0.2558, 0.7976, 0.3039),
    vec3(-0.8998, 0.0559, 0.4327));

// 4x4 ordered rotations: any 4x4 block of pixels has every one, so the
// composite's 4x4 blur averages the pattern away
const float bayer[16] = float[](
    0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0,
    3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);

float depthAt(ivec2 p) {
    if (any(lessThan(p, ivec2(0))) || any(greaterThanEqual(p, ivec2(region.zw)))) {
        return 0.0;
    }
    vec4 d = texelFetch(texDepth, ivec2(region.xy) + p, 0);
    return d.r + d.g;
}

// The depth with cracks closed: a pixel farther than both its neighbours on
// an axis is a gap between two polygons that don't quite meet, showing what
// is behind them; it takes their depth, or it would look buried.
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

// p: a position in the picture, in texels
vec3 viewPos(vec2 p, float z) {
    vec2 s = p / params.z;
    return vec3((s.x - proj.z) * z / proj.x, (s.y - proj.w) * z / proj.y, z);
}

// The surface's step to the next pixel on one axis, from the side whose two
// pixels, carried on in a line, land nearer the pixel's own depth: the side
// on the same polygon, where a seam or an edge lies next to it.
vec3 tangent(ivec2 p, vec2 pc, vec3 P, ivec2 d) {
    float a1 = depthAt(p + d), a2 = depthAt(p + 2 * d);
    float b1 = depthAt(p - d), b2 = depthAt(p - 2 * d);
    float ea = a1 <= 0.0 ? 1e30 : a2 > 0.0 ? abs(2.0 * a1 - a2 - P.z) : abs(a1 - P.z);
    float eb = b1 <= 0.0 ? 1e30 : b2 > 0.0 ? abs(2.0 * b1 - b2 - P.z) : abs(b1 - P.z);
    if (a1 > 0.0 && ea <= eb) {
        return viewPos(pc + vec2(d), a1) - P;
    }
    if (b1 > 0.0) {
        return P - viewPos(pc - vec2(d), b1);
    }
    return vec3(d, 0.0);
}

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);
    float z = surfaceDepthAt(p);
    if (z <= 0.0) {
        AoOut = vec4(1.0);
        return;
    }
    vec2 pc = vec2(p) + 0.5;
    vec3 P = viewPos(pc, z);
    vec3 n = normalize(cross(tangent(p, pc, P, ivec2(1, 0)),
                             tangent(p, pc, P, ivec2(0, 1))));
    if (dot(n, P) > 0.0) {
        n = -n;
    }

    float radius = params.x;
    float span = radius * proj.x * params.z / z;
    if (span < 1.0) {
        AoOut = vec4(1.0);
        return;
    }
    radius *= min(1.0, params.y / span);

    float a = 6.2831853 * (bayer[(p.y & 3) * 4 + (p.x & 3)] + 0.5) / 16.0;
    vec3 r = vec3(cos(a), sin(a), 0.0);
    vec3 t = r - n * dot(r, n);
    if (dot(t, t) < 1e-6) {
        t = vec3(0.0, cos(a), sin(a)) - n * dot(vec3(0.0, cos(a), sin(a)), n);
    }
    t = normalize(t);
    vec3 b = cross(n, t);

    // a surface must be this far in front of a sample to cover it: more with
    // distance, as the seams between polygons that don't quite meet (and
    // the depth's precision) are off by more
    float bias = radius * 0.03 + z * 0.01;
    float occ = 0.0;
    for (int i = 0; i < 16; i++) {
        vec3 k = kernel[i];
        vec3 S = P + (t * k.x + b * k.y + n * k.z) * radius;
        if (S.z <= 1.0) {
            continue;
        }
        vec2 sp = vec2(proj.z + proj.x * S.x / S.z, proj.w + proj.y * S.y / S.z) *
                  params.z;
        float sz = depthAt(ivec2(floor(sp)));
        if (sz <= 0.0) {
            continue;
        }
        float range = clamp(radius / abs(z - sz), 0.0, 1.0);
        occ += (sz < S.z - bias ? 1.0 : 0.0) * range;
    }
    AoOut = vec4(1.0 - occ / 16.0);
}
