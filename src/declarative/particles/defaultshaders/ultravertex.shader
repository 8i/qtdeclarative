attribute highp vec2 vPos;
attribute highp vec2 vTex;
attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
attribute lowp vec4 vColor;
attribute highp vec4 vDeformVec; //x,y x unit vector; z,w = y unit vector
attribute highp vec3 vRotation; //x = radians of rotation, y=rotation speed, z= bool autoRotate
attribute highp vec4 vAnimData;// idx, duration, frameCount (this anim), timestamp (this anim)

uniform highp mat4 matrix;
uniform highp float timestamp;
uniform highp float framecount; //maximum of all anims
uniform highp float animcount;
uniform sampler2D sizetable;

varying lowp float tt;
varying highp vec2 fTexA;
varying highp vec2 fTexB;
varying lowp float progress;
varying lowp vec4 fColor;


void main() {
    highp float size = vData.z;
    highp float endSize = vData.w;

    highp float t = (timestamp - vData.x) / vData.y;

    //Calculate frame location in texture
    highp float frameIndex = mod((((timestamp - vAnimData.w)*1000.)/vAnimData.y),vAnimData.z);
    progress = mod((timestamp - vAnimData.w)*1000., vAnimData.y) / vAnimData.y;

    frameIndex = floor(frameIndex);
    highp vec2 frameTex = vTex;
    if(vTex.x == 0.)
        frameTex.x = (frameIndex/framecount);
    else
        frameTex.x = 1. * ((frameIndex + 1.)/framecount);

    if(vTex.y == 0.)
        frameTex.y = (vAnimData.x/animcount);
    else
        frameTex.y = 1. * ((vAnimData.x + 1.)/animcount);

    fTexA = frameTex;
    //Next frame is also passed, for interpolation
    //### Should the next anim be precalculated to allow for interpolation there?
    if(frameIndex != vAnimData.z - 1.)//Can't do it for the last frame though, this anim may not loop
        frameIndex = mod(frameIndex+1., vAnimData.z);

    if(vTex.x == 0.)
        frameTex.x = (frameIndex/framecount);
    else
        frameTex.x = 1. * ((frameIndex + 1.)/framecount);

    if(vTex.y == 0.)
        frameTex.y = (vAnimData.x/animcount);
    else
        frameTex.y = 1. * ((vAnimData.x + 1.)/animcount);
    fTexB = frameTex;

    highp float currentSize = mix(size, endSize, t * t) * texture2D(sizetable, vec2(t,0.5)).w;

    if (t < 0. || t > 1.)
        currentSize = 0.;

    highp vec2 pos;
    highp float rotation = vRotation.x + vRotation.y * t * vData.y;
    if(vRotation.z == 1.0){
        highp vec2 curVel = vVec.zw * t * vData.y + vVec.xy;
        rotation += atan(curVel.y, curVel.x);
    }
    highp vec2 trigCalcs = vec2(cos(rotation), sin(rotation));
    highp vec2 xDeform = vDeformVec.xy * currentSize * (vTex.x-0.5);
    highp vec2 yDeform = vDeformVec.zw * currentSize * (vTex.y-0.5);
    highp vec2 xRotatedDeform;
    xRotatedDeform.x = trigCalcs.x*xDeform.x - trigCalcs.y*xDeform.y;
    xRotatedDeform.y = trigCalcs.y*xDeform.x + trigCalcs.x*xDeform.y;
    highp vec2 yRotatedDeform;
    yRotatedDeform.x = trigCalcs.x*yDeform.x - trigCalcs.y*yDeform.y;
    yRotatedDeform.y = trigCalcs.y*yDeform.x + trigCalcs.x*yDeform.y;
    pos = vPos
          + xRotatedDeform
          + yRotatedDeform
          //- vec2(1,1) * currentSize * 0.5 // 'center'
          + vVec.xy * t * vData.y         // apply speed
          + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration

    gl_Position = matrix * vec4(pos.x, pos.y, 0, 1);

    fColor = vColor;
    tt = t;

}
