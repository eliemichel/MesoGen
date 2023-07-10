/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

float mSphere(vec3 p, float radius) {
    return length(p) - radius;
}

float mBox(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float mRoundCone(vec3 p, vec3 a, vec3 b, float r1, float r2)
{
    // sampling independent computations (only depend on shape)
    vec3  ba = b - a;
    float l2 = dot(ba,ba);
    float rr = r1 - r2;
    float a2 = l2 - rr*rr;
    float il2 = 1.0/l2;
    
    // sampling dependant computations
    vec3 pa = p - a;
    float y = dot(pa,ba);
    float z = y - l2;
    vec3 paba = pa*l2 - ba*y;
    float x2 = dot(paba,paba);
    float y2 = y*y*l2;
    float z2 = z*z*l2;

    // single square root!
    float k = sign(rr)*rr*rr*x2;
    if( sign(z)*a2*z2 > k ) return  sqrt(x2 + z2)        *il2 - r2;
    if( sign(y)*a2*y2 < k ) return  sqrt(x2 + y2)        *il2 - r1;
                            return (sqrt(x2*a2*il2)+y*rr)*il2 - r1;
}

float mUnion(float a, float b) {
    return min(a, b);
}

vec2 mSmoothUnion( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    float m = h*h*0.5;
    float s = m*k*(1.0/2.0);
    return (a<b) ? vec2(a-s,m) : vec2(b-s,m-1.0);
}

float mIntersection(float a, float b) {
    return max(a, b);
}

float dot2( in vec2 v ) { return dot(v,v); }
float msign( in float x ) { return (x>0.0)?1.0:-1.0; }

// http://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
vec2 sdSqLine( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return vec2( dot2(pa-ba*h), ba.x*pa.y-ba.y*pa.x );
}

// http://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
vec2 sdSqArc( in vec2 p, in vec2 a, in vec2 b, in float h, float d2min )
{
    vec2  ba  = b-a;
    float l   = length(ba);
    float ra2 = h*h + l*l*0.25;

    // recenter
    p -= (a+b)/2.0 + vec2(-ba.y,ba.x)*h/l;
    
    float m = ba.y*p.x-ba.x*p.y;
    float n = dot(p,p);
    
    if( abs(h)*abs(ba.x*p.x+ba.y*p.y) < msign(h)*l*0.5*m )
    {
        d2min = min( d2min, n + ra2 - 2.0*sqrt(n*ra2) );
    }

    return vec2(d2min, -max(m,ra2-n) );
}

// SDF of a shape made of a set line and arc segments
// from https://www.shadertoy.com/view/3t33WH thx iq
float sdShape( in vec2 p, inout int kType[7], inout float kPath[17] )
{
    vec2 vb = vec2(kPath[0],kPath[1]);
    
    float d = dot2(p-vb);
    int off = 0;
    float s = 1.0;
    for( int i=0; i<kType.length(); i++ )
    {
        vec2 va = vb;
        vec2 ds;

        if( kType[i]==-1) // end
        {
            break;
        }
        else if( kType[i]==0) // line (x,y)
        {
            vb = vec2(kPath[off+2],kPath[off+3]);
            ds = sdSqLine( p, va, vb );
            off += 2;
        }
        else if( kType[i]==1) // arc (x,y,r)
        {
            vb = vec2(kPath[off+3],kPath[off+4]);
            ds = sdSqArc(p, va, vb, kPath[off+2], d );
            off += 3;

        }
        
        // in/out test
        bvec3 cond = bvec3( p.y>=va.y, p.y<vb.y, ds.y>0.0 );
        if( all(cond) || all(not(cond)) ) s*=-1.0;  

        d = min( d, ds.x );
    }
    return s*sqrt(d);
}
