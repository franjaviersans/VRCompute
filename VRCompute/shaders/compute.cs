//Store into a texture
layout(binding = 0, rgba8) uniform writeonly image2D destTex;
layout(binding = 1) uniform sampler1D transferFunction; 
//layout(binding = 1, rgba8) uniform readonly image1D transferFunction;
layout(binding = 2) uniform sampler3D volume; 
//layout(binding = 2, r8) uniform readonly image3D volume;
layout(binding = 3) uniform sampler2D lastHit;
layout(binding = 4) uniform sampler2D firstHit;

uniform float h, width, height, depth;
uniform int light = 0; //if light is enable or not
uniform vec3 lightDir = vec3(0.0f,0.0f,-1.0f); //light direction in eye space
uniform vec3 diffColor = vec3(1.0f,0.0f,1.0f); //diffuse color of the light
uniform vec3 voxelJump = vec3(0.01f, 0.01f, 0.01f); //distance between voxels


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
	    vec3 last = texelFetch(lastHit, storePos, 0).xyz;
        vec3 first = texelFetch(firstHit, storePos, 0).xyz;

	    //Get direction of the ray
	    vec3 direction = last.xyz - first.xyz;
	    float D = length (direction);
	    direction = normalize(direction);

	    vec4 color = vec4(0.0f);
	    color.a = 1.0f;

	    vec3 trans = first;
	    vec3 rayStep = direction * h;

	    for(float t =0; t<=D; t += h){
		
            //ivec3 volumePosition = ivec3(trans.x * width, trans.y * height, trans.z * depth);
		    
            //Sample in the scalar field and the transfer function	    
            float scalar = texture(volume,trans).x;
            //int scalar = int(imageLoad(volume, volumePosition).r * 255);
            vec4 samp = texture(transferFunction, scalar).rgba;
            //vec4 samp = imageLoad(transferFunction, scalar).rgba;

		    //Calculating alpa
		    samp.a = 1.0f - exp(-0.5 * samp.a);

		    //Acumulating color and alpha using under operator 
		    samp.rgb = samp.rgb * samp.a;

            

            //calculate lighting for the sample color
		    if(light != 0){
			    // sample neightbours
			    vec3 normal;
			    //sample right
			    vec3 displacement = vec3(voxelJump.x,0.0f,0.0f);
			    normal.x = texture(volume, trans + displacement).x - scalar;
			    displacement = vec3(0.0f,voxelJump.y,0.0f);
			    normal.y = texture(volume, trans + displacement).x - scalar;
			    displacement = vec3(0.0f,0.0f,-voxelJump.z);	//invert z coordinate
			    normal.z =  texture(volume, trans + displacement).x - scalar;

			    //normalize normal
			    normal = normalize(normal);

			    float d = max(dot(lightDir, normal), 0.0f);

			    samp.xyz = samp.xyz * d * diffColor;
		    }




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