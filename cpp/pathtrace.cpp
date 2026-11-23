// Standard library includes
#include <algorithm>
#include <cmath>
#include <vector>
#include <random> 
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>

// Project includes
#include <tira/graphics/camera.h>

extern std::vector<unsigned char> image_buffer;
extern size_t s_exp;

tira::graphics::Camera camera;

std::atomic<unsigned int> seedCounter{ 1 };

inline float RNG() {
    //return -1.0f + 2.0f * (float(rand()) / RAND_MAX);
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    thread_local std::mt19937 generator(seedCounter.fetch_add(1) * 2654435761u);
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


std::vector<glm::vec3> accumulation_buffer;
std::atomic<size_t> pass_count{ 0 };
size_t render_size = 0;
size_t lastDisplayedPass = 0;
glm::vec3 previousCameraPosition;
float previousFOV;
std::thread worker;
std::atomic<bool> running{ false };

struct WorkerCleanup {
    ~WorkerCleanup() {
        running = false;
        if (worker.joinable()) worker.join();
    }
};
WorkerCleanup workerCleanup;






//
//This function is called when the program is loaded. Use it to initialize
 //the path tracer as necessary. For example, you can load models (like
// spheres), initialize data structures, etc.
//

void InitializePathTracer() {
    running = false;
    if (worker.joinable()) worker.join();
    spheres.clear();

    //Test Sphere 0
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.9f, 0.0f, 0.9f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );


    //Test Sphere 1
    spheres.push_back(
        Sphere(
            glm::vec3(2.5f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.9f, 0.9f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );

    //Test Sphere 2
    spheres.push_back(
        Sphere(
            glm::vec3(-2.5f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.9f, 0.9f),   // emissive color (color)
            glm::vec3(0.0f, 0.0f, 0.0f)    // diffuse color (none)
        )
    );


    //Test Sphere 3
    spheres.push_back(
        Sphere(
            glm::vec3(-2.5f, 2.5f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.5f, 0.5f)    // diffuse color (none)
        )
    );



    //Test Sphere 4
    spheres.push_back(
        Sphere(
            glm::vec3(2.5f, -2.5f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.75f, 0.75f, 0.75f)    // diffuse color (none)
        )
    );


    //Test Sphere 5
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, 2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.25f, 0.25f, 0.25f)    // diffuse color (none)
        )
    );


    //Test Sphere 6
    spheres.push_back(
        Sphere(
            glm::vec3(0.0f, 0.0f, -2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.0f, 0.5f)    // diffuse color (none)
        )
    );


    //Test Sphere 7
    spheres.push_back(
        Sphere(
            glm::vec3(5.0f, 0.0f, 0.0f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.5f, 0.0f)    // diffuse color (none)
        )
    );


    //Test Sphere 8
    spheres.push_back(
        Sphere(
            glm::vec3(5.0f, 0.0f, 2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.5f, 0.5f, 0.0f)    // diffuse color (none)
        )
    );


    //Test Sphere 9
    spheres.push_back(
        Sphere(
            glm::vec3(-5.0f, 0.0f, -2.5f),   // position (x, y, z)
            1.0f,                           // radius
            glm::vec3(0.0f, 0.0f, 0.0f),   // emissive color (color)
            glm::vec3(0.0f, 0.5f, 0.5f)    // diffuse color (none)
        )
    );





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
    render_size = s;
    lastDisplayedPass = 0;

}


 //This function is called every time the user interface is rendered. When
 //this function returns, the value of the image buffer will be displayed.
 //Make sure that it returns in a reasonable time so that you have some
 //visual feedback ever frame.
 

float Rmin = 0.01f;
int maxBounces = 10;


void UpdateDisplayBuffer() {
    size_t passes = pass_count;
    if (passes == 0) return;

    size_t s = render_size;
    for (size_t i = 0; i < s * s; i++) {
        glm::vec3 avg = glm::clamp(accumulation_buffer[i] / float(passes), 0.0f, 1.0f);
        image_buffer[i * 3 + 0] = (unsigned char)(avg.x * 255);
        image_buffer[i * 3 + 1] = (unsigned char)(avg.y * 255);
        image_buffer[i * 3 + 2] = (unsigned char)(avg.z * 255);
    }
}




glm::vec3 TracePath(glm::vec3 start, glm::vec3 direction) {
    glm::vec3 color(0.0f);
    glm::vec3 throughput(1.0f);
    int bounce = 0;

    while (glm::length(throughput) >= Rmin && bounce < maxBounces) {
        // Find closest sphere
        quadraticFormula closestSphere;
        closestSphere.solve = std::numeric_limits<float>::infinity();
        int closestSphereIndex = -1;
        for (int i = 0; i < spheres.size(); i++) {
            quadraticFormula t = hit_sphere(start, direction, spheres[i].position, spheres[i].radius);
            if (t.hit && t.solve < closestSphere.solve) {
                closestSphere = t;
                closestSphereIndex = i;
            }
        }
        if (closestSphereIndex == -1) {
            glm::vec3 backgroundColor(
                0.0f,
                0.0f,
                0.0f
            );
            //color = color + throughput * backgroundColor;
            break;
        }

        //Add emission
        color = color + throughput * spheres[closestSphereIndex].emissiveColor;

        //Calculate normal
        glm::vec3 hitPoint = start + closestSphere.solve * direction;
        glm::vec3 normal = glm::normalize(hitPoint - spheres[closestSphereIndex].position);
        glm::vec3 newStart = hitPoint + 0.001f * normal;


        //Random bounce
        glm::vec3 newDirection(
            RNG(-1.0f, 1.0f),
            RNG(-1.0f, 1.0f),
            RNG(-1.0f, 1.0f)
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

    return color;
}

int strategy = 2;   // 0 = contiguous blocks, 1 = interleaved rows, 2 = dynamic queue
std::atomic<size_t> nextRow{ 0 };
size_t chunkRows = 4;

void RenderRows(size_t yStart, size_t yEnd, size_t s, glm::vec3 camPos) {
    float dx = 1.0f / s;
    for (size_t yi = yStart; yi < yEnd; yi++) {
        if (!running) return;
        for (size_t xi = 0; xi < s; xi++) {
            glm::vec3 direction = camera.Ray(-0.5f + xi * dx, -0.5f + yi * dx);
            accumulation_buffer[yi * s + xi] += TracePath(camPos, direction);
        }
    }
}


void RenderInterleaved(unsigned int threadId, unsigned int numThreads, size_t s, glm::vec3 camPos) {
    float dx = 1.0f / s;
    for (size_t yi = threadId; yi < s; yi += numThreads) {
        if (!running) return;
        for (size_t xi = 0; xi < s; xi++) {
            glm::vec3 direction = camera.Ray(-0.5f + xi * dx, -0.5f + yi * dx);
            accumulation_buffer[yi * s + xi] += TracePath(camPos, direction);
        }
    }
}



void RenderDynamic(size_t s, glm::vec3 camPos) {
    float dx = 1.0f / s;
    while (true) {
        size_t start = nextRow.fetch_add(chunkRows);
        if (start >= s) return;
        size_t end = std::min(s, start + chunkRows);
        for (size_t yi = start; yi < end; yi++) {
            if (!running) return;
            for (size_t xi = 0; xi < s; xi++) {
                glm::vec3 direction = camera.Ray(-0.5f + xi * dx, -0.5f + yi * dx);
                accumulation_buffer[yi * s + xi] += TracePath(camPos, direction);
            }
        }
    }
}

void RenderWorker() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    auto t0 = std::chrono::steady_clock::now();

    while (running) {
        size_t s = render_size;
        glm::vec3 camPos = camera.Position();
        size_t rowsPer = (s + numThreads - 1) / numThreads;

        std::vector<std::thread> threads;
        nextRow = 0;
        for (unsigned int t = 0; t < numThreads; t++) {
            if (strategy == 0) {
                size_t yStart = t * rowsPer;
                if (yStart >= s) break;
                size_t yEnd = std::min(s, yStart + rowsPer);
                threads.emplace_back(RenderRows, yStart, yEnd, s, camPos);
            }
            else if (strategy == 1) {
                threads.emplace_back(RenderInterleaved, t, numThreads, s, camPos);
            }
            else {
                threads.emplace_back(RenderDynamic, s, camPos);
            }
        }
        for (auto& th : threads) th.join();

        if (running) {
            size_t p = ++pass_count;
            if (p % 10 == 0) {
                double sec = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
                printf("strategy=%d  chunk=%zu  threads=%u  passes=%zu  %.1f ms/pass  %.2f passes/s\n",
                    strategy, chunkRows, numThreads, p, 1000.0 * sec / p, p / sec);
            }
        }
    }
}




void UpdatePathTracer() {
    glm::vec3 currentCameraPosition = camera.Position();
    float currentFOV = camera.FieldOfView();

    if (currentCameraPosition != previousCameraPosition ||
        currentFOV != previousFOV) {

        // stop the worker before touching the buffers
        running = false;
        if (worker.joinable()) worker.join();

        std::fill(accumulation_buffer.begin(), accumulation_buffer.end(), glm::vec3(0.0f));
        pass_count = 0;
        lastDisplayedPass = 0;

        previousCameraPosition = currentCameraPosition;
        previousFOV = currentFOV;
    }

    if (!running) {
        running = true;
        worker = std::thread(RenderWorker);
    }

    size_t currentPass = pass_count;
    if (currentPass != lastDisplayedPass) {
        UpdateDisplayBuffer();
        lastDisplayedPass = currentPass;
    }
}
