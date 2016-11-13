//Store into a texture
layout(binding = 0, rgba8) uniform writeonly image2D destTex;
layout(binding = 1) uniform sampler1D transferFunction; 
//layout(binding = 1, rgba8) uniform readonly image1D transferFunction;
layout(binding = 2) uniform sampler3D volume; 
//layout(binding = 2, r8) uniform readonly image3D volume;

uniform float h, width, height, depth;
uniform int constantWidth, constantHeight;
uniform float constantAngle, constantNCP;
uniform mat4 c_invViewMatrix;
uniform int light = 0; //if light is enable or not
uniform vec3 lightDir = vec3(0.0f,0.0f,-1.0f); //light direction in eye space
uniform vec3 diffColor = vec3(1.0f,0.0f,1.0f); //diffuse color of the light
uniform vec3 voxelJump = vec3(0.01f, 0.01f, 0.01f); //distance between voxels

#define opacityThreshold 0.99





struct Ray
{
	vec4 o;   // origin
	vec4 d;   // direction
};

// intersect ray with a box
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm

bool intersectBox(Ray r, out float tnear, out float tfar)
{
	vec3 boxmin = vec3(-.5f, -.5f, -.5f);
	vec3 boxmax = vec3(.5f, .5f, .5f);

	// compute intersection of ray with all six bbox planes
	vec3 invR = 1.0f / r.d.xyz;
	vec3 tbot = invR * (boxmin - r.o.xyz);
	vec3 ttop = invR * (boxmax - r.o.xyz);

	// re-order intersections to find smallest and largest on each axis

	vec3 tmin = vec3(100000.0f);
	vec3 tmax = vec3(0.0f);

	tmin = min(tmin, min(ttop, tbot));
	tmax = max(tmax, max(ttop, tbot));

	// find the largest tmin and the smallest tmax
	float largest_tmin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
	float smallest_tmax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));

	tnear = largest_tmin;
	tfar = smallest_tmax;

	return smallest_tmax > largest_tmin;
}




// Declare main program function which is executed once
// glDispatchCompute is called from the application.
void main()
{
    // Read current global position for this thread
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);

    // Calculate the global number of threads (size) for this
    if(storePos.x < constantWidth && storePos.y < constantHeight)
    {
        storePos.y = constantHeight - storePos.y;

        //ok u and v between -1 and 1
		float u = (((storePos.x + 0.5f) / float(constantWidth))*2.0f - 1.0f);
		float v = (((storePos.y + 0.5f) / float(constantHeight))*2.0f - 1.0f);		


		// calculate eye ray in world space
		Ray eyeRay;
		float tangent = tan(constantAngle / 2.0f); // angle in radians
		float ar = (float(constantWidth) / constantHeight);
		eyeRay.o = c_invViewMatrix * vec4 (0.0f, 0.0f, 0.0f, 1.0f);
		eyeRay.d = normalize(vec4(u * tangent * ar, v * tangent, -constantNCP, 0.0f));
		eyeRay.d = c_invViewMatrix * eyeRay.d;
		eyeRay.d = normalize(eyeRay.d);

		// find intersection with box
		float tnear, tfar;
		bool hit = intersectBox(eyeRay, tnear, tfar);  // this must be wrong....anything else seems to be ok now

        vec4 color = vec4(0.0f);

        if(hit){

           // if (tnear < constantNCP) tnear = constantNCP;     // clamp to near plane

			vec3 first = vec3(eyeRay.o + eyeRay.d*tnear);
            first = vec3(first.x + 0.5f, first.y + 0.5f , 1.0f - (first.z + 0.5f));
			vec3 last = vec3(eyeRay.o + eyeRay.d*tfar);
            last = vec3(last.x + 0.5f, last.y + 0.5f , 1.0f - (last.z + 0.5f));

	        //Get direction of the ray
	        vec3 direction = last - first;
	        float D = length (direction);
	        direction = normalize(direction);

	        
	        color.a = 1.0f;

	        vec3 trans = first;
	        vec3 rayStep = direction * h;

	        for(float t =0; t<=D; t += h){
		
                 // ivec3 volumePosition = ivec3((trans.x + 0.5f) * width, (trans.y + 0.5f) * height, (1.0f - (trans.z + 0.5f)) * depth);

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
		        color.a *= 1.0f - samp.a;

		        //Do early termination of the ray
		        if(1.0f - color.a > opacityThreshold) break;

		        //Increment ray step
		        trans += rayStep;
	        }

            color.a = 1.0f - color.a;
        }
        imageStore(destTex, storePos, color);
    }
}