// Standard library includes
#include <algorithm>
#include <cmath>
#include <vector>
#include <random> 
#include <cstdlib>

// Project includes
#include <tira/graphics/camera.h>

extern std::vector<unsigned char> image_buffer;
extern size_t s_exp;

tira::graphics::Camera camera;

inline float RNG() {
    //return -1.0f + 2.0f * (float(rand()) / RAND_MAX);
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline float RNG(float min, float max) {
    // Returns a random real in [min,max).
    return min + (max - min) * RNG();
}


class Sphere {
public:
    glm::vec3 position; // Center of the sphere: (x, y, z)
    float radius; // Size of the sphere
    glm::vec3 emissiveColor; // Light produced by the sphere: (R, G, B)
    glm::vec3 diffuseColor; // Color reflected by the sphere: (R, G, B)

    Sphere(glm::vec3 p, float r, glm::vec3 e, glm::vec3 d) {
        position = p;
        radius = r;
        emissiveColor = e;
        diffuseColor = d;
    }
};
std::vector<Sphere> spheres;


struct quadraticFormula {
    float solve;
    bool hit;
};


quadraticFormula hit_sphere(glm::vec3 rayStart, glm::vec3 rayDirection, glm::vec3 sphereCenter, double radius) {
    //calculate intersection
    quadraticFormula t;
    glm::vec3 ray2Sphere = sphereCenter - rayStart;
    auto a = dot(rayDirection, rayDirection);
    auto b = -2.0 * dot(rayDirection, ray2Sphere);
    auto c = dot(ray2Sphere, ray2Sphere) - radius * radius;
    auto discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        t.solve = std::numeric_limits<float>::infinity();
        t.hit = false;
        return t;
    }

    float solveMinus = (-b - sqrt(discriminant)) / (2.0 * a);
    float solvePlus = (-b + sqrt(discriminant)) / (2.0 * a);
    if (solveMinus >= 0 && solvePlus >= 0) {
        if (solveMinus < solvePlus) {
            t.solve = solveMinus;
            t.hit = (discriminant >= 0);
        }
        else {
            t.solve = solvePlus;
            t.hit = (discriminant >= 0);
        }
    }
    else if (solveMinus >= 0) {
        t.solve = solveMinus;
        t.hit = (discriminant >= 0);
    }
    else if (solvePlus >= 0) {
        t.solve = solvePlus;
        t.hit = (discriminant >= 0);
    }
    else {
        t.solve = std::numeric_limits<float>::infinity();
        t.hit = false;
    }
    return t;
}

std::vector<quadraticFormula> solvedSpheres;
std::vector<glm::vec3> accumulation_buffer;
size_t pass_count = 0;
glm::vec3 averageColor;
glm::vec3 previousCameraPosition;
float previousFOV;






/**
 * This function is called when the program is loaded. Use it to initialize
 * the path tracer as necessary. For example, you can load models (like
 * spheres), initialize data structures, etc.
 */
void InitializePathTracer() {

    spheres.clear();
    solvedSpheres.clear();

    //Test Sphere 0
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.9f, 0.0f, 0.9f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );
    quadraticFormula temp;
    solvedSpheres.push_back(temp);

    //Test Sphere 1
    spheres.push_back(
        Sphere(
            glm::vec3(2.5f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.9f, 0.9f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 2
    spheres.push_back(
        Sphere(
            glm::vec3(-2.5f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.9f, 0.9f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 3
    spheres.push_back(
        Sphere(
            glm::vec3(-2.5f, 2.5f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.5f, 0.5f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);


    //Test Sphere 4
    spheres.push_back(
        Sphere(
            glm::vec3(2.5f, -2.5f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.75f, 0.75f, 0.75f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 5
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, 2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.25f, 0.25f, 0.25f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 6
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, -2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.0f, 0.5f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 7
    spheres.push_back(
        Sphere(
            glm::vec3(5.0f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.5f, 0.0f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 8
    spheres.push_back(
        Sphere(
            glm::vec3(5.0f, 0.0f, 2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.5f, 0.0f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);

    //Test Sphere 9
    spheres.push_back(
        Sphere(
            glm::vec3(-5.0f, 0.0f, -2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.5f, 0.5f)    // diffuse color (none)
        )
    );
    solvedSpheres.push_back(temp);




    // allocate the image buffer and clear it (set it to zero)

    if (s_exp > 0) {
        image_buffer.resize(pow(2, s_exp) * pow(2, s_exp) * 3);
        std::fill(image_buffer.begin(), image_buffer.end(), 0);
    }
    size_t s = std::pow(2, s_exp);

    accumulation_buffer.resize(s * s);
    std::fill(
        accumulation_buffer.begin(),
        accumulation_buffer.end(),
        glm::vec3(0.0f)
    );

    pass_count = 0;
    previousCameraPosition = camera.Position();
    previousFOV = camera.FieldOfView();


}

/**
 * This function is called every time the user interface is rendered. When
 * this function returns, the value of the image buffer will be displayed.
 * Make sure that it returns in a reasonable time so that you have some
 * visual feedback ever frame.
 */

float Rmin = 0.01f;
int maxBounces = 10;
int bounce;


void UpdatePathTracer() {


    glm::vec3 currentCameraPosition = camera.Position();
    float currentFOV = camera.FieldOfView();

    if (currentCameraPosition != previousCameraPosition ||
        currentFOV != previousFOV) {

        std::fill(
            accumulation_buffer.begin(),
            accumulation_buffer.end(),
            glm::vec3(0.0f)
        );

        pass_count = 0;

        previousCameraPosition = currentCameraPosition;
        previousFOV = currentFOV;
    }


    if (pass_count >= 100) {
        return;
    }

    pass_count++;
    size_t s = std::pow(2, s_exp);
    float dx = 1.0f / s;



    // iterate across all pixels in the image buffer
    for (size_t yi = 0; yi < s; yi++) {
        for (size_t xi = 0; xi < s; xi++) {
            bounce = 0;


            // this gives you a vector from the camera through a pixel
            glm::vec3 direction = camera.Ray(-0.5f + xi * dx, -0.5f + yi * dx);



            // this gives you the position of the camera
            glm::vec3 start = camera.Position();


            glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 throughput = glm::vec3(1.0f, 1.0f, 1.0f);



            while (glm::length(throughput) >= Rmin && bounce < maxBounces) {

                // Find closest sphere
                quadraticFormula closestSphere;
                closestSphere.solve = std::numeric_limits<float>::infinity();
                int closestSphereIndex = -1;
                for (int i = 0; i < spheres.size(); i++) {
                    solvedSpheres[i] = hit_sphere(start, direction, spheres[i].position, spheres[i].radius);
                    if (solvedSpheres[i].hit) {
                        if (solvedSpheres[i].solve < closestSphere.solve) {
                            closestSphere = solvedSpheres[i];
                            closestSphereIndex = i;
                        }
                    }
                }
                if (closestSphereIndex == -1) {
                    glm::vec3 backgroundColor(
                        0.0f,
                        0.0f,
                        0.0f
                    );


                    //color = color + throughput * backgroundColor;
                    throughput = glm::vec3(0.0f, 0.0f, 0.0f);
                    break;
                }

                //Add emission
                color = color + throughput * spheres[closestSphereIndex].emissiveColor;

                //Calculate normal
                glm::vec3 hitPoint = start + closestSphere.solve * direction;
                glm::vec3 normal = glm::normalize(hitPoint - spheres[closestSphereIndex].position);
                glm::vec3 newStart = hitPoint + 0.001f * normal;



                float randomX = RNG();
                float randomY = RNG();
                float randomZ = RNG();

                //Random bounce
                glm::vec3 newDirection(
                    randomX,
                    randomY,
                    randomZ
                );
                newDirection = glm::normalize(newDirection);
                if (glm::dot(newDirection, normal) < 0) {
                    newDirection = -newDirection;
                }

                //Reduce throughput
                throughput = throughput * spheres[closestSphereIndex].diffuseColor;

                //Update Ray
                start = newStart;
                direction = newDirection;

                bounce++;

            }



            size_t pixelIndex = yi * s + xi;
            accumulation_buffer[pixelIndex] += color;
            averageColor =
                accumulation_buffer[pixelIndex] / float(pass_count);













            //image_buffer[yi * s * 3 + xi * 3 + 0] = color.x * 255;
           // image_buffer[yi * s * 3 + xi * 3 + 1] = color.y * 255;
            //image_buffer[yi * s * 3 + xi * 3 + 2] = color.z * 255;


            //averageColor = color;
            averageColor = glm::clamp(averageColor, 0.0f, 1.0f);

            image_buffer[yi * s * 3 + xi * 3 + 0] = averageColor.x * 255;
            image_buffer[yi * s * 3 + xi * 3 + 1] = averageColor.y * 255;
            image_buffer[yi * s * 3 + xi * 3 + 2] = averageColor.z * 255;



        }
    }
}
