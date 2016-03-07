//Store into a texture
layout(binding = 0, rgba8) uniform writeonly image2D destTex;
layout(binding = 1, rgba8) uniform readonly image1D transferFunction;
layout(binding = 2, r8) uniform readonly image3D volume;
layout(binding = 3, rgba16f) uniform readonly highp image2D lastHit;
layout(binding = 4, rgba16f) uniform readonly highp image2D firstHit;

uniform float h, width, height, depth;
#define opacityThreshold 0.99





// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
    // Read current global position for this thread
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);

    // Calculate the global number of threads (size) for this
    ivec2 size = imageSize(destTex);
    if(storePos.x < size.x && storePos.y < size.y)
    {
        storePos.y = size.y - storePos.y;
	    vec3 last = imageLoad(lastHit, storePos).xyz;
        vec3 first = imageLoad(firstHit, storePos).xyz;

	    //Get direction of the ray
	    vec3 direction = last.xyz - first.xyz;
	    float D = length (direction);
	    direction = normalize(direction);

	    vec4 color = vec4(0.0f);
	    color.a = 1.0f;

	    vec3 trans = first;
	    vec3 rayStep = direction * h;

	    for(float t =0; t<=D; t += h){
		
            ivec3 volumePosition = ivec3(trans.x * width, trans.y * height, trans.z * depth);
		    //Sample in the scalar field and the transfer function
            //Need to do tri-linear interpolation here
		    int scalar = int(imageLoad(volume, volumePosition).r * 255);
            vec4 samp = imageLoad(transferFunction, scalar).rgba;

		    //Calculating alpa
		    samp.a = 1.0f - exp(-0.5 * samp.a);

		    //Acumulating color and alpha using under operator 
		    samp.rgb = samp.rgb * samp.a;

		    color.rgb += samp.rgb * color.a;
		    color.a *= 1.0f - samp.w;

		    //Do early termination of the ray
		    if(1.0f - color.w > opacityThreshold) break;

		    //Increment ray step
		    trans += rayStep;
	    }

	    color.w = 1.0f - color.w;
        imageStore(destTex, storePos, color);
    }
}