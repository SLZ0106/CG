#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <CanvasPoint.h>
#include <ModelTriangle.h>
#include <TextureMap.h>
#include <Colour.h>
#include <Utils.h>
#include <RayTriangleIntersection.h>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#define WIDTH 960
#define HEIGHT 720
glm::vec3 cameraPosition (0.0, 0.0, 4.0);
glm::mat3 cameraOrientation (1, 0, 0, 0, 1, 0 ,0, 0, 1); 
glm::mat3 Rotation(1, 0, 0, 0, 1, 0 ,0, 0, 1);
glm::vec3 lightposition = glm::vec3(0.0, 0.7, 0.7);
std::vector<glm::vec3> lightpoints;
float focalLength = 2.0;
float x = 0.0;
float y = 0.0;
glm::vec3 newCameraPosition = cameraPosition;
float depthBuffer[HEIGHT][WIDTH];

void clearDepthBuffer(){
    for(int y = 0; y < HEIGHT; y++)
        for(int x = 0; x < WIDTH; x++)
            depthBuffer[y][x] = INT32_MIN;
}

int judge(float number){
    if (number < 0) return -1; // bottom-to-top
    else if (number == 0) return 0;
    else return 1; // top-to-bottom
}

void draw(DrawingWindow &window) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = rand() % 256;
			float green = 0.0;
			float blue = 0.0;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

TextureMap getTextureMap(const std::string& image) {
    TextureMap textureMap = TextureMap(image);
    std::cout << "width " << textureMap.width << "height " << textureMap.height << std::endl;
    return textureMap;
}

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValue) {
    std::vector<float> result;
    float difference = (to - from) / float(numberOfValue - 1);
    for(int i=0; i<numberOfValue; i++) result.push_back(from + (float)i*difference);
    return result;
}

std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValue) {
    std::vector<glm::vec3> result;
    glm::vec3 difference = (to - from) * (1 / (float(numberOfValue) - 1));
    for(float i=0.0; i<numberOfValue; i++) {
        result.push_back(from + difference*float(i));
    }
    return result;
}

void twoDimensionalColourInterpolation(DrawingWindow &window) {
    window.clearPixels();
    glm::vec3 red(255, 0, 0);
    glm::vec3 blue(0, 0, 255);
    glm::vec3 green(0, 255, 0);
    glm::vec3 yellow(255, 255, 0);
    std::vector<glm::vec3> left = interpolateThreeElementValues(red, yellow, int(window.height));
    std::vector<glm::vec3> right = interpolateThreeElementValues(blue, green, int(window.width));
    for(size_t y=0; y<window.height; y++) {
        std::vector<glm::vec3> row = interpolateThreeElementValues(left[y], right[y], int(window.width));
        for(size_t x=0; x<window.width; x++) {
            float colourred = row[x].x;
            float colourgreen = row[x].y;
            float colourblue = row[x].z;
            uint32_t colour = (255 << 24) + (int(colourred) << 16) + (int(colourgreen) << 8) + int(colourblue);
            window.setPixelColour(x, y, colour);
        }
    }
}

void drawLine(Colour colour, DrawingWindow &window, const CanvasPoint from, const CanvasPoint to) {
    uint32_t c = (255 << 24) + (colour.red << 16) + (colour.green << 8) + (colour.blue);
    if (x>=0 && y>=0 && x<WIDTH && y<HEIGHT) {
        for (float i = 0.0; i < fmax(fabs(to.x - from.x), fabs(to.y - from.y)); i++) {
            float x = from.x + (((to.x - from.x)/fmax(fabs(to.x - from.x), fabs(to.y - from.y))) * i);
            float y = from.y + (((to.y - from.y)/fmax(fabs(to.x - from.x), fabs(to.y - from.y))) * i);
            float z = from.depth + ((to.depth - from.depth)/fmax(fabs(to.x - from.x), fabs(to.y - from.y)) * i);
            if(depthBuffer[int(round(y))][int(round(x))] <= z) {
                depthBuffer[int(round(y))][int(round(x))] = z;
                window.setPixelColour(round(x), round(y), c);
            }
        }
    }
}

void drawStrokedTriangle(DrawingWindow &window, CanvasTriangle canvastriangle, Colour colour) {
    drawLine(colour, window, canvastriangle.v0(), canvastriangle.v1());
    drawLine(colour, window, canvastriangle.v1(), canvastriangle.v2());
    drawLine(colour, window, canvastriangle.v2(), canvastriangle.v0());
}

void triangleRasteriser(DrawingWindow &window, CanvasPoint v1, CanvasPoint v2, CanvasPoint v3, Colour colour) {
    CanvasPoint newV1 = v1;
    CanvasPoint newV2 = v1;
    for (float i = 1.00; i <= fabs(v2.y-v1.y); i++)  {
        newV1.x = v1.x + (v2.x - v1.x) * (i / fabs(v2.y-v1.y));
        newV1.y -= judge(v1.y - v3.y);
        newV1.depth = v1.depth + (v2.depth - v1.depth) * (i / fabs(v2.y-v1.y));
        newV2.x = v1.x + (v3.x - v1.x) * (i / fabs(v2.y-v1.y));
        newV2.y -= judge(v1.y - v3.y);
        newV2.depth = v1.depth + (v3.depth - v1.depth) * (i / fabs(v2.y-v1.y));
        drawLine(colour, window, newV1, newV2);
    }

}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    CanvasPoint topPoint;
    CanvasPoint bottomPoint;
    CanvasPoint givenPoint;
    CanvasPoint extraPoint;
    if(triangle.v0().y <= triangle.v1().y && triangle.v0().y <= triangle.v2().y) {
        topPoint = triangle.v0();
        if(triangle.v1().y >= triangle.v2().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v1();
        }
    } else if(triangle.v1().y <= triangle.v0().y && triangle.v1().y <= triangle.v2().y) {
        topPoint = triangle.v1();
        if(triangle.v0().y >= triangle.v2().y) {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v0();
        }
    } else if(triangle.v2().y <= triangle.v0().y && triangle.v2().y <= triangle.v1().y) {
        topPoint = triangle.v2();
        if(triangle.v1().y >= triangle.v0().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v0();
        } else {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v1();
        }
    }
    double R = (bottomPoint.x - topPoint.x)/(bottomPoint.y - topPoint.y);
    float EX;
    float EY;
    float EDepth;
    if(R >= 0) {
        EX = topPoint.x + ((givenPoint.y - topPoint.y) * fabs(R));
    } else {
        EX = topPoint.x - ((givenPoint.y - topPoint.y) * fabs(R));
    }
    EY = givenPoint.y;
    float u = (-(EX-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(EY-triangle.v1().y)*(triangle.v2().x-triangle.v1().x))/(-(triangle.v0().x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(triangle.v0().y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x));
    float v = (-(EX-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(EY-triangle.v2().y)*(triangle.v0().x-triangle.v2().x))/(-(triangle.v1().x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(triangle.v1().y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x));
    float w = 1 - u - v;
    EDepth = u * triangle.v0().depth + v * triangle.v1().depth + w * triangle.v2().depth;
    extraPoint = CanvasPoint(EX, givenPoint.y, EDepth);
    triangleRasteriser(window, topPoint, givenPoint, extraPoint, colour);
    triangleRasteriser(window, bottomPoint, givenPoint, extraPoint, colour);
}

uint32_t textureMapper(TextureMap textureMap, CanvasTriangle triangle, CanvasPoint point){
    float u = (-(point.x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(point.y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x))/(-(triangle.v0().x-triangle.v1().x)*(triangle.v2().y-triangle.v1().y)+(triangle.v0().y-triangle.v1().y)*(triangle.v2().x-triangle.v1().x));
    float v = (-(point.x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(point.y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x))/(-(triangle.v1().x-triangle.v2().x)*(triangle.v0().y-triangle.v2().y)+(triangle.v1().y-triangle.v2().y)*(triangle.v0().x-triangle.v2().x));
    float w = 1 - u - v;
    CanvasPoint texturePoint((u * triangle.v0().texturePoint.x + v * triangle.v1().texturePoint.x + w * triangle.v2().texturePoint.x), (u * triangle.v0().texturePoint.y + v * triangle.v1().texturePoint.y + w * triangle.v2().texturePoint.y));
    int index = int(texturePoint.y) * textureMap.width + int(texturePoint.x);
    std::cout << "Index of texture is:" << index << " corresponds to texture point x:" << texturePoint.x << " and y: " << texturePoint.y << "\n" << std::endl;
    uint32_t colour = textureMap.pixels[index - 1];
    return colour;
}

void textureTriangle(DrawingWindow &window, const TextureMap& textureMap, CanvasTriangle triangle, CanvasPoint v1, CanvasPoint v2, CanvasPoint v3) {
    CanvasPoint newV1=v1;
    CanvasPoint newV2=v1;
    for (int i = 1; i <= fabs(v2.y-v1.y); i++)  {
        float f = i / fabs(v2.y-v1.y);
        newV1.x = v1.x + (v2.x - v1.x) * f;
        newV1.y -= judge(v1.y - v3.y);
        newV2.x = v1.x + (v3.x - v1.x) * f;
        newV2.y -= judge(v1.y - v3.y);
        for (float j = 0.0; j < (newV2.x - newV1.x); j++) {
            float x = newV1.x + j;
            float y = newV1.y;
            uint32_t colour = textureMapper(textureMap, triangle, CanvasPoint(x, y));
            window.setPixelColour(x, y, colour);
        }
    }
}

void drawTextureTriangle(DrawingWindow &window, const TextureMap& textureMap, CanvasTriangle triangle) {
    drawStrokedTriangle(window, triangle, Colour(255,255,255));
    CanvasPoint topPoint;
    CanvasPoint bottomPoint;
    CanvasPoint givenPoint;
    CanvasPoint extraPoint;
    if(triangle.v0().y <= triangle.v1().y && triangle.v0().y <= triangle.v2().y) {
        topPoint = triangle.v0();
        if(triangle.v1().y >= triangle.v2().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v1();
        }
    } else if(triangle.v1().y <= triangle.v0().y && triangle.v1().y <= triangle.v2().y) {
        topPoint = triangle.v1();
        if(triangle.v0().y >= triangle.v2().y) {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v2();
        } else {
            bottomPoint = triangle.v2();
            givenPoint = triangle.v0();
        }
    } else if(triangle.v2().y <= triangle.v0().y && triangle.v2().y <= triangle.v1().y) {
        topPoint = triangle.v2();
        if(triangle.v1().y >= triangle.v0().y) {
            bottomPoint = triangle.v1();
            givenPoint = triangle.v0();
        } else {
            bottomPoint = triangle.v0();
            givenPoint = triangle.v1();
        }
    }
    double R = (bottomPoint.x - topPoint.x)/(bottomPoint.y - topPoint.y);
    float EX;
    if(R >= 0) {
        EX = topPoint.x + ((givenPoint.y - topPoint.y) * fabs(R));
    } else {
        EX = topPoint.x - ((givenPoint.y - topPoint.y) * fabs(R));
    }
    extraPoint = CanvasPoint(EX, givenPoint.y);
    textureTriangle(window, textureMap, triangle, topPoint, givenPoint, extraPoint);
    textureTriangle(window, textureMap, triangle, bottomPoint, givenPoint, extraPoint);
}

Colour mtlConverter(double r, double g, double b) {
    Colour packedColour = Colour(int(r*255), int(g*255), int(b*255));
    return packedColour;
}

std::map<std::string, Colour> mtlReader(const std::string& file) {
    std::map<std::string, Colour> colours;
    std::string myText;
    std::string name;
    std::ifstream File(file);
    while(getline(File, myText)) {
        std::vector<std::string> text = split(myText, ' ');
        if(text[0] == "newmtl") {
            name = text[1];
        } else if (text[0] == "Kd") {
            colours[name] = mtlConverter(std::stod(text[1]), std::stod(text[2]), std::stod(text[3]));
        }
    }
    File.close();
    return colours;
}

std::vector<ModelTriangle> objReader(const std::string& objFile, const std::string& mtlFile, float scalingFactor) {
    std::vector<glm::vec3> vertex;
    std::vector<std::vector<std::string>> facets;
    std::vector<ModelTriangle> modelTriangles;
    std::string myText;
    std::string colourName;
    std::ifstream File(objFile);
    while(getline(File, myText)) {
        std::vector<std::string> text = split(myText, ' ');
        if(text[0] == "usemtl") {
            colourName = text[1];
        } else if(text[0] == "v") {
            glm::vec3 v = glm::vec3(std::stod(text[1]), std::stod(text[2]), std::stod(text[3]));
            vertex.push_back(v);
        } else if(text[0] == "f") {
            std::vector<std::string> f {text[1], text[2], text[3], colourName};
            facets.push_back(f);
        }
    } 
    File.close();
    std::map<std::string, Colour> colourMap = mtlReader(mtlFile);
    for(std::vector<std::string> i : facets) {
        glm::vec3 v0 = vertex[std::stoi(i[0])-1];
        glm::vec3 v1 = vertex[std::stoi(i[1])-1];
        glm::vec3 v2 = vertex[std::stoi(i[2])-1];
        glm::vec3 normal = glm::normalize(glm::cross(v1-v0, v2-v0));
        Colour colour = colourMap[i[3]];
        ModelTriangle triangle = ModelTriangle(v0*scalingFactor, v1*scalingFactor, v2*scalingFactor, colour);
        triangle.normal = normal;
        modelTriangles.push_back(triangle);
    }

    return modelTriangles;
}

std::vector<ModelTriangle> SphereReader(const std::string& objFile, float scalingFactor) {
    std::vector<glm::vec3> vertex;
    std::vector<std::vector<std::string>> facets;
    std::vector<ModelTriangle> modelTriangles;
    std::string myText;
    std::ifstream File(objFile);
    while(getline(File, myText)) {
        std::vector<std::string> text = split(myText, ' ');
        if(text[0] == "v") {
            glm::vec3 v = glm::vec3(std::stod(text[1]), std::stod(text[2]), std::stod(text[3]));
            vertex.push_back(v);
        } else if(text[0] == "f") {
            std::vector<std::string> f {text[1], text[2], text[3]};
            facets.push_back(f);
        }
    }
    
    File.close();
    for(std::vector<std::string> i : facets) {
        glm::vec3 v0 = vertex[std::stoi(i[0])-1];
        glm::vec3 v1 = vertex[std::stoi(i[1])-1];
        glm::vec3 v2 = vertex[std::stoi(i[2])-1];
        glm::vec3 normal = glm::normalize(glm::cross(v1-v0, v2-v0));
        float red = 255.0;
        float green = 0.0;
        float blue = 0.0;
        Colour colour = Colour(int(red), int(green), int(blue));
        ModelTriangle triangle = ModelTriangle(v0*scalingFactor, v1*scalingFactor, v2*scalingFactor, colour);
        triangle.normal = normal;
        modelTriangles.push_back(triangle);
    }
    return modelTriangles;
}

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::mat3 cameraOrientation, glm::vec3 vertexPosition, float focallength, float scalingFactor) {
    CanvasPoint twoDPoint;
    glm::vec3 cameraToVertex = {vertexPosition.x - cameraPosition.x, vertexPosition.y - cameraPosition.y, vertexPosition.z - cameraPosition.z};
    glm::vec3 cameraToVertexInCameraSpace =  cameraToVertex * cameraOrientation;
    twoDPoint.x = round(focallength * (-scalingFactor) * cameraToVertexInCameraSpace.x / cameraToVertexInCameraSpace.z + float(WIDTH/2));
    twoDPoint.y = round(focallength * scalingFactor * cameraToVertexInCameraSpace.y / cameraToVertexInCameraSpace.z + float(HEIGHT/2));
    twoDPoint.depth = 1/fabs(cameraToVertexInCameraSpace.z);
    return twoDPoint;
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec3 cameraPosition, glm::mat3 cameraOrientation, ModelTriangle triangle, float focallength, float scalingFactor) {
    CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[0], focallength, scalingFactor);
    CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[1], focallength, scalingFactor);
    CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, cameraOrientation, triangle.vertices[2], focallength, scalingFactor);
    CanvasTriangle canvasTriangle = CanvasTriangle(v0, v1, v2);
    return canvasTriangle;
}

void PointCloud(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour) {
    int red = colour.red;
    int green = colour.green;
    int blue = colour.blue;
    uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        window.setPixelColour(canvasTriangle.v0().x, canvasTriangle.v0().y, c);
        window.setPixelColour(canvasTriangle.v1().x, canvasTriangle.v1().y, c);
        window.setPixelColour(canvasTriangle.v2().x, canvasTriangle.v2().y, c);
    }
}

void WireFrame(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour){
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        drawLine(triangle.colour, window, canvasTriangle.v0(), canvasTriangle.v1());
        drawLine(triangle.colour, window, canvasTriangle.v1(), canvasTriangle.v2());
        drawLine(triangle.colour, window, canvasTriangle.v2(), canvasTriangle.v0());
    }
}

void Rasterised(DrawingWindow &window, const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::mat3 cameraOrientation, float focallength, float scalingFactor, const Colour& colour) {
    for(ModelTriangle triangle : triangles) {
        CanvasTriangle canvasTriangle = getCanvasIntersectionTriangle(cameraPosition, cameraOrientation, triangle, focallength, scalingFactor);
        //ImageDepth(canvasTriangle, Colour(255, 255, 255));
        drawFilledTriangle(window, canvasTriangle, triangle.colour);
    }
}

glm::mat3 lookAt(glm::vec3 cameraPosition) {
    glm::vec3 vertical (0, 1, 0);
    glm::vec3 center (0, 0, 0);
    glm::vec3 forward = glm::normalize(cameraPosition - center);
    glm::vec3 right = glm::cross(vertical, forward);
    glm::vec3 up = glm::cross(forward, right);
    glm::mat3 newCameraOrientation = glm::mat3(right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
    return newCameraOrientation;
}

void IntersectionSort(std::vector<RayTriangleIntersection> &intersections){
    class myCompare{
    public:
        bool operator()(const RayTriangleIntersection &a, const RayTriangleIntersection &b) {
            return a.distanceFromCamera < b.distanceFromCamera;
        }
    };
    std::sort(intersections.begin(), intersections.end(), myCompare());
}

RayTriangleIntersection getClosestIntersection(const std::vector<ModelTriangle> triangles, glm::vec3 cameraPosition, glm::vec3 rayDirection){
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = FLT_MAX;
    for(size_t i = 0; i < triangles.size(); i++) {
        glm::vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
        glm::vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
        glm::vec3 SPVector = cameraPosition - triangles[i].vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
        if(possibleSolution[0] <= closestIntersection.distanceFromCamera && possibleSolution[0] >= 0.00001 && possibleSolution[1] >= 0 && possibleSolution[1] <= 1 && possibleSolution[2] >= 0 && possibleSolution[2] <= 1 && (possibleSolution[1] + possibleSolution[2]) <= 1){
            glm::vec3 intersectionPoint = cameraPosition + possibleSolution[0] * rayDirection;
            closestIntersection = RayTriangleIntersection(intersectionPoint, possibleSolution[0], triangles[i], i);
            closestIntersection.triangleIndex = i;
        }
    }
    return closestIntersection;
}

RayTriangleIntersection getShadowIntersection(const std::vector<ModelTriangle> modelTriangles, glm::vec3 cameraPosition, glm::vec3 rayDirection){
    RayTriangleIntersection closest;
    closest.distanceFromCamera = FLT_MAX; 
	for (unsigned i = 0; i < modelTriangles.size(); i++) {
		glm::vec3 e0 = modelTriangles[i].vertices[1] - modelTriangles[i].vertices[0];
		glm::vec3 e1 = modelTriangles[i].vertices[2] - modelTriangles[i].vertices[0];
		glm::vec3 SPVector = cameraPosition - modelTriangles[i].vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
		if(possibleSolution[0] <= closest.distanceFromCamera && possibleSolution[0] >= 0.00001 && possibleSolution[1] >= 0 && possibleSolution[1] <= 1 && possibleSolution[2] >= 0 && possibleSolution[2] <= 1 && (possibleSolution[1] + possibleSolution[2]) <= 1) {
			glm::vec3 point = cameraPosition + possibleSolution[0] * rayDirection;
			closest = RayTriangleIntersection(point, possibleSolution[0], modelTriangles[i], i);
			closest.triangleIndex = i;
		}
	}
	return closest;
}

RayTriangleIntersection getReflectionIntersection(const std::vector<ModelTriangle> modelTriangles, glm::vec3 cameraPosition, glm::vec3 rayDirection){
    RayTriangleIntersection closest;
    closest.distanceFromCamera = FLT_MAX; 
    for (unsigned i = 0; i < modelTriangles.size(); i++) {
        glm::vec3 e0 = modelTriangles[i].vertices[1] - modelTriangles[i].vertices[0];
        glm::vec3 e1 = modelTriangles[i].vertices[2] - modelTriangles[i].vertices[0];
        glm::vec3 SPVector = cameraPosition - modelTriangles[i].vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
        if(possibleSolution[0] <= closest.distanceFromCamera && possibleSolution[0] >= 0.00001 && possibleSolution[1] >= 0 && possibleSolution[1] <= 1 && possibleSolution[2] >= 0 && possibleSolution[2] <= 1 && (possibleSolution[1] + possibleSolution[2]) <= 1) {
            glm::vec3 point = cameraPosition + possibleSolution[0] * rayDirection;
            closest = RayTriangleIntersection(point, possibleSolution[0], modelTriangles[i], i);
            closest.triangleIndex = i;
        }
    }
    return closest;
}

glm::vec3 get3DPoint(CanvasPoint point, glm::vec3 cameraPosition, float focalLength, float scalingFactor) {
    float u = point.x;
    float v = point.y;
    float z = cameraPosition.z - focalLength;
    float x = ((u - (float(WIDTH)/2)) / (focalLength * (scalingFactor))) * z;
    float y = ((v - (float(HEIGHT)/2)) / (focalLength * (-scalingFactor))) * z;
    glm::vec3 threeDPoint (x, y, z);
    return threeDPoint;
}

std::vector<glm::vec3> MultiLight(glm::vec3 center, int num, float spread) {
    float x = center.x;
    float y = center.y;
    float z = center.z;
    std::vector<glm::vec3> lightPosition;
    for (int i = -num; i <= num; i++){
        for (int j = -num; j <= num; j++){
            if(fabs(i) + fabs(j) <= num){
                lightPosition.push_back(glm::vec3(x + i * spread, y, z + j * spread));
            }
        }
    }
    return lightPosition;
}


void drawRasterisedScene(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            red = colour.red;
            green = colour.green;
            blue = colour.blue;
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
}

void drawRasterisedShadowScene(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            //int S = 16;
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = glm::normalize(closestIntersection.intersectionPoint - lightPosition);
            //glm::vec3 normal = glm::normalize(closestIntersection.intersectedTriangle.normal);
            //glm::vec3 reflection = glm::normalize(rayDirection - 2.0f * glm::dot(rayDirection, normal) * normal);
            //float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = ((colour.red)*0.3);
                green = ((colour.red)*0.3);
                blue = ((colour.red)*0.3);

            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red, 255);
                green = fmin(colour.green, 255);
                blue = fmin(colour.blue, 255);
                }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 

void drawIncidence(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            int S = 16;
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = closestIntersection.intersectionPoint - lightPosition;
            ModelTriangle triangle = closestIntersection.intersectedTriangle;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            float incidenceintensity = glm::clamp<float>(glm::dot(triangle.normal, -lightDirection), 0.0, 1.0);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = 0;
                green = 0;
                blue = 0;
            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*incidenceintensity*intensity,255);
                green = fmin(colour.green*incidenceintensity*intensity,255);
                blue = fmin(colour.blue*incidenceintensity*intensity,255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 

void drawSpecular(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    lightpoints = MultiLight(lightPosition, 3, 0.03);
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = threeDPoint - cameraPosition;
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            int S = 16;
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = closestIntersection.intersectionPoint - lightPosition;
            ModelTriangle triangle = closestIntersection.intersectedTriangle;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            glm::vec3 reflection = glm::normalize(lightDirection - (2*glm::dot(lightDirection, triangle.normal)*triangle.normal));
            glm::vec3 view = glm::normalize(closestIntersection.intersectionPoint- cameraPosition);
            float specularintensity = glm::dot(view, reflection);
            specularintensity = pow(specularintensity, 64);
            float incidenceintensity = glm::clamp<float>(glm::dot(triangle.normal, -lightDirection), 0.0, 1.0);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            Colour Background = Colour(0,0,0);
            float Shadowresult = 0.7;
            if (closestIntersection.distanceFromCamera ==FLT_MAX){
                    uint32_t backgroundc = (255 << 24) + (Background.red << 16) + (Background.green << 8) + (Background.blue);
                    window.setPixelColour(x, y, backgroundc);
                    continue;
                }
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                for (unsigned i = 0; i < lightpoints.size(); i++) {
						glm::vec3 lRay = glm::normalize(closestIntersection.intersectionPoint - lightpoints[i]);
						RayTriangleIntersection multiShadowIntersection = getShadowIntersection(modelTriangles, lightpoints[i], lRay);
						if (closestIntersection.triangleIndex != multiShadowIntersection.triangleIndex) Shadowresult -= 0.025;
					}
                    red = (colour.red*(incidenceintensity*intensity + specularintensity)+35)*Shadowresult;
                    green = (colour.red*(incidenceintensity*intensity + specularintensity)+35)*Shadowresult;
                    blue = (colour.red*(incidenceintensity*intensity + specularintensity)+35)*Shadowresult;
                //red = 0;
                //green = 0;
                //blue = 0;
            }else{
                //Plus 25 means that the colour is never black and for Ambient Lighting
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*(incidenceintensity*intensity + specularintensity)+35,255);
                green = fmin(colour.green*(incidenceintensity*intensity + specularintensity)+35,255);
                blue = fmin(colour.blue*(incidenceintensity*intensity + specularintensity)+35,255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 


glm::vec3 NormalCalculator(glm::vec3 vertex, const std::vector<ModelTriangle>& modelTriangles){
    float faceindex = 0.0;
    glm::vec3 vertexnormal;
    for(ModelTriangle triangle : modelTriangles){
        if(triangle.vertices[0] == vertex || triangle.vertices[1] == vertex || triangle.vertices[2] == vertex){
            faceindex++;
            vertexnormal += triangle.normal;
        }
    }
    return glm::normalize(vertexnormal/faceindex);
}

void drawGouraud(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = threeDPoint - cameraPosition;
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            glm::vec3 lightDirection = glm::normalize(closestIntersection.intersectionPoint - lightPosition);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 view = glm::normalize(closestIntersection.intersectionPoint- cameraPosition);
            glm::vec3 pointA = closestIntersection.intersectedTriangle.vertices[0];
            glm::vec3 pointB = closestIntersection.intersectedTriangle.vertices[1];
            glm::vec3 pointC = closestIntersection.intersectedTriangle.vertices[2];
            glm::vec3 n1 = NormalCalculator(pointA, modelTriangles);
            glm::vec3 n2 = NormalCalculator(pointB, modelTriangles);
            glm::vec3 n3 = NormalCalculator(pointC, modelTriangles);
            glm::vec3 lightDirection1 = glm::normalize(lightPosition - n1);
            glm::vec3 lightDirection2 = glm::normalize(lightPosition - n2);
            glm::vec3 lightDirection3 = glm::normalize(lightPosition - n3);
            glm::vec3 normal = glm::normalize(closestIntersection.intersectedTriangle.normal); 
            glm::vec3 reflection = glm::normalize(rayDirection - 2.0f * glm::dot(rayDirection, normal) * normal);
            float l1,l2,l3;
                if (pointA.x == pointB.x && pointA.x == pointC.x){
                    float det = ((pointB.z - pointC.z) * (pointA.y - pointC.y)) + ((pointA.z - pointC.z) * (pointC.y - pointB.y));
                    l1 = ((pointB.z - pointC.z) * (closestIntersection.intersectionPoint.y - pointC.y) + (pointC.y - pointB.y) * (closestIntersection.intersectionPoint.z - pointC.z)) / det;
                    l2 = ((pointC.z - pointA.z) * (closestIntersection.intersectionPoint.y - pointC.y) + (pointA.y - pointC.y) * (closestIntersection.intersectionPoint.z - pointC.z)) / det;
                    l3 = 1 - l1 - l2;
                }else if (pointA.y == pointB.y && pointA.y == pointC.y){
                    float det = ((pointB.z - pointC.z) * (pointA.x - pointC.x)) + ((pointC.x - pointB.x) * (pointA.z - pointC.z));
                    l1 = (((pointB.z - pointC.z) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointC.x - pointB.x) * (closestIntersection.intersectionPoint.z - pointC.z))) / det;
                    l2 = (((pointC.z - pointA.z) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointA.x - pointC.x) * (closestIntersection.intersectionPoint.z - pointC.z))) / det;
                    l3 = 1 - l1 - l2;
                }else{
                    float det = ((pointB.y - pointC.y) * (pointA.x - pointC.x)) + ((pointC.x - pointB.x) * (pointA.y - pointC.y));
                    l1 = (((pointB.y - pointC.y) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointC.x - pointB.x) * (closestIntersection.intersectionPoint.y - pointC.y))) / det;
                    l2 = (((pointC.y - pointA.y) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointA.x - pointC.x) * (closestIntersection.intersectionPoint.y - pointC.y))) / det;
                    l3 = 1 - l1 - l2;
                }
            float incidenceintensity1 = glm::clamp<float>(glm::dot(n1, -lightDirection), 0.0, 1.0);
            float incidenceintensity2 = glm::clamp<float>(glm::dot(n2, -lightDirection), 0.0, 1.0);
            float incidenceintensity3 = glm::clamp<float>(glm::dot(n3, -lightDirection), 0.0, 1.0);
            glm::vec3 reflection1 = glm::normalize(lightDirection - (2.0f*n1*glm::dot(lightDirection, n1)));
            glm::vec3 reflection2 = glm::normalize(lightDirection - (2.0f*n2*glm::dot(lightDirection, n2)));
            glm::vec3 reflection3 = glm::normalize(lightDirection - (2.0f*n3*glm::dot(lightDirection, n3)));
            float specularintensity1 = glm::dot(view, reflection1);
            float specularintensity2 = glm::dot(view, reflection2);
            float specularintensity3 = glm::dot(view, reflection3);
            specularintensity1 = fabs(pow(specularintensity1, 128));
            specularintensity2 = fabs(pow(specularintensity2, 128));
            specularintensity3 = fabs(pow(specularintensity3, 128));
            int S = 16;
            float intensity1 = S /(4*3.1415*glm::length(lightDirection1)*glm::length(lightDirection1));
            float intensity2 = S /(4*3.1415*glm::length(lightDirection2)*glm::length(lightDirection2));
            float intensity3 = S /(4*3.1415*glm::length(lightDirection3)*glm::length(lightDirection3));
            float totalintensity1 = (incidenceintensity1*intensity1 + specularintensity1);
            float totalintensity2 = (incidenceintensity2*intensity2 + specularintensity2);
            float totalintensity3 = (incidenceintensity3*intensity3 + specularintensity3);
            float totalinternsity = l1*totalintensity1 + l2*totalintensity2 + l3*totalintensity3;
            Colour Background = Colour(0,0,0);
            Colour yellowcolour = Colour(255, 0, 255);
            if (closestIntersection.distanceFromCamera ==FLT_MAX){
                uint32_t backgroundc = (255 << 24) + (Background.red << 16) + (Background.green << 8) + (Background.blue);
                window.setPixelColour(x, y, backgroundc);
                continue;
            }
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                //Colour colour = closestIntersection.intersectedTriangle.colour;
                //red = (colour.red*totalinternsity+55)*0.3;
                //green = ((colour.green*totalinternsity+55)*0.3);
                //blue = ((colour.blue*totalinternsity+55)*0.3);
                Colour colour = closestIntersection.intersectedTriangle.colour;
                red = colour.red*totalinternsity*0.5f;
                green = colour.green*totalinternsity*0.5f;
                blue = colour.blue*totalinternsity*0.5f;
            }else{
                Colour colour = closestIntersection.intersectedTriangle.colour;
                //colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*totalinternsity,255);
                green = fmin(colour.green*totalinternsity,255);
                blue = fmin(colour.blue*totalinternsity,255);
            }
            if(closestIntersection.intersectedTriangle.colour.red == yellowcolour.red && closestIntersection.intersectedTriangle.colour.green == yellowcolour.green && closestIntersection.intersectedTriangle.colour.blue == yellowcolour.blue){
                    RayTriangleIntersection reflectionIntersection = getReflectionIntersection(modelTriangles, closestIntersection.intersectionPoint, reflection);
                    Colour colour = reflectionIntersection.intersectedTriangle.colour;
                    glm::vec3 reflectiondirection = glm::normalize(reflectionIntersection.intersectionPoint - lightPosition);
                    RayTriangleIntersection lightIntersection1 = getReflectionIntersection(modelTriangles, reflectionIntersection.intersectionPoint, reflectiondirection);
                    if(closestIntersection.triangleIndex != lightIntersection1.triangleIndex){
                        red = colour.red*totalinternsity*0.5f;
                        green = colour.green*totalinternsity*0.5f;
                        blue = colour.blue*totalinternsity*0.5f;
                    }else{
                    red = fmin(colour.red*totalinternsity*0.8f, 255);
                    green = fmin(colour.green*totalinternsity*0.8f, 255);
                    blue = fmin(colour.blue*totalinternsity*0.8f, 255);
                }
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 

void drawPhong(DrawingWindow &window, const std::vector<ModelTriangle>& modelTriangles, glm::vec3 cameraPosition, glm::vec3 lightPosition, float focalLength, float scalingFactor) {
    int red;
    int green;
    int blue;
    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            int S = 16;
            CanvasPoint point = CanvasPoint(float(x), float(y));
            glm::vec3 threeDPoint = get3DPoint(point, cameraPosition, focalLength, scalingFactor);
            glm::vec3 rayDirection = glm::normalize(threeDPoint - cameraPosition);
            RayTriangleIntersection closestIntersection = getClosestIntersection(modelTriangles, cameraPosition, rayDirection);
            Colour colour = closestIntersection.intersectedTriangle.colour;
            glm::vec3 lightDirection = glm::normalize(closestIntersection.intersectionPoint - lightPosition);
            ModelTriangle triangle = closestIntersection.intersectedTriangle;
            float intensity = S /(4*3.1415*glm::length(lightDirection)*glm::length(lightDirection));
            glm::vec3 pointA = closestIntersection.intersectedTriangle.vertices[0];
            glm::vec3 pointB = closestIntersection.intersectedTriangle.vertices[1];
            glm::vec3 pointC = closestIntersection.intersectedTriangle.vertices[2];
            glm::vec3 n1 = NormalCalculator(pointA, modelTriangles);
            glm::vec3 n2 = NormalCalculator(pointB, modelTriangles);
            glm::vec3 n3 = NormalCalculator(pointC, modelTriangles);
            float l1,l2,l3;
                if (pointA.x == pointB.x && pointA.x == pointC.x){
                    float det = ((pointB.z - pointC.z) * (pointA.y - pointC.y)) + ((pointA.z - pointC.z) * (pointC.y - pointB.y));
                    l1 = ((pointB.z - pointC.z) * (closestIntersection.intersectionPoint.y - pointC.y) + (pointC.y - pointB.y) * (closestIntersection.intersectionPoint.z - pointC.z)) / det;
                    l2 = ((pointC.z - pointA.z) * (closestIntersection.intersectionPoint.y - pointC.y) + (pointA.y - pointC.y) * (closestIntersection.intersectionPoint.z - pointC.z)) / det;
                    l3 = 1 - l1 - l2;
                }else if (pointA.y == pointB.y && pointB.y == pointC.y){
                    float det = ((pointB.z - pointC.z) * (pointA.x - pointC.x)) + ((pointC.x - pointB.x) * (pointA.z - pointC.z));
                    l1 = (((pointB.z - pointC.z) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointC.x - pointB.x) * (closestIntersection.intersectionPoint.z - pointC.z))) / det;
                    l2 = (((pointC.z - pointA.z) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointA.x - pointC.x) * (closestIntersection.intersectionPoint.z - pointC.z))) / det;
                    l3 = 1 - l1 - l2;
                }else{
                    float det = ((pointB.y - pointC.y) * (pointA.x - pointC.x)) + ((pointC.x - pointB.x) * (pointA.y - pointC.y));
                    l1 = (((pointB.y - pointC.y) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointC.x - pointB.x) * (closestIntersection.intersectionPoint.y - pointC.y))) / det;
                    l2 = (((pointC.y - pointA.y) * (closestIntersection.intersectionPoint.x - pointC.x)) + ((pointA.x - pointC.x) * (closestIntersection.intersectionPoint.y - pointC.y))) / det;
                    l3 = 1 - l1 - l2;}
            glm::vec3 normal = (l1*n1) + (l2*n2) + (l3*n3);
            glm::vec3 reflection = glm::normalize(lightDirection - (2.0f*normal*glm::dot(lightDirection, normal)));
            glm::vec3 view = glm::normalize(closestIntersection.intersectionPoint- cameraPosition);
            float specularintensity = glm::dot(view, reflection);
            specularintensity = pow(specularintensity, 128);
            float incidenceintensity = glm::clamp<float>(glm::dot(normal, -lightDirection), 0.0, 1.0);
            RayTriangleIntersection lightIntersection = getClosestIntersection(modelTriangles, lightPosition, lightDirection);
            Colour Background = Colour(0,0,0);
            if (closestIntersection.distanceFromCamera ==FLT_MAX){
                    uint32_t backgroundc = (255 << 24) + (Background.red << 16) + (Background.green << 8) + (Background.blue);
                    window.setPixelColour(x, y, backgroundc);
                    continue;
                }
            if (closestIntersection.triangleIndex != lightIntersection.triangleIndex) {
                red = ((colour.red*(incidenceintensity*intensity + specularintensity)+35)*0.3);
                green = ((colour.green*(incidenceintensity*intensity + specularintensity)+35)*0.3);
                blue = ((colour.blue*(incidenceintensity*intensity + specularintensity)+35)*0.3);
                //red = 0;
                //green = 0;
                //blue = 0;
            }else{
                //Plus 25 means that the colour is never black and for Ambient Lighting
                //Colour colour = closestIntersection.intersectedTriangle.colour;
                colour = closestIntersection.intersectedTriangle.colour;
                red = fmin(colour.red*(incidenceintensity*intensity + specularintensity)+35,255);
                green = fmin(colour.green*(incidenceintensity*intensity + specularintensity)+35,255);
                blue = fmin(colour.blue*(incidenceintensity*intensity + specularintensity)+35,255);
            }
            uint32_t c = (255 << 24) + (red << 16) + (green << 8) + (blue);
            window.setPixelColour(x, y, c);
        } 
    }
} 





// ---------------------- Show in window ---------------------- //
void handleEvent(SDL_Event event, DrawingWindow &window) {
    //auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
    auto modelTriangles = SphereReader("sphere.obj", 0.5);
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) {
            std::cout << "LEFT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            std::cout << "RIGHT" << std::endl;
        } else if (event.key.keysym.sym == SDLK_UP) {
            std::cout << "UP" << std::endl;
        } else if (event.key.keysym.sym == SDLK_DOWN) {
            std::cout << "DOWN" << std::endl;
        } else if (event.key.keysym.sym == SDLK_u) {
            CanvasPoint point1,point2,point3;
            CanvasTriangle triangle;
            point1 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point2 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point3 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            triangle = CanvasTriangle(point1, point2, point3);
            drawStrokedTriangle(window, triangle, Colour(rand()%256, rand()%256, rand()%256));
        } else if (event.key.keysym.sym == SDLK_f) {
            CanvasPoint point1,point2,point3;
            CanvasTriangle triangle;
            point1 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point2 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            point3 = CanvasPoint(float(rand()%window.width), float(rand()%window.height));
            triangle = CanvasTriangle(point1, point2, point3);
            drawStrokedTriangle(window, triangle, Colour(255, 255, 255));
            drawFilledTriangle(window, triangle, Colour(rand()%256, rand()%256, rand()%256));
        } else if(event.key.keysym.sym == SDLK_o) {
            CanvasPoint point1,point2,point3;
            point1 = CanvasPoint(160, 10);
            point2 = CanvasPoint(300, 230);
            point3 = CanvasPoint(10, 150);
            point1.texturePoint.x = 195; point1.texturePoint.y = 5;
            point2.texturePoint.x = 395; point2.texturePoint.y = 380;
            point3.texturePoint.x = 65; point3.texturePoint.y = 330;
            drawTextureTriangle(window, getTextureMap("texture.ppm"), CanvasTriangle(point1, point2, point3));
        } else if(event.key.keysym.sym == SDLK_p) {
            PointCloud(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            WireFrame(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if(event.key.keysym.sym == SDLK_w) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.y -= 0.05;
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_s) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.y += 0.05;
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_a) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.x += 0.05;
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_d) {
            window.clearPixels();
            clearDepthBuffer();
            cameraPosition.x -= 0.05;
            Rasterised(window, modelTriangles, cameraPosition, cameraOrientation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_q) {
            //left
            window.clearPixels();
            clearDepthBuffer();
            x = 1.0;
            float radianx = glm::radians(x);
            glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, sin(radianx), 0, 1, 0, -sin(radianx), 0, cos(radianx));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_e) {
            //right
            window.clearPixels();
            clearDepthBuffer();
            x = 1.0;
            float radianx = glm::radians(x);
            glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, -sin(radianx), 0, 1, 0, sin(radianx), 0, cos(radianx));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;   
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));  
        } else if (event.key.keysym.sym == SDLK_z) {
            //downwards
            window.clearPixels();
            clearDepthBuffer();
            y = 0.3;
            float radiany = glm::radians(y);
            glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radiany), sin(radiany), 0, -sin(radiany), cos(radiany));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_x) {
            //upwards
            window.clearPixels();
            clearDepthBuffer();
            y = 0.3;
            float radiany = glm::radians(y);
            glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radiany), -sin(radiany), 0, sin(radiany), cos(radiany));
            cameraPosition = cameraPosition * cameraRotation;
            Rotation = Rotation * cameraRotation;
            Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));
        } else if (event.key.keysym.sym == SDLK_r) {
            drawRasterisedScene(window, modelTriangles, cameraPosition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_t) {
            drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_y) {
            drawIncidence(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_m) {
            drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_1) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.x += 0.05;
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_2) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.x -= 0.05;
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_3) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.y += 0.05;
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_4) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.y -= 0.05;
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_5) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.z += 0.05; 
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_6) {
            window.clearPixels();
            clearDepthBuffer();
            lightposition.z -= 0.05;
            //drawRasterisedShadowScene(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawSpecular(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
            //drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_n) {
            drawGouraud(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_b) {
            drawPhong(window, modelTriangles, cameraPosition, lightposition, focalLength, float(HEIGHT)*2/3);
        } else if (event.key.keysym.sym == SDLK_c) {
            clearDepthBuffer();
            window.clearPixels();
            }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}
int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
        //Orbit
            //auto modelTriangles = objReader("cornell-box.obj", "cornell-box.mtl", 0.35);
            //float radianx = 0.01;
            //window.clearPixels();
            //clearDepthBuffer();
            // y axis anticlockwise
            //glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, sin(radianx), 0, 1, 0, -sin(radianx), 0, cos(radianx));
            // y axis clockwise
            //glm::mat3 cameraRotation = glm::mat3(cos(radianx), 0, -sin(radianx), 0, 1, 0, sin(radianx), 0, cos(radianx));
            // x axis anticlockwise
            //glm::mat3 cameraRotation = glm::mat3(1, 0, 0, 0, cos(radianx), -sin(radianx), 0, sin(radianx), cos(radianx));
            // use only in y axis rotation
            //Rotation = lookAt(cameraPosition);
            // use only in y axis rotation
            //Rotation = Rotation * cameraRotation;
            //cameraPosition =  cameraRotation * cameraPosition;
            //float focalLength = 2.0;
            //Rasterised(window, modelTriangles, cameraPosition, Rotation, focalLength, float(HEIGHT)*2/3, Colour(255, 255, 255));

		window.renderFrame();
	}
}
