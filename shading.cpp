#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
using namespace std;

#define PI 3.141592653589793

float clamp(const float &lo, const float &hi, const float &v)
{ return max(lo, min(hi, v)); }
class Vector3 {
private:
    float x, y, z;

public:
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    float X() const { return x; }
    float Y() const { return y; }
    float Z() const { return z; }

    void normalize() {
        float norm = sqrt(x * x + y * y + z * z);
        x /= norm;
        y /= norm;
        z /= norm;
    }

    Vector3 operator-(const Vector3 &other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator-() const {
        return Vector3(-x, -y, -z);
    }

    Vector3 operator+(const Vector3 &other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator*(float k) const {
        return Vector3(x * k, y * k, z * k);
    }

    static float dotProduct(const Vector3 &vec1, const Vector3 &vec2) {
        return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
    }

    Vector3 crossProduct(const Vector3 &other) const {
        return Vector3(y * other.Z() - z * other.Y(),
                       z * other.X() - x * other.Z(),
                       x * other.Y() - y * other.X());
    }

    void print() const {
        cout << "print" << endl;
        cout << '(' << x << ',' << y << ',' << z << ')' << endl;
    }
};

class Sphere {
private:
    Vector3 center;
    float radius;
    Vector3 color;
    float albedo;

public:
    Sphere(Vector3 cen, float r, Vector3 c, float alb)
        : center(cen), radius(r), color(c), albedo(alb) {}

    Vector3 getColor() const { return color; }
    float getAlbedo() const { return albedo;}

    bool intersect(const Vector3 &origin, const Vector3 &direction, float &t0, float &t1) const {
        Vector3 L = center - origin;
        float tca = Vector3::dotProduct(L, direction);
        float d2 = Vector3::dotProduct(L, L) - tca * tca;
        float radius2 = radius * radius;
        
        if (d2 > radius2) return false;
        
        float thc = sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;
        
        if (t0 > t1) swap(t0, t1);
        
        if (t0 < 0) {
            t0 = t1;
            if (t0 < 0) return false;
        }
        
        return true;
    }

    Vector3 getNormalAtPoint(Vector3 point) const {
        Vector3 normalVec = point - center;
        normalVec.normalize();
        return normalVec;
    }
};

class Light {
public:
    Vector3 position;
    Vector3 color;
    float intensity;

    Light(Vector3 pos, Vector3 c, float in)
        : position(pos), color(c), intensity(in) {}

    Vector3 getPosition() const { return position; }
    Vector3 getColor() const { return color; }
    float getIntensity() const { return intensity; }
};

bool trace(const Vector3 &origin, const Vector3 &direction, const vector<Sphere> &spheres, float &tnear, const Sphere *&hitSphere) {
    tnear = INFINITY;
    for (unsigned i = 0; i < spheres.size(); ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(origin, direction, t0, t1)) {
            if (t0 < tnear) {
                tnear = t0;
                hitSphere = &spheres[i];
            }
        }
    }
    return hitSphere != nullptr;
}

Vector3 castRay(const Vector3 &origin, const Vector3 &direction, const vector<Sphere> &spheres, const vector<Light> &lights) {
    float t0 = INFINITY;
    const Sphere *hitSphere = nullptr;
    if (trace(origin, direction, spheres, t0, hitSphere)) {
        Vector3 hitPoint = origin + direction * t0;
        Vector3 hitNormal = hitSphere->getNormalAtPoint(hitPoint);
        Vector3 color(0, 0, 0);

        for (const auto &light : lights) {
            Vector3 lightDir = light.getPosition() - hitPoint;
            lightDir.normalize();
            Vector3 shadowRayDir = lightDir;

            // Check for shadows
            float tShadow = INFINITY;
            const Sphere *shadowSphere = nullptr;
            bool inShadow = trace(hitPoint + hitNormal * 0.001f, shadowRayDir, spheres, tShadow, shadowSphere);
            if (inShadow && shadowSphere != hitSphere) {
                continue;
            }

            float cosTheta = max(Vector3::dotProduct(lightDir, hitNormal), 0.0f);
            Vector3 diffuseColor = hitSphere->getColor() * cosTheta;
            color = color + diffuseColor * (light.getIntensity() * hitSphere->getAlbedo() / PI);
        }

        return color;
    } else {
        return Vector3(0.5, 0.5, 0.5); // Gray background color
    }
}
void render(const vector<Sphere> &spheres, vector<Light> &lights) {
    int width = 640, height = 480;
    Vector3 *image = new Vector3[width * height];
    float invWidth = 1 / float(width), invHeight = 1 / float(height);
    float fov = 30, aspectRatio = width / float(height);
    float scale = tan(PI * 0.5 * fov / 180.);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            float x = (2 * ((i + 0.5) * invWidth) - 1) * scale * aspectRatio;
            float y = (1 - 2 * ((j + 0.5) * invHeight)) * scale;
            Vector3 direction = Vector3(x, y, -1);
            direction.normalize();
            image[j * width + i] = castRay(Vector3(0, 0, 0), direction, spheres, lights);
        }
    }

    ofstream ofs("./shading.ppm", ios::out | ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < height * width; ++i) {
        ofs << (unsigned char)(clamp(0, 1, image[i].X()) * 255)
            << (unsigned char)(clamp(0, 1, image[i].Y()) * 255)
            << (unsigned char)(clamp(0, 1, image[i].Z()) * 255);
    }
    ofs.close();
    delete[] image;
}

int main() {
    vector<Sphere> spheres;
    spheres.push_back(Sphere(Vector3(0, 0, -30), 2, Vector3(1.00, 0.32, 0.36), 0.7));
    spheres.push_back(Sphere(Vector3(-7, 0, -30), 2, Vector3(0.31, 1.0, 0.36), 0.7));
    spheres.push_back(Sphere(Vector3(7, 0, -30), 2, Vector3(0.36, 0.32, 1.0), 0.7));

    vector<Light> lights;
    lights.push_back(Light(Vector3(-3, -0.5, -28), Vector3(1, 1, 1), 1));
    lights.push_back(Light(Vector3(11, 2, -29.5), Vector3(1, 1, 1), 1));

    render(spheres, lights);

    return 0;
}
