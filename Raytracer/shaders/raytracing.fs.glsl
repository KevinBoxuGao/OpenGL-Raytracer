#version 330 core

#define MAX_OBJECT_COUNT 128
#define MAX_LIGHT_COUNT 4

out vec4 FragColor;

struct Interval {
    float min;
    float max;
};

//lambertian diffuse
//metal
//dielectric
struct Material {
    int type;
    vec3 albedo;
    float fuzz;
    float refraction_index;
};

//scale:
//first represents radius of sphere
struct Object {
    int type;
    vec3 position;
    vec3 scale;
    Material material;
};

struct HitRecord {
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

uniform vec3 u_pixel00;
uniform vec3 u_pixelDeltaU;
uniform vec3 u_pixelDeltaV;
uniform vec3 u_cameraCenter;

uniform Object u_objects[MAX_OBJECT_COUNT];

uniform float u_time;
uniform int u_samplesPerPixel;
uniform int u_lightBounces;

const float INFINITY = float(1.0 / 0.0);
const float PI = 3.1415926;

//vector Functions
float lengthSquared(vec3 v) {
    return dot(v, v);
}
bool isNearZero(vec3 vector) {
    float threshold = 1e-8;
    return length(vector) < threshold;
}

// Interval Functions
bool surrounds(Interval i, float x) {
    return i.min < x && x < i.max;
}

//random number generation
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}
float _rand() {
    vec2 seed = vec2(u_time, length(gl_FragCoord) * 0.1);
    return rand(seed);
}
vec3 randomUnitVector(vec2 co) {
    float theta = rand(co) * 2.0 * PI;       // Random angle in radians
    float phi = rand(co * 0.5) * 0.5 * PI;   // Random angle for elevation, scaled for better results
    float x = cos(theta) * sin(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(phi);
    return normalize(vec3(x, y, z));
}
vec3 _randomUnitVector() {
    vec2 seed = vec2(u_time, length(gl_FragCoord) * 0.1);
    return randomUnitVector(seed);
}

//material functions
vec3 reflect(vec3 v, vec3 n) {
    return v - 2*dot(v,n)*n;
}
vec3 refract(vec3 r_in, vec3 normal, float etaI_over_etaT) {
    //snell's law
    float cos_theta = min(1.0, dot(-r_in, normal));
    vec3 r_out_perp = etaI_over_etaT * (r_in + cos_theta * normal);
    vec3 r_out_parallel = -sqrt(abs(1.0-dot(r_out_perp, r_out_perp))) * normal;
    return r_out_perp + r_out_parallel;
}
float reflectance(float cosine, float ref_idx) {
    //schlick approximation
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1-cosine), 5);
}
bool scatter(Ray r_in, HitRecord rec, inout vec3 attenuation, inout Ray scattered, vec2 seed) {
    switch (rec.material.type) {
        case 0: //unitialized
            break;
        case 1: //diffuse
            vec3 scatter_direction = rec.normal + randomUnitVector(seed);
            if (isNearZero(scatter_direction)) {
                scatter_direction = rec.normal;
            }
            scattered.origin = rec.p;
            scattered.direction = scatter_direction;
            attenuation = rec.material.albedo;
            break;
        case 2: //metal
            vec3 reflected = reflect(normalize(r_in.direction), rec.normal);
            scattered.origin = rec.p;
            scattered.direction = reflected+rec.material.fuzz*randomUnitVector(seed);
            attenuation = rec.material.albedo;
            return (dot(scattered.direction, rec.normal) > 0);
        case 3: //dielectric
            attenuation = vec3(1.0);
            float refraction_ratio = rec.front_face ? (1.0/rec.material.refraction_index) : rec.material.refraction_index;
            vec3 unit_direction = normalize(r_in.direction);
            vec3 refracted = refract(unit_direction, rec.normal, refraction_ratio);

            float cos_theta = min(1.0, dot(-unit_direction, rec.normal));
            float sin_theta = sqrt(1.0-cos_theta*cos_theta);

            bool cannot_refract = refraction_ratio * sin_theta > 1.0;
            vec3 direction; 

            float reflect_prob = reflectance(cos_theta, refraction_ratio);

            //a deterministic value makes the object more smooth. Randomness causes quite a bit of noise.
            float random_val = 0.5;

            if(cannot_refract || reflectance(cos_theta, refraction_ratio) > random_val) {
                direction = reflect(unit_direction, rec.normal);
            } else {
                direction = refract(unit_direction, rec.normal, refraction_ratio);
            }
            scattered.origin = rec.p;
            scattered.direction = direction;

            break; 
    }
    return true;
}

// Ray Functions
vec3 at(Ray r, float t) {
    return r.origin + t * r.direction;
}
bool hitSphere(const vec3 center, float radius, const Ray r, Interval ray_t, inout HitRecord rec, Material material) {
    vec3 oc = r.origin - center;
    float a = dot(r.direction, r.direction);
    float half_b = dot(oc, r.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0) {
        return false;
    }
    float root = (-half_b - sqrt(discriminant)) / a;
    if (!surrounds(ray_t, root)) {
        root = (-half_b + sqrt(discriminant)) / a;
        if (!surrounds(ray_t, root)) {
            return false;
        }
    }
    rec.t = root;
    rec.p = at(r, rec.t);
    vec3 outward_normal = (rec.p - center) / radius;
    rec.front_face = dot(r.direction, outward_normal) < 0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    rec.material = material;
    //rec.material.type = material.type;
    //rec.material.albedo = ;
    return true;
}
bool hit(Ray r, Interval ray_t, inout HitRecord rec) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = ray_t.max;
    for (int i = 0; i < u_objects.length(); i++) {
        switch (u_objects[i].type) {
            case 0:
                break;
            case 1:
                Interval temp_interval;
                temp_interval.min = ray_t.min;
                temp_interval.max = closest_so_far;
                if (hitSphere(u_objects[i].position, u_objects[i].scale.x, r, temp_interval, temp_rec, u_objects[i].material)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
                break;
            default:
                break;
        }
    }
    return hit_anything;
}
vec3 getRayColor(Ray r, vec2 seed) {
    vec3 color = vec3(1.0);
    Ray currentRay = r;
    for (int i = 0; i < u_lightBounces; i++) {
        HitRecord rec;
        if (hit(currentRay, Interval(0.001, INFINITY), rec)) {
            Ray scattered;
            vec3 attenuation;
            if (scatter(currentRay, rec, attenuation, scattered, seed)) {
                color *= attenuation;
                currentRay = scattered;
            }
            else {
                break;
            }
        } else {
            vec3 unit_direction = normalize(currentRay.direction);
            float a = 0.5 * (unit_direction.y + 1.0);
            return color * ((1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0));
        }
    }
    return color;
}
vec3 pixelSampleSquare(vec2 seed) {
    float px = rand(seed) - 0.5;
    float py = rand(seed * 0.5) - 0.5;
    return (px * u_pixelDeltaU) + (py * u_pixelDeltaV);
}
Ray getRay(vec2 seed) {
    Ray r;
    vec3 pixel_center = u_pixel00 + (gl_FragCoord.x * u_pixelDeltaU) + (gl_FragCoord.y * u_pixelDeltaV);
    vec3 pixel_sample = pixel_center + pixelSampleSquare(seed);
    r.origin = u_cameraCenter;
    r.direction = pixel_sample - r.origin;
    return r;
}
vec3 gammaCorrect(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}
void main() {
    vec3 pixel_color = vec3(0.0, 0.0, 0.0);
    for(int sample=0; sample < u_samplesPerPixel; sample++) {
        vec2 seed = vec2(u_time, length(gl_FragCoord) * 0.1 + sample);
        Ray r = getRay(seed);
        pixel_color += getRayColor(r, seed);
    }
    float scale = 1.0f / float(u_samplesPerPixel);
    pixel_color *= scale;
    pixel_color = gammaCorrect(pixel_color, 2.2);
    FragColor = vec4(pixel_color, 1.0f);
}
